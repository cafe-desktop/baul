/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 *  Caja
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <cairo-gobject.h>

#include <eel/eel-gtk-extensions.h>

#include <libbaul-private/baul-bookmark.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-sidebar-provider.h>
#include <libbaul-private/baul-module.h>
#include <libbaul-private/baul-signaller.h>
#include <libbaul-private/baul-window-info.h>
#include <libbaul-private/baul-window-slot-info.h>

#include "baul-history-sidebar.h"

#define CAJA_HISTORY_SIDEBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CAJA_TYPE_HISTORY_SIDEBAR, CajaHistorySidebarClass))
#define CAJA_IS_HISTORY_SIDEBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAJA_TYPE_HISTORY_SIDEBAR))
#define CAJA_IS_HISTORY_SIDEBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CAJA_TYPE_HISTORY_SIDEBAR))

typedef struct
{
    GtkScrolledWindowClass parent;
} CajaHistorySidebarClass;

typedef struct
{
    GObject parent;
} CajaHistorySidebarProvider;

typedef struct
{
    GObjectClass parent;
} CajaHistorySidebarProviderClass;


enum
{
    HISTORY_SIDEBAR_COLUMN_ICON,
    HISTORY_SIDEBAR_COLUMN_NAME,
    HISTORY_SIDEBAR_COLUMN_BOOKMARK,
    HISTORY_SIDEBAR_COLUMN_COUNT
};

static void  baul_history_sidebar_iface_init        (CajaSidebarIface         *iface);
static void  sidebar_provider_iface_init                (CajaSidebarProviderIface *iface);
static GType baul_history_sidebar_provider_get_type (void);
static void  baul_history_sidebar_style_updated	        (GtkWidget *widget);

G_DEFINE_TYPE_WITH_CODE (CajaHistorySidebar, baul_history_sidebar, GTK_TYPE_SCROLLED_WINDOW,
                         G_IMPLEMENT_INTERFACE (CAJA_TYPE_SIDEBAR,
                                 baul_history_sidebar_iface_init));

G_DEFINE_TYPE_WITH_CODE (CajaHistorySidebarProvider, baul_history_sidebar_provider, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CAJA_TYPE_SIDEBAR_PROVIDER,
                                 sidebar_provider_iface_init));

