/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 1999, 2000 Eazel, Inc.
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
 * Authors: John Sullivan <sullivan@eazel.com>
 */

/* baul-bookmarks-window.c - implementation of bookmark-editing window.
 */

#include <config.h>
#include <cairo-gobject.h>

#include <ctk/ctk.h>
#include <gdk/gdkkeysyms.h>

#include <eel/eel-ctk-extensions.h>
#include <eel/eel-cafe-extensions.h>

#include <libbaul-private/baul-global-preferences.h>

#include "baul-bookmarks-window.h"
#include "baul-window.h"
#include "baul-navigation-window.h"
#include "baul-spatial-window.h"

/* Static variables to keep track of window state. If there were
 * more than one bookmark-editing window, these would be struct or
 * class fields.
 */
static int		     bookmark_list_changed_signal_id;
static BaulBookmarkList *bookmarks = NULL;
static CtkTreeView	    *bookmark_list_widget = NULL; /* awkward name to distinguish from BaulBookmarkList */
static CtkListStore	    *bookmark_list_store = NULL;
static CtkListStore	    *bookmark_empty_list_store = NULL;
static CtkTreeSelection     *bookmark_selection = NULL;
static int                   selection_changed_id = 0;
static CtkWidget	    *name_field = NULL;
static int		     name_field_changed_signal_id;
static CtkWidget	    *remove_button = NULL;
static CtkWidget            *jump_button = NULL;
static gboolean		     text_changed = FALSE;
static gboolean		     name_text_changed = FALSE;
static CtkWidget	    *uri_field = NULL;
static int		     uri_field_changed_signal_id;
static int		     row_changed_signal_id;
static int		     row_deleted_signal_id;
static int                   row_activated_signal_id;
static int                   button_pressed_signal_id;
static int                   key_pressed_signal_id;
static int                   jump_button_signal_id;
static BaulApplication  *application;
static gboolean              parent_is_browser_window;

/* forward declarations */
static guint    get_selected_row                            (void);
static gboolean get_selection_exists                        (void);
static void     name_or_uri_field_activate                  (BaulEntry        *entry);
static void     baul_bookmarks_window_restore_geometry  (CtkWidget            *window);
static void     on_bookmark_list_changed                    (BaulBookmarkList *list,
        gpointer              user_data);
static void     on_name_field_changed                       (CtkEditable          *editable,
        gpointer              user_data);
static void     on_remove_button_clicked                    (CtkButton            *button,
        gpointer              user_data);
static void     on_jump_button_clicked                      (CtkButton            *button,
        gpointer              user_data);
static void	on_row_changed				    (CtkListStore	  *store,
        CtkTreePath	  *path,
        CtkTreeIter	  *iter,
        gpointer		   user_data);
static void	on_row_deleted				    (CtkListStore	  *store,
        CtkTreePath	  *path,
        gpointer		   user_data);
static void	on_row_activated			    (CtkTreeView	  *view,
        CtkTreePath	  *path,
        CtkTreeViewColumn    *column,
        gpointer		   user_data);
static gboolean	on_button_pressed                           (CtkTreeView	  *view,
        GdkEventButton       *event,
        gpointer		   user_data);
static gboolean	on_key_pressed                              (CtkTreeView	  *view,
        GdkEventKey          *event,
        gpointer		   user_data);
static void     on_selection_changed                        (CtkTreeSelection     *treeselection,
        gpointer              user_data);

static gboolean on_text_field_focus_out_event               (CtkWidget            *widget,
        GdkEventFocus        *event,
        gpointer              user_data);
static void     on_uri_field_changed                        (CtkEditable          *editable,
        gpointer              user_data);
static gboolean on_window_delete_event                      (CtkWidget            *widget,
        GdkEvent             *event,
        gpointer              user_data);
static void     on_window_hide_event                        (CtkWidget            *widget,
        gpointer              user_data);
static void     on_window_destroy_event                     (CtkWidget            *widget,
        gpointer              user_data);
static void     repopulate                                  (void);
static void     set_up_close_accelerator                    (CtkWidget            *window);
static void	open_selected_bookmark 			    (gpointer   user_data, GdkScreen *screen);
static void	update_bookmark_from_text		    (void);

/* We store a pointer to the bookmark in a column so when an item is moved
   with DnD we know which item it is. However we have to be careful to keep
   this in sync with the actual bookmark. Note that
   baul_bookmark_list_insert_item() makes a copy of the bookmark, so we
   have to fetch the new copy and update our pointer. */
