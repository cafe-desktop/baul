/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Baul
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 1999, 2000, 2001 Eazel, Inc.
 *
 *  Baul is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  Baul is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Authors: Elliot Lee <sopwith@redhat.com>
 *  	     John Sullivan <sullivan@eazel.com>
 *
 */

/* baul-window.c: Implementation of the main window object */

#include <config.h>

#include <cdk/cdkkeysyms.h>
#include <cdk/cdkx.h>
#include <ctk/ctk.h>
#include <glib/gi18n.h>

#include <eel/eel-ctk-extensions.h>
#include <eel/eel-ctk-macros.h>
#include <eel/eel-string.h>

#include <libbaul-private/baul-dnd.h>
#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-ui-utilities.h>
#include <libbaul-private/baul-file-attributes.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-metadata.h>
#include <libbaul-private/baul-mime-actions.h>
#include <libbaul-private/baul-program-choosing.h>
#include <libbaul-private/baul-search-directory.h>
#include <libbaul-private/baul-search-engine.h>
#include <libbaul-private/baul-signaller.h>

#include "baul-spatial-window.h"
#include "baul-window-private.h"
#include "baul-window-bookmarks.h"
#include "baul-actions.h"
#include "baul-application.h"
#include "baul-desktop-window.h"
#include "baul-bookmarks-window.h"
#include "baul-location-dialog.h"
#include "baul-query-editor.h"
#include "baul-search-bar.h"
#include "baul-window-manage-views.h"

#define MAX_TITLE_LENGTH 180
#define MAX_SHORTNAME_PATH 16

#define SPATIAL_ACTION_PLACES               "Places"
#define SPATIAL_ACTION_GO_TO_LOCATION       "Go to Location"
#define SPATIAL_ACTION_CLOSE_PARENT_FOLDERS "Close Parent Folders"
#define SPATIAL_ACTION_CLOSE_ALL_FOLDERS    "Close All Folders"
#define MENU_PATH_SPATIAL_BOOKMARKS_PLACEHOLDER	"/MenuBar/Other Menus/Places/Bookmarks Placeholder"

struct _BaulSpatialWindowPrivate
{
    CtkActionGroup *spatial_action_group; /* owned by ui_manager */
    char *last_geometry;
    guint save_geometry_timeout_id;

    gboolean saved_data_on_close;
    CtkWidget *content_box;
    CtkWidget *location_button;
    CtkWidget *location_label;
    CtkWidget *location_icon;
};

static const CtkTargetEntry location_button_drag_types[] =
{
    { BAUL_ICON_DND_CAFE_ICON_LIST_TYPE, 0, BAUL_ICON_DND_CAFE_ICON_LIST },
    { BAUL_ICON_DND_URI_LIST_TYPE, 0, BAUL_ICON_DND_URI_LIST },
};

G_DEFINE_TYPE_WITH_PRIVATE (BaulSpatialWindow, baul_spatial_window, BAUL_TYPE_WINDOW)

static void baul_spatial_window_save_geometry (BaulSpatialWindow *window,
						   BaulFile *viewed_file);

static gboolean
save_window_geometry_timeout (gpointer callback_data)
{
    BaulSpatialWindow *window;
    BaulWindowSlot *slot;

    window = BAUL_SPATIAL_WINDOW (callback_data);
    slot = baul_window_get_active_slot (BAUL_WINDOW (window));

    if (slot != NULL)
    {
        baul_spatial_window_save_geometry (window, slot->viewed_file);
    }

    window->details->save_geometry_timeout_id = 0;

    return FALSE;
}

static gboolean
baul_spatial_window_configure_event (CtkWidget *widget,
                                     CdkEventConfigure *event)
{
    BaulSpatialWindow *window;

    window = BAUL_SPATIAL_WINDOW (widget);

    CTK_WIDGET_CLASS (baul_spatial_window_parent_class)->configure_event (widget, event);

    /* Only save the geometry if the user hasn't resized the window
     * for a second. Otherwise delay the callback another second.
     */
    if (window->details->save_geometry_timeout_id != 0)
    {
        g_source_remove (window->details->save_geometry_timeout_id);
    }

	window->details->save_geometry_timeout_id =
		g_timeout_add_seconds (1, save_window_geometry_timeout, window);

    return FALSE;
}

