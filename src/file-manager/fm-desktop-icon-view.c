/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* fm-desktop-icon-view.c - implementation of icon view for managing the desktop.

   Copyright (C) 2000, 2001 Eazel, Inc.mou

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Cafe Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Mike Engber <engber@eazel.com>
   	    Gene Z. Ragan <gzr@eazel.com>
	    Miguel de Icaza <miguel@ximian.com>
*/

#include <config.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <ctk/ctk.h>
#include <cdk/cdkx.h>
#include <glib/gi18n.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <eel/eel-glib-extensions.h>
#include <eel/eel-ctk-extensions.h>
#include <eel/eel-vfs-extensions.h>

#include <libbaul-private/baul-desktop-icon-file.h>
#include <libbaul-private/baul-directory-background.h>
#include <libbaul-private/baul-directory-notify.h>
#include <libbaul-private/baul-file-changes-queue.h>
#include <libbaul-private/baul-file-operations.h>
#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-ui-utilities.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-view-factory.h>
#include <libbaul-private/baul-link.h>
#include <libbaul-private/baul-metadata.h>
#include <libbaul-private/baul-monitor.h>
#include <libbaul-private/baul-program-choosing.h>
#include <libbaul-private/baul-trash-monitor.h>

#include "fm-icon-container.h"
#include "fm-desktop-icon-view.h"
#include "fm-actions.h"

/* Timeout to check the desktop directory for updates */
#define RESCAN_TIMEOUT 4

struct _FMDesktopIconViewPrivate
{
    CdkWindow *root_window;
    CtkActionGroup *desktop_action_group;
    guint desktop_merge_id;

    /* For the desktop rescanning
     */
    gulong delayed_init_signal;
    guint reload_desktop_timeout;
    gboolean pending_rescan;
};

static void     default_zoom_level_changed                        (gpointer                user_data);
static gboolean real_supports_auto_layout                         (FMIconView             *view);
static gboolean real_supports_scaling	                          (FMIconView             *view);
static gboolean real_supports_keep_aligned                        (FMIconView             *view);
static gboolean real_supports_labels_beside_icons                 (FMIconView             *view);
static void     real_merge_menus                                  (FMDirectoryView        *view);
static void     real_update_menus                                 (FMDirectoryView        *view);
static gboolean real_supports_zooming                             (FMDirectoryView        *view);
static void     fm_desktop_icon_view_update_icon_container_fonts  (FMDesktopIconView      *view);
static void     font_changed_callback                             (gpointer                callback_data);

G_DEFINE_TYPE_WITH_PRIVATE (FMDesktopIconView, fm_desktop_icon_view, FM_TYPE_ICON_VIEW)

static char *desktop_directory;
static time_t desktop_dir_modify_time;

static void
desktop_directory_changed_callback (gpointer callback_data G_GNUC_UNUSED)
{
    g_free (desktop_directory);
    desktop_directory = baul_get_desktop_directory ();
}

static BaulIconContainer *
get_icon_container (FMDesktopIconView *icon_view)
{
    g_return_val_if_fail (FM_IS_DESKTOP_ICON_VIEW (icon_view), NULL);
    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (ctk_bin_get_child (CTK_BIN (icon_view))), NULL);

    return BAUL_ICON_CONTAINER (ctk_bin_get_child (CTK_BIN (icon_view)));
}

static void
icon_container_set_workarea (BaulIconContainer *icon_container,
                             CdkScreen             *screen,
                             long                  *workareas,
                             int                    n_items)
{
    int left, right, top, bottom;
    int screen_width, screen_height;
    int scale;
    int i;

    left = right = top = bottom = 0;

    scale = cdk_window_get_scale_factor (cdk_screen_get_root_window (screen));

    screen_width  = WidthOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale;
    screen_height = HeightOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale;

    for (i = 0; i < n_items; i += 4)
    {
        int x      = workareas [i] / scale;
        int y      = workareas [i + 1] / scale;
        int width  = workareas [i + 2] / scale;
        int height = workareas [i + 3] / scale;

        if ((x + width) > screen_width || (y + height) > screen_height)
            continue;

        left   = MAX (left, x);
        right  = MAX (right, screen_width - width - x);
        top    = MAX (top, y);
        bottom = MAX (bottom, screen_height - height - y);
    }

    baul_icon_container_set_margins (icon_container,
                                     left, right, top, bottom);
}