#define BOOKMARK_LIST_COLUMN_ICON		0
#define BOOKMARK_LIST_COLUMN_NAME		1
#define BOOKMARK_LIST_COLUMN_BOOKMARK		2
#define BOOKMARK_LIST_COLUMN_STYLE		3
#define BOOKMARK_LIST_COLUMN_COUNT		4

/* layout constants */

/* Keep window from shrinking down ridiculously small; numbers are somewhat arbitrary */
#define BOOKMARKS_WINDOW_MIN_WIDTH	300
#define BOOKMARKS_WINDOW_MIN_HEIGHT	100

/* Larger size initially; user can stretch or shrink (but not shrink below min) */
#define BOOKMARKS_WINDOW_INITIAL_WIDTH	500
#define BOOKMARKS_WINDOW_INITIAL_HEIGHT	200

static void
baul_bookmarks_window_response_callback (CtkDialog *dialog,
        int response_id,
        gpointer callback_data)
{
    if (response_id == GTK_RESPONSE_HELP)
    {
        GError *error = NULL;

        ctk_show_uri_on_window (GTK_WINDOW (dialog),
                                "help:cafe-user-guide/gosbaul-36",
                                ctk_get_current_event_time (), &error);

        if (error)
        {
            CtkWidget *err_dialog;
            err_dialog = ctk_message_dialog_new (GTK_WINDOW (dialog),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_OK,
                                                 _("There was an error displaying help: \n%s"),
                                                 error->message);

            g_signal_connect (G_OBJECT (err_dialog),
                              "response", G_CALLBACK (ctk_widget_destroy),
                              NULL);
            ctk_window_set_resizable (GTK_WINDOW (err_dialog), FALSE);
            ctk_widget_show (err_dialog);
            g_error_free (error);
        }
    }
    else if (response_id == GTK_RESPONSE_CLOSE)
    {
        ctk_widget_hide (GTK_WIDGET (dialog));
    }
}

static CtkListStore *
create_bookmark_store (void)
{
    return ctk_list_store_new (BOOKMARK_LIST_COLUMN_COUNT,
                               CAIRO_GOBJECT_TYPE_SURFACE,
                               G_TYPE_STRING,
                               G_TYPE_OBJECT,
                               PANGO_TYPE_STYLE);
}

static void
setup_empty_list (void)
{
    CtkTreeIter iter;

    bookmark_empty_list_store = create_bookmark_store ();
    ctk_list_store_append (bookmark_empty_list_store, &iter);

    ctk_list_store_set (bookmark_empty_list_store, &iter,
                        BOOKMARK_LIST_COLUMN_NAME, _("No bookmarks defined"),
                        BOOKMARK_LIST_COLUMN_STYLE, PANGO_STYLE_ITALIC,
                        -1);
}

static void
bookmarks_set_empty (gboolean empty)
{
    CtkTreeIter iter;

    if (empty)
    {
        ctk_tree_view_set_model (bookmark_list_widget,
                                 GTK_TREE_MODEL (bookmark_empty_list_store));
        ctk_widget_set_sensitive (GTK_WIDGET (bookmark_list_widget), FALSE);
    }
    else
    {
        ctk_tree_view_set_model (bookmark_list_widget,
                                 GTK_TREE_MODEL (bookmark_list_store));
        ctk_widget_set_sensitive (GTK_WIDGET (bookmark_list_widget), TRUE);

        if (baul_bookmark_list_length (bookmarks) > 0 &&
                !get_selection_exists ())
        {
            ctk_tree_model_iter_nth_child (GTK_TREE_MODEL (bookmark_list_store),
                                           &iter, NULL, 0);
            ctk_tree_selection_select_iter (bookmark_selection, &iter);
        }
    }

    on_selection_changed (bookmark_selection, NULL);
}

static void
edit_bookmarks_dialog_reset_signals (gpointer data,
                                     GObject *obj)
{
    g_signal_handler_disconnect (jump_button,
                                 jump_button_signal_id);
    g_signal_handler_disconnect (bookmark_list_widget,
                                 row_activated_signal_id);
    jump_button_signal_id =
        g_signal_connect (jump_button, "clicked",
                          G_CALLBACK (on_jump_button_clicked), NULL);
    row_activated_signal_id =
        g_signal_connect (bookmark_list_widget, "row_activated",
                          G_CALLBACK (on_row_activated), NULL);
}

/**
 * create_bookmarks_window:
 *
 * Create a new bookmark-editing window.
 * @list: The BaulBookmarkList that this window will edit.
 *
 * Return value: A pointer to the new window.
 **/
