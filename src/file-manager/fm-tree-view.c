/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Copyright (C) 2000, 2001 Eazel, Inc
 * Copyright (C) 2002 Anders Carlsson
 * Copyright (C) 2002 Darin Adler
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Authors:
 *       Maciej Stachowiak <mjs@eazel.com>
 *       Anders Carlsson <andersca@gnu.org>
 *       Darin Adler <darin@bentspoon.com>
 */

/* fm-tree-view.c - tree sidebar panel
 */

#include <config.h>
#include <string.h>
#include <cairo-gobject.h>

#include <ctk/ctk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <eel/eel-ctk-extensions.h>

#include <libbaul-private/baul-clipboard.h>
#include <libbaul-private/baul-clipboard-monitor.h>
#include <libbaul-private/baul-desktop-icon-file.h>
#include <libbaul-private/baul-debug-log.h>
#include <libbaul-private/baul-file-attributes.h>
#include <libbaul-private/baul-file-operations.h>
#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-icon-names.h>
#include <libbaul-private/baul-mime-actions.h>
#include <libbaul-private/baul-program-choosing.h>
#include <libbaul-private/baul-tree-view-drag-dest.h>
#include <libbaul-private/baul-sidebar-provider.h>
#include <libbaul-private/baul-module.h>
#include <libbaul-private/baul-window-info.h>
#include <libbaul-private/baul-window-slot-info.h>
#include <libbaul-private/baul-directory.h>
#include <libbaul-private/baul-directory-private.h>
#include <libbaul-private/baul-file.h>
#include <libbaul-private/baul-file-private.h>

#include "fm-tree-view.h"
#include "fm-tree-model.h"
#include "fm-properties-window.h"

typedef struct
{
    GObject parent;
} FMTreeViewProvider;

typedef struct
{
    GObjectClass parent;
} FMTreeViewProviderClass;


struct FMTreeViewDetails
{
    BaulWindowInfo *window;
    CtkTreeView *tree_widget;
    CtkTreeModelSort *sort_model;
    FMTreeModel *child_model;

    GVolumeMonitor *volume_monitor;

    BaulFile *activation_file;
    BaulWindowOpenFlags activation_flags;

    BaulTreeViewDragDest *drag_dest;

    char *selection_location;
    gboolean selecting;

    guint show_selection_idle_id;
    gulong clipboard_handler_id;

    CtkWidget *popup;
    CtkWidget *popup_open;
    CtkWidget *popup_open_in_new_window;
    CtkWidget *popup_create_folder;
    CtkWidget *popup_cut;
    CtkWidget *popup_copy;
    CtkWidget *popup_paste;
    CtkWidget *popup_rename;
    CtkWidget *popup_trash;
    CtkWidget *popup_delete;
    CtkWidget *popup_properties;
    CtkWidget *popup_unmount_separator;
    CtkWidget *popup_unmount;
    CtkWidget *popup_eject;
    BaulFile *popup_file;
    guint popup_file_idle_handler;

    guint selection_changed_timer;
};

typedef struct
{
    GList *uris;
    FMTreeView *view;
} PrependURIParameters;

static CdkAtom copied_files_atom;

static void  fm_tree_view_iface_init        (BaulSidebarIface         *iface);
static void  sidebar_provider_iface_init    (BaulSidebarProviderIface *iface);
static void  fm_tree_view_activate_file     (FMTreeView *view,
        BaulFile *file,
        BaulWindowOpenFlags flags);
static GType fm_tree_view_provider_get_type (void);
static CtkWindow *fm_tree_view_get_containing_window (FMTreeView *view);

static void create_popup_menu (FMTreeView *view);

G_DEFINE_TYPE_WITH_CODE (FMTreeView, fm_tree_view, CTK_TYPE_SCROLLED_WINDOW,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_SIDEBAR,
                                 fm_tree_view_iface_init));
#define parent_class fm_tree_view_parent_class

G_DEFINE_TYPE_WITH_CODE (FMTreeViewProvider, fm_tree_view_provider, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_SIDEBAR_PROVIDER,
                                 sidebar_provider_iface_init));

static void
notify_clipboard_info (BaulClipboardMonitor *monitor,
                       BaulClipboardInfo *info,
                       FMTreeView *view)
{
    if (info != NULL && info->cut)
    {
        fm_tree_model_set_highlight_for_files (view->details->child_model, info->files);
    }
    else
    {
        fm_tree_model_set_highlight_for_files (view->details->child_model, NULL);
    }
}


static gboolean
show_iter_for_file (FMTreeView *view, BaulFile *file, CtkTreeIter *iter)
{
    CtkTreeModel *model;
    BaulFile *parent_file;
    CtkTreeIter parent_iter;
    CtkTreePath *path, *sort_path;
    CtkTreeIter cur_iter;

    if (view->details->child_model == NULL)
    {
        return FALSE;
    }
    model = CTK_TREE_MODEL (view->details->child_model);

    /* check if file is visible in the same root as the currently selected folder is */
    ctk_tree_view_get_cursor (view->details->tree_widget, &path, NULL);
    if (path != NULL)
    {
        if (ctk_tree_model_get_iter (model, &cur_iter, path) &&
                fm_tree_model_file_get_iter (view->details->child_model, iter,
                                             file, &cur_iter))
        {
            ctk_tree_path_free (path);
            return TRUE;
        }
        ctk_tree_path_free (path);
    }
    /* check if file is visible at all */
    if (fm_tree_model_file_get_iter (view->details->child_model,
                                     iter, file, NULL))
    {
        return TRUE;
    }

    parent_file = baul_file_get_parent (file);

    if (parent_file == NULL)
    {
        return FALSE;
    }
    if (!show_iter_for_file (view, parent_file, &parent_iter))
    {
        baul_file_unref (parent_file);
        return FALSE;
    }
    baul_file_unref (parent_file);

    if (parent_iter.user_data == NULL || parent_iter.stamp == 0)
    {
        return FALSE;
    }
    path = ctk_tree_model_get_path (model, &parent_iter);
    sort_path = ctk_tree_model_sort_convert_child_path_to_path
                (view->details->sort_model, path);
    ctk_tree_path_free (path);
    ctk_tree_view_expand_row (view->details->tree_widget, sort_path, FALSE);
    ctk_tree_path_free (sort_path);

    return FALSE;
}

static void
refresh_highlight (FMTreeView *view)
{
    BaulClipboardMonitor *monitor;
    BaulClipboardInfo *info;

    monitor = baul_clipboard_monitor_get ();
    info = baul_clipboard_monitor_get_clipboard_info (monitor);

    notify_clipboard_info (monitor, info, view);
}