static void
net_workarea_changed (FMDesktopIconView *icon_view,
                      CdkWindow         *window)
{
    CdkDisplay *display;
    long *nworkareas = NULL;
    long *workareas = NULL;
    CdkAtom type_returned;
    int format_returned;
    int length_returned;
    BaulIconContainer *icon_container;

    g_return_if_fail (FM_IS_DESKTOP_ICON_VIEW (icon_view));

    icon_container = get_icon_container (icon_view);

    /* Find the number of desktops so we know how long the
     * workareas array is going to be (each desktop will have four
     * elements in the workareas array describing
     * x,y,width,height) */
    display = ctk_widget_get_display (CTK_WIDGET (icon_view));
    cdk_x11_display_error_trap_push (display);
    if (!cdk_property_get (window,
                           cdk_atom_intern ("_NET_NUMBER_OF_DESKTOPS", FALSE),
                           cdk_x11_xatom_to_atom (XA_CARDINAL),
                           0, 4, FALSE,
                           &type_returned,
                           &format_returned,
                           &length_returned,
                           (guchar **) &nworkareas))
    {
        g_warning("Can not calculate _NET_NUMBER_OF_DESKTOPS");
    }
    if (cdk_x11_display_error_trap_pop (display)
            || nworkareas == NULL
            || type_returned != cdk_x11_xatom_to_atom (XA_CARDINAL)
            || format_returned != 32)
        g_warning("Can not calculate _NET_NUMBER_OF_DESKTOPS");

    /* Note : cdk_property_get() is broken (API documents admit
     * this).  As a length argument, it expects the number of
     * _bytes_ of data you require.  Internally, cdk_property_get
     * converts that value to a count of 32 bit (4 byte) elements.
     * However, the length returned is in bytes, but is calculated
     * via the count of returned elements * sizeof(long).  This
     * means on a 64 bit system, the number of bytes you have to
     * request does not correspond to the number of bytes you get
     * back, and is the reason for the workaround below.
     */
    cdk_x11_display_error_trap_push (display);
    if (nworkareas == NULL || (*nworkareas < 1)
            || !cdk_property_get (window,
                                  cdk_atom_intern ("_NET_WORKAREA", FALSE),
                                  cdk_x11_xatom_to_atom (XA_CARDINAL),
                                  0, ((*nworkareas) * 4 * 4), FALSE,
                                  &type_returned,
                                  &format_returned,
                                  &length_returned,
                                  (guchar **) &workareas))
    {
        g_warning("Can not get _NET_WORKAREA");
        workareas = NULL;
    }

    if (cdk_x11_display_error_trap_pop (display)
            || workareas == NULL
            || type_returned != cdk_x11_xatom_to_atom (XA_CARDINAL)
            || ((*nworkareas) * 4 * sizeof(long)) != length_returned
            || format_returned != 32)
    {
        g_warning("Can not determine workarea, guessing at layout");
        baul_icon_container_set_margins (icon_container,
                                         0, 0, 0, 0);
    }
    else
    {
        CdkScreen *screen;

        screen = cdk_window_get_screen (window);

        icon_container_set_workarea (
            icon_container, screen, workareas, length_returned / sizeof (long));
    }

    if (nworkareas != NULL)
        g_free (nworkareas);

    if (workareas != NULL)
        g_free (workareas);
}

static CdkFilterReturn
desktop_icon_view_property_filter (CdkXEvent *cdk_xevent,
                                   CdkEvent *event,
                                   gpointer data)
{
    XEvent *xevent = cdk_xevent;
    FMDesktopIconView *icon_view;

    icon_view = FM_DESKTOP_ICON_VIEW (data);

    switch (xevent->type)
    {
    case PropertyNotify:
        if (xevent->xproperty.atom == cdk_x11_get_xatom_by_name ("_NET_WORKAREA"))
            net_workarea_changed (icon_view, event->any.window);
        break;
    default:
        break;
    }

    return CDK_FILTER_CONTINUE;
}