CtkWindow *
create_bookmarks_window (BaulBookmarkList *list, BaulWindow *window_source)
{
    CtkWidget         *window;
    CtkTreeViewColumn *col;
    CtkCellRenderer   *rend;
    CtkBuilder        *builder;

    bookmarks = list;

    builder = ctk_builder_new ();
    if (!ctk_builder_add_from_file (builder,
                                    UIDIR  "/baul-bookmarks-window.ui",
                                    NULL))
    {
        return NULL;
    }

    window = (CtkWidget *)ctk_builder_get_object (builder, "bookmarks_dialog");
    bookmark_list_widget = (CtkTreeView *)ctk_builder_get_object (builder, "bookmark_tree_view");
    remove_button = (CtkWidget *)ctk_builder_get_object (builder, "bookmark_delete_button");
    jump_button = (CtkWidget *)ctk_builder_get_object (builder, "bookmark_jump_button");

    application = window_source->application;

    if (BAUL_IS_NAVIGATION_WINDOW (window_source))
    {
        parent_is_browser_window = TRUE;
    }
    else
    {
        parent_is_browser_window = FALSE;
    }

    set_up_close_accelerator (window);

    baul_bookmarks_window_restore_geometry (window);

    g_object_weak_ref (G_OBJECT (window_source), edit_bookmarks_dialog_reset_signals,
                       window_source);

    bookmark_list_widget = GTK_TREE_VIEW (ctk_builder_get_object (builder, "bookmark_tree_view"));

    rend = ctk_cell_renderer_pixbuf_new ();
    col = ctk_tree_view_column_new_with_attributes ("Icon",
            rend,
            "surface",
            BOOKMARK_LIST_COLUMN_ICON,
            NULL);
    ctk_tree_view_append_column (bookmark_list_widget,
                                 GTK_TREE_VIEW_COLUMN (col));
    ctk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (col),
                                          BAUL_ICON_SIZE_SMALLER);

    rend = ctk_cell_renderer_text_new ();
    g_object_set (rend,
                  "ellipsize", PANGO_ELLIPSIZE_END,
                  "ellipsize-set", TRUE,
                  NULL);

    col = ctk_tree_view_column_new_with_attributes ("Icon",
            rend,
            "text",
            BOOKMARK_LIST_COLUMN_NAME,
            "style",
            BOOKMARK_LIST_COLUMN_STYLE,
            NULL);
    ctk_tree_view_append_column (bookmark_list_widget,
                                 GTK_TREE_VIEW_COLUMN (col));

    bookmark_list_store = create_bookmark_store ();
    setup_empty_list ();
    ctk_tree_view_set_model (bookmark_list_widget,
                             GTK_TREE_MODEL (bookmark_empty_list_store));

    bookmark_selection =
        GTK_TREE_SELECTION (ctk_tree_view_get_selection (bookmark_list_widget));

    name_field = baul_entry_new ();

    ctk_widget_show (name_field);
    ctk_box_pack_start (GTK_BOX (ctk_builder_get_object (builder, "bookmark_name_placeholder")),
                        name_field, TRUE, TRUE, 0);

    ctk_label_set_mnemonic_widget (
        GTK_LABEL (ctk_builder_get_object (builder, "bookmark_name_label")),
        name_field);

    uri_field = baul_entry_new ();
    ctk_widget_show (uri_field);
    ctk_box_pack_start (GTK_BOX (ctk_builder_get_object (builder, "bookmark_location_placeholder")),
                        uri_field, TRUE, TRUE, 0);

    ctk_label_set_mnemonic_widget (
        GTK_LABEL (ctk_builder_get_object (builder, "bookmark_location_label")),
        uri_field);

    bookmark_list_changed_signal_id =
        g_signal_connect (bookmarks, "contents_changed",
                          G_CALLBACK (on_bookmark_list_changed), NULL);
    row_changed_signal_id =
        g_signal_connect (bookmark_list_store, "row_changed",
                          G_CALLBACK (on_row_changed), NULL);
    row_deleted_signal_id =
        g_signal_connect (bookmark_list_store, "row_deleted",
                          G_CALLBACK (on_row_deleted), NULL);
    row_activated_signal_id =
        g_signal_connect (bookmark_list_widget, "row_activated",
                          G_CALLBACK (on_row_activated), window_source);
    button_pressed_signal_id =
        g_signal_connect (bookmark_list_widget, "button_press_event",
                          G_CALLBACK (on_button_pressed), NULL);
    key_pressed_signal_id =
        g_signal_connect (bookmark_list_widget, "key_press_event",
                          G_CALLBACK (on_key_pressed), NULL);
    selection_changed_id =
        g_signal_connect (bookmark_selection, "changed",
                          G_CALLBACK (on_selection_changed), NULL);

    g_signal_connect (window, "delete_event",
                      G_CALLBACK (on_window_delete_event), NULL);
    g_signal_connect (window, "hide",
                      G_CALLBACK (on_window_hide_event), NULL);
    g_signal_connect (window, "destroy",
                      G_CALLBACK (on_window_destroy_event), NULL);
    g_signal_connect (window, "response",
                      G_CALLBACK (baul_bookmarks_window_response_callback), NULL);

    name_field_changed_signal_id =
        g_signal_connect (name_field, "changed",
                          G_CALLBACK (on_name_field_changed), NULL);

    g_signal_connect (name_field, "focus_out_event",
                      G_CALLBACK (on_text_field_focus_out_event), NULL);
    g_signal_connect (name_field, "activate",
                      G_CALLBACK (name_or_uri_field_activate), NULL);

    uri_field_changed_signal_id =
        g_signal_connect (uri_field, "changed",
                          G_CALLBACK (on_uri_field_changed), NULL);

    g_signal_connect (uri_field, "focus_out_event",
                      G_CALLBACK (on_text_field_focus_out_event), NULL);
    g_signal_connect (uri_field, "activate",
                      G_CALLBACK (name_or_uri_field_activate), NULL);
    g_signal_connect (remove_button, "clicked",
                      G_CALLBACK (on_remove_button_clicked), NULL);
    jump_button_signal_id =
        g_signal_connect (jump_button, "clicked",
                          G_CALLBACK (on_jump_button_clicked), window_source);

    ctk_tree_selection_set_mode (bookmark_selection, GTK_SELECTION_BROWSE);

    /* Fill in list widget with bookmarks, must be after signals are wired up. */
    repopulate();

    g_object_unref (builder);

    return GTK_WINDOW (window);
}

