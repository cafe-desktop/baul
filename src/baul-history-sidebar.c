/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 *  Baul
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 2000, 2001 Eazel, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Authors: Elliot Lee <sopwith@redhat.com>
 *           Darin Adler <darin@bentspoon.com>
 *
 */

#include <config.h>

#include <ctk/ctk.h>
#include <glib/gi18n.h>
#include <cairo-gobject.h>

#include <eel/eel-ctk-extensions.h>

#include <libbaul-private/baul-bookmark.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-sidebar-provider.h>
#include <libbaul-private/baul-module.h>
#include <libbaul-private/baul-signaller.h>
#include <libbaul-private/baul-window-info.h>
#include <libbaul-private/baul-window-slot-info.h>

#include "baul-history-sidebar.h"

#define BAUL_HISTORY_SIDEBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_HISTORY_SIDEBAR, BaulHistorySidebarClass))
#define BAUL_IS_HISTORY_SIDEBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_HISTORY_SIDEBAR))
#define BAUL_IS_HISTORY_SIDEBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_HISTORY_SIDEBAR))

typedef struct
{
    CtkScrolledWindowClass parent;
} BaulHistorySidebarClass;

typedef struct
{
    GObject parent;
} BaulHistorySidebarProvider;

typedef struct
{
    GObjectClass parent;
} BaulHistorySidebarProviderClass;


enum
{
    HISTORY_SIDEBAR_COLUMN_ICON,
    HISTORY_SIDEBAR_COLUMN_NAME,
    HISTORY_SIDEBAR_COLUMN_BOOKMARK,
    HISTORY_SIDEBAR_COLUMN_COUNT
};

static void  baul_history_sidebar_iface_init        (BaulSidebarIface         *iface);
static void  sidebar_provider_iface_init                (BaulSidebarProviderIface *iface);
static GType baul_history_sidebar_provider_get_type (void);
static void  baul_history_sidebar_style_updated	        (CtkWidget *widget);

G_DEFINE_TYPE_WITH_CODE (BaulHistorySidebar, baul_history_sidebar, CTK_TYPE_SCROLLED_WINDOW,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_SIDEBAR,
                                 baul_history_sidebar_iface_init));

G_DEFINE_TYPE_WITH_CODE (BaulHistorySidebarProvider, baul_history_sidebar_provider, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_SIDEBAR_PROVIDER,
                                 sidebar_provider_iface_init));

static void
update_history (BaulHistorySidebar *sidebar)
{
    CtkListStore         *store;
    CtkTreeSelection     *selection;
    CtkTreeIter           iter;
    GList *l, *history;
    BaulBookmark         *bookmark = NULL;
    cairo_surface_t      *surface = NULL;

    store = CTK_LIST_STORE (ctk_tree_view_get_model (sidebar->tree_view));

    ctk_list_store_clear (store);

    history = baul_window_info_get_history (sidebar->window);
    for (l = history; l != NULL; l = l->next)
    {
        char *name;

        bookmark = baul_bookmark_copy (l->data);

        surface = baul_bookmark_get_surface (bookmark, CTK_ICON_SIZE_MENU);
        name = baul_bookmark_get_name (bookmark);
        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            HISTORY_SIDEBAR_COLUMN_ICON, surface,
                            HISTORY_SIDEBAR_COLUMN_NAME, name,
                            HISTORY_SIDEBAR_COLUMN_BOOKMARK, bookmark,
                            -1);
        g_object_unref (bookmark);

        if (surface != NULL)
        {
            cairo_surface_destroy (surface);
        }
        g_free (name);
    }
    g_list_free_full (history, g_object_unref);

    selection = CTK_TREE_SELECTION (ctk_tree_view_get_selection (sidebar->tree_view));

    if (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter))
    {
        ctk_tree_selection_select_iter (selection, &iter);
    }
}

static void
history_changed_callback (GObject *signaller,
                          BaulHistorySidebar *sidebar)
{
    update_history (sidebar);
}