static gboolean
show_selection_idle_callback (gpointer callback_data)
{
    FMTreeView *view;
    BaulFile *file;
    CtkTreeIter iter;
    CtkTreePath *path, *sort_path;

    view = FM_TREE_VIEW (callback_data);

    view->details->show_selection_idle_id = 0;

    file = baul_file_get_by_uri (view->details->selection_location);
    if (file == NULL)
    {
        return FALSE;
    }

    if (!baul_file_is_directory (file))
    {
        BaulFile *old_file;

        old_file = file;
        file = baul_file_get_parent (file);
        baul_file_unref (old_file);
        if (file == NULL)
        {
            return FALSE;
        }
    }

    view->details->selecting = TRUE;
    if (!show_iter_for_file (view, file, &iter))
    {
        baul_file_unref (file);
        return FALSE;
    }
    view->details->selecting = FALSE;

    path = ctk_tree_model_get_path (CTK_TREE_MODEL (view->details->child_model), &iter);
    sort_path = ctk_tree_model_sort_convert_child_path_to_path
                (view->details->sort_model, path);
    ctk_tree_path_free (path);
    ctk_tree_view_set_cursor (view->details->tree_widget, sort_path, NULL, FALSE);
    if (ctk_widget_get_realized (CTK_WIDGET (view->details->tree_widget)))
    {
        ctk_tree_view_scroll_to_cell (view->details->tree_widget, sort_path, NULL, FALSE, 0, 0);
    }
    ctk_tree_path_free (sort_path);

    baul_file_unref (file);
    refresh_highlight (view);

    return FALSE;
}

static void
schedule_show_selection (FMTreeView *view)
{
    if (view->details->show_selection_idle_id == 0)
    {
        view->details->show_selection_idle_id = g_idle_add (show_selection_idle_callback, view);
    }
}

static void
schedule_select_and_show_location (FMTreeView *view, char *location)
{
    if (view->details->selection_location != NULL)
    {
        g_free (view->details->selection_location);
    }
    view->details->selection_location = g_strdup (location);
    schedule_show_selection (view);
}

static void
row_loaded_callback (CtkTreeModel     *tree_model,
                     CtkTreeIter      *iter,
                     FMTreeView *view)
{
    BaulFile *file, *tmp_file, *selection_file;

    if (view->details->selection_location == NULL
            || !view->details->selecting
            || iter->user_data == NULL || iter->stamp == 0)
    {
        return;
    }

    file = fm_tree_model_iter_get_file (view->details->child_model, iter);
    if (file == NULL)
    {
        return;
    }
    if (!baul_file_is_directory (file))
    {
        baul_file_unref(file);
        return;
    }

    /* if iter is ancestor of wanted selection_location then update selection */
    selection_file = baul_file_get_by_uri (view->details->selection_location);
    while (selection_file != NULL)
    {
        if (file == selection_file)
        {
            baul_file_unref (file);
            baul_file_unref (selection_file);

            schedule_show_selection (view);
            return;
        }
        tmp_file = baul_file_get_parent (selection_file);
        baul_file_unref (selection_file);
        selection_file = tmp_file;
    }
    baul_file_unref (file);
}

static BaulFile *
sort_model_iter_to_file (FMTreeView *view, CtkTreeIter *iter)
{
    CtkTreeIter child_iter;

    ctk_tree_model_sort_convert_iter_to_child_iter (view->details->sort_model, &child_iter, iter);
    return fm_tree_model_iter_get_file (view->details->child_model, &child_iter);
}

static BaulFile *
sort_model_path_to_file (FMTreeView *view, CtkTreePath *path)
{
    CtkTreeIter iter;

    if (!ctk_tree_model_get_iter (CTK_TREE_MODEL (view->details->sort_model), &iter, path))
    {
        return NULL;
    }
    return sort_model_iter_to_file (view, &iter);
}

static void
got_activation_uri_callback (BaulFile *file, gpointer callback_data)
{
    char *uri, *file_uri;
    FMTreeView *view;
    CdkScreen *screen;
    GFile *location;
    BaulWindowSlotInfo *slot;
    gboolean open_in_same_slot;

    view = FM_TREE_VIEW (callback_data);

    screen = ctk_widget_get_screen (CTK_WIDGET (view->details->tree_widget));

    g_assert (file == view->details->activation_file);

    open_in_same_slot =
        (view->details->activation_flags &
         (BAUL_WINDOW_OPEN_FLAG_NEW_WINDOW |
          BAUL_WINDOW_OPEN_FLAG_NEW_TAB)) == 0;

    slot = baul_window_info_get_active_slot (view->details->window);

    uri = baul_file_get_activation_uri (file);
    if (baul_file_is_launcher (file))
    {
        file_uri = baul_file_get_uri (file);
        baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                        "tree view launch_desktop_file window=%p: %s",
                        view->details->window, file_uri);
        baul_launch_desktop_file (screen, file_uri, NULL, NULL);
        g_free (file_uri);
    }
    else if (uri != NULL
             && baul_file_is_executable (file)
             && baul_file_can_execute (file)
             && !baul_file_is_directory (file))
    {

        file_uri = g_filename_from_uri (uri, NULL, NULL);

        /* Non-local executables don't get launched. They act like non-executables. */
        if (file_uri == NULL)
        {
            baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                            "tree view window_info_open_location window=%p: %s",
                            view->details->window, uri);
            location = g_file_new_for_uri (uri);
            baul_window_slot_info_open_location
            (slot,
             location,
             BAUL_WINDOW_OPEN_ACCORDING_TO_MODE,
             view->details->activation_flags,
             NULL);
            g_object_unref (location);
        }
        else
        {
            baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                            "tree view launch_application_from_command window=%p: %s",
                            view->details->window, file_uri);
            baul_launch_application_from_command (screen, NULL, file_uri, FALSE, NULL);
            g_free (file_uri);
        }

    }
    else if (uri != NULL)
    {
        if (!open_in_same_slot ||
                view->details->selection_location == NULL ||
                strcmp (uri, view->details->selection_location) != 0)
        {
            if (open_in_same_slot)
            {
                if (view->details->selection_location != NULL)
                {
                    g_free (view->details->selection_location);
                }
                view->details->selection_location = g_strdup (uri);
            }

            baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                            "tree view window_info_open_location window=%p: %s",
                            view->details->window, uri);
            location = g_file_new_for_uri (uri);
            baul_window_slot_info_open_location
            (slot,
             location,
             BAUL_WINDOW_OPEN_ACCORDING_TO_MODE,
             view->details->activation_flags,
             NULL);
            g_object_unref (location);
        }
    }

    g_free (uri);
    baul_file_unref (view->details->activation_file);
    view->details->activation_file = NULL;
}

