/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
   baul-mime-application-chooser.c: an mime-application chooser

   Copyright (C) 2004 Novell, Inc.
   Copyright (C) 2007 Red Hat, Inc.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but APPLICATIONOUT ANY WARRANTY; applicationout even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along application the Cafe Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Dave Camp <dave@novell.com>
            Alexander Larsson <alexl@redhat.com>
*/

#include <config.h>
#include <string.h>

#include <glib/gi18n-lib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <ctk/ctk.h>
#include <gio/gio.h>

#include <eel/eel-stock-dialogs.h>

#include "baul-mime-application-chooser.h"
#include "baul-open-with-dialog.h"
#include "baul-signaller.h"
#include "baul-file.h"

struct _BaulMimeApplicationChooserDetails
{
    char *uri;

    char *content_type;
    char *extension;
    char *type_description;
    char *orig_mime_type;

    guint refresh_timeout;

    CtkWidget *label;
    CtkWidget *entry;
    CtkWidget *treeview;
    CtkWidget *remove_button;

    gboolean for_multiple_files;

    CtkListStore *model;
    CtkCellRenderer *toggle_renderer;
};

enum
{
    COLUMN_APPINFO,
    COLUMN_DEFAULT,
    COLUMN_ICON,
    COLUMN_NAME,
    NUM_COLUMNS
};

G_DEFINE_TYPE (BaulMimeApplicationChooser, baul_mime_application_chooser, GTK_TYPE_BOX);

static void refresh_model             (BaulMimeApplicationChooser *chooser);
static void refresh_model_soon        (BaulMimeApplicationChooser *chooser);
static void mime_type_data_changed_cb (GObject                        *signaller,
                                       gpointer                        user_data);

static void
baul_mime_application_chooser_finalize (GObject *object)
{
    BaulMimeApplicationChooser *chooser;

    chooser = BAUL_MIME_APPLICATION_CHOOSER (object);

    if (chooser->details->refresh_timeout)
    {
        g_source_remove (chooser->details->refresh_timeout);
    }

    g_signal_handlers_disconnect_by_func (baul_signaller_get_current (),
                                          G_CALLBACK (mime_type_data_changed_cb),
                                          chooser);


    g_free (chooser->details->uri);
    g_free (chooser->details->content_type);
    g_free (chooser->details->extension);
    g_free (chooser->details->type_description);
    g_free (chooser->details->orig_mime_type);

    g_free (chooser->details);

    G_OBJECT_CLASS (baul_mime_application_chooser_parent_class)->finalize (object);
}

static void
baul_mime_application_chooser_class_init (BaulMimeApplicationChooserClass *class)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = baul_mime_application_chooser_finalize;
}

static void
default_toggled_cb (CtkCellRendererToggle *renderer,
                    const char *path_str,
                    gpointer user_data)
{
    BaulMimeApplicationChooser *chooser;
    CtkTreeIter iter;
    CtkTreePath *path;
    GError *error;

    chooser = BAUL_MIME_APPLICATION_CHOOSER (user_data);

    path = ctk_tree_path_new_from_string (path_str);
    if (ctk_tree_model_get_iter (GTK_TREE_MODEL (chooser->details->model),
                                 &iter, path))
    {
        gboolean is_default;
        gboolean success;
        GAppInfo *info;

        ctk_tree_model_get (GTK_TREE_MODEL (chooser->details->model),
                            &iter,
                            COLUMN_DEFAULT, &is_default,
                            COLUMN_APPINFO, &info,
                            -1);

        if (!is_default && info != NULL)
        {
            error = NULL;
            if (chooser->details->extension)
            {
                success = g_app_info_set_as_default_for_extension (info,
                          chooser->details->extension,
                          &error);
            }
            else
            {
                success = g_app_info_set_as_default_for_type (info,
                          chooser->details->content_type,
                          &error);
            }

            if (!success)
            {
                char *message;

                message = g_strdup_printf (_("Could not set application as the default: %s"), error->message);
                eel_show_error_dialog (_("Could not set as default application"),
                                       message,
                                       GTK_WINDOW (ctk_widget_get_toplevel (GTK_WIDGET (chooser))));
                g_free (message);
                g_error_free (error);
            }

            g_signal_emit_by_name (baul_signaller_get_current (),
                                   "mime_data_changed");
        }
        g_object_unref (info);
    }
    ctk_tree_path_free (path);
}