static void
open_selected_item (BaulHistorySidebar *sidebar,
                    CtkTreePath *path,
                    BaulWindowOpenFlags flags)
{
    BaulWindowSlotInfo *slot;
    CtkTreeModel *model;
    CtkTreeIter iter;
    BaulBookmark *bookmark;
    GFile *location;

    model = ctk_tree_view_get_model (sidebar->tree_view);

    if (!ctk_tree_model_get_iter (model, &iter, path))
    {
        return;
    }

    ctk_tree_model_get
    (model, &iter, HISTORY_SIDEBAR_COLUMN_BOOKMARK, &bookmark, -1);

    /* Navigate to the clicked location. */
    location = baul_bookmark_get_location (BAUL_BOOKMARK (bookmark));
    slot = baul_window_info_get_active_slot (sidebar->window);
    baul_window_slot_info_open_location
    (slot,
     location, BAUL_WINDOW_OPEN_ACCORDING_TO_MODE,
     flags, NULL);
    g_object_unref (location);
}

static void
row_activated_callback (CtkTreeView *tree_view,
                        CtkTreePath *path,
                        CtkTreeViewColumn *column,
                        gpointer user_data)
{
    BaulHistorySidebar *sidebar;

    sidebar = BAUL_HISTORY_SIDEBAR (user_data);
    g_assert (sidebar->tree_view == tree_view);

    open_selected_item (sidebar, path, 0);
}

static gboolean
button_press_event_callback (CtkWidget *widget,
                             CdkEventButton *event,
                             gpointer user_data)
{
    if (event->button == 2 && event->type == CDK_BUTTON_PRESS)
    {
        /* Open new tab on middle click. */
        BaulHistorySidebar *sidebar;
        CtkTreePath *path;

        sidebar = BAUL_HISTORY_SIDEBAR (user_data);
        g_assert (sidebar->tree_view == CTK_TREE_VIEW (widget));

        if (ctk_tree_view_get_path_at_pos (sidebar->tree_view,
                                           event->x, event->y,
                                           &path, NULL, NULL, NULL))
        {
            open_selected_item (sidebar,
                                path,
                                BAUL_WINDOW_OPEN_FLAG_NEW_TAB);
            ctk_tree_path_free (path);
        }
    }

    return FALSE;
}

static void
update_click_policy (BaulHistorySidebar *sidebar)
{
    int policy;

    policy = g_settings_get_enum (baul_preferences, BAUL_PREFERENCES_CLICK_POLICY);

    eel_ctk_tree_view_set_activate_on_single_click
    (sidebar->tree_view, policy == BAUL_CLICK_POLICY_SINGLE);
}

static void
click_policy_changed_callback (gpointer user_data)
{
    BaulHistorySidebar *sidebar;

    sidebar = BAUL_HISTORY_SIDEBAR (user_data);

    update_click_policy (sidebar);
}

static void
baul_history_sidebar_init (BaulHistorySidebar *sidebar)
{
    CtkTreeView       *tree_view;
    CtkTreeViewColumn *col;
    CtkCellRenderer   *cell;
    CtkListStore      *store;
    CtkTreeSelection  *selection;

    tree_view = CTK_TREE_VIEW (ctk_tree_view_new ());
    ctk_tree_view_set_headers_visible (tree_view, FALSE);
    ctk_widget_show (CTK_WIDGET (tree_view));

    col = CTK_TREE_VIEW_COLUMN (ctk_tree_view_column_new ());

    cell = ctk_cell_renderer_pixbuf_new ();
    ctk_tree_view_column_pack_start (col, cell, FALSE);
    ctk_tree_view_column_set_attributes (col, cell,
                                         "surface", HISTORY_SIDEBAR_COLUMN_ICON,
                                         NULL);

    cell = ctk_cell_renderer_text_new ();
    ctk_tree_view_column_pack_start (col, cell, TRUE);
    ctk_tree_view_column_set_attributes (col, cell,
                                         "text", HISTORY_SIDEBAR_COLUMN_NAME,
                                         NULL);

    ctk_tree_view_column_set_fixed_width (col, BAUL_ICON_SIZE_SMALLER);
    ctk_tree_view_append_column (tree_view, col);

    store = ctk_list_store_new (HISTORY_SIDEBAR_COLUMN_COUNT,
                                CAIRO_GOBJECT_TYPE_SURFACE,
                                G_TYPE_STRING,
                                BAUL_TYPE_BOOKMARK);

    ctk_tree_view_set_model (tree_view, CTK_TREE_MODEL (store));
    g_object_unref (store);

    ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sidebar),
                                    CTK_POLICY_AUTOMATIC,
                                    CTK_POLICY_AUTOMATIC);
    ctk_scrolled_window_set_hadjustment (CTK_SCROLLED_WINDOW (sidebar), NULL);
    ctk_scrolled_window_set_vadjustment (CTK_SCROLLED_WINDOW (sidebar), NULL);
    ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (sidebar), CTK_SHADOW_IN);
    ctk_scrolled_window_set_overlay_scrolling (CTK_SCROLLED_WINDOW (sidebar), FALSE);

    ctk_container_add (CTK_CONTAINER (sidebar), CTK_WIDGET (tree_view));
    ctk_widget_show (CTK_WIDGET (sidebar));

    sidebar->tree_view = tree_view;

    selection = ctk_tree_view_get_selection (tree_view);
    ctk_tree_selection_set_mode (selection, CTK_SELECTION_SINGLE);

    g_signal_connect_object
    (tree_view, "row_activated",
     G_CALLBACK (row_activated_callback), sidebar, 0);

    g_signal_connect_object (baul_signaller_get_current (),
                             "history_list_changed",
                             G_CALLBACK (history_changed_callback), sidebar, 0);

    g_signal_connect (tree_view, "button-press-event",
                      G_CALLBACK (button_press_event_callback), sidebar);

    g_signal_connect_swapped (baul_preferences,
                              "changed::" BAUL_PREFERENCES_CLICK_POLICY,
                              G_CALLBACK(click_policy_changed_callback),
                              sidebar);
    update_click_policy (sidebar);
}