static void
cancel_activation (FMTreeView *view)
{
    if (view->details->activation_file == NULL)
    {
        return;
    }

    baul_file_cancel_call_when_ready
    (view->details->activation_file,
     got_activation_uri_callback, view);
    baul_file_unref (view->details->activation_file);
    view->details->activation_file = NULL;
}

static void
row_activated_callback (CtkTreeView *treeview, CtkTreePath *path,
                        CtkTreeViewColumn *column, FMTreeView *view)
{
    if (ctk_tree_view_row_expanded (view->details->tree_widget, path))
    {
        ctk_tree_view_collapse_row (view->details->tree_widget, path);
    }
    else
    {
        ctk_tree_view_expand_row (view->details->tree_widget,
                                  path, FALSE);
    }
}

static gboolean
selection_changed_timer_callback(FMTreeView *view)
{
    BaulFileAttributes attributes;
    CtkTreeIter iter;
    CtkTreeSelection *selection;

    selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (view->details->tree_widget));

    /* no activation if popup menu is open */
    if (view->details->popup_file != NULL)
    {
        return FALSE;
    }

    cancel_activation (view);

    if (!ctk_tree_selection_get_selected (selection, NULL, &iter))
    {
        return FALSE;
    }

    view->details->activation_file = sort_model_iter_to_file (view, &iter);
    if (view->details->activation_file == NULL)
    {
        return FALSE;
    }
    view->details->activation_flags = 0;

    attributes = BAUL_FILE_ATTRIBUTE_INFO | BAUL_FILE_ATTRIBUTE_LINK_INFO;
    baul_file_call_when_ready (view->details->activation_file, attributes,
                               got_activation_uri_callback, view);
    return FALSE; /* remove timeout */
}

static void
selection_changed_callback (CtkTreeSelection *selection,
                            FMTreeView *view)
{
    CdkEvent *event;
    gboolean is_keyboard;

    if (view->details->selection_changed_timer)
    {
        g_source_remove (view->details->selection_changed_timer);
        view->details->selection_changed_timer = 0;
    }

    event = ctk_get_current_event ();
    if (event)
    {
        is_keyboard = (event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE);
        cdk_event_free (event);

        if (is_keyboard)
        {
            /* on keyboard event: delay the change */
            /* TODO: make dependent on keyboard repeat rate as per Markus Bertheau ? */
            view->details->selection_changed_timer = g_timeout_add (300, (GSourceFunc) selection_changed_timer_callback, view);
        }
        else
        {
            /* on mouse event: show the change immediately */
            selection_changed_timer_callback (view);
        }
    }
}

static int
compare_rows (CtkTreeModel *model, CtkTreeIter *a, CtkTreeIter *b, gpointer callback_data)
{
    BaulFile *file_a, *file_b;
    int result;

    /* Dummy rows are always first */
    if (a->user_data == NULL)
    {
        return -1;
    }
    else if (b->user_data == NULL)
    {
        return 1;
    }

    /* don't sort root nodes */
    if (fm_tree_model_iter_is_root (FM_TREE_MODEL (model), a) &&
            fm_tree_model_iter_is_root (FM_TREE_MODEL (model), b))
    {
        return fm_tree_model_iter_compare_roots (FM_TREE_MODEL (model), a, b);
    }

    file_a = fm_tree_model_iter_get_file (FM_TREE_MODEL (model), a);
    file_b = fm_tree_model_iter_get_file (FM_TREE_MODEL (model), b);

    if (file_a == file_b)
    {
        result = 0;
    }
    else if (file_a == NULL)
    {
        result = -1;
    }
    else if (file_b == NULL)
    {
        result = +1;
    }
    else
    {
        result = baul_file_compare_for_sort (file_a, file_b,
                                             BAUL_FILE_SORT_BY_DISPLAY_NAME,
                                             FALSE, FALSE);
    }

    baul_file_unref (file_a);
    baul_file_unref (file_b);

    return result;
}


static char *
get_root_uri_callback (BaulTreeViewDragDest *dest,
                       gpointer user_data)
{
    /* Don't allow drops on background */
    return NULL;
}

static BaulFile *
get_file_for_path_callback (BaulTreeViewDragDest *dest,
                            CtkTreePath *path,
                            gpointer user_data)
{
    FMTreeView *view;

    view = FM_TREE_VIEW (user_data);

    return sort_model_path_to_file (view, path);
}

static void
move_copy_items_callback (BaulTreeViewDragDest *dest,
                          const GList *item_uris,
                          const char *target_uri,
                          CdkDragAction action,
                          int x,
                          int y,
                          gpointer user_data)
{
    FMTreeView *view;

    view = FM_TREE_VIEW (user_data);

    baul_clipboard_clear_if_colliding_uris (CTK_WIDGET (view),
                                            item_uris,
                                            copied_files_atom);
    baul_file_operations_copy_move
    (item_uris,
     NULL,
     target_uri,
     action,
     CTK_WIDGET (view->details->tree_widget),
     NULL, NULL);
}

static void
add_root_for_mount (FMTreeView *view,
                    GMount *mount)
{
    char *mount_uri, *name;
    GFile *root;
    GIcon *icon;

    if (g_mount_is_shadowed (mount))
        return;

    icon = g_mount_get_icon (mount);
    root = g_mount_get_root (mount);
    mount_uri = g_file_get_uri (root);
    g_object_unref (root);
    name = g_mount_get_name (mount);

    fm_tree_model_add_root_uri(view->details->child_model,
                               mount_uri, name, icon, mount);

    g_object_unref (icon);
    g_free (name);
    g_free (mount_uri);

}

static void
mount_added_callback (GVolumeMonitor *volume_monitor,
                      GMount *mount,
                      FMTreeView *view)
{
    add_root_for_mount (view, mount);
}

static void
mount_removed_callback (GVolumeMonitor *volume_monitor,
                        GMount *mount,
                        FMTreeView *view)
{
    GFile *root;
    char *mount_uri;

    root = g_mount_get_root (mount);
    mount_uri = g_file_get_uri (root);
    g_object_unref (root);
    fm_tree_model_remove_root_uri (view->details->child_model,
                                   mount_uri);
    g_free (mount_uri);
}