static GAppInfo *
get_selected_application (BaulMimeApplicationChooser *chooser)
{
    CtkTreeIter iter;
    CtkTreeSelection *selection;
    GAppInfo *info;

    selection = ctk_tree_view_get_selection (GTK_TREE_VIEW (chooser->details->treeview));

    info = NULL;
    if (ctk_tree_selection_get_selected (selection,
                                         NULL,
                                         &iter))
    {
        ctk_tree_model_get (GTK_TREE_MODEL (chooser->details->model),
                            &iter,
                            COLUMN_APPINFO, &info,
                            -1);
    }

    return info;
}

static void
selection_changed_cb (CtkTreeSelection *selection,
                      gpointer user_data)
{
    BaulMimeApplicationChooser *chooser;
    GAppInfo *info;

    chooser = BAUL_MIME_APPLICATION_CHOOSER (user_data);

    info = get_selected_application (chooser);
    if (info)
    {
        ctk_widget_set_sensitive (chooser->details->remove_button,
                                  g_app_info_can_remove_supports_type (info));

        g_object_unref (info);
    }
    else
    {
        ctk_widget_set_sensitive (chooser->details->remove_button,
                                  FALSE);
    }
}

static CtkWidget *
create_tree_view (BaulMimeApplicationChooser *chooser)
{
    CtkWidget *treeview;
    CtkListStore *store;
    CtkTreeViewColumn *column;
    CtkCellRenderer *renderer;
    CtkTreeSelection *selection;

    treeview = ctk_tree_view_new ();
    ctk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);

    store = ctk_list_store_new (NUM_COLUMNS,
                                G_TYPE_APP_INFO,
                                G_TYPE_BOOLEAN,
                                G_TYPE_ICON,
                                G_TYPE_STRING);
    ctk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                          COLUMN_NAME,
                                          GTK_SORT_ASCENDING);
    ctk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                             GTK_TREE_MODEL (store));
    chooser->details->model = store;

    renderer = ctk_cell_renderer_toggle_new ();
    g_signal_connect (renderer, "toggled",
                      G_CALLBACK (default_toggled_cb),
                      chooser);
    ctk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer),
                                        TRUE);

    column = ctk_tree_view_column_new_with_attributes (_("Default"),
             renderer,
             "active",
             COLUMN_DEFAULT,
             NULL);
    ctk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
    chooser->details->toggle_renderer = renderer;

    renderer = ctk_cell_renderer_pixbuf_new ();
    g_object_set (renderer, "stock-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
    column = ctk_tree_view_column_new_with_attributes (_("Icon"),
             renderer,
             "gicon",
             COLUMN_ICON,
             NULL);
    ctk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = ctk_cell_renderer_text_new ();
    column = ctk_tree_view_column_new_with_attributes (_("Name"),
             renderer,
             "markup",
             COLUMN_NAME,
             NULL);
    ctk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    selection = ctk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    g_signal_connect (selection, "changed",
                      G_CALLBACK (selection_changed_cb),
                      chooser);

    return treeview;
}

static void
add_clicked_cb (CtkButton *button,
                gpointer user_data)
{
    BaulMimeApplicationChooser *chooser;
    CtkWidget *dialog;

    chooser = BAUL_MIME_APPLICATION_CHOOSER (user_data);

    if (chooser->details->for_multiple_files)
    {
        dialog = baul_add_application_dialog_new_for_multiple_files (chooser->details->extension,
                 chooser->details->orig_mime_type);
    }
    else
    {
        dialog = baul_add_application_dialog_new (chooser->details->uri,
                 chooser->details->orig_mime_type);
    }
    ctk_window_set_screen (GTK_WINDOW (dialog),
                           ctk_widget_get_screen (GTK_WIDGET (chooser)));
    ctk_widget_show (dialog);
}