static void
fm_desktop_icon_view_dispose (GObject *object)
{
    FMDesktopIconView *icon_view;
    CtkUIManager *ui_manager;

    icon_view = FM_DESKTOP_ICON_VIEW (object);

    /* Remove desktop rescan timeout. */
    if (icon_view->priv->reload_desktop_timeout != 0)
    {
        g_source_remove (icon_view->priv->reload_desktop_timeout);
        icon_view->priv->reload_desktop_timeout = 0;
    }

    ui_manager = fm_directory_view_get_ui_manager (FM_DIRECTORY_VIEW (icon_view));
    if (ui_manager != NULL)
    {
        baul_ui_unmerge_ui (ui_manager,
                            &icon_view->priv->desktop_merge_id,
                            &icon_view->priv->desktop_action_group);
    }

    g_signal_handlers_disconnect_by_func (baul_icon_view_preferences,
                                          default_zoom_level_changed,
                                          icon_view);
    g_signal_handlers_disconnect_by_func (baul_desktop_preferences,
                                          font_changed_callback,
                                          icon_view);

    g_signal_handlers_disconnect_by_func (cafe_lockdown_preferences,
                                          fm_directory_view_update_menus,
                                          icon_view);
    g_signal_handlers_disconnect_by_func (baul_preferences,
                                          desktop_directory_changed_callback,
                                          NULL);

    G_OBJECT_CLASS (fm_desktop_icon_view_parent_class)->dispose (object);
}

static void
fm_desktop_icon_view_class_init (FMDesktopIconViewClass *class)
{
    G_OBJECT_CLASS (class)->dispose = fm_desktop_icon_view_dispose;

    FM_DIRECTORY_VIEW_CLASS (class)->merge_menus = real_merge_menus;
    FM_DIRECTORY_VIEW_CLASS (class)->update_menus = real_update_menus;
    FM_DIRECTORY_VIEW_CLASS (class)->supports_zooming = real_supports_zooming;

    FM_ICON_VIEW_CLASS (class)->supports_auto_layout = real_supports_auto_layout;
    FM_ICON_VIEW_CLASS (class)->supports_scaling = real_supports_scaling;
    FM_ICON_VIEW_CLASS (class)->supports_keep_aligned = real_supports_keep_aligned;
    FM_ICON_VIEW_CLASS (class)->supports_labels_beside_icons = real_supports_labels_beside_icons;
}

static void
fm_desktop_icon_view_handle_middle_click (BaulIconContainer *icon_container,
					  CdkEventButton    *event,
					  FMDesktopIconView *desktop_icon_view G_GNUC_UNUSED)
{
    XButtonEvent x_event;
    CdkDevice *keyboard = NULL, *pointer = NULL;
    CdkSeat *seat;
    CdkDisplay *display;

    seat = cdk_display_get_default_seat (ctk_widget_get_display (CTK_WIDGET (icon_container)));
    pointer = cdk_seat_get_pointer (seat);
    keyboard = cdk_seat_get_keyboard (seat);

    /* During a mouse click we have the pointer and keyboard grab.
     * We will send a fake event to the root window which will cause it
     * to try to get the grab so we need to let go ourselves.
     */

    if (pointer != NULL) {
            cdk_seat_ungrab (seat);
    }

    if (keyboard != NULL) {
            cdk_seat_ungrab (seat);
    }

    /* Stop the event because we don't want anyone else dealing with it. */
    display = ctk_widget_get_display (CTK_WIDGET (icon_container));
    cdk_display_flush (display);
    g_signal_stop_emission_by_name (icon_container, "middle_click");

    /* build an X event to represent the middle click. */
    x_event.type = ButtonPress;
    x_event.send_event = True;
    x_event.display = CDK_DISPLAY_XDISPLAY (cdk_display_get_default ());
    x_event.window = CDK_ROOT_WINDOW ();
    x_event.root = CDK_ROOT_WINDOW ();
    x_event.subwindow = 0;
    x_event.time = event->time;
    x_event.x = event->x;
    x_event.y = event->y;
    x_event.x_root = event->x_root;
    x_event.y_root = event->y_root;
    x_event.state = event->state;
    x_event.button = event->button;
    x_event.same_screen = True;

    /* Send it to the root window, the window manager will handle it. */
    XSendEvent (CDK_DISPLAY_XDISPLAY (cdk_display_get_default ()), CDK_ROOT_WINDOW (), True,
                ButtonPressMask, (XEvent *) &x_event);
}

static void
unrealized_callback (CtkWidget         *widget G_GNUC_UNUSED,
		     FMDesktopIconView *desktop_icon_view)
{
    g_return_if_fail (desktop_icon_view->priv->root_window != NULL);

    /* Remove the property filter */
    cdk_window_remove_filter (desktop_icon_view->priv->root_window,
                              desktop_icon_view_property_filter,
                              desktop_icon_view);
    desktop_icon_view->priv->root_window = NULL;
}