static void
update_history (CajaHistorySidebar *sidebar)
{
    GtkListStore         *store;
    GtkTreeSelection     *selection;
    GtkTreeIter           iter;
    GList *l, *history;
    CajaBookmark         *bookmark = NULL;
    cairo_surface_t      *surface = NULL;

    store = GTK_LIST_STORE (gtk_tree_view_get_model (sidebar->tree_view));

    gtk_list_store_clear (store);

    history = baul_window_info_get_history (sidebar->window);
    for (l = history; l != NULL; l = l->next)
    {
        char *name;

        bookmark = baul_bookmark_copy (l->data);

        surface = baul_bookmark_get_surface (bookmark, GTK_ICON_SIZE_MENU);
        name = baul_bookmark_get_name (bookmark);
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
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

    selection = GTK_TREE_SELECTION (gtk_tree_view_get_selection (sidebar->tree_view));

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
    {
        gtk_tree_selection_select_iter (selection, &iter);
    }
}

static void
history_changed_callback (GObject *signaller,
                          CajaHistorySidebar *sidebar)
{
    update_history (sidebar);
}

static void
open_selected_item (CajaHistorySidebar *sidebar,
                    GtkTreePath *path,
                    CajaWindowOpenFlags flags)
{
    CajaWindowSlotInfo *slot;
    GtkTreeModel *model;
    GtkTreeIter iter;
    CajaBookmark *bookmark;
    GFile *location;

    model = gtk_tree_view_get_model (sidebar->tree_view);

    if (!gtk_tree_model_get_iter (model, &iter, path))
    {
        return;
    }

    gtk_tree_model_get
    (model, &iter, HISTORY_SIDEBAR_COLUMN_BOOKMARK, &bookmark, -1);

    /* Navigate to the clicked location. */
    location = baul_bookmark_get_location (CAJA_BOOKMARK (bookmark));
    slot = baul_window_info_get_active_slot (sidebar->window);
    baul_window_slot_info_open_location
    (slot,
     location, CAJA_WINDOW_OPEN_ACCORDING_TO_MODE,
     flags, NULL);
    g_object_unref (location);
}

static void
row_activated_callback (GtkTreeView *tree_view,
                        GtkTreePath *path,
                        GtkTreeViewColumn *column,
                        gpointer user_data)
{
    CajaHistorySidebar *sidebar;

    sidebar = CAJA_HISTORY_SIDEBAR (user_data);
    g_assert (sidebar->tree_view == tree_view);

    open_selected_item (sidebar, path, 0);
}

static gboolean
button_press_event_callback (GtkWidget *widget,
                             GdkEventButton *event,
                             gpointer user_data)
{
    if (event->button == 2 && event->type == GDK_BUTTON_PRESS)
    {
        /* Open new tab on middle click. */
        CajaHistorySidebar *sidebar;
        GtkTreePath *path;

        sidebar = CAJA_HISTORY_SIDEBAR (user_data);
        g_assert (sidebar->tree_view == GTK_TREE_VIEW (widget));

        if (gtk_tree_view_get_path_at_pos (sidebar->tree_view,
                                           event->x, event->y,
                                           &path, NULL, NULL, NULL))
        {
            open_selected_item (sidebar,
                                path,
                                CAJA_WINDOW_OPEN_FLAG_NEW_TAB);
            gtk_tree_path_free (path);
        }
    }

    return FALSE;
}

static void
update_click_policy (CajaHistorySidebar *sidebar)
{
    int policy;

    policy = g_settings_get_enum (baul_preferences, CAJA_PREFERENCES_CLICK_POLICY);

    eel_gtk_tree_view_set_activate_on_single_click
    (sidebar->tree_view, policy == CAJA_CLICK_POLICY_SINGLE);
}

static void
click_policy_changed_callback (gpointer user_data)
{
    CajaHistorySidebar *sidebar;

    sidebar = CAJA_HISTORY_SIDEBAR (user_data);

    update_click_policy (sidebar);
}

static void
baul_history_sidebar_init (CajaHistorySidebar *sidebar)
{
    GtkTreeView       *tree_view;
    GtkTreeViewColumn *col;
    GtkCellRenderer   *cell;
    GtkListStore      *store;
    GtkTreeSelection  *selection;

    tree_view = GTK_TREE_VIEW (gtk_tree_view_new ());
    gtk_tree_view_set_headers_visible (tree_view, FALSE);
    gtk_widget_show (GTK_WIDGET (tree_view));

    col = GTK_TREE_VIEW_COLUMN (gtk_tree_view_column_new ());

    cell = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (col, cell, FALSE);
    gtk_tree_view_column_set_attributes (col, cell,
                                         "surface", HISTORY_SIDEBAR_COLUMN_ICON,
                                         NULL);

    cell = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (col, cell, TRUE);
    gtk_tree_view_column_set_attributes (col, cell,
                                         "text", HISTORY_SIDEBAR_COLUMN_NAME,
                                         NULL);

    gtk_tree_view_column_set_fixed_width (col, CAJA_ICON_SIZE_SMALLER);
    gtk_tree_view_append_column (tree_view, col);

    store = gtk_list_store_new (HISTORY_SIDEBAR_COLUMN_COUNT,
                                CAIRO_GOBJECT_TYPE_SURFACE,
                                G_TYPE_STRING,
                                CAJA_TYPE_BOOKMARK);

    gtk_tree_view_set_model (tree_view, GTK_TREE_MODEL (store));
    g_object_unref (store);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sidebar),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (sidebar), NULL);
    gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (sidebar), NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sidebar), GTK_SHADOW_IN);
    gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (sidebar), FALSE);

    gtk_container_add (GTK_CONTAINER (sidebar), GTK_WIDGET (tree_view));
    gtk_widget_show (GTK_WIDGET (sidebar));

    sidebar->tree_view = tree_view;

    selection = gtk_tree_view_get_selection (tree_view);
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

    g_signal_connect_object
    (tree_view, "row_activated",
     G_CALLBACK (row_activated_callback), sidebar, 0);

    g_signal_connect_object (baul_signaller_get_current (),
                             "history_list_changed",
                             G_CALLBACK (history_changed_callback), sidebar, 0);

    g_signal_connect (tree_view, "button-press-event",
                      G_CALLBACK (button_press_event_callback), sidebar);

    g_signal_connect_swapped (baul_preferences,
                              "changed::" CAJA_PREFERENCES_CLICK_POLICY,
                              G_CALLBACK(click_policy_changed_callback),
                              sidebar);
    update_click_policy (sidebar);
}