static void
remove_clicked_cb (CtkButton *button,
                   gpointer user_data)
{
    BaulMimeApplicationChooser *chooser;
    GError *error;
    GAppInfo *info;

    chooser = BAUL_MIME_APPLICATION_CHOOSER (user_data);

    info = get_selected_application (chooser);

    if (info)
    {
        error = NULL;
        if (!g_app_info_remove_supports_type (info,
                                              chooser->details->content_type,
                                              &error))
        {
            eel_show_error_dialog (_("Could not remove application"),
                                   error->message,
                                   GTK_WINDOW (ctk_widget_get_toplevel (GTK_WIDGET (chooser))));
            g_error_free (error);

        }
        g_signal_emit_by_name (baul_signaller_get_current (),
                               "mime_data_changed");
        g_object_unref (info);
    }
}

static void
reset_clicked_cb (CtkButton *button,
                  gpointer   user_data)
{
    BaulMimeApplicationChooser *chooser;

    chooser = BAUL_MIME_APPLICATION_CHOOSER (user_data);

    g_app_info_reset_type_associations (chooser->details->content_type);

    g_signal_emit_by_name (baul_signaller_get_current (),
                           "mime_data_changed");
}

static void
mime_type_data_changed_cb (GObject *signaller,
                           gpointer user_data)
{
    BaulMimeApplicationChooser *chooser;

    chooser = BAUL_MIME_APPLICATION_CHOOSER (user_data);

    refresh_model_soon (chooser);
}

static void
baul_mime_application_chooser_init (BaulMimeApplicationChooser *chooser)
{
    CtkWidget *box;
    CtkWidget *scrolled;
    CtkWidget *button;

    chooser->details = g_new0 (BaulMimeApplicationChooserDetails, 1);

    chooser->details->for_multiple_files = FALSE;

    ctk_orientable_set_orientation (GTK_ORIENTABLE (chooser), GTK_ORIENTATION_VERTICAL);

    ctk_container_set_border_width (GTK_CONTAINER (chooser), 8);
    ctk_box_set_spacing (GTK_BOX (chooser), 0);
    ctk_box_set_homogeneous (GTK_BOX (chooser), FALSE);

    chooser->details->label = ctk_label_new ("");
    ctk_label_set_xalign (GTK_LABEL (chooser->details->label), 0);
    ctk_label_set_line_wrap (GTK_LABEL (chooser->details->label), TRUE);
    ctk_label_set_line_wrap_mode (GTK_LABEL (chooser->details->label),
                                  PANGO_WRAP_WORD_CHAR);
    ctk_box_pack_start (GTK_BOX (chooser), chooser->details->label,
                        FALSE, FALSE, 0);

    ctk_widget_show (chooser->details->label);

    scrolled = ctk_scrolled_window_new (NULL, NULL);

    ctk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    ctk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                         GTK_SHADOW_IN);
    ctk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (scrolled), FALSE);

    ctk_widget_show (scrolled);
    ctk_box_pack_start (GTK_BOX (chooser), scrolled, TRUE, TRUE, 6);

    chooser->details->treeview = create_tree_view (chooser);
    ctk_widget_show (chooser->details->treeview);

    ctk_container_add (GTK_CONTAINER (scrolled),
                       chooser->details->treeview);

    box = ctk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    ctk_box_set_spacing (GTK_BOX (box), 6);
    ctk_button_box_set_layout (GTK_BUTTON_BOX (box), GTK_BUTTONBOX_END);
    ctk_box_pack_start (GTK_BOX (chooser), box, FALSE, FALSE, 6);
    ctk_widget_show (box);

    button = ctk_button_new_with_mnemonic (_("_Add"));
    ctk_button_set_image (GTK_BUTTON (button), ctk_image_new_from_icon_name ("list-add", GTK_ICON_SIZE_BUTTON));

    g_signal_connect (button, "clicked",
                      G_CALLBACK (add_clicked_cb),
                      chooser);

    ctk_widget_show (button);
    ctk_container_add (GTK_CONTAINER (box), button);

    button = ctk_button_new_with_mnemonic (_("_Remove"));
    ctk_button_set_image (GTK_BUTTON (button), ctk_image_new_from_icon_name ("list-remove", GTK_ICON_SIZE_BUTTON));

    g_signal_connect (button, "clicked",
                      G_CALLBACK (remove_clicked_cb),
                      chooser);

    ctk_widget_show (button);
    ctk_container_add (GTK_CONTAINER (box), button);

    chooser->details->remove_button = button;

    button = ctk_button_new_with_label (_("Reset"));
    g_signal_connect (button, "clicked",
                      G_CALLBACK (reset_clicked_cb),
                      chooser);

    ctk_widget_show (button);
    ctk_container_add (GTK_CONTAINER (box), button);

    g_signal_connect (baul_signaller_get_current (),
                      "mime_data_changed",
                      G_CALLBACK (mime_type_data_changed_cb),
                      chooser);
}