void
edit_bookmarks_dialog_set_signals (BaulWindow *window)
{

    g_signal_handler_disconnect (jump_button,
                                 jump_button_signal_id);
    g_signal_handler_disconnect (bookmark_list_widget,
                                 row_activated_signal_id);

    jump_button_signal_id =
        g_signal_connect (jump_button, "clicked",
                          G_CALLBACK (on_jump_button_clicked), window);
    row_activated_signal_id =
        g_signal_connect (bookmark_list_widget, "row_activated",
                          G_CALLBACK (on_row_activated), window);

    g_object_weak_ref (G_OBJECT (window), edit_bookmarks_dialog_reset_signals,
                       window);
}

static BaulBookmark *
get_selected_bookmark (void)
{
    g_return_val_if_fail(BAUL_IS_BOOKMARK_LIST(bookmarks), NULL);

    if (!get_selection_exists())
        return NULL;

    if (baul_bookmark_list_length (bookmarks) < 1)
        return NULL;

    return baul_bookmark_list_item_at(bookmarks, get_selected_row ());
}

static guint
get_selected_row (void)
{
    CtkTreeIter       iter;
    CtkTreePath      *path;
    CtkTreeModel     *model;
    gint		 *indices, row;

    g_assert (get_selection_exists());

    model = GTK_TREE_MODEL (bookmark_list_store);
    ctk_tree_selection_get_selected (bookmark_selection,
                                     &model,
                                     &iter);

    path = ctk_tree_model_get_path (model, &iter);
    indices = ctk_tree_path_get_indices (path);
    row = indices[0];
    ctk_tree_path_free (path);
    return row;
}

static gboolean
get_selection_exists (void)
{
    return ctk_tree_selection_get_selected (bookmark_selection, NULL, NULL);
}

static void
baul_bookmarks_window_restore_geometry (CtkWidget *window)
{
    const char *window_geometry;

    g_return_if_fail (GTK_IS_WINDOW (window));
    g_return_if_fail (BAUL_IS_BOOKMARK_LIST (bookmarks));

    window_geometry = baul_bookmark_list_get_window_geometry (bookmarks);

    if (window_geometry != NULL)
    {
        eel_ctk_window_set_initial_geometry_from_string
        (GTK_WINDOW (window), window_geometry,
         BOOKMARKS_WINDOW_MIN_WIDTH, BOOKMARKS_WINDOW_MIN_HEIGHT, FALSE);

    }
    else
    {
        /* use default since there was no stored geometry */
        ctk_window_set_default_size (GTK_WINDOW (window),
                                     BOOKMARKS_WINDOW_INITIAL_WIDTH,
                                     BOOKMARKS_WINDOW_INITIAL_HEIGHT);

        /* Let window manager handle default position if no position stored */
    }
}