static void
clipboard_contents_received_callback (CtkClipboard     *clipboard,
                                      CtkSelectionData *selection_data,
                                      gpointer          data)
{
    FMTreeView *view;

    view = FM_TREE_VIEW (data);

    if (ctk_selection_data_get_data_type (selection_data) == copied_files_atom
            && ctk_selection_data_get_length (selection_data) > 0 &&
            view->details->popup != NULL)
    {
        ctk_widget_set_sensitive (view->details->popup_paste, TRUE);
    }

    g_object_unref (view);
}

static gboolean
is_parent_writable (BaulFile *file)
{
    BaulFile *parent;
    gboolean result;

    parent = baul_file_get_parent (file);

    /* No parent directory, return FALSE */
    if (parent == NULL)
    {
        return FALSE;
    }

    result = baul_file_can_write (parent);
    baul_file_unref (parent);

    return result;
}

static gboolean
button_pressed_callback (CtkTreeView *treeview, CdkEventButton *event,
                         FMTreeView *view)
{
    CtkTreePath *path, *cursor_path;
    gboolean parent_file_is_writable;
    gboolean file_is_home_or_desktop;
    gboolean file_is_special_link;
    gboolean can_move_file_to_trash;
    gboolean can_delete_file;

    if (event->button == 3)
    {
        gboolean show_unmount = FALSE;
        gboolean show_eject = FALSE;
        GMount *mount = NULL;

        if (view->details->popup_file != NULL)
        {
            return FALSE; /* Already up, ignore */
        }

        if (!ctk_tree_view_get_path_at_pos (treeview, event->x, event->y,
                                            &path, NULL, NULL, NULL))
        {
            return FALSE;
        }

        view->details->popup_file = sort_model_path_to_file (view, path);
        if (view->details->popup_file == NULL)
        {
            ctk_tree_path_free (path);
            return FALSE;
        }
        ctk_tree_view_get_cursor (view->details->tree_widget, &cursor_path, NULL);
        ctk_tree_view_set_cursor (view->details->tree_widget, path, NULL, FALSE);
        ctk_tree_path_free (path);

        create_popup_menu (view);

        ctk_widget_set_sensitive (view->details->popup_open_in_new_window,
                                  baul_file_is_directory (view->details->popup_file));
        ctk_widget_set_sensitive (view->details->popup_create_folder,
                                  baul_file_is_directory (view->details->popup_file) &&
                                  baul_file_can_write (view->details->popup_file));
        ctk_widget_set_sensitive (view->details->popup_paste, FALSE);
        if (baul_file_is_directory (view->details->popup_file) &&
                baul_file_can_write (view->details->popup_file))
        {
            ctk_clipboard_request_contents (baul_clipboard_get (CTK_WIDGET (view->details->tree_widget)),
                                            copied_files_atom,
                                            clipboard_contents_received_callback, g_object_ref (view));
        }
        can_move_file_to_trash = baul_file_can_trash (view->details->popup_file);
        ctk_widget_set_sensitive (view->details->popup_trash, can_move_file_to_trash);

        if (g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_ENABLE_DELETE))
        {
            parent_file_is_writable = is_parent_writable (view->details->popup_file);
            file_is_home_or_desktop = baul_file_is_home (view->details->popup_file)
                                      || baul_file_is_desktop_directory (view->details->popup_file);
            file_is_special_link = BAUL_IS_DESKTOP_ICON_FILE (view->details->popup_file);

            can_delete_file = parent_file_is_writable
                              && !file_is_home_or_desktop
                              && !file_is_special_link;

            ctk_widget_show (view->details->popup_delete);
            ctk_widget_set_sensitive (view->details->popup_delete, can_delete_file);
        }
        else
        {
            ctk_widget_hide (view->details->popup_delete);
        }

        mount = fm_tree_model_get_mount_for_root_node_file (view->details->child_model, view->details->popup_file);
        if (mount)
        {
            show_unmount = g_mount_can_unmount (mount);
            show_eject = g_mount_can_eject (mount);
        }

        if (show_unmount)
        {
            ctk_widget_show (view->details->popup_unmount);
        }
        else
        {
            ctk_widget_hide (view->details->popup_unmount);
        }

        if (show_eject)
        {
            ctk_widget_show (view->details->popup_eject);
        }
        else
        {
            ctk_widget_hide (view->details->popup_eject);
        }

        if (show_unmount || show_eject)
        {
            ctk_widget_show (view->details->popup_unmount_separator);
        }
        else
        {
            ctk_widget_hide (view->details->popup_unmount_separator);
        }

        ctk_menu_popup_at_pointer (CTK_MENU (view->details->popup),
                                   (const CdkEvent*) event);

        ctk_tree_view_set_cursor (view->details->tree_widget, cursor_path, NULL, FALSE);
        ctk_tree_path_free (cursor_path);

        return TRUE;
    }
    else if (event->button == 2 && event->type == GDK_BUTTON_PRESS)
    {
        BaulFile *file;

        if (!ctk_tree_view_get_path_at_pos (treeview, event->x, event->y,
                                            &path, NULL, NULL, NULL))
        {
            return FALSE;
        }

        file = sort_model_path_to_file (view, path);
        if (file)
        {
            fm_tree_view_activate_file (view, file,
                                        (event->state & GDK_CONTROL_MASK) != 0 ?
                                        BAUL_WINDOW_OPEN_FLAG_NEW_WINDOW :
                                        BAUL_WINDOW_OPEN_FLAG_NEW_TAB);
            baul_file_unref (file);
        }

        ctk_tree_path_free (path);

        return TRUE;
    }

    return FALSE;
}

static void
fm_tree_view_activate_file (FMTreeView *view,
                            BaulFile *file,
                            BaulWindowOpenFlags flags)
{
    BaulFileAttributes attributes;

    cancel_activation (view);

    view->details->activation_file = baul_file_ref (file);
    view->details->activation_flags = flags;

    attributes = BAUL_FILE_ATTRIBUTE_INFO | BAUL_FILE_ATTRIBUTE_LINK_INFO;
    baul_file_call_when_ready (view->details->activation_file, attributes,
                               got_activation_uri_callback, view);
}

static void
fm_tree_view_open_cb (CtkWidget *menu_item,
                      FMTreeView *view)
{
    fm_tree_view_activate_file (view, view->details->popup_file, 0);
}

static void
fm_tree_view_open_in_new_tab_cb (CtkWidget *menu_item,
                                 FMTreeView *view)
{
    fm_tree_view_activate_file (view, view->details->popup_file, BAUL_WINDOW_OPEN_FLAG_NEW_TAB);
}