static void
baul_spatial_window_unrealize (CtkWidget *widget)
{
    BaulSpatialWindow *window;
    BaulWindowSlot *slot;

    window = BAUL_SPATIAL_WINDOW (widget);
    slot = baul_window_get_active_slot (BAUL_WINDOW (window));

    CTK_WIDGET_CLASS (baul_spatial_window_parent_class)->unrealize (widget);

    if (window->details->save_geometry_timeout_id != 0)
    {
        g_source_remove (window->details->save_geometry_timeout_id);
        window->details->save_geometry_timeout_id = 0;

        if (slot != NULL)
        {
            baul_spatial_window_save_geometry (window, slot->viewed_file);
        }
    }
}

static gboolean
baul_spatial_window_state_event (CtkWidget *widget,
                                 CdkEventWindowState *event)
{
    BaulWindow *window;
    BaulWindowSlot *slot;
    BaulFile *viewed_file;

    window = BAUL_WINDOW (widget);
    slot = window->details->active_pane->active_slot;
    viewed_file = slot->viewed_file;

    if (!BAUL_IS_DESKTOP_WINDOW (widget))
    {

        if (event->changed_mask & CDK_WINDOW_STATE_MAXIMIZED &&
                viewed_file != NULL)
        {
            baul_file_set_boolean_metadata (viewed_file,
                                            BAUL_METADATA_KEY_WINDOW_MAXIMIZED,
                                            FALSE,
                                            event->new_window_state & CDK_WINDOW_STATE_MAXIMIZED);
        }

        if (event->changed_mask & CDK_WINDOW_STATE_STICKY &&
                viewed_file != NULL)
        {
            baul_file_set_boolean_metadata (viewed_file,
                                            BAUL_METADATA_KEY_WINDOW_STICKY,
                                            FALSE,
                                            event->new_window_state & CDK_WINDOW_STATE_STICKY);
        }

        if (event->changed_mask & CDK_WINDOW_STATE_ABOVE &&
                viewed_file != NULL)
        {
            baul_file_set_boolean_metadata (viewed_file,
                                            BAUL_METADATA_KEY_WINDOW_KEEP_ABOVE,
                                            FALSE,
                                            event->new_window_state & CDK_WINDOW_STATE_ABOVE);
        }

    }

    if (CTK_WIDGET_CLASS (baul_spatial_window_parent_class)->window_state_event != NULL)
    {
        return CTK_WIDGET_CLASS (baul_spatial_window_parent_class)->window_state_event (widget, event);
    }

    return FALSE;
}

static void
baul_spatial_window_finalize (GObject *object)
{
    BaulSpatialWindow *window;

    window = BAUL_SPATIAL_WINDOW (object);

    g_free (window->details->last_geometry);

    G_OBJECT_CLASS (baul_spatial_window_parent_class)->finalize (object);
}

static void
baul_spatial_window_save_geometry (BaulSpatialWindow *window,
        		   BaulFile *viewed_file)
{
    char *geometry_string;

    if (viewed_file == NULL)
    {
        /* We never showed a file */
        return;
    }

    if (ctk_widget_get_window (CTK_WIDGET (window)) &&
    	    ctk_widget_get_visible (CTK_WIDGET (window)) &&
	    !BAUL_IS_DESKTOP_WINDOW (window) &&
            !(cdk_window_get_state (ctk_widget_get_window (CTK_WIDGET(window))) & CDK_WINDOW_STATE_MAXIMIZED)) {

        geometry_string = eel_ctk_window_get_geometry_string (CTK_WINDOW (window));

        if (!g_strcmp0 (window->details->last_geometry, geometry_string)) {
        	/* Don't save geometry if it's the same as before. */
        	g_free (geometry_string);
        	return;
        }

        g_free (window->details->last_geometry);
        window->details->last_geometry = geometry_string;

        baul_file_set_metadata (viewed_file,
                                BAUL_METADATA_KEY_WINDOW_GEOMETRY,
                                NULL,
                                geometry_string);
    }
}

static void
baul_spatial_window_save_scroll_position (BaulSpatialWindow *window G_GNUC_UNUSED,
					  BaulWindowSlot    *slot)
{
    char *scroll_string;

    if (slot->content_view == NULL ||
            slot->viewed_file == NULL)
    {
        return;
    }

    scroll_string = baul_view_get_first_visible_file (slot->content_view);
    baul_file_set_metadata (slot->viewed_file,
                            BAUL_METADATA_KEY_WINDOW_SCROLL_POSITION,
                            NULL,
                            scroll_string);
    g_free (scroll_string);
}