/**
 * baul_bookmarks_window_save_geometry:
 *
 * Save window size & position to disk.
 * @window: The bookmarks window whose geometry should be saved.
 **/
void
baul_bookmarks_window_save_geometry (CtkWindow *window)
{
    g_return_if_fail (GTK_IS_WINDOW (window));
    g_return_if_fail (BAUL_IS_BOOKMARK_LIST (bookmarks));

    /* Don't bother if window is already closed */
    if (ctk_widget_get_visible (GTK_WIDGET (window)))
    {
        char *geometry_string;

        geometry_string = eel_ctk_window_get_geometry_string (window);

        baul_bookmark_list_set_window_geometry (bookmarks, geometry_string);
        g_free (geometry_string);
    }
}

static void
on_bookmark_list_changed (BaulBookmarkList *bookmarks, gpointer data)
{
    g_return_if_fail (BAUL_IS_BOOKMARK_LIST (bookmarks));

    /* maybe add logic here or in repopulate to save/restore selection */
    repopulate ();
}

static void
on_name_field_changed (CtkEditable *editable,
                       gpointer     user_data)
{
    CtkTreeIter   iter;
    g_return_if_fail(GTK_IS_TREE_VIEW(bookmark_list_widget));
    g_return_if_fail(GTK_IS_ENTRY(name_field));

    if (!get_selection_exists())
        return;

    /* Update text displayed in list instantly. Also remember that
     * user has changed text so we update real bookmark later.
     */
    ctk_tree_selection_get_selected (bookmark_selection,
                                     NULL,
                                     &iter);

    ctk_list_store_set (bookmark_list_store,
                        &iter, BOOKMARK_LIST_COLUMN_NAME,
                        ctk_entry_get_text (GTK_ENTRY (name_field)),
                        -1);
    text_changed = TRUE;
    name_text_changed = TRUE;
}

static void
open_selected_bookmark (gpointer user_data, GdkScreen *screen)
{
    BaulBookmark *selected;
    BaulWindow *window;
    GFile *location;

    selected = get_selected_bookmark ();

    if (!selected)
    {
        return;
    }

    location = baul_bookmark_get_location (selected);
    if (location == NULL)
    {
        return;
    }

    if (BAUL_IS_NAVIGATION_WINDOW (user_data))
    {
        window = user_data;
    }
    else if (BAUL_IS_SPATIAL_WINDOW (user_data))
    {
        window = baul_application_get_spatial_window (application,
                                                      NULL,
                                                      NULL,
                                                      location,
                                                      screen,
                                                      NULL);
    } else { /* window that opened bookmarks window has been closed */
        if (parent_is_browser_window || g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_ALWAYS_USE_BROWSER)) {
            window = baul_application_create_navigation_window (application,
                     screen);
        }
        else
        {
            window = baul_application_get_spatial_window (application,
                                                          NULL,
                                                          NULL,
                                                          location,
                                                          screen,
                                                          NULL);
        }
    }

    baul_window_go_to (window, location);

    g_object_unref (location);
}

static void
on_jump_button_clicked (CtkButton *button,
                        gpointer   user_data)
{
    GdkScreen *screen;

    screen = ctk_widget_get_screen (GTK_WIDGET (button));
    open_selected_bookmark (user_data, screen);
}

static void
bookmarks_delete_bookmark (void)
{
    CtkTreeIter iter;
    CtkTreePath *path;
    gint *indices, row, rows;

    g_assert (GTK_IS_TREE_VIEW (bookmark_list_widget));

    if (!ctk_tree_selection_get_selected (bookmark_selection, NULL, &iter))
        return;

    /* Remove the selected item from the list store. on_row_deleted() will
       remove it from the bookmark list. */
    path = ctk_tree_model_get_path (GTK_TREE_MODEL (bookmark_list_store),
                                    &iter);
    indices = ctk_tree_path_get_indices (path);
    row = indices[0];
    ctk_tree_path_free (path);

    ctk_list_store_remove (bookmark_list_store, &iter);

    /* Try to select the same row, or the last one in the list. */
    rows = ctk_tree_model_iter_n_children (GTK_TREE_MODEL (bookmark_list_store), NULL);
    if (row >= rows)
        row = rows - 1;

    if (row < 0)
    {
        bookmarks_set_empty (TRUE);
    }
    else
    {
        ctk_tree_model_iter_nth_child (GTK_TREE_MODEL (bookmark_list_store),
                                       &iter, NULL, row);
        ctk_tree_selection_select_iter (bookmark_selection, &iter);
    }
}