static void
fm_tree_view_open_in_new_window_cb (CtkWidget *menu_item,
                                    FMTreeView *view)
{
    /* fm_tree_view_activate_file (view, view->details->popup_file, BAUL_WINDOW_OPEN_FLAG_NEW_WINDOW); */

    baul_mime_activate_file  (fm_tree_view_get_containing_window (view),
                              baul_window_info_get_active_slot (view->details->window),
                              view->details->popup_file,
                              g_file_get_path (view->details->popup_file->details->directory->details->location),
                              BAUL_WINDOW_OPEN_FLAG_NEW_WINDOW,
                              0);
}

static void
new_folder_done (GFile *new_folder, gpointer data)
{
    GList *list;

    /* show the properties window for the newly created
     * folder so the user can change its name
     */
    list = g_list_prepend (NULL, baul_file_get (new_folder));

    fm_properties_window_present (list, CTK_WIDGET (data));

    baul_file_list_free (list);
}

static void
fm_tree_view_create_folder_cb (CtkWidget *menu_item,
                               FMTreeView *view)
{
    char *parent_uri;

    parent_uri = baul_file_get_uri (view->details->popup_file);
    baul_file_operations_new_folder (CTK_WIDGET (view->details->tree_widget),
                                     NULL,
                                     parent_uri,
                                     new_folder_done, view->details->tree_widget);

    g_free (parent_uri);
}

static void
copy_or_cut_files (FMTreeView *view,
                   gboolean cut)
{
    char *status_string, *name;
    BaulClipboardInfo info;
    CtkTargetList *target_list;
    CtkTargetEntry *targets;
    int n_targets;

    info.cut = cut;
    info.files = g_list_prepend (NULL, view->details->popup_file);

    target_list = ctk_target_list_new (NULL, 0);
    ctk_target_list_add (target_list, copied_files_atom, 0, 0);
    ctk_target_list_add_uri_targets (target_list, 0);
    ctk_target_list_add_text_targets (target_list, 0);

    targets = ctk_target_table_new_from_list (target_list, &n_targets);
    ctk_target_list_unref (target_list);

    ctk_clipboard_set_with_data (baul_clipboard_get (CTK_WIDGET (view->details->tree_widget)),
                                 targets, n_targets,
                                 baul_get_clipboard_callback, baul_clear_clipboard_callback,
                                 NULL);
    ctk_target_table_free (targets, n_targets);

    baul_clipboard_monitor_set_clipboard_info (baul_clipboard_monitor_get (),
            &info);
    g_list_free (info.files);

    name = baul_file_get_display_name (view->details->popup_file);
    if (cut)
    {
        status_string = g_strdup_printf (_("\"%s\" will be moved "
                                           "if you select the Paste command"),
                                         name);
    }
    else
    {
        status_string = g_strdup_printf (_("\"%s\" will be copied "
                                           "if you select the Paste command"),
                                         name);
    }
    g_free (name);

    baul_window_info_push_status (view->details->window,
                                  status_string);
    g_free (status_string);
}

static void
fm_tree_view_cut_cb (CtkWidget *menu_item,
                     FMTreeView *view)
{
    copy_or_cut_files (view, TRUE);
}

static void
fm_tree_view_copy_cb (CtkWidget *menu_item,
                      FMTreeView *view)
{
    copy_or_cut_files (view, FALSE);
}

static void
paste_clipboard_data (FMTreeView *view,
                      CtkSelectionData *selection_data,
                      char *destination_uri)
{
    gboolean cut;
    GList *item_uris;

    cut = FALSE;
    item_uris = baul_clipboard_get_uri_list_from_selection_data (selection_data, &cut,
                copied_files_atom);

    if (item_uris == NULL|| destination_uri == NULL)
    {
        baul_window_info_push_status (view->details->window,
                                      _("There is nothing on the clipboard to paste."));
    }
    else
    {
        baul_file_operations_copy_move
        (item_uris, NULL, destination_uri,
         cut ? GDK_ACTION_MOVE : GDK_ACTION_COPY,
         CTK_WIDGET (view->details->tree_widget),
         NULL, NULL);

        /* If items are cut then remove from clipboard */
        if (cut)
        {
            ctk_clipboard_clear (baul_clipboard_get (CTK_WIDGET (view)));
        }

    	g_list_free_full (item_uris, g_free);
    }
}

static void
paste_into_clipboard_received_callback (CtkClipboard     *clipboard,
                                        CtkSelectionData *selection_data,
                                        gpointer          data)
{
    FMTreeView *view;
    char *directory_uri;

    view = FM_TREE_VIEW (data);

    directory_uri = baul_file_get_uri (view->details->popup_file);

    paste_clipboard_data (view, selection_data, directory_uri);

    g_free (directory_uri);
}

static void
fm_tree_view_paste_cb (CtkWidget *menu_item,
                       FMTreeView *view)
{
    ctk_clipboard_request_contents (baul_clipboard_get (CTK_WIDGET (view->details->tree_widget)),
                                    copied_files_atom,
                                    paste_into_clipboard_received_callback, view);
}

static CtkWindow *
fm_tree_view_get_containing_window (FMTreeView *view)
{
    CtkWidget *window;

    g_assert (FM_IS_TREE_VIEW (view));

    window = ctk_widget_get_ancestor (CTK_WIDGET (view), CTK_TYPE_WINDOW);
    if (window == NULL)
    {
        return NULL;
    }

    return CTK_WINDOW (window);
}

static void
fm_tree_view_trash_cb (CtkWidget *menu_item,
                       FMTreeView *view)
{
    GList *list;

    if (!baul_file_can_trash (view->details->popup_file))
    {
        return;
    }

    list = g_list_prepend (NULL,
                           baul_file_get_location (view->details->popup_file));

    baul_file_operations_trash_or_delete (list,
                                          fm_tree_view_get_containing_window (view),
                                          NULL, NULL);
    g_list_free_full (list, g_object_unref);
}

static void
fm_tree_view_delete_cb (CtkWidget *menu_item,
                        FMTreeView *view)
{
    GList *location_list;

    if (!g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_ENABLE_DELETE))
    {
        return;
    }

    location_list = g_list_prepend (NULL,
                                    baul_file_get_location (view->details->popup_file));

    baul_file_operations_delete (location_list, fm_tree_view_get_containing_window (view), NULL, NULL);
    g_list_free_full (location_list, g_object_unref);
}