static void
baul_history_sidebar_finalize (GObject *object)
{
    BaulHistorySidebar *sidebar;

    sidebar = BAUL_HISTORY_SIDEBAR (object);

    g_signal_handlers_disconnect_by_func (baul_preferences,
                                          click_policy_changed_callback,
                                          sidebar);

    G_OBJECT_CLASS (baul_history_sidebar_parent_class)->finalize (object);
}

static void
baul_history_sidebar_class_init (BaulHistorySidebarClass *class)
{
    G_OBJECT_CLASS (class)->finalize = baul_history_sidebar_finalize;

    CTK_WIDGET_CLASS (class)->style_updated = baul_history_sidebar_style_updated;
}

static const char *
baul_history_sidebar_get_sidebar_id (BaulSidebar *sidebar)
{
    return BAUL_HISTORY_SIDEBAR_ID;
}

static char *
baul_history_sidebar_get_tab_label (BaulSidebar *sidebar)
{
    return g_strdup (_("History"));
}

static char *
baul_history_sidebar_get_tab_tooltip (BaulSidebar *sidebar)
{
    return g_strdup (_("Show History"));
}

static GdkPixbuf *
baul_history_sidebar_get_tab_icon (BaulSidebar *sidebar)
{
    return NULL;
}

static void
baul_history_sidebar_is_visible_changed (BaulSidebar *sidebar,
        gboolean         is_visible)
{
    /* Do nothing */
}

static void
baul_history_sidebar_iface_init (BaulSidebarIface *iface)
{
    iface->get_sidebar_id = baul_history_sidebar_get_sidebar_id;
    iface->get_tab_label = baul_history_sidebar_get_tab_label;
    iface->get_tab_tooltip = baul_history_sidebar_get_tab_tooltip;
    iface->get_tab_icon = baul_history_sidebar_get_tab_icon;
    iface->is_visible_changed = baul_history_sidebar_is_visible_changed;
}

static void
baul_history_sidebar_set_parent_window (BaulHistorySidebar *sidebar,
                                        BaulWindowInfo *window)
{
    sidebar->window = window;
    update_history (sidebar);
}

static void
baul_history_sidebar_style_updated (CtkWidget *widget)
{
    BaulHistorySidebar *sidebar;

    sidebar = BAUL_HISTORY_SIDEBAR (widget);

    update_history (sidebar);
}

static BaulSidebar *
baul_history_sidebar_create (BaulSidebarProvider *provider,
                             BaulWindowInfo *window)
{
    BaulHistorySidebar *sidebar;

    sidebar = g_object_new (baul_history_sidebar_get_type (), NULL);
    baul_history_sidebar_set_parent_window (sidebar, window);
    g_object_ref_sink (sidebar);

    return BAUL_SIDEBAR (sidebar);
}

static void
sidebar_provider_iface_init (BaulSidebarProviderIface *iface)
{
    iface->create = baul_history_sidebar_create;
}

static void
baul_history_sidebar_provider_init (BaulHistorySidebarProvider *sidebar)
{
}

static void
baul_history_sidebar_provider_class_init (BaulHistorySidebarProviderClass *class)
{
}

void
baul_history_sidebar_register (void)
{
    baul_module_add_type (baul_history_sidebar_provider_get_type ());
}