static void
on_remove_button_clicked (CtkButton *button,
                          gpointer   user_data)
{
    bookmarks_delete_bookmark ();
}


/* This is a bit of a kludge to get DnD to work. We check if the row in the
   CtkListStore matches the one in the bookmark list. If it doesn't, we assume
   the bookmark has just been dragged here and we insert it into the bookmark
   list. */
static void
on_row_changed (CtkListStore *store,
                CtkTreePath *path,
                CtkTreeIter *iter,
                gpointer user_data)
{
    BaulBookmark *bookmark = NULL;
    gint *indices, row;
    gboolean insert_bookmark = TRUE;

    store = bookmark_list_store;

    indices = ctk_tree_path_get_indices (path);
    row = indices[0];
    ctk_tree_model_get (GTK_TREE_MODEL (store), iter,
                        BOOKMARK_LIST_COLUMN_BOOKMARK, &bookmark,
                        -1);

    /* If the bookmark in the list doesn't match the changed one, it must
       have been dragged here, so we insert it into the list. */
    if (row < (gint) baul_bookmark_list_length (bookmarks))
    {
        BaulBookmark *bookmark_in_list;

        bookmark_in_list = baul_bookmark_list_item_at (bookmarks,
                           row);

        if (bookmark_in_list == bookmark)
            insert_bookmark = FALSE;
    }

    if (insert_bookmark)
    {
        g_signal_handler_block (bookmarks,
                                bookmark_list_changed_signal_id);
        baul_bookmark_list_insert_item (bookmarks, bookmark, row);
        g_signal_handler_unblock (bookmarks,
                                  bookmark_list_changed_signal_id);

        /* The bookmark will be copied when inserted into the list, so
           we have to update the pointer in the list store. */
        bookmark = baul_bookmark_list_item_at (bookmarks, row);
        g_signal_handler_block (store, row_changed_signal_id);
        ctk_list_store_set (store, iter,
                            BOOKMARK_LIST_COLUMN_BOOKMARK, bookmark,
                            -1);
        g_signal_handler_unblock (store, row_changed_signal_id);
    }
}

/* The update_bookmark_from_text() calls in the
 * on_button_pressed() and on_key_pressed() handlers
 * of the tree view are a hack.
 *
 * The purpose is to track selection changes to the view
 * and write the text fields back before the selection
 * actually changed.
 *
 * Note that the focus-out event of the text entries is emitted
 * after the selection changed, else this would not not be neccessary.
 */

static gboolean
on_button_pressed (CtkTreeView *view,
                   GdkEventButton *event,
                   gpointer user_data)
{
    update_bookmark_from_text ();

    return FALSE;
}

static gboolean
on_key_pressed (CtkTreeView *view,
                GdkEventKey *event,
                gpointer user_data)
{
    if (event->keyval == GDK_KEY_Delete || event->keyval == GDK_KEY_KP_Delete)
    {
        bookmarks_delete_bookmark ();
        return TRUE;
    }

    update_bookmark_from_text ();

    return FALSE;
}

static void
on_row_activated (CtkTreeView       *view,
                  CtkTreePath       *path,
                  CtkTreeViewColumn *column,
                  gpointer           user_data)
{
    GdkScreen *screen;

    screen = ctk_widget_get_screen (GTK_WIDGET (view));
    open_selected_bookmark (user_data, screen);
}

static void
on_row_deleted (CtkListStore *store,
                CtkTreePath *path,
                gpointer user_data)
{
    gint *indices, row;

    indices = ctk_tree_path_get_indices (path);
    row = indices[0];

    g_signal_handler_block (bookmarks, bookmark_list_changed_signal_id);
    baul_bookmark_list_delete_item_at (bookmarks, row);
    g_signal_handler_unblock (bookmarks, bookmark_list_changed_signal_id);
}