static void
fm_tree_view_properties_cb (CtkWidget *menu_item,
                            FMTreeView *view)
{
    GList *list;

    list = g_list_prepend (NULL, baul_file_ref (view->details->popup_file));

    fm_properties_window_present (list, CTK_WIDGET (view->details->tree_widget));

    baul_file_list_free (list);
}

static void
fm_tree_view_unmount_cb (CtkWidget *menu_item,
                         FMTreeView *view)
{
    BaulFile *file = view->details->popup_file;
    GMount *mount;

    if (file == NULL)
    {
        return;
    }

    mount = fm_tree_model_get_mount_for_root_node_file (view->details->child_model, file);

    if (mount != NULL)
    {
        baul_file_operations_unmount_mount (fm_tree_view_get_containing_window (view),
                                            mount, FALSE, TRUE);
    }
}

static void
fm_tree_view_eject_cb (CtkWidget *menu_item,
                       FMTreeView *view)
{
    BaulFile *file = view->details->popup_file;
    GMount *mount;

    if (file == NULL)
    {
        return;
    }

    mount = fm_tree_model_get_mount_for_root_node_file (view->details->child_model, file);

    if (mount != NULL)
    {
        baul_file_operations_unmount_mount (fm_tree_view_get_containing_window (view),
                                            mount, TRUE, TRUE);
    }
}

static gboolean
free_popup_file_in_idle_cb (gpointer data)
{
    FMTreeView *view;

    view = FM_TREE_VIEW (data);

    if (view->details->popup_file != NULL)
    {
        baul_file_unref (view->details->popup_file);
        view->details->popup_file = NULL;
    }
    view->details->popup_file_idle_handler = 0;
    return FALSE;
}

static void
popup_menu_deactivated (CtkMenuShell *menu_shell, gpointer data)
{
    FMTreeView *view;

    view = FM_TREE_VIEW (data);

    /* The popup menu is deactivated. (I.E. hidden)
       We want to free popup_file, but can't right away as it might immediately get
       used if we're deactivation due to activating a menu item. So, we free it in
       idle */

    if (view->details->popup_file != NULL &&
            view->details->popup_file_idle_handler == 0)
    {
        view->details->popup_file_idle_handler = g_idle_add (free_popup_file_in_idle_cb, view);
    }
}

static void
create_popup_menu (FMTreeView *view)
{
    CtkWidget *popup, *menu_item;

    if (view->details->popup != NULL)
    {
        /* already created */
        return;
    }

    popup = ctk_menu_new ();

    ctk_menu_set_reserve_toggle_size (CTK_MENU (popup), FALSE);

    g_signal_connect (popup, "deactivate",
                      G_CALLBACK (popup_menu_deactivated),
                      view);


    /* add the "open" menu item */
    menu_item = eel_image_menu_item_new_from_icon ("document-open", _("_Open"));
    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (fm_tree_view_open_cb),
                      view);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup), menu_item);
    view->details->popup_open = menu_item;

    /* add the "open in new tab" menu item */
    menu_item = eel_image_menu_item_new_from_icon (NULL, _("Open in New _Tab"));
    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (fm_tree_view_open_in_new_tab_cb),
                      view);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup), menu_item);
    view->details->popup_open_in_new_window = menu_item;

    /* add the "open in new window" menu item */
    menu_item = eel_image_menu_item_new_from_icon (NULL, _("Open in New _Window"));
    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (fm_tree_view_open_in_new_window_cb),
                      view);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup), menu_item);
    view->details->popup_open_in_new_window = menu_item;

    eel_ctk_menu_append_separator (CTK_MENU (popup));

    /* add the "create folder" menu item */
    menu_item = eel_image_menu_item_new_from_icon (NULL, _("Create _Folder"));
    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (fm_tree_view_create_folder_cb),
                      view);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup), menu_item);
    view->details->popup_create_folder = menu_item;

    eel_ctk_menu_append_separator (CTK_MENU (popup));

    /* add the "cut folder" menu item */
    menu_item = eel_image_menu_item_new_from_icon ("edit-cut", _("Cu_t"));
    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (fm_tree_view_cut_cb),
                      view);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup), menu_item);
    view->details->popup_cut = menu_item;

    /* add the "copy folder" menu item */
    menu_item = eel_image_menu_item_new_from_icon ("edit-copy", _("_Copy"));
    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (fm_tree_view_copy_cb),
                      view);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup), menu_item);
    view->details->popup_copy = menu_item;

    /* add the "paste files into folder" menu item */
    menu_item = eel_image_menu_item_new_from_icon ("edit-paste", _("_Paste Into Folder"));
    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (fm_tree_view_paste_cb),
                      view);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup), menu_item);
    view->details->popup_paste = menu_item;

    eel_ctk_menu_append_separator (CTK_MENU (popup));

    /* add the "move to trash" menu item */
    menu_item = eel_image_menu_item_new_from_icon (BAUL_ICON_TRASH_FULL, _("Mo_ve to Trash"));
    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (fm_tree_view_trash_cb),
                      view);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup), menu_item);
    view->details->popup_trash = menu_item;

    /* add the "delete" menu item */
    menu_item = eel_image_menu_item_new_from_icon (BAUL_ICON_DELETE, _("_Delete"));
    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (fm_tree_view_delete_cb),
                      view);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup), menu_item);
    view->details->popup_delete = menu_item;

    eel_ctk_menu_append_separator (CTK_MENU (popup));

    /* add the "Unmount" menu item */
    menu_item = eel_image_menu_item_new_from_icon ("media-eject", _("_Unmount"));
    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (fm_tree_view_unmount_cb),
                      view);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup), menu_item);
    view->details->popup_unmount = menu_item;

    /* add the "Eject" menu item */
    menu_item = eel_image_menu_item_new_from_icon ("media-eject", _("_Eject"));
    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (fm_tree_view_eject_cb),
                      view);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup), menu_item);
    view->details->popup_eject = menu_item;

    /* add the unmount separator menu item */
    view->details->popup_unmount_separator =
        CTK_WIDGET (eel_ctk_menu_append_separator (CTK_MENU (popup)));

    /* add the "properties" menu item */
    menu_item = eel_image_menu_item_new_from_icon ("document-properties", _("_Properties"));
    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (fm_tree_view_properties_cb),
                      view);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup), menu_item);
    view->details->popup_properties = menu_item;

    view->details->popup = popup;
}

