/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Baul is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Baul is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Authors: Darin Adler <darin@bentspoon.com>
 */

#include <config.h>

#include <X11/Xatom.h>
#include <cdk/cdkx.h>
#include <ctk/ctk.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#include <eel/eel-background.h>
#include <eel/eel-vfs-extensions.h>

#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-icon-names.h>

#include "baul-desktop-window.h"
#include "baul-window-private.h"
#include "baul-actions.h"

/* Tell screen readers that this is a desktop window */

G_DEFINE_TYPE (BaulDesktopWindowAccessible, baul_desktop_window_accessible,
               CTK_TYPE_WINDOW_ACCESSIBLE);

static AtkAttributeSet *
desktop_get_attributes (AtkObject *accessible)
{
    AtkAttributeSet *attributes;
    AtkAttribute *is_desktop;

    attributes = ATK_OBJECT_CLASS (baul_desktop_window_accessible_parent_class)->get_attributes (accessible);

    is_desktop = g_malloc (sizeof (AtkAttribute));
    is_desktop->name = g_strdup ("is-desktop");
    is_desktop->value = g_strdup ("true");

    attributes = g_slist_append (attributes, is_desktop);

    return attributes;
}

static void
baul_desktop_window_accessible_init (BaulDesktopWindowAccessible *window)
{
}

static void
baul_desktop_window_accessible_class_init (BaulDesktopWindowAccessibleClass *klass)
{
    AtkObjectClass *aclass = ATK_OBJECT_CLASS (klass);

    aclass->get_attributes = desktop_get_attributes;
}

struct _BaulDesktopWindowPrivate
{
    gulong size_changed_id;

    gboolean loaded;
};

G_DEFINE_TYPE_WITH_PRIVATE (BaulDesktopWindow, baul_desktop_window,
               BAUL_TYPE_SPATIAL_WINDOW);

static void
baul_desktop_window_init (BaulDesktopWindow *window)
{
    CtkAction *action;
    AtkObject *accessible;

    window->details = baul_desktop_window_get_instance_private (window);

    CtkStyleContext *context;

    context = ctk_widget_get_style_context (CTK_WIDGET (window));
    ctk_style_context_add_class (context, "baul-desktop-window");

    ctk_window_move (CTK_WINDOW (window), 0, 0);

    /* shouldn't really be needed given our semantic type
     * of _NET_WM_TYPE_DESKTOP, but why not
     */
    ctk_window_set_resizable (CTK_WINDOW (window),
                              FALSE);

    g_object_set_data (G_OBJECT (window), "is_desktop_window",
                       GINT_TO_POINTER (1));

    ctk_widget_hide (BAUL_WINDOW (window)->details->statusbar);
    ctk_widget_hide (BAUL_WINDOW (window)->details->menubar);

    /* Don't allow close action on desktop */
    action = ctk_action_group_get_action (BAUL_WINDOW (window)->details->main_action_group,
                                          BAUL_ACTION_CLOSE);
    ctk_action_set_sensitive (action, FALSE);

    /* Set the accessible name so that it doesn't inherit the cryptic desktop URI. */
    accessible = ctk_widget_get_accessible (CTK_WIDGET (window));

    if (accessible) {
        atk_object_set_name (accessible, _("Desktop"));
    }
}

static gint
baul_desktop_window_delete_event (BaulDesktopWindow *window)
{
    /* Returning true tells CTK+ not to delete the window. */
    return TRUE;
}

void
baul_desktop_window_update_directory (BaulDesktopWindow *window)
{
    GFile *location;

    g_assert (BAUL_IS_DESKTOP_WINDOW (window));

    location = g_file_new_for_uri (EEL_DESKTOP_URI);
    baul_window_go_to (BAUL_WINDOW (window), location);
    window->details->loaded = TRUE;

    g_object_unref (location);
}

static void
baul_desktop_window_screen_size_changed (CdkScreen             *screen,
        BaulDesktopWindow *window)
{
    int width_request, height_request;
    int scale;

    scale = cdk_window_get_scale_factor (cdk_screen_get_root_window (screen));

    width_request = WidthOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale;
    height_request = HeightOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale;

    g_object_set (window,
                  "width_request", width_request,
                  "height_request", height_request,
                  NULL);
}

BaulDesktopWindow *
baul_desktop_window_new (BaulApplication *application,
                         CdkScreen           *screen)
{
    BaulDesktopWindow *window;
    int width_request, height_request;
    int scale;

    scale = cdk_window_get_scale_factor (cdk_screen_get_root_window (screen));

    width_request = WidthOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale;
    height_request = HeightOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale;

    window = BAUL_DESKTOP_WINDOW
             (ctk_widget_new (baul_desktop_window_get_type(),
                              "app", application,
                              "width_request", width_request,
                              "height_request", height_request,
                              "screen", screen,
                              NULL));
    /* Stop wrong desktop window size in CTK 3.20*/
    /* We don't want to set a default size, which the parent does, since this */
    /* will cause the desktop window to open at the wrong size in ctk 3.20 */
    ctk_window_set_default_size (CTK_WINDOW (window), -1, -1);

    /* Special sawmill setting*/
    CdkWindow *cdkwin;
    ctk_widget_realize (CTK_WIDGET (window));
    cdkwin = ctk_widget_get_window (CTK_WIDGET (window));
    if (cdk_window_ensure_native (cdkwin)) {
        Display *disp = CDK_DISPLAY_XDISPLAY (cdk_window_get_display (cdkwin));
        XClassHint *xch = XAllocClassHint ();
        xch->res_name = "desktop_window";
        xch->res_class = "Baul";
        XSetClassHint (disp, CDK_WINDOW_XID(cdkwin), xch);
        XFree(xch);
    }

    cdk_window_set_title (cdkwin, _("Desktop"));

    g_signal_connect (window, "delete_event", G_CALLBACK (baul_desktop_window_delete_event), NULL);

    /* Point window at the desktop folder.
     * Note that baul_desktop_window_init is too early to do this.
     */
    baul_desktop_window_update_directory (window);

    return window;
}

