/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-column-chooser.h - A column chooser widget

   Copyright (C) 2004 Novell, Inc.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Cafe Library; see the column COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Dave Camp <dave@ximian.com>
*/

#include <config.h>
#include "baul-column-chooser.h"

#include <string.h>
#include <ctk/ctk.h>
#include <glib/gi18n.h>

#include "baul-column-utilities.h"

struct _BaulColumnChooserPrivate
{
    CtkTreeView *view;
    CtkListStore *store;

    CtkWidget *move_up_button;
    CtkWidget *move_down_button;
    CtkWidget *use_default_button;

    BaulFile *file;
};

enum
{
    COLUMN_VISIBLE,
    COLUMN_LABEL,
    COLUMN_NAME,
    NUM_COLUMNS
};

enum
{
    PROP_FILE = 1,
    NUM_PROPERTIES
};

enum
{
    CHANGED,
    USE_DEFAULT,
    LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (BaulColumnChooser, baul_column_chooser, CTK_TYPE_BOX);

static void baul_column_chooser_constructed (GObject *object);

static void
baul_column_chooser_set_property (GObject *object,
                                  guint param_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
    BaulColumnChooser *chooser;

    chooser = BAUL_COLUMN_CHOOSER (object);

    switch (param_id)
    {
    case PROP_FILE:
        chooser->details->file = g_value_get_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
baul_column_chooser_class_init (BaulColumnChooserClass *chooser_class)
{
    GObjectClass *oclass;

    oclass = G_OBJECT_CLASS (chooser_class);

    oclass->set_property = baul_column_chooser_set_property;
    oclass->constructed = baul_column_chooser_constructed;

    signals[CHANGED] = g_signal_new
                       ("changed",
                        G_TYPE_FROM_CLASS (chooser_class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulColumnChooserClass,
                                         changed),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);

    signals[USE_DEFAULT] = g_signal_new
                           ("use_default",
                            G_TYPE_FROM_CLASS (chooser_class),
                            G_SIGNAL_RUN_LAST,
                            G_STRUCT_OFFSET (BaulColumnChooserClass,
                                    use_default),
                            NULL, NULL,
                            g_cclosure_marshal_VOID__VOID,
                            G_TYPE_NONE, 0);

    g_object_class_install_property (oclass,
                                     PROP_FILE,
                                     g_param_spec_object ("file",
                                             "File",
                                             "The file this column chooser is for",
                                             BAUL_TYPE_FILE,
                                             G_PARAM_CONSTRUCT_ONLY |
                                             G_PARAM_WRITABLE));
}

static void
update_buttons (BaulColumnChooser *chooser)
{
    CtkTreeSelection *selection;
    CtkTreeIter iter;

    selection = ctk_tree_view_get_selection (chooser->details->view);

    if (ctk_tree_selection_get_selected (selection, NULL, &iter))
    {
        gboolean visible;
        gboolean top;
        gboolean bottom;
        CtkTreePath *first;
        CtkTreePath *path;

        ctk_tree_model_get (CTK_TREE_MODEL (chooser->details->store),
                            &iter,
                            COLUMN_VISIBLE, &visible,
                            -1);

        path = ctk_tree_model_get_path (CTK_TREE_MODEL (chooser->details->store),
                                        &iter);
        first = ctk_tree_path_new_first ();

        top = (ctk_tree_path_compare (path, first) == 0);

        ctk_tree_path_free (path);
        ctk_tree_path_free (first);

        bottom = !ctk_tree_model_iter_next (CTK_TREE_MODEL (chooser->details->store),
                                            &iter);

        ctk_widget_set_sensitive (chooser->details->move_up_button,
                                  !top);
        ctk_widget_set_sensitive (chooser->details->move_down_button,
                                  !bottom);
    }
    else
    {
        ctk_widget_set_sensitive (chooser->details->move_up_button,
                                  FALSE);
        ctk_widget_set_sensitive (chooser->details->move_down_button,
                                  FALSE);
    }
}

static void
list_changed (BaulColumnChooser *chooser)
{
    update_buttons (chooser);
    g_signal_emit (chooser, signals[CHANGED], 0);
}

static void
visible_toggled_callback (CtkCellRendererToggle *cell G_GNUC_UNUSED,
			  char                  *path_string,
			  gpointer               user_data)
{
    BaulColumnChooser *chooser;
    CtkTreePath *path;
    CtkTreeIter iter;
    gboolean visible;

    chooser = BAUL_COLUMN_CHOOSER (user_data);

    path = ctk_tree_path_new_from_string (path_string);
    ctk_tree_model_get_iter (CTK_TREE_MODEL (chooser->details->store),
                             &iter, path);
    ctk_tree_model_get (CTK_TREE_MODEL (chooser->details->store),
                        &iter, COLUMN_VISIBLE, &visible, -1);
    ctk_list_store_set (chooser->details->store,
                        &iter, COLUMN_VISIBLE, !visible, -1);
    ctk_tree_path_free (path);
    list_changed (chooser);
}

static void
selection_changed_callback (CtkTreeSelection *selection G_GNUC_UNUSED,
			    gpointer          user_data)
{
    update_buttons (BAUL_COLUMN_CHOOSER (user_data));
}

static void
row_deleted_callback (CtkTreeModel *model G_GNUC_UNUSED,
		      CtkTreePath  *path G_GNUC_UNUSED,
		      gpointer      user_data)
{
    list_changed (BAUL_COLUMN_CHOOSER (user_data));
}

static void
add_tree_view (BaulColumnChooser *chooser)
{
    CtkWidget *scrolled;
    CtkWidget *view;
    CtkListStore *store;
    CtkCellRenderer *cell;
    CtkTreeSelection *selection;

    view = ctk_tree_view_new ();
    ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (view), FALSE);

    store = ctk_list_store_new (NUM_COLUMNS,
                                G_TYPE_BOOLEAN,
                                G_TYPE_STRING,
                                G_TYPE_STRING);

    ctk_tree_view_set_model (CTK_TREE_VIEW (view),
                             CTK_TREE_MODEL (store));
    g_object_unref (store);

    ctk_tree_view_set_reorderable (CTK_TREE_VIEW (view), TRUE);

    selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (view));
    g_signal_connect (selection, "changed",
                      G_CALLBACK (selection_changed_callback), chooser);

    cell = ctk_cell_renderer_toggle_new ();

    g_signal_connect (G_OBJECT (cell), "toggled",
                      G_CALLBACK (visible_toggled_callback), chooser);

    ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (view),
            -1, NULL,
            cell,
            "active", COLUMN_VISIBLE,
            NULL);