static void
realized_callback (CtkWidget *widget, FMDesktopIconView *desktop_icon_view)
{
    CdkWindow *root_window;
    CdkScreen *screen;
    CtkAllocation allocation;
    gint scale;

    g_return_if_fail (desktop_icon_view->priv->root_window == NULL);

    screen = ctk_widget_get_screen (widget);
    scale = ctk_widget_get_scale_factor (widget);

    /* Ugly HACK for the problem that the views realize at the
     * wrong size and then get resized. (This is a problem with
     * CafeComponentPlug.) This was leading to problems where initial
     * layout was done at 60x60 stacking all desktop icons in
     * the top left corner.
     */
    allocation.x = 0;
    allocation.y = 0;
    allocation.width = WidthOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale;
    allocation.height = HeightOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale;
    ctk_widget_size_allocate (CTK_WIDGET(get_icon_container(desktop_icon_view)),
                              &allocation);

    root_window = cdk_screen_get_root_window (screen);

    desktop_icon_view->priv->root_window = root_window;

    /* Read out the workarea geometry and update the icon container accordingly */
    net_workarea_changed (desktop_icon_view, root_window);

    /* Setup the property filter */
    cdk_window_set_events (root_window, CDK_PROPERTY_CHANGE_MASK);
    cdk_window_add_filter (root_window,
                           desktop_icon_view_property_filter,
                           desktop_icon_view);

}

static BaulZoomLevel
get_default_zoom_level (void)
{
    static gboolean auto_storage_added = FALSE;
    static BaulZoomLevel default_zoom_level = BAUL_ZOOM_LEVEL_STANDARD;

    if (!auto_storage_added)
    {
        auto_storage_added = TRUE;
        eel_g_settings_add_auto_enum (baul_icon_view_preferences,
                                      BAUL_PREFERENCES_ICON_VIEW_DEFAULT_ZOOM_LEVEL,
                                      (int *) &default_zoom_level);
    }

    return CLAMP (default_zoom_level, BAUL_ZOOM_LEVEL_SMALLEST, BAUL_ZOOM_LEVEL_LARGEST);
}

static void
default_zoom_level_changed (gpointer user_data)
{
    BaulZoomLevel new_level;
    FMDesktopIconView *desktop_icon_view;

    desktop_icon_view = FM_DESKTOP_ICON_VIEW (user_data);
    new_level = get_default_zoom_level ();

    baul_icon_container_set_zoom_level (get_icon_container (desktop_icon_view),
                                        new_level);
}

static gboolean
do_desktop_rescan (gpointer data)
{
    FMDesktopIconView *desktop_icon_view;
    struct stat buf;

    desktop_icon_view = FM_DESKTOP_ICON_VIEW (data);
    if (desktop_icon_view->priv->pending_rescan)
    {
        return TRUE;
    }

    if (stat (desktop_directory, &buf) == -1)
    {
        return TRUE;
    }

    if (buf.st_ctime == desktop_dir_modify_time)
    {
        return TRUE;
    }

    desktop_icon_view->priv->pending_rescan = TRUE;

    baul_directory_force_reload (
        fm_directory_view_get_model (
            FM_DIRECTORY_VIEW (desktop_icon_view)));
    return TRUE;
}

static void
done_loading (BaulDirectory     *model G_GNUC_UNUSED,
	      FMDesktopIconView *desktop_icon_view)
{
    struct stat buf;

    desktop_icon_view->priv->pending_rescan = FALSE;
    if (stat (desktop_directory, &buf) == -1)
    {
        return;
    }

    desktop_dir_modify_time = buf.st_ctime;
}

/* This function is used because the BaulDirectory model does not
 * exist always in the desktop_icon_view, so we wait until it has been
 * instantiated.
 */
static void
delayed_init (FMDesktopIconView *desktop_icon_view)
{
    /* Keep track of the load time. */
    g_signal_connect_object (fm_directory_view_get_model (FM_DIRECTORY_VIEW (desktop_icon_view)),
                             "done_loading",
                             G_CALLBACK (done_loading), desktop_icon_view, 0);

    /* Monitor desktop directory. */
    desktop_icon_view->priv->reload_desktop_timeout =
        g_timeout_add_seconds (RESCAN_TIMEOUT, do_desktop_rescan, desktop_icon_view);

    g_signal_handler_disconnect (desktop_icon_view,
                                 desktop_icon_view->priv->delayed_init_signal);

    desktop_icon_view->priv->delayed_init_signal = 0;
}