static void
map (CtkWidget *widget)
{
    /* Chain up to realize our children */
    CTK_WIDGET_CLASS (baul_desktop_window_parent_class)->map (widget);
    cdk_window_lower (ctk_widget_get_window (widget));
}

static void
unrealize (CtkWidget *widget)
{
    BaulDesktopWindow *window;
    BaulDesktopWindowPrivate *details;
    CdkWindow *root_window;

    window = BAUL_DESKTOP_WINDOW (widget);
    details = window->details;

    root_window = cdk_screen_get_root_window (
                      ctk_window_get_screen (CTK_WINDOW (window)));

    cdk_property_delete (root_window,
                         cdk_atom_intern ("BAUL_DESKTOP_WINDOW_ID", TRUE));

    if (details->size_changed_id != 0) {
        g_signal_handler_disconnect (ctk_window_get_screen (CTK_WINDOW (window)),
                         details->size_changed_id);
        details->size_changed_id = 0;
    }

    CTK_WIDGET_CLASS (baul_desktop_window_parent_class)->unrealize (widget);
}

static void
set_wmspec_desktop_hint (CdkWindow *window)
{
    CdkAtom atom;

    atom = cdk_atom_intern ("_NET_WM_WINDOW_TYPE_DESKTOP", FALSE);

    cdk_property_change (window,
                         cdk_atom_intern ("_NET_WM_WINDOW_TYPE", FALSE),
                         cdk_x11_xatom_to_atom (XA_ATOM), 32,
                         CDK_PROP_MODE_REPLACE, (guchar *) &atom, 1);
}

static void
set_desktop_window_id (BaulDesktopWindow *window,
                       CdkWindow             *cdkwindow)
{
    /* Tuck the desktop windows xid in the root to indicate we own the desktop.
     */
    Window window_xid;
    CdkWindow *root_window;

    root_window = cdk_screen_get_root_window (
                      ctk_window_get_screen (CTK_WINDOW (window)));

    window_xid = CDK_WINDOW_XID (cdkwindow);

    cdk_property_change (root_window,
                         cdk_atom_intern ("BAUL_DESKTOP_WINDOW_ID", FALSE),
                         cdk_x11_xatom_to_atom (XA_WINDOW), 32,
                         CDK_PROP_MODE_REPLACE, (guchar *) &window_xid, 1);
}

static void
realize (CtkWidget *widget)
{
    BaulDesktopWindow *window;
    BaulDesktopWindowPrivate *details;
    window = BAUL_DESKTOP_WINDOW (widget);
    details = window->details;

    /* Make sure we get keyboard events */
    ctk_widget_set_events (widget, ctk_widget_get_events (widget)
                           | CDK_KEY_PRESS_MASK | CDK_KEY_RELEASE_MASK);
    /* Do the work of realizing. */
    CTK_WIDGET_CLASS (baul_desktop_window_parent_class)->realize (widget);

    /* This is the new way to set up the desktop window */
    set_wmspec_desktop_hint (ctk_widget_get_window (widget));

    set_desktop_window_id (window, ctk_widget_get_window (widget));

    details->size_changed_id =
        g_signal_connect (ctk_window_get_screen (CTK_WINDOW (window)), "size_changed",
                          G_CALLBACK (baul_desktop_window_screen_size_changed), window);
}

static gboolean
draw (CtkWidget *widget,
      cairo_t   *cr)
{
    eel_background_draw (widget, cr);

    return CTK_WIDGET_CLASS (baul_desktop_window_parent_class)->draw (widget, cr);
}

static BaulIconInfo *
real_get_icon (BaulWindow *window,
               BaulWindowSlot *slot)
{
    gint scale = ctk_widget_get_scale_factor (CTK_WIDGET (window));
    return baul_icon_info_lookup_from_name (BAUL_ICON_DESKTOP, 48, scale);
}

static void
baul_desktop_window_class_init (BaulDesktopWindowClass *klass)
{
    CtkWidgetClass *wclass = CTK_WIDGET_CLASS (klass);
    BaulWindowClass *nclass = BAUL_WINDOW_CLASS (klass);

    wclass->realize = realize;
    wclass->unrealize = unrealize;
    wclass->map = map;
    wclass->draw = draw;

    ctk_widget_class_set_accessible_type (wclass, BAUL_TYPE_DESKTOP_WINDOW_ACCESSIBLE);

    nclass->window_type = BAUL_WINDOW_DESKTOP;
    nclass->get_icon = real_get_icon;
}

gboolean
baul_desktop_window_loaded (BaulDesktopWindow *window)
{
    return window->details->loaded;
}