    cell = ctk_cell_renderer_text_new ();

    ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (view),
            -1, NULL,
            cell,
            "text", COLUMN_LABEL,
            NULL);

    chooser->details->view = CTK_TREE_VIEW (view);
    chooser->details->store = store;

    ctk_widget_show (view);

    scrolled = ctk_scrolled_window_new (NULL, NULL);
    ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled),
                                         CTK_SHADOW_IN);
    ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled),
                                    CTK_POLICY_AUTOMATIC,
                                    CTK_POLICY_AUTOMATIC);
    ctk_scrolled_window_set_overlay_scrolling (CTK_SCROLLED_WINDOW (scrolled), FALSE);

    ctk_widget_show (CTK_WIDGET (scrolled));

    ctk_container_add (CTK_CONTAINER (scrolled), view);
    ctk_box_pack_start (CTK_BOX (chooser), scrolled, TRUE, TRUE, 0);
}

static void
move_up_clicked_callback (CtkWidget *button G_GNUC_UNUSED,
			  gpointer   user_data)
{
    BaulColumnChooser *chooser;
    CtkTreeIter iter;
    CtkTreeSelection *selection;

    chooser = BAUL_COLUMN_CHOOSER (user_data);

    selection = ctk_tree_view_get_selection (chooser->details->view);

    if (ctk_tree_selection_get_selected (selection, NULL, &iter))
    {
        CtkTreePath *path;
        CtkTreeIter prev;

        path = ctk_tree_model_get_path (CTK_TREE_MODEL (chooser->details->store), &iter);
        ctk_tree_path_prev (path);
        if (ctk_tree_model_get_iter (CTK_TREE_MODEL (chooser->details->store), &prev, path))
        {
            ctk_list_store_move_before (chooser->details->store,
                                        &iter,
                                        &prev);
        }
        ctk_tree_path_free (path);
    }

    list_changed (chooser);
}

static void
move_down_clicked_callback (CtkWidget *button G_GNUC_UNUSED,
			    gpointer   user_data)
{
    BaulColumnChooser *chooser;
    CtkTreeIter iter;
    CtkTreeSelection *selection;

    chooser = BAUL_COLUMN_CHOOSER (user_data);

    selection = ctk_tree_view_get_selection (chooser->details->view);

    if (ctk_tree_selection_get_selected (selection, NULL, &iter))
    {
        CtkTreeIter next;

        next = iter;

        if (ctk_tree_model_iter_next (CTK_TREE_MODEL (chooser->details->store), &next))
        {
            ctk_list_store_move_after (chooser->details->store,
                                       &iter,
                                       &next);
        }
    }

    list_changed (chooser);
}