static void
baul_spatial_window_save_show_hidden_files_mode (BaulSpatialWindow *window,
						 BaulFile *viewed_file)
{
    BaulWindowShowHiddenFilesMode mode;

    if (viewed_file == NULL) {
        return;
    }

    mode = BAUL_WINDOW (window)->details->show_hidden_files_mode;

    if (mode != BAUL_WINDOW_SHOW_HIDDEN_FILES_DEFAULT) {
        char *show_hidden_file_setting;

        if (mode == BAUL_WINDOW_SHOW_HIDDEN_FILES_ENABLE) {
            show_hidden_file_setting = "1";
        } else {
            show_hidden_file_setting = "0";
        }
        baul_file_set_metadata (viewed_file,
                                BAUL_METADATA_KEY_WINDOW_SHOW_HIDDEN_FILES,
                                NULL,
                                show_hidden_file_setting);
    }
}

static void
baul_spatial_window_show (CtkWidget *widget)
{
    BaulWindow *window;
    BaulWindowSlot *slot;
    GFile *location;

    window = BAUL_WINDOW (widget);
    slot = baul_window_get_active_slot (window);

    CTK_WIDGET_CLASS (baul_spatial_window_parent_class)->show (widget);

    if (slot != NULL && slot->query_editor != NULL)
    {
        baul_query_editor_grab_focus (BAUL_QUERY_EDITOR (slot->query_editor));
    }

    location = baul_window_slot_get_location (slot);
    g_return_if_fail (location != NULL);

    while (location != NULL) {
        BaulFile *file;

        file = baul_file_get (location);

        if  (!baul_file_check_if_ready (file, BAUL_FILE_ATTRIBUTE_INFO)) {
            baul_file_call_when_ready (file,
                                       BAUL_FILE_ATTRIBUTE_INFO,
                                       NULL,
                                       NULL);
        }

        location = g_file_get_parent (location);
    }

    if (location) {
        g_object_unref (location);
    }
}

static void
action_close_parent_folders_callback (CtkAction *action G_GNUC_UNUSED,
				      gpointer   user_data)
{
    baul_application_close_parent_windows (BAUL_SPATIAL_WINDOW (user_data));
}

static void
action_close_all_folders_callback (CtkAction *action G_GNUC_UNUSED,
				   gpointer   user_data G_GNUC_UNUSED)
{
    baul_application_close_all_spatial_windows ();
}

static void
real_prompt_for_location (BaulWindow *window,
                          const char     *initial)
{
    CtkWidget *dialog;

    dialog = baul_location_dialog_new (window);
    if (initial != NULL)
    {
        baul_location_dialog_set_location (BAUL_LOCATION_DIALOG (dialog),
                                           initial);
    }

    ctk_widget_show (dialog);
}

static BaulIconInfo *
real_get_icon (BaulWindow *window,
               BaulWindowSlot *slot)
{
    return baul_file_get_icon (slot->viewed_file,
                               48, ctk_widget_get_scale_factor (CTK_WIDGET (window)),
                               BAUL_FILE_ICON_FLAGS_IGNORE_VISITING |
                               BAUL_FILE_ICON_FLAGS_USE_MOUNT_ICON);
}

static void
sync_window_title (BaulWindow *window)
{
    BaulWindowSlot *slot;

    slot = baul_window_get_active_slot (window);

    /* Don't change desktop's title, it would override the one already defined */
    if (BAUL_IS_DESKTOP_WINDOW (window))
        return;

    if (slot->title == NULL || slot->title[0] == '\0')
    {
        ctk_window_set_title (CTK_WINDOW (window), _("Baul"));
    }
    else
    {
        char *window_title;

        window_title = eel_str_middle_truncate (slot->title, MAX_TITLE_LENGTH);
        ctk_window_set_title (CTK_WINDOW (window), window_title);
        g_free (window_title);
    }
}

static void
real_sync_title (BaulWindow *window,
                 BaulWindowSlot *slot)
{
    g_assert (slot == baul_window_get_active_slot (window));

    sync_window_title (window);
}

static void
real_get_min_size (BaulWindow *window G_GNUC_UNUSED,
		   guint      *min_width,
		   guint      *min_height)
{
    if (min_width)
    {
        *min_width = BAUL_SPATIAL_WINDOW_MIN_WIDTH;
    }
    if (min_height)
    {
        *min_height = BAUL_SPATIAL_WINDOW_MIN_HEIGHT;
    }
}