static void
font_changed_callback (gpointer callback_data)
{
    g_return_if_fail (FM_IS_DESKTOP_ICON_VIEW (callback_data));

    fm_desktop_icon_view_update_icon_container_fonts (FM_DESKTOP_ICON_VIEW (callback_data));
}

static void
fm_desktop_icon_view_update_icon_container_fonts (FMDesktopIconView *icon_view)
{
    BaulIconContainer *icon_container;
    char *font;

    icon_container = get_icon_container (icon_view);
    g_assert (icon_container != NULL);

    font = g_settings_get_string (baul_desktop_preferences, BAUL_PREFERENCES_DESKTOP_FONT);

    baul_icon_container_set_font (icon_container, font);

    g_free (font);
}

static void
fm_desktop_icon_view_init (FMDesktopIconView *desktop_icon_view)
{
    BaulIconContainer *icon_container;
    CtkAllocation allocation;
    CtkAdjustment *hadj, *vadj;

    desktop_icon_view->priv = fm_desktop_icon_view_get_instance_private (desktop_icon_view);

    if (desktop_directory == NULL)
    {
        g_signal_connect_swapped (baul_preferences, "changed::" BAUL_PREFERENCES_DESKTOP_IS_HOME_DIR,
                                  G_CALLBACK(desktop_directory_changed_callback),
                                  NULL);
        desktop_directory_changed_callback (NULL);
    }

    fm_icon_view_filter_by_screen (FM_ICON_VIEW (desktop_icon_view), TRUE);
    icon_container = get_icon_container (desktop_icon_view);
    baul_icon_container_set_use_drop_shadows (icon_container, TRUE);
    fm_icon_container_set_sort_desktop (FM_ICON_CONTAINER (icon_container), TRUE);

    /* Do a reload on the desktop if we don't have FAM, a smarter
     * way to keep track of the items on the desktop.
     */
    if (!baul_monitor_active ())
    {
        desktop_icon_view->priv->delayed_init_signal = g_signal_connect_object
                (desktop_icon_view, "begin_loading",
                 G_CALLBACK (delayed_init), desktop_icon_view, 0);
    }

    baul_icon_container_set_is_fixed_size (icon_container, TRUE);
    baul_icon_container_set_is_desktop (icon_container, TRUE);
    baul_icon_container_set_store_layout_timestamps (icon_container, TRUE);

    /* Set allocation to be at 0, 0 */
    ctk_widget_get_allocation (CTK_WIDGET (icon_container), &allocation);
    allocation.x = 0;
    allocation.y = 0;
    ctk_widget_set_allocation (CTK_WIDGET (icon_container), &allocation);

    ctk_widget_queue_resize (CTK_WIDGET (icon_container));

    hadj = ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (icon_container));
    vadj = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (icon_container));

    ctk_adjustment_set_value (hadj, 0);
    ctk_adjustment_set_value (vadj, 0);

    ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (desktop_icon_view),
                                         CTK_SHADOW_NONE);
    ctk_scrolled_window_set_overlay_scrolling (CTK_SCROLLED_WINDOW (desktop_icon_view), FALSE);

    fm_directory_view_ignore_hidden_file_preferences
    (FM_DIRECTORY_VIEW (desktop_icon_view));

    fm_directory_view_set_show_foreign (FM_DIRECTORY_VIEW (desktop_icon_view),
                                        FALSE);

    /* Set our default layout mode */
    baul_icon_container_set_layout_mode (icon_container,
                                         ctk_widget_get_direction (CTK_WIDGET(icon_container)) == CTK_TEXT_DIR_RTL ?
                                         BAUL_ICON_LAYOUT_T_B_R_L :
                                         BAUL_ICON_LAYOUT_T_B_L_R);

    g_signal_connect_object (icon_container, "middle_click",
                             G_CALLBACK (fm_desktop_icon_view_handle_middle_click), desktop_icon_view, 0);
    g_signal_connect_object (desktop_icon_view, "realize",
                             G_CALLBACK (realized_callback), desktop_icon_view, 0);
    g_signal_connect_object (desktop_icon_view, "unrealize",
                             G_CALLBACK (unrealized_callback), desktop_icon_view, 0);

    default_zoom_level_changed (desktop_icon_view);

    g_signal_connect_swapped (baul_icon_view_preferences,
                              "changed::" BAUL_PREFERENCES_ICON_VIEW_DEFAULT_ZOOM_LEVEL,
                              G_CALLBACK (default_zoom_level_changed),
                              desktop_icon_view);

    g_signal_connect_swapped (baul_desktop_preferences,
                              "changed::" BAUL_PREFERENCES_DESKTOP_FONT,
                              G_CALLBACK (font_changed_callback),
                              desktop_icon_view);

    fm_desktop_icon_view_update_icon_container_fonts (desktop_icon_view);

    g_signal_connect_swapped (cafe_lockdown_preferences,
                              "changed::" BAUL_PREFERENCES_LOCKDOWN_COMMAND_LINE,
                              G_CALLBACK (fm_directory_view_update_menus),
                              desktop_icon_view);

}