static void
create_tree (FMTreeView *view)
{
    CtkCellRenderer *cell;
    CtkTreeViewColumn *column;
    GVolumeMonitor *volume_monitor;
    char *home_uri;
    GList *mounts, *l;
    char *location;
    GIcon *icon;
    BaulWindowSlotInfo *slot;

    view->details->child_model = fm_tree_model_new ();
    view->details->sort_model = CTK_TREE_MODEL_SORT
                                (ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (view->details->child_model)));
    view->details->tree_widget = CTK_TREE_VIEW
                                 (ctk_tree_view_new_with_model (CTK_TREE_MODEL (view->details->sort_model)));
    g_object_unref (view->details->sort_model);

    ctk_tree_sortable_set_default_sort_func (CTK_TREE_SORTABLE (view->details->sort_model),
            compare_rows, view, NULL);

    g_signal_connect_object
    (view->details->child_model, "row_loaded",
     G_CALLBACK (row_loaded_callback),
     view, G_CONNECT_AFTER);
    home_uri = baul_get_home_directory_uri ();
    icon = g_themed_icon_new (BAUL_ICON_HOME);
    fm_tree_model_add_root_uri (view->details->child_model, home_uri, _("Home Folder"), icon, NULL);
    g_object_unref (icon);
    g_free (home_uri);
    icon = g_themed_icon_new (BAUL_ICON_FILESYSTEM);
    fm_tree_model_add_root_uri (view->details->child_model, "file:///", _("File System"), icon, NULL);
    g_object_unref (icon);
    icon = g_themed_icon_new (BAUL_ICON_TRASH);
    fm_tree_model_add_root_uri (view->details->child_model, "trash:///", _("Trash"), icon, NULL);
    g_object_unref (icon);


    volume_monitor = g_volume_monitor_get ();
    view->details->volume_monitor = volume_monitor;
    mounts = g_volume_monitor_get_mounts (volume_monitor);
    for (l = mounts; l != NULL; l = l->next)
    {
        add_root_for_mount (view, l->data);
        g_object_unref (l->data);
    }
    g_list_free (mounts);

    g_signal_connect_object (volume_monitor, "mount_added",
                             G_CALLBACK (mount_added_callback), view, 0);
    g_signal_connect_object (volume_monitor, "mount_removed",
                             G_CALLBACK (mount_removed_callback), view, 0);

    g_object_unref (view->details->child_model);

    ctk_tree_view_set_headers_visible (view->details->tree_widget, FALSE);

    view->details->drag_dest =
        baul_tree_view_drag_dest_new (view->details->tree_widget);
    g_signal_connect_object (view->details->drag_dest,
                             "get_root_uri",
                             G_CALLBACK (get_root_uri_callback),
                             view, 0);
    g_signal_connect_object (view->details->drag_dest,
                             "get_file_for_path",
                             G_CALLBACK (get_file_for_path_callback),
                             view, 0);
    g_signal_connect_object (view->details->drag_dest,
                             "move_copy_items",
                             G_CALLBACK (move_copy_items_callback),
                             view, 0);

    /* Create column */
    column = ctk_tree_view_column_new ();

    cell = ctk_cell_renderer_pixbuf_new ();
    ctk_tree_view_column_pack_start (column, cell, FALSE);
    ctk_tree_view_column_set_attributes (column, cell,
                                         "surface", FM_TREE_MODEL_CLOSED_SURFACE_COLUMN,
                                         NULL);

    cell = ctk_cell_renderer_text_new ();
    ctk_tree_view_column_pack_start (column, cell, TRUE);
    ctk_tree_view_column_set_attributes (column, cell,
                                         "text", FM_TREE_MODEL_DISPLAY_NAME_COLUMN,
                                         "style", FM_TREE_MODEL_FONT_STYLE_COLUMN,
                                         NULL);

    ctk_tree_view_append_column (view->details->tree_widget, column);

    ctk_widget_show (CTK_WIDGET (view->details->tree_widget));

    ctk_container_add (CTK_CONTAINER (view),
                       CTK_WIDGET (view->details->tree_widget));

    g_signal_connect_object (ctk_tree_view_get_selection (CTK_TREE_VIEW (view->details->tree_widget)), "changed",
                             G_CALLBACK (selection_changed_callback), view, 0);

    g_signal_connect (G_OBJECT (view->details->tree_widget),
                      "row-activated", G_CALLBACK (row_activated_callback),
                      view);

    g_signal_connect (G_OBJECT (view->details->tree_widget),
                      "button_press_event", G_CALLBACK (button_pressed_callback),
                      view);

    slot = baul_window_info_get_active_slot (view->details->window);
    location = baul_window_slot_info_get_current_location (slot);
    schedule_select_and_show_location (view, location);
    g_free (location);
}

static void
update_filtering_from_preferences (FMTreeView *view)
{
    BaulWindowShowHiddenFilesMode mode;

    if (view->details->child_model == NULL)
    {
        return;
    }

    mode = baul_window_info_get_hidden_files_mode (view->details->window);

    if (mode == BAUL_WINDOW_SHOW_HIDDEN_FILES_DEFAULT)
    {
        fm_tree_model_set_show_hidden_files
        (view->details->child_model,
         g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_SHOW_HIDDEN_FILES));
    }
    else
    {
        fm_tree_model_set_show_hidden_files
        (view->details->child_model,
         mode == BAUL_WINDOW_SHOW_HIDDEN_FILES_ENABLE);
    }
    fm_tree_model_set_show_only_directories
    (view->details->child_model,
     g_settings_get_boolean (baul_tree_sidebar_preferences, BAUL_PREFERENCES_TREE_SHOW_ONLY_DIRECTORIES));
}

static void
parent_set_callback (CtkWidget        *widget,
                     CtkWidget        *previous_parent,
                     gpointer          callback_data)
{
    FMTreeView *view;

    view = FM_TREE_VIEW (callback_data);

    if (ctk_widget_get_parent (widget) != NULL && view->details->tree_widget == NULL)
    {
        create_tree (view);
        update_filtering_from_preferences (view);
    }
}

static void
filtering_changed_callback (gpointer callback_data)
{
    update_filtering_from_preferences (FM_TREE_VIEW (callback_data));
}

static void
loading_uri_callback (BaulWindowInfo *window,
                      char *location,
                      gpointer callback_data)
{
    FMTreeView *view;

    view = FM_TREE_VIEW (callback_data);
    schedule_select_and_show_location (view, location);
}