static void
real_get_default_size (BaulWindow *window G_GNUC_UNUSED,
		       guint      *default_width,
		       guint      *default_height)
{
    if (default_width)
    {
        *default_width = BAUL_SPATIAL_WINDOW_DEFAULT_WIDTH;
    }
    if (default_height)
    {
        *default_height = BAUL_SPATIAL_WINDOW_DEFAULT_HEIGHT;
    }
}

static void
real_sync_allow_stop (BaulWindow     *window G_GNUC_UNUSED,
		      BaulWindowSlot *slot G_GNUC_UNUSED)
{
}

static void
real_set_allow_up (BaulWindow *window, gboolean allow)
{
    BaulSpatialWindow *spatial;
    CtkAction *action;

    spatial = BAUL_SPATIAL_WINDOW (window);

    action = ctk_action_group_get_action (spatial->details->spatial_action_group,
                                          SPATIAL_ACTION_CLOSE_PARENT_FOLDERS);
    ctk_action_set_sensitive (action, allow);

    BAUL_WINDOW_CLASS (baul_spatial_window_parent_class)->set_allow_up (window, allow);
}

static BaulWindowSlot *
real_open_slot (BaulWindowPane         *pane,
		BaulWindowOpenSlotFlags flags G_GNUC_UNUSED)
{
    BaulWindowSlot *slot;
    GList *slots;

    g_assert (baul_window_get_active_slot (pane->window) == NULL);

    slots = baul_window_get_slots (pane->window);
    g_assert (slots == NULL);
    g_list_free (slots);

    slot = g_object_new (BAUL_TYPE_WINDOW_SLOT, NULL);
    slot->pane = pane;
    ctk_container_add (CTK_CONTAINER (BAUL_SPATIAL_WINDOW (pane->window)->details->content_box),
                       slot->content_box);
    ctk_widget_show (slot->content_box);
    return slot;
}

static void
save_spatial_data (BaulSpatialWindow *window,
		   BaulWindowSlot *slot)
{
    baul_spatial_window_save_geometry (window, slot->viewed_file);
    baul_spatial_window_save_scroll_position (window, slot);
    baul_spatial_window_save_show_hidden_files_mode (window, slot->viewed_file);
}

static void
real_close_slot (BaulWindowPane *pane,
                 BaulWindowSlot *slot)
{
    BaulSpatialWindow *window;

    window = BAUL_SPATIAL_WINDOW (pane->window);

    /* Save spatial data for close if we didn't already */
    if (!window->details->saved_data_on_close) {
        save_spatial_data (window, slot);
    }

    BAUL_WINDOW_CLASS (baul_spatial_window_parent_class)->close_slot (pane, slot);
}

static void
real_window_close (BaulWindow *window)
{
    BaulWindowSlot *slot;
    BaulSpatialWindow *self;

    self = BAUL_SPATIAL_WINDOW (window);

    /* We're closing the window, save the geometry. */
    /* Note that we do this in window close, not slot close, because slot
     * close is too late, by then the widgets have been unrealized.
     * This is for the close by WM case, if you're closing via Ctrl-W that
     * means we close the slots first and this is not an issue */
    slot = baul_window_get_active_slot (window);

    if (slot != NULL) {
        save_spatial_data (self, slot);
        self->details->saved_data_on_close = TRUE;
    }

    if (BAUL_WINDOW_CLASS (baul_spatial_window_parent_class)->close != NULL) {
        BAUL_WINDOW_CLASS (baul_spatial_window_parent_class)->close (window);
    }
}

static void
location_menu_item_activated_callback (CtkWidget *menu_item,
                                       BaulWindow *window)
{
    BaulWindowSlot *slot;
    GFile *current;
    GFile *dest;
    CdkEvent *event;

	slot = baul_window_get_active_slot (window);
	current = baul_window_slot_get_location (slot);
	dest = g_object_get_data (G_OBJECT (menu_item), "location");

    event = ctk_get_current_event();

    if (!g_file_equal (current, dest))
    {
        GFile *child;
        gboolean close_behind;
        GList *selection;

        close_behind = FALSE;
        selection = NULL;

        child = g_object_get_data (G_OBJECT(menu_item), "child_location");
        if (child != NULL) {
            selection = g_list_prepend (NULL, g_object_ref (child));
        }

        if (event != NULL && ((CdkEventAny *) event)->type == CDK_BUTTON_RELEASE &&
                (((CdkEventButton *) event)->button == 2 ||
                 (((CdkEventButton *) event)->state & CDK_SHIFT_MASK) != 0)) {
            close_behind = TRUE;
        }

        baul_window_slot_open_location_with_selection
        (slot, dest, selection, close_behind);

    	g_list_free_full (selection, g_object_unref);
    }

    if (event != NULL) {
        cdk_event_free (event);
    }

    g_object_unref (current);
}