static void
action_new_launcher_callback (CtkAction *action G_GNUC_UNUSED,
			      gpointer   data)
{
    char *desktop_directory;

    g_assert (FM_DIRECTORY_VIEW (data));

    desktop_directory = baul_get_desktop_directory ();

    baul_launch_application_from_command (ctk_widget_get_screen (CTK_WIDGET (data)),
                                          "cafe-desktop-item-edit",
                                          "cafe-desktop-item-edit",
                                          FALSE,
                                          "--create-new", desktop_directory, NULL);
    g_free (desktop_directory);

}

static void
action_change_background_callback (CtkAction *action G_GNUC_UNUSED,
				   gpointer   data)
{
    g_assert (FM_DIRECTORY_VIEW (data));

    baul_launch_application_from_command (ctk_widget_get_screen (CTK_WIDGET (data)),
                                          _("Background"),
                                          "cafe-appearance-properties",
                                          FALSE,
                                          "--show-page=background", NULL);
}

static void
action_empty_trash_conditional_callback (CtkAction *action G_GNUC_UNUSED,
					 gpointer   data)
{
    g_assert (FM_IS_DIRECTORY_VIEW (data));

    baul_file_operations_empty_trash (CTK_WIDGET (data));
}

static gboolean
trash_link_is_selection (FMDirectoryView *view)
{
    GList *selection;
    gboolean result;

    result = FALSE;

    selection = fm_directory_view_get_selection (view);

    if (eel_g_list_exactly_one_item (selection) &&
            BAUL_IS_DESKTOP_ICON_FILE (selection->data))
    {
        BaulDesktopLink *link;

        link = baul_desktop_icon_file_get_link (BAUL_DESKTOP_ICON_FILE (selection->data));
        /* link may be NULL if the link was recently removed (unmounted) */
        if (link != NULL &&
                baul_desktop_link_get_link_type (link) == BAUL_DESKTOP_LINK_TRASH)
        {
            result = TRUE;
        }
        if (link)
        {
            g_object_unref (link);
        }
    }

    baul_file_list_free (selection);

    return result;
}

static void
real_update_menus (FMDirectoryView *view)
{
    FMDesktopIconView *desktop_view;
    gboolean disable_command_line;
    gboolean include_empty_trash;
    CtkAction *action;

    g_assert (FM_IS_DESKTOP_ICON_VIEW (view));

    FM_DIRECTORY_VIEW_CLASS (fm_desktop_icon_view_parent_class)->update_menus (view);

    desktop_view = FM_DESKTOP_ICON_VIEW (view);

    /* New Launcher */
    disable_command_line = g_settings_get_boolean (cafe_lockdown_preferences, BAUL_PREFERENCES_LOCKDOWN_COMMAND_LINE);
    action = ctk_action_group_get_action (desktop_view->priv->desktop_action_group,
                                          FM_ACTION_NEW_LAUNCHER_DESKTOP);
    ctk_action_set_visible (action,
                            !disable_command_line);

    /* Empty Trash */
    include_empty_trash = trash_link_is_selection (view);
    action = ctk_action_group_get_action (desktop_view->priv->desktop_action_group,
                                          FM_ACTION_EMPTY_TRASH_CONDITIONAL);
    ctk_action_set_visible (action,
                            include_empty_trash);
    if (include_empty_trash)
    {
        char *label;

        label = g_strdup (_("E_mpty Trash"));
        g_object_set (action , "label", label, NULL);
        ctk_action_set_sensitive (action,
                                  !baul_trash_monitor_is_empty ());
        g_free (label);
    }
}