static void
baul_history_sidebar_finalize (GObject *object)
{
    CajaHistorySidebar *sidebar;

    sidebar = CAJA_HISTORY_SIDEBAR (object);

    g_signal_handlers_disconnect_by_func (baul_preferences,
                                          click_policy_changed_callback,
                                          sidebar);

    G_OBJECT_CLASS (baul_history_sidebar_parent_class)->finalize (object);
}

static void
baul_history_sidebar_class_init (CajaHistorySidebarClass *class)
{
    G_OBJECT_CLASS (class)->finalize = baul_history_sidebar_finalize;

    GTK_WIDGET_CLASS (class)->style_updated = baul_history_sidebar_style_updated;
}

static const char *
baul_history_sidebar_get_sidebar_id (CajaSidebar *sidebar)
{
    return CAJA_HISTORY_SIDEBAR_ID;
}

static char *
baul_history_sidebar_get_tab_label (CajaSidebar *sidebar)
{
    return g_strdup (_("History"));
}

static char *
baul_history_sidebar_get_tab_tooltip (CajaSidebar *sidebar)
{
    return g_strdup (_("Show History"));
}

static GdkPixbuf *
baul_history_sidebar_get_tab_icon (CajaSidebar *sidebar)
{
    return NULL;
}

static void
baul_history_sidebar_is_visible_changed (CajaSidebar *sidebar,
        gboolean         is_visible)
{
    /* Do nothing */
}

static void
baul_history_sidebar_iface_init (CajaSidebarIface *iface)
{
    iface->get_sidebar_id = baul_history_sidebar_get_sidebar_id;
    iface->get_tab_label = baul_history_sidebar_get_tab_label;
    iface->get_tab_tooltip = baul_history_sidebar_get_tab_tooltip;
    iface->get_tab_icon = baul_history_sidebar_get_tab_icon;
    iface->is_visible_changed = baul_history_sidebar_is_visible_changed;
}

static void
baul_history_sidebar_set_parent_window (CajaHistorySidebar *sidebar,
                                        CajaWindowInfo *window)
{
    sidebar->window = window;
    update_history (sidebar);
}

static void
baul_history_sidebar_style_updated (GtkWidget *widget)
{
    CajaHistorySidebar *sidebar;

    sidebar = CAJA_HISTORY_SIDEBAR (widget);

    update_history (sidebar);
}

static CajaSidebar *
baul_history_sidebar_create (CajaSidebarProvider *provider,
                             CajaWindowInfo *window)
{
    CajaHistorySidebar *sidebar;

    sidebar = g_object_new (baul_history_sidebar_get_type (), NULL);
    baul_history_sidebar_set_parent_window (sidebar, window);
    g_object_ref_sink (sidebar);

    return CAJA_SIDEBAR (sidebar);
}

static void
sidebar_provider_iface_init (CajaSidebarProviderIface *iface)
{
    iface->create = baul_history_sidebar_create;
}

static void
baul_history_sidebar_provider_init (CajaHistorySidebarProvider *sidebar)
{
}

static void
baul_history_sidebar_provider_class_init (CajaHistorySidebarProviderClass *class)
{
}

void
baul_history_sidebar_register (void)
{
    baul_module_add_type (baul_history_sidebar_provider_get_type ());
}