static void
on_selection_changed (CtkTreeSelection *treeselection,
                      gpointer user_data)
{
    BaulBookmark *selected;
    char *name = NULL, *entry_text = NULL;

    g_assert (GTK_IS_ENTRY (name_field));
    g_assert (GTK_IS_ENTRY (uri_field));

    selected = get_selected_bookmark ();

    if (selected)
    {
        GFile *location;

        name = baul_bookmark_get_name (selected);
        location = baul_bookmark_get_location (selected);
        entry_text = g_file_get_parse_name (location);

        g_object_unref (location);
    }

    /* Set the sensitivity of widgets that require a selection */
    ctk_widget_set_sensitive (remove_button, selected != NULL);
    ctk_widget_set_sensitive (jump_button, selected != NULL);
    ctk_widget_set_sensitive (name_field, selected != NULL);
    ctk_widget_set_sensitive (uri_field, selected != NULL);

    g_signal_handler_block (name_field, name_field_changed_signal_id);
    baul_entry_set_text (BAUL_ENTRY (name_field),
                         name ? name : "");
    g_signal_handler_unblock (name_field, name_field_changed_signal_id);

    g_signal_handler_block (uri_field, uri_field_changed_signal_id);
    baul_entry_set_text (BAUL_ENTRY (uri_field),
                         entry_text ? entry_text : "");
    g_signal_handler_unblock (uri_field, uri_field_changed_signal_id);

    text_changed = FALSE;
    name_text_changed = FALSE;

    g_free (name);
    g_free (entry_text);
}


static void
update_bookmark_from_text (void)
{
    if (text_changed)
    {
        BaulBookmark *bookmark, *bookmark_in_list;
        char *name;
        cairo_surface_t *surface;
        guint selected_row;
        CtkTreeIter iter;
        GFile *location;

        g_assert (GTK_IS_ENTRY (name_field));
        g_assert (GTK_IS_ENTRY (uri_field));

        if (ctk_entry_get_text_length (GTK_ENTRY (uri_field)) == 0)
        {
            return;
        }

        location = g_file_parse_name
                   (ctk_entry_get_text (GTK_ENTRY (uri_field)));

        bookmark = baul_bookmark_new (location, ctk_entry_get_text (GTK_ENTRY (name_field)),
                                      name_text_changed, NULL);

        g_object_unref (location);

        selected_row = get_selected_row ();

        /* turn off list updating 'cuz otherwise the list-reordering code runs
         * after repopulate(), thus reordering the correctly-ordered list.
         */
        g_signal_handler_block (bookmarks,
                                bookmark_list_changed_signal_id);
        baul_bookmark_list_delete_item_at (bookmarks, selected_row);
        baul_bookmark_list_insert_item (bookmarks, bookmark, selected_row);
        g_signal_handler_unblock (bookmarks,
                                  bookmark_list_changed_signal_id);
        g_object_unref (bookmark);

        /* We also have to update the bookmark pointer in the list
           store. */
        ctk_tree_selection_get_selected (bookmark_selection,
                                         NULL, &iter);
        g_signal_handler_block (bookmark_list_store,
                                row_changed_signal_id);

        bookmark_in_list = baul_bookmark_list_item_at (bookmarks,
                           selected_row);

        name = baul_bookmark_get_name (bookmark_in_list);

        surface = baul_bookmark_get_surface (bookmark_in_list, GTK_ICON_SIZE_MENU);

        ctk_list_store_set (bookmark_list_store, &iter,
                            BOOKMARK_LIST_COLUMN_BOOKMARK, bookmark_in_list,
                            BOOKMARK_LIST_COLUMN_NAME, name,
                            BOOKMARK_LIST_COLUMN_ICON, surface,
                            -1);
        g_signal_handler_unblock (bookmark_list_store,
                                  row_changed_signal_id);

        cairo_surface_destroy (surface);
        g_free (name);
    }
}

static gboolean
on_text_field_focus_out_event (CtkWidget *widget,
                               GdkEventFocus *event,
                               gpointer user_data)
{
    g_assert (BAUL_IS_ENTRY (widget));

    update_bookmark_from_text ();
    return FALSE;
}

static void
name_or_uri_field_activate (BaulEntry *entry)
{
    g_assert (BAUL_IS_ENTRY (entry));

    update_bookmark_from_text ();
    baul_entry_select_all_at_idle (entry);
}

static void
on_uri_field_changed (CtkEditable *editable,
                      gpointer user_data)
{
    /* Remember that user has changed text so we
     * update real bookmark later.
     */
    text_changed = TRUE;
}

static gboolean
on_window_delete_event (CtkWidget *widget,
                        GdkEvent *event,
                        gpointer user_data)
{
    ctk_widget_hide (widget);
    return TRUE;
}

static gboolean
restore_geometry (gpointer data)
{
    g_assert (GTK_IS_WINDOW (data));

    baul_bookmarks_window_restore_geometry (GTK_WIDGET (data));

    /* Don't call this again */
    return FALSE;
}

static void
on_window_hide_event (CtkWidget *widget,
                      gpointer user_data)
{
    baul_bookmarks_window_save_geometry (GTK_WINDOW (widget));

    /* restore_geometry only works after window is hidden */
    g_idle_add (restore_geometry, widget);
}