static const CtkActionEntry desktop_view_entries[] =
{
    /* name, stock id */
    {
        "New Launcher Desktop", NULL,
        /* label, accelerator */
        N_("Create L_auncher..."), NULL,
        /* tooltip */
        N_("Create a new launcher"),
        G_CALLBACK (action_new_launcher_callback)
    },
    /* name, stock id */
    {
        "Change Background", NULL,
        /* label, accelerator */
        N_("Change Desktop _Background"), NULL,
        /* tooltip */
        N_("Show a window that lets you set your desktop background's pattern or color"),
        G_CALLBACK (action_change_background_callback)
    },
    /* name, stock id */
    {
        "Empty Trash Conditional", NULL,
        /* label, accelerator */
        N_("Empty Trash"), NULL,
        /* tooltip */
        N_("Delete all items in the Trash"),
        G_CALLBACK (action_empty_trash_conditional_callback)
    },
};

static void
real_merge_menus (FMDirectoryView *view)
{
    FMDesktopIconView *desktop_view;
    CtkUIManager *ui_manager;
    CtkActionGroup *action_group;
    const char *ui;

    FM_DIRECTORY_VIEW_CLASS (fm_desktop_icon_view_parent_class)->merge_menus (view);

    desktop_view = FM_DESKTOP_ICON_VIEW (view);

    ui_manager = fm_directory_view_get_ui_manager (view);

    action_group = ctk_action_group_new ("DesktopViewActions");
    ctk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    desktop_view->priv->desktop_action_group = action_group;
    ctk_action_group_add_actions (action_group,
                                  desktop_view_entries, G_N_ELEMENTS (desktop_view_entries),
                                  view);


    ctk_ui_manager_insert_action_group (ui_manager, action_group, 0);
    g_object_unref (action_group); /* owned by ui manager */

    ui = baul_ui_string_get ("baul-desktop-icon-view-ui.xml");
    desktop_view->priv->desktop_merge_id =
        ctk_ui_manager_add_ui_from_string (ui_manager, ui, -1, NULL);
}

static gboolean
real_supports_auto_layout (FMIconView *view G_GNUC_UNUSED)
{
    /* Can't use auto-layout on the desktop, because doing so
     * would cause all sorts of complications involving the
     * fixed-size window.
     */
    return FALSE;
}

static gboolean
real_supports_scaling (FMIconView *view G_GNUC_UNUSED)
{
    return TRUE;
}

static gboolean
real_supports_keep_aligned (FMIconView *view G_GNUC_UNUSED)
{
    return TRUE;
}

static gboolean
real_supports_labels_beside_icons (FMIconView *view G_GNUC_UNUSED)
{
    return FALSE;
}

static gboolean
real_supports_zooming (FMDirectoryView *view G_GNUC_UNUSED)
{
    /* Can't zoom on the desktop, because doing so would cause all
     * sorts of complications involving the fixed-size window.
     */
    return FALSE;
}

static BaulView *
fm_desktop_icon_view_create (BaulWindowSlotInfo *slot)
{
    FMIconView *view;

    view = g_object_new (FM_TYPE_DESKTOP_ICON_VIEW,
                         "window-slot", slot,
                         NULL);
    return BAUL_VIEW (view);
}

static gboolean
fm_desktop_icon_view_supports_uri (const char *uri,
				   GFileType   file_type G_GNUC_UNUSED,
				   const char *mime_type G_GNUC_UNUSED)
{
    if (g_str_has_prefix (uri, EEL_DESKTOP_URI))
    {
        return TRUE;
    }

    return FALSE;
}

static BaulViewInfo fm_desktop_icon_view =
{
    .id = FM_DESKTOP_ICON_VIEW_ID,
    .view_combo_label = N_("Desktop View"),
    .view_menu_label_with_mnemonic = N_("_Desktop"),
    .error_label = N_("The desktop view encountered an error."),
    .startup_error_label = N_("The desktop view encountered an error while starting up."),
    .display_location_label = N_("Display this location with the desktop view."),
    .create = fm_desktop_icon_view_create,
    .supports_uri = fm_desktop_icon_view_supports_uri
};

void
fm_desktop_icon_view_register (void)
{
    fm_desktop_icon_view.error_label = _(fm_desktop_icon_view.error_label);
    fm_desktop_icon_view.startup_error_label = _(fm_desktop_icon_view.startup_error_label);

    baul_view_factory_register (&fm_desktop_icon_view);
}