static void
menu_deactivate_callback (CtkWidget *menu G_GNUC_UNUSED,
			  gpointer   data)
{
    GMainLoop *loop;

    loop = data;

    if (g_main_loop_is_running (loop))
    {
        g_main_loop_quit (loop);
    }
}

static gboolean
location_button_pressed_callback (CtkWidget      *widget G_GNUC_UNUSED,
				  CdkEventButton *event,
				  BaulWindow     *window)
{
	BaulWindowSlot *slot;
    BaulView *view;

	slot = baul_window_get_active_slot (window);
	view = slot->content_view;

    if (event->button == 3 && view != NULL)
    {
        baul_view_pop_up_location_context_menu (view, event, NULL);
    }

    return FALSE;
}

static void
location_button_clicked_callback (CtkWidget         *widget,
                                  BaulSpatialWindow *window)
{
    BaulWindowSlot *slot;
    CtkWidget *popup, *menu_item, *first_item = NULL;
    cairo_surface_t *surface = NULL;
    GFile *location;
    GFile *child_location;
    GMainLoop *loop;

    slot = baul_window_get_active_slot (BAUL_WINDOW (window));

    popup = ctk_menu_new ();

    ctk_menu_set_reserve_toggle_size (CTK_MENU (popup), FALSE);

    first_item = NULL;

    location = baul_window_slot_get_location (slot);
    g_return_if_fail (location != NULL);

    child_location = NULL;
    while (location != NULL) {
        BaulFile *file;
        char *name;

        file = baul_file_get (location);

        name = baul_file_get_display_name (file);

        surface = NULL;

        surface = baul_file_get_icon_surface (file,
                                              baul_get_icon_size_for_stock_size (CTK_ICON_SIZE_MENU),
                                              TRUE,
                                              ctk_widget_get_scale_factor (widget),
                                              BAUL_FILE_ICON_FLAGS_IGNORE_VISITING);

        if (surface != NULL)
        {
            menu_item = eel_image_menu_item_new_from_surface (surface, name);
            cairo_surface_destroy (surface);
        }
        else
        {
            menu_item = eel_image_menu_item_new_from_icon ("document-open", name);
        }

        if (first_item == NULL) {
            first_item = menu_item;
        }

        ctk_widget_show (menu_item);
        g_signal_connect (menu_item, "activate",
                          G_CALLBACK (location_menu_item_activated_callback),
                          window);

        g_object_set_data_full (G_OBJECT (menu_item),
                                "location",
                                g_object_ref (location),
                                (GDestroyNotify)g_object_unref);

        if (child_location) {
            g_object_set_data_full (G_OBJECT (menu_item),
                                    "child_location",
                                    g_object_ref (child_location),
                                    (GDestroyNotify)g_object_unref);
        }

        ctk_menu_shell_prepend (CTK_MENU_SHELL (popup), menu_item);

        if (child_location) {
            g_object_unref (child_location);
        }
        child_location = location;
        location = g_file_get_parent (location);
    }

    if (child_location) {
        g_object_unref (child_location);
    }
    if (location) {
        g_object_unref (location);
    }

    ctk_menu_set_screen (CTK_MENU (popup), ctk_widget_get_screen (widget));

    loop = g_main_loop_new (NULL, FALSE);

    g_signal_connect (popup, "deactivate",
                      G_CALLBACK (menu_deactivate_callback),
                      loop);

    ctk_grab_add (popup);
    ctk_menu_popup_at_widget (CTK_MENU (popup),
                              widget,
                              CDK_GRAVITY_SOUTH_WEST,
                              CDK_GRAVITY_NORTH_WEST,
                              NULL);

    ctk_menu_shell_select_item (CTK_MENU_SHELL (popup), first_item);
    g_main_loop_run (loop);
    ctk_grab_remove (popup);
    g_main_loop_unref (loop);
    g_object_ref_sink (popup);
    g_object_unref (popup);
}

static int
get_dnd_icon_size (BaulSpatialWindow *window)
{
	BaulWindowSlot *active_slot;
    BaulView *view;
    BaulZoomLevel zoom_level;

	active_slot = baul_window_get_active_slot (BAUL_WINDOW (window));
	view = active_slot->content_view;

    if (view == NULL)
    {
        return BAUL_ICON_SIZE_STANDARD;
    }
    else
    {
        zoom_level = baul_view_get_zoom_level (view);
        return baul_get_icon_size_for_zoom_level (zoom_level);
    }
}