static void
on_window_destroy_event (CtkWidget *widget,
                         gpointer user_data)
{
    g_object_unref (bookmark_list_store);
    g_object_unref (bookmark_empty_list_store);
    g_source_remove_by_user_data (widget);
}

static void
repopulate (void)
{
    BaulBookmark *selected;
    CtkListStore *store;
    CtkTreeRowReference *reference;
    guint index;
    CtkTreePath *path = NULL;

    g_assert (GTK_IS_TREE_VIEW (bookmark_list_widget));
    g_assert (BAUL_IS_BOOKMARK_LIST (bookmarks));

    store = GTK_LIST_STORE (bookmark_list_store);

    selected = get_selected_bookmark ();

    g_signal_handler_block (bookmark_selection,
                            selection_changed_id);
    g_signal_handler_block (bookmark_list_store,
                            row_deleted_signal_id);
    g_signal_handler_block (bookmark_list_widget,
                            row_activated_signal_id);
    g_signal_handler_block (bookmark_list_widget,
                            key_pressed_signal_id);
    g_signal_handler_block (bookmark_list_widget,
                            button_pressed_signal_id);

    ctk_list_store_clear (store);

    g_signal_handler_unblock (bookmark_list_widget,
                              row_activated_signal_id);
    g_signal_handler_unblock (bookmark_list_widget,
                              key_pressed_signal_id);
    g_signal_handler_unblock (bookmark_list_widget,
                              button_pressed_signal_id);
    g_signal_handler_unblock (bookmark_list_store,
                              row_deleted_signal_id);
    g_signal_handler_unblock (bookmark_selection,
                              selection_changed_id);

    /* Fill the list in with the bookmark names. */
    g_signal_handler_block (store, row_changed_signal_id);

    reference = NULL;

    for (index = 0; index < baul_bookmark_list_length (bookmarks); ++index)
    {
        BaulBookmark *bookmark;
        char             *bookmark_name;
        cairo_surface_t  *bookmark_surface;
        CtkTreeIter       iter;

        bookmark = baul_bookmark_list_item_at (bookmarks, index);
        bookmark_name = baul_bookmark_get_name (bookmark);
        bookmark_surface = baul_bookmark_get_surface (bookmark, GTK_ICON_SIZE_MENU);

        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            BOOKMARK_LIST_COLUMN_ICON, bookmark_surface,
                            BOOKMARK_LIST_COLUMN_NAME, bookmark_name,
                            BOOKMARK_LIST_COLUMN_BOOKMARK, bookmark,
                            BOOKMARK_LIST_COLUMN_STYLE, PANGO_STYLE_NORMAL,
                            -1);

        if (bookmark == selected)
        {
            /* save old selection */
            CtkTreePath *path;

            path = ctk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
            reference = ctk_tree_row_reference_new (GTK_TREE_MODEL (store), path);
            ctk_tree_path_free (path);
        }

        g_free (bookmark_name);
        cairo_surface_destroy (bookmark_surface);

    }
    g_signal_handler_unblock (store, row_changed_signal_id);

    if (reference != NULL)
    {
        /* restore old selection */

        /* bookmarks_set_empty() will call the selection change handler,
         * so we block it here in case of selection change.
         */
        g_signal_handler_block (bookmark_selection, selection_changed_id);

        g_assert (index != 0);
        g_assert (ctk_tree_row_reference_valid (reference));

        path = ctk_tree_row_reference_get_path (reference);
        ctk_tree_selection_select_path (bookmark_selection, path);
        ctk_tree_row_reference_free (reference);
        ctk_tree_path_free (path);

        g_signal_handler_unblock (bookmark_selection, selection_changed_id);
    }

    bookmarks_set_empty (index == 0);
}

static int
handle_close_accelerator (CtkWindow *window,
                          GdkEventKey *event,
                          gpointer user_data)
{
    g_assert (GTK_IS_WINDOW (window));
    g_assert (event != NULL);
    g_assert (user_data == NULL);

	if (event->state & GDK_CONTROL_MASK && event->keyval == GDK_KEY_w) {
        ctk_widget_hide (GTK_WIDGET (window));
        return TRUE;
    }

    return FALSE;
}

static void
set_up_close_accelerator (CtkWidget *window)
{
    /* Note that we don't call eel_ctk_window_set_up_close_accelerator
     * here because we have to handle saving geometry before hiding the
     * window.
     */
    g_signal_connect (window, "key_press_event",
                      G_CALLBACK (handle_close_accelerator), NULL);
}