static void
fm_tree_view_init (FMTreeView *view)
{
    view->details = g_new0 (FMTreeViewDetails, 1);

    ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (view),
                                    CTK_POLICY_AUTOMATIC,
                                    CTK_POLICY_AUTOMATIC);
    ctk_scrolled_window_set_hadjustment (CTK_SCROLLED_WINDOW (view), NULL);
    ctk_scrolled_window_set_vadjustment (CTK_SCROLLED_WINDOW (view), NULL);
    ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (view), CTK_SHADOW_IN);
    ctk_scrolled_window_set_overlay_scrolling (CTK_SCROLLED_WINDOW (view), FALSE);

    ctk_widget_show (CTK_WIDGET (view));

    g_signal_connect_object (view, "parent_set",
                             G_CALLBACK (parent_set_callback), view, 0);

    view->details->selection_location = NULL;

    view->details->selecting = FALSE;

    g_signal_connect_swapped (baul_preferences,
                              "changed::" BAUL_PREFERENCES_SHOW_HIDDEN_FILES,
                              G_CALLBACK(filtering_changed_callback),
                              view);
    g_signal_connect_swapped (baul_tree_sidebar_preferences,
                              "changed::" BAUL_PREFERENCES_TREE_SHOW_ONLY_DIRECTORIES,
                              G_CALLBACK (filtering_changed_callback), view);
    view->details->popup_file = NULL;

    view->details->clipboard_handler_id =
        g_signal_connect (baul_clipboard_monitor_get (),
                          "clipboard_info",
                          G_CALLBACK (notify_clipboard_info), view);
}

static void
fm_tree_view_dispose (GObject *object)
{
    FMTreeView *view;

    view = FM_TREE_VIEW (object);

    if (view->details->selection_changed_timer)
    {
        g_source_remove (view->details->selection_changed_timer);
        view->details->selection_changed_timer = 0;
    }

    if (view->details->drag_dest)
    {
        g_object_unref (view->details->drag_dest);
        view->details->drag_dest = NULL;
    }

    if (view->details->show_selection_idle_id)
    {
        g_source_remove (view->details->show_selection_idle_id);
        view->details->show_selection_idle_id = 0;
    }

    if (view->details->clipboard_handler_id != 0)
    {
        g_signal_handler_disconnect (baul_clipboard_monitor_get (),
                                     view->details->clipboard_handler_id);
        view->details->clipboard_handler_id = 0;
    }

    cancel_activation (view);

    if (view->details->popup != NULL)
    {
        ctk_widget_destroy (view->details->popup);
        view->details->popup = NULL;
    }

    if (view->details->popup_file_idle_handler != 0)
    {
        g_source_remove (view->details->popup_file_idle_handler);
        view->details->popup_file_idle_handler = 0;
    }

    if (view->details->popup_file != NULL)
    {
        baul_file_unref (view->details->popup_file);
        view->details->popup_file = NULL;
    }

    if (view->details->selection_location != NULL)
    {
        g_free (view->details->selection_location);
        view->details->selection_location = NULL;
    }

    if (view->details->volume_monitor != NULL)
    {
        g_object_unref (view->details->volume_monitor);
        view->details->volume_monitor = NULL;
    }

    g_signal_handlers_disconnect_by_func (baul_preferences,
                                          G_CALLBACK(filtering_changed_callback),
                                          view);

    g_signal_handlers_disconnect_by_func (baul_tree_sidebar_preferences,
                                          G_CALLBACK(filtering_changed_callback),
                                          view);

    view->details->window = NULL;

    G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
fm_tree_view_finalize (GObject *object)
{
    FMTreeView *view;

    view = FM_TREE_VIEW (object);

    g_free (view->details);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
fm_tree_view_class_init (FMTreeViewClass *class)
{
    G_OBJECT_CLASS (class)->dispose = fm_tree_view_dispose;
    G_OBJECT_CLASS (class)->finalize = fm_tree_view_finalize;

    copied_files_atom = cdk_atom_intern ("x-special/cafe-copied-files", FALSE);
}

static const char *
fm_tree_view_get_sidebar_id (BaulSidebar *sidebar)
{
    return TREE_SIDEBAR_ID;
}

static char *
fm_tree_view_get_tab_label (BaulSidebar *sidebar)
{
    return g_strdup (_("Tree"));
}

static char *
fm_tree_view_get_tab_tooltip (BaulSidebar *sidebar)
{
    return g_strdup (_("Show Tree"));
}

static CdkPixbuf *
fm_tree_view_get_tab_icon (BaulSidebar *sidebar)
{
    return NULL;
}

static void
fm_tree_view_is_visible_changed (BaulSidebar *sidebar,
                                 gboolean         is_visible)
{
    /* Do nothing */
}

static void
hidden_files_mode_changed_callback (BaulWindowInfo *window,
                                    FMTreeView *view)
{
    update_filtering_from_preferences (view);
}

static void
fm_tree_view_iface_init (BaulSidebarIface *iface)
{
    iface->get_sidebar_id = fm_tree_view_get_sidebar_id;
    iface->get_tab_label = fm_tree_view_get_tab_label;
    iface->get_tab_tooltip = fm_tree_view_get_tab_tooltip;
    iface->get_tab_icon = fm_tree_view_get_tab_icon;
    iface->is_visible_changed = fm_tree_view_is_visible_changed;
}

static void
fm_tree_view_set_parent_window (FMTreeView *sidebar,
                                BaulWindowInfo *window)
{
    char *location;
    BaulWindowSlotInfo *slot;

    sidebar->details->window = window;

    slot = baul_window_info_get_active_slot (window);

    g_signal_connect_object (window, "loading_uri",
                             G_CALLBACK (loading_uri_callback), sidebar, 0);
    location = baul_window_slot_info_get_current_location (slot);
    loading_uri_callback (window, location, sidebar);
    g_free (location);

    g_signal_connect_object (window, "hidden_files_mode_changed",
                             G_CALLBACK (hidden_files_mode_changed_callback), sidebar, 0);

}

static BaulSidebar *
fm_tree_view_create (BaulSidebarProvider *provider,
                     BaulWindowInfo *window)
{
    FMTreeView *sidebar;

    sidebar = g_object_new (fm_tree_view_get_type (), NULL);
    fm_tree_view_set_parent_window (sidebar, window);
    g_object_ref_sink (sidebar);

    return BAUL_SIDEBAR (sidebar);
}

static void
sidebar_provider_iface_init (BaulSidebarProviderIface *iface)
{
    iface->create = fm_tree_view_create;
}

static void
fm_tree_view_provider_init (FMTreeViewProvider *sidebar)
{
}

static void
fm_tree_view_provider_class_init (FMTreeViewProviderClass *class)
{
}

void
fm_tree_view_register (void)
{
    baul_module_add_type (fm_tree_view_provider_get_type ());
}