static void
use_default_clicked_callback (CtkWidget *button G_GNUC_UNUSED,
			      gpointer   user_data)
{
    g_signal_emit (BAUL_COLUMN_CHOOSER (user_data),
                   signals[USE_DEFAULT], 0);
}

static CtkWidget *
button_new_with_mnemonic (const gchar *icon_name, const gchar *str)
{
    CtkWidget *image;
    CtkWidget *button;

    button = ctk_button_new_with_mnemonic (str);
    image = ctk_image_new_from_icon_name (icon_name, CTK_ICON_SIZE_BUTTON);

    ctk_button_set_image (CTK_BUTTON (button), image);

    return button;
}

static void
add_buttons (BaulColumnChooser *chooser)
{
    CtkWidget *box;
    CtkWidget *separator;

    box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
    ctk_widget_show (box);

    chooser->details->move_up_button = button_new_with_mnemonic ("go-up",
                                       _("Move _Up"));
    g_signal_connect (chooser->details->move_up_button,
                      "clicked",  G_CALLBACK (move_up_clicked_callback),
                      chooser);
    ctk_widget_show_all (chooser->details->move_up_button);
    ctk_widget_set_sensitive (chooser->details->move_up_button, FALSE);
    ctk_box_pack_start (CTK_BOX (box), chooser->details->move_up_button,
                        FALSE, FALSE, 0);

    chooser->details->move_down_button = button_new_with_mnemonic ("go-down",
                                         _("Move Dow_n"));
    g_signal_connect (chooser->details->move_down_button,
                      "clicked",  G_CALLBACK (move_down_clicked_callback),
                      chooser);
    ctk_widget_show_all (chooser->details->move_down_button);
    ctk_widget_set_sensitive (chooser->details->move_down_button, FALSE);
    ctk_box_pack_start (CTK_BOX (box), chooser->details->move_down_button,
                        FALSE, FALSE, 0);

    separator = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
    ctk_widget_show (separator);
    ctk_box_pack_start (CTK_BOX (box), separator, FALSE, FALSE, 0);

    chooser->details->use_default_button = ctk_button_new_with_mnemonic (_("Use De_fault"));
    g_signal_connect (chooser->details->use_default_button,
                      "clicked",  G_CALLBACK (use_default_clicked_callback),
                      chooser);
    ctk_widget_show (chooser->details->use_default_button);
    ctk_box_pack_start (CTK_BOX (box), chooser->details->use_default_button,
                        FALSE, FALSE, 0);

    ctk_box_pack_start (CTK_BOX (chooser), box,
                        FALSE, FALSE, 0);
}

static void
populate_tree (BaulColumnChooser *chooser)
{
    GList *columns;
    GList *l;

    columns = baul_get_columns_for_file (chooser->details->file);

    for (l = columns; l != NULL; l = l->next)
    {
        CtkTreeIter iter;
        BaulColumn *column;
        char *name;
        char *label;

        column = BAUL_COLUMN (l->data);

        g_object_get (G_OBJECT (column),
                      "name", &name, "label", &label,
                      NULL);

        ctk_list_store_append (chooser->details->store, &iter);
        ctk_list_store_set (chooser->details->store, &iter,
                            COLUMN_VISIBLE, FALSE,
                            COLUMN_LABEL, label,
                            COLUMN_NAME, name,
                            -1);

        g_free (name);
        g_free (label);
    }

    baul_column_list_free (columns);
}

static void
baul_column_chooser_constructed (GObject *object)
{
    BaulColumnChooser *chooser;

    chooser = BAUL_COLUMN_CHOOSER (object);

    populate_tree (chooser);

    g_signal_connect (chooser->details->store, "row_deleted",
                      G_CALLBACK (row_deleted_callback), chooser);
}

static void
baul_column_chooser_init (BaulColumnChooser *chooser)
{
    chooser->details = baul_column_chooser_get_instance_private (chooser);

    g_object_set (G_OBJECT (chooser),
                  "homogeneous", FALSE,
                  "spacing", 8,
                  "orientation", CTK_ORIENTATION_HORIZONTAL,
                  NULL);

    add_tree_view (chooser);
    add_buttons (chooser);
}