static void
location_button_drag_begin_callback (CtkWidget             *widget,
                                     CdkDragContext        *context,
                                     BaulSpatialWindow *window)
{
    BaulWindowSlot *slot;
    cairo_surface_t *surface;

    slot = BAUL_WINDOW (window)->details->active_pane->active_slot;

    surface = baul_file_get_icon_surface (slot->viewed_file,
                                          get_dnd_icon_size (window),
                                          FALSE,
                                          ctk_widget_get_scale_factor (widget),
                                          BAUL_FILE_ICON_FLAGS_IGNORE_VISITING | BAUL_FILE_ICON_FLAGS_FOR_DRAG_ACCEPT);

    ctk_drag_set_icon_surface (context, surface);

    cairo_surface_destroy (surface);
}

/* build CAFE icon list, which only contains the window's URI.
 * If we just used URIs, moving the folder to trash
 * wouldn't work */
static void
get_data_binder (BaulDragEachSelectedItemDataGet iteratee,
                 gpointer                            iterator_context,
                 gpointer                            data)
{
    BaulSpatialWindow *window;
    BaulWindowSlot *slot;
    char *location;
    int icon_size;

    g_assert (BAUL_IS_SPATIAL_WINDOW (iterator_context));
    window = BAUL_SPATIAL_WINDOW (iterator_context);

    slot = BAUL_WINDOW (window)->details->active_pane->active_slot;

    location = baul_window_slot_get_location_uri (slot);
    icon_size = get_dnd_icon_size (window);

    iteratee (location,
              0,
              0,
              icon_size,
              icon_size,
              data);

    g_free (location);
}

static void
location_button_drag_data_get_callback (CtkWidget             *widget,
                                        CdkDragContext        *context,
                                        CtkSelectionData      *selection_data,
                                        guint                  info,
                                        guint                  time,
                                        BaulSpatialWindow *window)
{
    baul_drag_drag_data_get (widget, context, selection_data,
                             info, time, window, get_data_binder);
}

void
baul_spatial_window_set_location_button  (BaulSpatialWindow *window,
        GFile                 *location)
{
    if (location != NULL)
    {
        BaulFile *file;
        char *name;
        GError *error;

        file = baul_file_get (location);

        /* FIXME: monitor for name change... */
        name = baul_file_get_display_name (file);
        ctk_label_set_label (CTK_LABEL (window->details->location_label),
                             name);
        g_free (name);
        ctk_widget_set_sensitive (window->details->location_button, TRUE);

        error = baul_file_get_file_info_error (file);
        if (error == NULL)
        {
            cairo_surface_t *surface;

            surface = baul_file_get_icon_surface (file,
                                                  baul_get_icon_size_for_stock_size (CTK_ICON_SIZE_MENU),
                                                  TRUE,
                                                  ctk_widget_get_scale_factor (window->details->location_button),
                                                  BAUL_FILE_ICON_FLAGS_IGNORE_VISITING);

            if (surface != NULL)
            {
                ctk_image_set_from_surface (CTK_IMAGE (window->details->location_icon), surface);
                cairo_surface_destroy (surface);
            }
            else
            {
                ctk_image_set_from_icon_name (CTK_IMAGE (window->details->location_icon),
                                              "document-open", CTK_ICON_SIZE_MENU);
            }
        }
        g_object_unref (file);

    }
    else
    {
        ctk_label_set_label (CTK_LABEL (window->details->location_label),
                             "");
        ctk_widget_set_sensitive (window->details->location_button, FALSE);
    }
}

static void
action_go_to_location_callback (CtkAction *action G_GNUC_UNUSED,
				gpointer   user_data)
{
    BaulWindow *window;

    window = BAUL_WINDOW (user_data);

    baul_window_prompt_for_location (window, NULL);
}

static void
action_add_bookmark_callback (CtkAction *action G_GNUC_UNUSED,
			      gpointer   user_data)
{
    BaulWindow *window;

    window = BAUL_WINDOW (user_data);

    if (!BAUL_IS_DESKTOP_WINDOW (window))   /* don't bookmark x-baul-desktop:/// */
    {
        baul_window_add_bookmark_for_current_location (window);
    }
}