static char *
get_extension (const char *basename)
{
    char *p;

    p = strrchr (basename, '.');

    if (p && *(p + 1) != '\0')
    {
        return g_strdup (p + 1);
    }
    else
    {
        return NULL;
    }
}

static gboolean
refresh_model_timeout (gpointer data)
{
    BaulMimeApplicationChooser *chooser = data;

    chooser->details->refresh_timeout = 0;

    refresh_model (chooser);

    return FALSE;
}

/* This adds a slight delay so that we're sure the mime data is
   done writing */
static void
refresh_model_soon (BaulMimeApplicationChooser *chooser)
{
    if (chooser->details->refresh_timeout != 0)
        return;

    chooser->details->refresh_timeout =
        g_timeout_add (300,
                       refresh_model_timeout,
                       chooser);
}

static void
refresh_model (BaulMimeApplicationChooser *chooser)
{
    GList *applications;
    GAppInfo *default_app;
    GList *l;
    CtkTreeSelection *selection;
    CtkTreeViewColumn *column;

    column = ctk_tree_view_get_column (GTK_TREE_VIEW (chooser->details->treeview), 0);
    ctk_tree_view_column_set_visible (column, TRUE);

    ctk_list_store_clear (chooser->details->model);

    applications = g_app_info_get_all_for_type (chooser->details->content_type);
    default_app = g_app_info_get_default_for_type (chooser->details->content_type, FALSE);

    for (l = applications; l != NULL; l = l->next)
    {
        CtkTreeIter iter;
        gboolean is_default;
        GAppInfo *application;
        char *escaped;
        GIcon *icon;

        application = l->data;

        is_default = default_app && g_app_info_equal (default_app, application);

        escaped = g_markup_escape_text (g_app_info_get_display_name (application), -1);

        icon = g_app_info_get_icon (application);

        ctk_list_store_append (chooser->details->model, &iter);
        ctk_list_store_set (chooser->details->model, &iter,
                            COLUMN_APPINFO, application,
                            COLUMN_DEFAULT, is_default,
                            COLUMN_ICON, icon,
                            COLUMN_NAME, escaped,
                            -1);

        g_free (escaped);
    }

    selection = ctk_tree_view_get_selection (GTK_TREE_VIEW (chooser->details->treeview));

    if (applications)
    {
        g_object_set (chooser->details->toggle_renderer,
                      "visible", TRUE,
                      NULL);
        ctk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    }
    else
    {
        CtkTreeIter iter;
        char *name;

        ctk_tree_view_column_set_visible (column, FALSE);
        ctk_list_store_append (chooser->details->model, &iter);
        name = g_strdup_printf ("<i>%s</i>", _("No applications selected"));
        ctk_list_store_set (chooser->details->model, &iter,
                            COLUMN_NAME, name,
                            COLUMN_APPINFO, NULL,
                            -1);
        g_free (name);

        ctk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);
    }

    if (default_app)
    {
        g_object_unref (default_app);
    }

    g_list_free_full (applications, g_object_unref);
}