static void
set_visible_columns (BaulColumnChooser *chooser,
                     char **visible_columns)
{
    GHashTable *visible_columns_hash;
    CtkTreeIter iter;
    int i;

    visible_columns_hash = g_hash_table_new (g_str_hash, g_str_equal);
    for (i = 0; visible_columns[i] != NULL; ++i)
    {
        g_hash_table_insert (visible_columns_hash,
                             visible_columns[i],
                             visible_columns[i]);
    }

    if (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (chooser->details->store),
                                       &iter))
    {
        do
        {
            char *name;
            gboolean visible;

            ctk_tree_model_get (CTK_TREE_MODEL (chooser->details->store),
                                &iter,
                                COLUMN_NAME, &name,
                                -1);

            visible = (g_hash_table_lookup (visible_columns_hash, name) != NULL);

            ctk_list_store_set (chooser->details->store,
                                &iter,
                                COLUMN_VISIBLE, visible,
                                -1);
            g_free (name);

        }
        while (ctk_tree_model_iter_next (CTK_TREE_MODEL (chooser->details->store), &iter));
    }

    g_hash_table_destroy (visible_columns_hash);
}

static char **
get_column_names (BaulColumnChooser *chooser, gboolean only_visible)
{
    GPtrArray *ret;
    CtkTreeIter iter;

    ret = g_ptr_array_new ();
    if (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (chooser->details->store),
                                       &iter))
    {
        do
        {
            char *name;
            gboolean visible;
            ctk_tree_model_get (CTK_TREE_MODEL (chooser->details->store),
                                &iter,
                                COLUMN_VISIBLE, &visible,
                                COLUMN_NAME, &name,
                                -1);
            if (!only_visible || visible)
            {
                /* give ownership to the array */
                g_ptr_array_add (ret, name);
            }

        }
        while (ctk_tree_model_iter_next (CTK_TREE_MODEL (chooser->details->store), &iter));
    }
    g_ptr_array_add (ret, NULL);

    return (char **) g_ptr_array_free (ret, FALSE);
}

static gboolean
get_column_iter (BaulColumnChooser *chooser,
                 BaulColumn *column,
                 CtkTreeIter *iter)
{
    char *column_name;

    g_object_get (BAUL_COLUMN (column), "name", &column_name, NULL);

    if (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (chooser->details->store),
                                       iter))
    {
        do
        {
            char *name;


            ctk_tree_model_get (CTK_TREE_MODEL (chooser->details->store),
                                iter,
                                COLUMN_NAME, &name,
                                -1);
            if (!strcmp (name, column_name))
            {
                g_free (column_name);
                g_free (name);
                return TRUE;
            }

            g_free (name);
        }
        while (ctk_tree_model_iter_next (CTK_TREE_MODEL (chooser->details->store), iter));
    }
    g_free (column_name);
    return FALSE;
}

static void
set_column_order (BaulColumnChooser *chooser,
                  char **column_order)

{
    GList *columns;
    GList *l;
    CtkTreePath *path;

    columns = baul_get_columns_for_file (chooser->details->file);
    columns = baul_sort_columns (columns, column_order);

    g_signal_handlers_block_by_func (chooser->details->store,
                                     G_CALLBACK (row_deleted_callback),
                                     chooser);

    path = ctk_tree_path_new_first ();
    for (l = columns; l != NULL; l = l->next)
    {
        CtkTreeIter iter;

        if (get_column_iter (chooser, BAUL_COLUMN (l->data), &iter))
        {
            CtkTreeIter before;
            if (path)
            {
                ctk_tree_model_get_iter (CTK_TREE_MODEL (chooser->details->store),
                                         &before, path);
                ctk_list_store_move_after (chooser->details->store,
                                           &iter, &before);
                ctk_tree_path_next (path);

            }
            else
            {
                ctk_list_store_move_after (chooser->details->store,
                                           &iter, NULL);
            }
        }
    }
    ctk_tree_path_free (path);
    g_signal_handlers_unblock_by_func (chooser->details->store,
                                       G_CALLBACK (row_deleted_callback),
                                       chooser);

    baul_column_list_free (columns);
}

void
baul_column_chooser_set_settings (BaulColumnChooser *chooser,
                                  char **visible_columns,
                                  char **column_order)
{
    g_return_if_fail (BAUL_IS_COLUMN_CHOOSER (chooser));
    g_return_if_fail (visible_columns != NULL);
    g_return_if_fail (column_order != NULL);

    set_visible_columns (chooser, visible_columns);
    set_column_order (chooser, column_order);

    list_changed (chooser);
}

void
baul_column_chooser_get_settings (BaulColumnChooser *chooser,
                                  char ***visible_columns,
                                  char ***column_order)
{
    g_return_if_fail (BAUL_IS_COLUMN_CHOOSER (chooser));
    g_return_if_fail (visible_columns != NULL);
    g_return_if_fail (column_order != NULL);

    *visible_columns = get_column_names (chooser, TRUE);
    *column_order = get_column_names (chooser, FALSE);
}

CtkWidget *
baul_column_chooser_new (BaulFile *file)
{
    return g_object_new (BAUL_TYPE_COLUMN_CHOOSER, "file", file, NULL);
}