static void
action_edit_bookmarks_callback (CtkAction *action G_GNUC_UNUSED,
				gpointer   user_data)
{
    baul_window_edit_bookmarks (BAUL_WINDOW (user_data));
}

static void
action_search_callback (CtkAction *action G_GNUC_UNUSED,
			gpointer   user_data)
{
    BaulWindow *window;
    char *uri;
    GFile *f;

    window = BAUL_WINDOW (user_data);

    uri = baul_search_directory_generate_new_uri ();
    f = g_file_new_for_uri (uri);
    baul_window_go_to (window, f);
    g_object_unref (f);
    g_free (uri);
}

static const CtkActionEntry spatial_entries[] =
{
    /* name, icon name, label */ { SPATIAL_ACTION_PLACES, NULL, N_("_Places") },
    /* name, icon name, label */ {
        SPATIAL_ACTION_GO_TO_LOCATION, NULL, N_("Open _Location..."),
        "<control>L", N_("Specify a location to open"),
        G_CALLBACK (action_go_to_location_callback)
    },
    /* name, icon name, label */ {
        SPATIAL_ACTION_CLOSE_PARENT_FOLDERS, NULL, N_("Close P_arent Folders"),
        "<control><shift>W", N_("Close this folder's parents"),
        G_CALLBACK (action_close_parent_folders_callback)
    },
    /* name, icon name, label */ {
        SPATIAL_ACTION_CLOSE_ALL_FOLDERS, NULL, N_("Clos_e All Folders"),
        "<control>Q", N_("Close all folder windows"),
        G_CALLBACK (action_close_all_folders_callback)
    },
    /* name, icon name, label */ { "Add Bookmark", "list-add", N_("_Add Bookmark"),
        "<control>d", N_("Add a bookmark for the current location to this menu"),
        G_CALLBACK (action_add_bookmark_callback)
    },
    /* name, icon name, label */ { "Edit Bookmarks", NULL, N_("_Edit Bookmarks..."),
        "<control>b", N_("Display a window that allows editing the bookmarks in this menu"),
        G_CALLBACK (action_edit_bookmarks_callback)
    },
    /* name, icon name, label */ { "Search", "edit-find", N_("_Search for Files..."),
        "<control>F", N_("Locate documents and folders on this computer by name or content"),
        G_CALLBACK (action_search_callback)
    },
};