static void
set_extension_and_description (BaulMimeApplicationChooser *chooser,
                               const char *extension,
                               const char *mime_type)
{
    if (extension != NULL &&
            g_content_type_is_unknown (mime_type))
    {
        chooser->details->extension = g_strdup (extension);
        chooser->details->content_type = g_strdup_printf ("application/x-extension-%s", extension);
        /* the %s here is a file extension */
        chooser->details->type_description =
            g_strdup_printf (_("%s document"), extension);
    }
    else
    {
        char *description;

        chooser->details->content_type = g_strdup (mime_type);
        description = g_content_type_get_description (mime_type);
        if (description == NULL)
        {
            description = g_strdup (_("Unknown"));
        }

        chooser->details->type_description = description;
    }
}

static gboolean
set_uri_and_type (BaulMimeApplicationChooser *chooser,
                  const char *uri,
                  const char *mime_type)
{
    char *label;
    char *name;
    char *emname;
    char *extension;
    GFile *file;

    chooser->details->uri = g_strdup (uri);

    file = g_file_new_for_uri (uri);
    name = g_file_get_basename (file);
    g_object_unref (file);

    chooser->details->orig_mime_type = g_strdup (mime_type);

    extension = get_extension (name);
    set_extension_and_description (BAUL_MIME_APPLICATION_CHOOSER (chooser),
                                   extension, mime_type);
    g_free (extension);

    /* first %s is filename, second %s is mime-type description */
    emname = g_strdup_printf ("<i>%s</i>", name);
    label = g_strdup_printf (_("Select an application to open %s and other files of type \"%s\""),
                             emname, chooser->details->type_description);
    g_free (emname);

    ctk_label_set_markup (GTK_LABEL (chooser->details->label), label);

    g_free (label);
    g_free (name);

    refresh_model (chooser);

    return TRUE;
}

static char *
get_extension_from_file (BaulFile *nfile)
{
    char *name;
    char *extension;

    name = baul_file_get_name (nfile);
    extension = get_extension (name);

    g_free (name);

    return extension;
}

static gboolean
set_uri_and_type_for_multiple_files (BaulMimeApplicationChooser *chooser,
                                     GList *uris,
                                     const char *mime_type)
{
    char *label;
    char *first_extension;
    gboolean same_extension;
    GList *iter;

    chooser->details->for_multiple_files = TRUE;
    chooser->details->uri = NULL;
    chooser->details->orig_mime_type = g_strdup (mime_type);
    same_extension = TRUE;
    first_extension = get_extension_from_file (BAUL_FILE (uris->data));
    iter = uris->next;

    while (iter != NULL)
    {
        char *extension_current;

        extension_current = get_extension_from_file (BAUL_FILE (iter->data));
        if (g_strcmp0 (first_extension, extension_current)) {
            same_extension = FALSE;
            g_free (extension_current);
            break;
        }
        iter = iter->next;

        g_free (extension_current);
    }
    if (!same_extension)
    {
        set_extension_and_description (BAUL_MIME_APPLICATION_CHOOSER (chooser),
                                       NULL, mime_type);
    }
    else
    {
        set_extension_and_description (BAUL_MIME_APPLICATION_CHOOSER (chooser),
                                       first_extension, mime_type);
    }

    g_free (first_extension);

    label = g_strdup_printf (_("Open all files of type \"%s\" with:"),
                             chooser->details->type_description);
    ctk_label_set_markup (GTK_LABEL (chooser->details->label), label);

    g_free (label);

    refresh_model (chooser);

    return TRUE;
}

CtkWidget *
baul_mime_application_chooser_new (const char *uri,
                                   const char *mime_type)
{
    CtkWidget *chooser;

    chooser = ctk_widget_new (BAUL_TYPE_MIME_APPLICATION_CHOOSER, NULL);

    set_uri_and_type (BAUL_MIME_APPLICATION_CHOOSER (chooser), uri, mime_type);

    return chooser;
}

CtkWidget *
baul_mime_application_chooser_new_for_multiple_files (GList *uris,
        const char *mime_type)
{
    CtkWidget *chooser;

    chooser = ctk_widget_new (BAUL_TYPE_MIME_APPLICATION_CHOOSER, NULL);

    set_uri_and_type_for_multiple_files (BAUL_MIME_APPLICATION_CHOOSER (chooser),
                                         uris, mime_type);

    return chooser;
}