static void
baul_spatial_window_init (BaulSpatialWindow *window)
{
    CtkWidget *arrow;
    CtkWidget *hbox, *vbox;
    CtkActionGroup *action_group;
    CtkUIManager *ui_manager;
    CtkTargetList *targets;
    const char *ui;
    BaulWindow *win;
    BaulWindowPane *pane;

    window->details = baul_spatial_window_get_instance_private (window);

    win = BAUL_WINDOW (window);

    ctk_widget_set_hexpand (win->details->statusbar, TRUE);
    ctk_grid_attach (CTK_GRID (win->details->grid),
                     win->details->statusbar,
                     0, 5, 1, 1);
    ctk_widget_show (win->details->statusbar);

    pane = baul_window_pane_new (win);
    win->details->panes = g_list_prepend (win->details->panes, pane);

    vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
    ctk_box_set_homogeneous (CTK_BOX (vbox), TRUE);
    ctk_widget_set_hexpand (vbox, TRUE);
    ctk_widget_set_vexpand (vbox, TRUE);
    ctk_grid_attach (CTK_GRID (BAUL_WINDOW (window)->details->grid),
                     vbox,
                     0, 1, 1, 3);

    ctk_widget_show (vbox);
    window->details->content_box = vbox;

    window->details->location_button = ctk_button_new ();
    g_signal_connect (window->details->location_button,
                      "button-press-event",
                      G_CALLBACK (location_button_pressed_callback),
                      window);
    ctk_button_set_relief (CTK_BUTTON (window->details->location_button),
                           CTK_RELIEF_NORMAL);

    ctk_widget_show (window->details->location_button);
    hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 3);
    ctk_container_add (CTK_CONTAINER (window->details->location_button),
                       hbox);
    ctk_widget_show (hbox);

    window->details->location_icon = ctk_image_new_from_icon_name ("document-open", CTK_ICON_SIZE_MENU);
    ctk_box_pack_start (CTK_BOX (hbox), window->details->location_icon, FALSE, FALSE, 0);
    ctk_widget_show (window->details->location_icon);

    window->details->location_label = ctk_label_new ("");
    ctk_label_set_ellipsize (CTK_LABEL (window->details->location_label), PANGO_ELLIPSIZE_END);
    ctk_label_set_max_width_chars (CTK_LABEL (window->details->location_label), MAX_SHORTNAME_PATH);
    ctk_box_pack_start (CTK_BOX (hbox), window->details->location_label,
                        FALSE, FALSE, 0);
    ctk_widget_show (window->details->location_label);

    arrow = ctk_image_new_from_icon_name ("pan-down-symbolic", CTK_ICON_SIZE_BUTTON);
    ctk_box_pack_start (CTK_BOX (hbox), arrow, FALSE, FALSE, 0);
    ctk_widget_show (arrow);

    ctk_drag_source_set (window->details->location_button,
                         CDK_BUTTON1_MASK | CDK_BUTTON2_MASK, location_button_drag_types,
                         G_N_ELEMENTS (location_button_drag_types),
                         CDK_ACTION_MOVE | CDK_ACTION_COPY | CDK_ACTION_LINK | CDK_ACTION_ASK);
    g_signal_connect (window->details->location_button,
                      "drag_begin",
                      G_CALLBACK (location_button_drag_begin_callback),
                      window);
    g_signal_connect (window->details->location_button,
                      "drag_data_get",
                      G_CALLBACK (location_button_drag_data_get_callback),
                      window);

    targets = ctk_drag_source_get_target_list (window->details->location_button);
    ctk_target_list_add_text_targets (targets, BAUL_ICON_DND_TEXT);

    ctk_widget_set_sensitive (window->details->location_button, FALSE);
    g_signal_connect (window->details->location_button,
                      "clicked",
                      G_CALLBACK (location_button_clicked_callback), window);
    ctk_box_pack_start (CTK_BOX (BAUL_WINDOW (window)->details->statusbar),
                        window->details->location_button,
                        FALSE, TRUE, 0);

    ctk_box_reorder_child (CTK_BOX (BAUL_WINDOW (window)->details->statusbar),
                           window->details->location_button, 0);

    action_group = ctk_action_group_new ("SpatialActions");
    ctk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    window->details->spatial_action_group = action_group;
    ctk_action_group_add_actions (action_group,
                                  spatial_entries, G_N_ELEMENTS (spatial_entries),
                                  window);

    ui_manager = baul_window_get_ui_manager (BAUL_WINDOW (window));
    ctk_ui_manager_insert_action_group (ui_manager, action_group, 0);
    g_object_unref (action_group); /* owned by ui manager */

    ui = baul_ui_string_get ("baul-spatial-window-ui.xml");
    ctk_ui_manager_add_ui_from_string (ui_manager, ui, -1, NULL);

    baul_window_set_active_pane (win, pane);
}

static void
baul_spatial_window_class_init (BaulSpatialWindowClass *klass)
{
    CtkBindingSet *binding_set;
	BaulWindowClass *nclass = BAUL_WINDOW_CLASS (klass);
	CtkWidgetClass *wclass = CTK_WIDGET_CLASS (klass);

	nclass->window_type = BAUL_WINDOW_SPATIAL;
	nclass->bookmarks_placeholder = MENU_PATH_SPATIAL_BOOKMARKS_PLACEHOLDER;
	nclass->prompt_for_location = real_prompt_for_location;
	nclass->get_icon = real_get_icon;
	nclass->sync_title = real_sync_title;
	nclass->get_min_size = real_get_min_size;
	nclass->get_default_size = real_get_default_size;
	nclass->sync_allow_stop = real_sync_allow_stop;
	nclass->set_allow_up = real_set_allow_up;
	nclass->open_slot = real_open_slot;
	nclass->close = real_window_close;
	nclass->close_slot = real_close_slot;

	wclass->show = baul_spatial_window_show;
	wclass->configure_event = baul_spatial_window_configure_event;
	wclass->unrealize = baul_spatial_window_unrealize;
	wclass->window_state_event = baul_spatial_window_state_event;

	G_OBJECT_CLASS (klass)->finalize = baul_spatial_window_finalize;

	binding_set = ctk_binding_set_by_class (klass);
	ctk_binding_entry_add_signal (binding_set, CDK_KEY_BackSpace, CDK_SHIFT_MASK,
                                  "go_up", 1,
                                  G_TYPE_BOOLEAN, TRUE);
	ctk_binding_entry_add_signal (binding_set, CDK_KEY_Up, CDK_SHIFT_MASK | CDK_MOD1_MASK,
                                  "go_up", 1,
                                  G_TYPE_BOOLEAN, TRUE);
}
