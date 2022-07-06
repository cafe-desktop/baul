/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-file-management-properties.c - Functions to create and show the baul preference dialog.

   Copyright (C) 2002 Jan Arne Petersen

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

   Authors: Jan Arne Petersen <jpetersen@uni-bonn.de>
*/

#include <config.h>
#include <string.h>
#include <time.h>

#include <ctk/ctk.h>
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <cairo-gobject.h>

#include <eel/eel-glib-extensions.h>
#include <eel/eel-ctk-extensions.h>

#include <libbaul-private/baul-column-chooser.h>
#include <libbaul-private/baul-column-utilities.h>
#include <libbaul-private/baul-extensions.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-module.h>
#include <libbaul-private/baul-autorun.h>

#include <libbaul-extension/baul-configurable.h>

#include "baul-file-management-properties.h"

/* string enum preferences */
#define BAUL_FILE_MANAGEMENT_PROPERTIES_DEFAULT_VIEW_WIDGET "default_view_combobox"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_ICON_VIEW_ZOOM_WIDGET "icon_view_zoom_combobox"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_COMPACT_VIEW_ZOOM_WIDGET "compact_view_zoom_combobox"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_LIST_VIEW_ZOOM_WIDGET "list_view_zoom_combobox"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_SORT_ORDER_WIDGET "sort_order_combobox"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_DATE_FORMAT_WIDGET "date_format_combobox"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_PREVIEW_TEXT_WIDGET "preview_text_combobox"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_PREVIEW_IMAGE_WIDGET "preview_image_combobox"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_PREVIEW_SOUND_WIDGET "preview_sound_combobox"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_PREVIEW_FOLDER_WIDGET "preview_folder_combobox"

/* bool preferences */
#define BAUL_FILE_MANAGEMENT_PROPERTIES_FOLDERS_FIRST_WIDGET "sort_folders_first_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_COMPACT_LAYOUT_WIDGET "compact_layout_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_LABELS_BESIDE_ICONS_WIDGET "labels_beside_icons_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_ALL_COLUMNS_SAME_WIDTH "all_columns_same_width_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_ALWAYS_USE_BROWSER_WIDGET "always_use_browser_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_ALWAYS_USE_LOCATION_ENTRY_WIDGET "always_use_location_entry_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_TRASH_CONFIRM_WIDGET "trash_confirm_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_TRASH_CONFIRM_TRASH_WIDGET "trash_confirm_trash_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_TRASH_DELETE_WIDGET "trash_delete_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_SHOW_HIDDEN_WIDGET "hidden_files_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_SHOW_BACKUP_WIDGET "backup_files_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_TREE_VIEW_FOLDERS_WIDGET "treeview_folders_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_MEDIA_AUTOMOUNT_OPEN "media_automount_open_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_MEDIA_AUTORUN_NEVER "media_autorun_never_checkbutton"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_USE_IEC_UNITS_WIDGET "use_iec_units"
#define BAUL_FILE_MANAGEMENT_PROPERTIES_SHOW_ICONS_IN_LIST_VIEW "show_icons_in_list_view"

/* int enums */
#define BAUL_FILE_MANAGEMENT_PROPERTIES_THUMBNAIL_LIMIT_WIDGET "preview_image_size_combobox"

static const char * const default_view_values[] =
{
    "icon-view",
    "list-view",
    "compact-view",
    NULL
};

static const char * const zoom_values[] =
{
    "smallest",
    "smaller",
    "small",
    "standard",
    "large",
    "larger",
    "largest",
    NULL
};

/*
 * This array corresponds to the object with id "model2" in
 * baul-file-management-properties.ui. It has to positionally match with it.
 * The purpose is to map values from a combo box to values of the gsettings
 * enum.
 */
static const char * const sort_order_values[] =
{
    "name",
    "directory",
    "size",
    "size_on_disk",
    "type",
    "mtime",
    "atime",
    "emblems",
    "extension",
    "trash-time",
    NULL
};

static const char * const date_format_values[] =
{
    "locale",
    "iso",
    "informal",
    NULL
};

static const char * const preview_values[] =
{
    "always",
    "local-only",
    "never",
    NULL
};

static const char * const click_behavior_components[] =
{
    "single_click_radiobutton",
    "double_click_radiobutton",
    NULL
};

static const char * const click_behavior_values[] =
{
    "single",
    "double",
    NULL
};

static const char * const executable_text_components[] =
{
    "scripts_execute_radiobutton",
    "scripts_view_radiobutton",
    "scripts_confirm_radiobutton",
    NULL
};

static const char * const executable_text_values[] =
{
    "launch",
    "display",
    "ask",
    NULL
};

static const guint64 thumbnail_limit_values[] =
{
    102400,
    512000,
    1048576,
    3145728,
    5242880,
    10485760,
    104857600,
    1073741824,
    2147483648U,
    4294967295U
};

static const char * const icon_captions_components[] =
{
    "captions_0_combobox",
    "captions_1_combobox",
    "captions_2_combobox",
    NULL
};

enum
{
	EXT_STATE_COLUMN,
	EXT_ICON_COLUMN,
	EXT_INFO_COLUMN,
	EXT_STRUCT_COLUMN
};

static void baul_file_management_properties_dialog_update_media_sensitivity (CtkBuilder *builder);

static void
baul_file_management_properties_size_group_create (CtkBuilder *builder,
        char *prefix,
        int items)
{
    CtkSizeGroup *size_group;
    int i;
    CtkWidget *widget = NULL;

    size_group = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);

    for (i = 0; i < items; i++)
    {
        char *item_name;

        item_name = g_strdup_printf ("%s_%d", prefix, i);
        widget = CTK_WIDGET (ctk_builder_get_object (builder, item_name));
        ctk_size_group_add_widget (size_group, widget);
        g_free (item_name);
    }
    g_object_unref (G_OBJECT (size_group));
}

static void
preferences_show_help (CtkWindow *parent,
                       char const *helpfile,
                       char const *sect_id)
{
    GError *error = NULL;
    char *help_string;

    g_assert (helpfile != NULL);
    g_assert (sect_id != NULL);

    help_string = g_strdup_printf ("help:%s/%s", helpfile, sect_id);

    ctk_show_uri_on_window (parent,
                            help_string, ctk_get_current_event_time (),
                            &error);
    g_free (help_string);

    if (error)
    {
        CtkWidget *dialog;

        dialog = ctk_message_dialog_new (CTK_WINDOW (parent),
                                         CTK_DIALOG_DESTROY_WITH_PARENT,
                                         CTK_MESSAGE_ERROR,
                                         CTK_BUTTONS_OK,
                                         _("There was an error displaying help: \n%s"),
                                         error->message);

        g_signal_connect (G_OBJECT (dialog),
                          "response", G_CALLBACK (ctk_widget_destroy),
                          NULL);
        ctk_window_set_resizable (CTK_WINDOW (dialog), FALSE);
        ctk_widget_show (dialog);
        g_error_free (error);
    }
}


static void
baul_file_management_properties_dialog_response_cb (CtkDialog *parent,
        int response_id,
        CtkBuilder *builder)
{
    if (response_id == CTK_RESPONSE_HELP)
    {
        char *section;

        switch (ctk_notebook_get_current_page (CTK_NOTEBOOK (ctk_builder_get_object (builder, "notebook1"))))
        {
        default:
        case 0:
            section = "gosbaul-438";
            break;
        case 1:
            section = "gosbaul-56";
            break;
        case 2:
            section = "gosbaul-439";
            break;
        case 3:
            section = "gosbaul-490";
            break;
        case 4:
            section = "gosbaul-60";
            break;
        case 5:
            section = "gosbaul-61";
            break;
        }
        preferences_show_help (CTK_WINDOW (parent), "cafe-user-guide", section);
    }
    else if (response_id == CTK_RESPONSE_CLOSE)
    {
        g_signal_handlers_disconnect_by_func (baul_media_preferences,
                                              baul_file_management_properties_dialog_update_media_sensitivity,
                                              builder);
    }
}

static void
columns_changed_callback (BaulColumnChooser *chooser,
                          gpointer callback_data)
{
    char **visible_columns;
    char **column_order;

    baul_column_chooser_get_settings (BAUL_COLUMN_CHOOSER (chooser),
                                      &visible_columns,
                                      &column_order);

    g_settings_set_strv (baul_list_view_preferences,
                         BAUL_PREFERENCES_LIST_VIEW_DEFAULT_VISIBLE_COLUMNS,
                         (const char * const *)visible_columns);
    g_settings_set_strv (baul_list_view_preferences,
                         BAUL_PREFERENCES_LIST_VIEW_DEFAULT_COLUMN_ORDER,
                         (const char * const *)column_order);

    g_strfreev (visible_columns);
    g_strfreev (column_order);
}

static void
free_column_names_array (GPtrArray *column_names)
{
    g_ptr_array_foreach (column_names, (GFunc) g_free, NULL);
    g_ptr_array_free (column_names, TRUE);
}

static void
create_icon_caption_combo_box_items (CtkComboBoxText *combo_box,
                                     GList *columns)
{
    GList *l;
    GPtrArray *column_names;

    column_names = g_ptr_array_new ();

    /* Translators: this is referred to captions under icons. */
    ctk_combo_box_text_append_text (combo_box, _("None"));
    g_ptr_array_add (column_names, g_strdup ("none"));

    for (l = columns; l != NULL; l = l->next)
    {
        BaulColumn *column;
        char *name;
        char *label;

        column = BAUL_COLUMN (l->data);

        g_object_get (G_OBJECT (column),
                      "name", &name, "label", &label,
                      NULL);

        /* Don't show name here, it doesn't make sense */
        if (!strcmp (name, "name"))
        {
            g_free (name);
            g_free (label);
            continue;
        }

        ctk_combo_box_text_append_text (combo_box, label);
        g_ptr_array_add (column_names, name);

        g_free (label);
    }
    g_object_set_data_full (G_OBJECT (combo_box), "column_names",
                            column_names,
                            (GDestroyNotify) free_column_names_array);
}

static void
icon_captions_changed_callback (CtkComboBox *combo_box,
                                gpointer user_data)
{
    GPtrArray *captions;
    CtkBuilder *builder;
    int i;

    builder = CTK_BUILDER (user_data);

    captions = g_ptr_array_new ();

    for (i = 0; icon_captions_components[i] != NULL; i++)
    {
        CtkWidget *combo_box;
        int active;
        GPtrArray *column_names;
        char *name;

        combo_box = CTK_WIDGET (ctk_builder_get_object
                                (builder, icon_captions_components[i]));
        active = ctk_combo_box_get_active (CTK_COMBO_BOX (combo_box));

        column_names = g_object_get_data (G_OBJECT (combo_box),
                                          "column_names");

        name = g_ptr_array_index (column_names, active);
        g_ptr_array_add (captions, name);
    }
    g_ptr_array_add (captions, NULL);

    g_settings_set_strv (baul_icon_view_preferences,
                         BAUL_PREFERENCES_ICON_VIEW_CAPTIONS,
                         (const char **)captions->pdata);
    g_ptr_array_free (captions, TRUE);
}

static void
update_caption_combo_box (CtkBuilder *builder,
                          const char *combo_box_name,
                          const char *name)
{
    CtkWidget *combo_box;
    int i;
    GPtrArray *column_names;

    combo_box = CTK_WIDGET (ctk_builder_get_object (builder, combo_box_name));

    g_signal_handlers_block_by_func
    (combo_box,
     G_CALLBACK (icon_captions_changed_callback),
     builder);

    column_names = g_object_get_data (G_OBJECT (combo_box),
                                      "column_names");

    for (i = 0; i < column_names->len; ++i)
    {
        if (!strcmp (name, g_ptr_array_index (column_names, i)))
        {
            ctk_combo_box_set_active (CTK_COMBO_BOX (combo_box), i);
            break;
        }
    }

    g_signal_handlers_unblock_by_func
    (combo_box,
     G_CALLBACK (icon_captions_changed_callback),
     builder);
}

static void
update_icon_captions_from_settings (CtkBuilder *builder)
{
    char **captions;
    int i, j;

    captions = g_settings_get_strv (baul_icon_view_preferences, BAUL_PREFERENCES_ICON_VIEW_CAPTIONS);
    if (captions == NULL)
        return;

    for (i = 0, j = 0;
            icon_captions_components[i] != NULL;
            i++)
    {
        char *data;

        if (captions[j])
        {
            data = captions[j];
            ++j;
        }
        else
        {
            data = "none";
        }

        update_caption_combo_box (builder,
                                  icon_captions_components[i],
                                  data);
    }

    g_strfreev (captions);
}

static void
baul_file_management_properties_dialog_setup_icon_caption_page (CtkBuilder *builder)
{
    GList *columns;
    int i;
    gboolean writable;

    writable = g_settings_is_writable (baul_icon_view_preferences, BAUL_PREFERENCES_ICON_VIEW_CAPTIONS);

    columns = baul_get_common_columns ();

    for (i = 0; icon_captions_components[i] != NULL; i++)
    {
        CtkWidget *combo_box;

        combo_box = CTK_WIDGET (ctk_builder_get_object (builder,
                                icon_captions_components[i]));

        create_icon_caption_combo_box_items (CTK_COMBO_BOX_TEXT (combo_box), columns);
        ctk_widget_set_sensitive (combo_box, writable);

        g_signal_connect (combo_box, "changed",
                          G_CALLBACK (icon_captions_changed_callback),
                          builder);
    }

    baul_column_list_free (columns);

    update_icon_captions_from_settings (builder);
}

static void
create_date_format_menu (CtkBuilder *builder)
{
    CtkComboBoxText *combo_box;
    gchar *date_string;
    GDateTime *now;

    combo_box = CTK_COMBO_BOX_TEXT
            (ctk_builder_get_object (builder,
                                     BAUL_FILE_MANAGEMENT_PROPERTIES_DATE_FORMAT_WIDGET));

    now = g_date_time_new_now_local ();

    date_string = g_date_time_format (now, "%c");
    ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), date_string);
    g_free (date_string);

    date_string = g_date_time_format (now, "%Y-%m-%d %H:%M:%S");
    ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), date_string);
    g_free (date_string);

    date_string = g_date_time_format (now, _("today at %-I:%M:%S %p"));
    ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), date_string);
    g_free (date_string);

    g_date_time_unref (now);
}

static void
set_columns_from_settings (BaulColumnChooser *chooser)
{
    char **visible_columns;
    char **column_order;

    visible_columns = g_settings_get_strv (baul_list_view_preferences,
                                           BAUL_PREFERENCES_LIST_VIEW_DEFAULT_VISIBLE_COLUMNS);
    column_order = g_settings_get_strv (baul_list_view_preferences,
                                        BAUL_PREFERENCES_LIST_VIEW_DEFAULT_COLUMN_ORDER);

    baul_column_chooser_set_settings (BAUL_COLUMN_CHOOSER (chooser),
                                      visible_columns,
                                      column_order);

    g_strfreev (visible_columns);
    g_strfreev (column_order);
}

static void
use_default_callback (BaulColumnChooser *chooser,
                      gpointer user_data)
{
    g_settings_reset (baul_list_view_preferences,
                      BAUL_PREFERENCES_LIST_VIEW_DEFAULT_VISIBLE_COLUMNS);
    g_settings_reset (baul_list_view_preferences,
                      BAUL_PREFERENCES_LIST_VIEW_DEFAULT_COLUMN_ORDER);
    set_columns_from_settings (chooser);
}

static void
baul_file_management_properties_dialog_setup_list_column_page (CtkBuilder *builder)
{
    CtkWidget *chooser;
    CtkWidget *box;

    chooser = baul_column_chooser_new (NULL);
    g_signal_connect (chooser, "changed",
                      G_CALLBACK (columns_changed_callback), chooser);
    g_signal_connect (chooser, "use_default",
                      G_CALLBACK (use_default_callback), chooser);

    set_columns_from_settings (BAUL_COLUMN_CHOOSER (chooser));

    ctk_widget_show (chooser);
    box = CTK_WIDGET (ctk_builder_get_object (builder, "list_columns_vbox"));

    ctk_box_pack_start (CTK_BOX (box), chooser, TRUE, TRUE, 0);
}

static void
baul_file_management_properties_dialog_update_media_sensitivity (CtkBuilder *builder)
{
    ctk_widget_set_sensitive (CTK_WIDGET (ctk_builder_get_object (builder, "media_handling_vbox")),
                              ! g_settings_get_boolean (baul_media_preferences, BAUL_PREFERENCES_MEDIA_AUTORUN_NEVER));
}

static void
other_type_combo_box_changed (CtkComboBox *combo_box, CtkComboBox *action_combo_box)
{
    CtkTreeIter iter;
    CtkTreeModel *model;
    char *x_content_type;

    x_content_type = NULL;

    if (!ctk_combo_box_get_active_iter (combo_box, &iter))
    {
        goto out;
    }

    model = ctk_combo_box_get_model (combo_box);
    if (model == NULL)
    {
        goto out;
    }

    ctk_tree_model_get (model, &iter,
                        2, &x_content_type,
                        -1);

    baul_autorun_prepare_combo_box (CTK_WIDGET (action_combo_box),
                                    x_content_type,
                                    TRUE,
                                    TRUE,
                                    TRUE,
                                    NULL, NULL);
out:
    g_free (x_content_type);
}

static gulong extension_about_id = 0;
static gulong extension_configure_id = 0;

static void
extension_about_clicked (CtkButton *button, Extension *ext)
{
    CtkAboutDialog *extension_about_dialog;

    extension_about_dialog = (CtkAboutDialog *) ctk_about_dialog_new();
    ctk_about_dialog_set_program_name (extension_about_dialog, ext->name != NULL ? ext->name : ext->filename);
    ctk_about_dialog_set_comments (extension_about_dialog, ext->description);
    ctk_about_dialog_set_logo_icon_name (extension_about_dialog, ext->icon != NULL ? ext->icon : "system-run");
    ctk_about_dialog_set_copyright (extension_about_dialog, ext->copyright);
    ctk_about_dialog_set_authors (extension_about_dialog, (const gchar **) ext->author);
    ctk_about_dialog_set_version (extension_about_dialog, ext->version);
    ctk_about_dialog_set_website (extension_about_dialog, ext->website);
    ctk_window_set_title (CTK_WINDOW(extension_about_dialog), _("About Extension"));
    ctk_window_set_icon_name (CTK_WINDOW(extension_about_dialog), ext->icon != NULL ? ext->icon : "system-run");
    ctk_dialog_run (CTK_DIALOG (extension_about_dialog));
    ctk_widget_destroy (CTK_WIDGET (extension_about_dialog));
}

static int extension_configure_check (Extension *ext)
{
    if (!ext->state) // For now, only allow configuring enabled extensions.
    {
        return 0;
    }
    if (!BAUL_IS_CONFIGURABLE(ext->module))
    {
        return 0;
    }
    return 1;
}

static void
extension_configure_clicked (CtkButton *button, Extension *ext)
{
    if (extension_configure_check(ext)) {
        baul_configurable_run_config(BAUL_CONFIGURABLE(ext->module));
    }
}

static void
extension_list_selection_changed_about (CtkTreeSelection *selection, CtkButton *about_button)
{
    CtkTreeModel *model;
    CtkTreeIter iter;
    Extension *ext;

    ctk_widget_set_sensitive (CTK_WIDGET (about_button), FALSE);

    if (extension_about_id > 0)
    {
        g_signal_handler_disconnect (about_button, extension_about_id);
        extension_about_id = 0;
    }

    if (!ctk_tree_selection_get_selected (selection, &model, &iter))
        return;

    ctk_tree_model_get (model, &iter, EXT_STRUCT_COLUMN, &ext, -1);
    if (ext != NULL) {
        ctk_widget_set_sensitive (CTK_WIDGET (about_button), TRUE);
        extension_about_id = g_signal_connect (about_button, "clicked", G_CALLBACK (extension_about_clicked), ext);
    }
}

static void
extension_list_selection_changed_configure (CtkTreeSelection *selection, CtkButton *configure_button)
{
    CtkTreeModel *model;
    CtkTreeIter iter;
    Extension *ext;

    ctk_widget_set_sensitive (CTK_WIDGET (configure_button), FALSE);

    if (extension_configure_id > 0)
    {
        g_signal_handler_disconnect (configure_button, extension_configure_id);
        extension_configure_id = 0;
    }

    if (!ctk_tree_selection_get_selected (selection, &model, &iter))
        return;

    ctk_tree_model_get (model, &iter, EXT_STRUCT_COLUMN, &ext, -1);
    if (ext != NULL) {
        // Unconfigurable extensions remain unconfigurable.
        if (extension_configure_check(ext)) {
            ctk_widget_set_sensitive (CTK_WIDGET (configure_button), TRUE);
            extension_configure_id = g_signal_connect (configure_button, "clicked", G_CALLBACK (extension_configure_clicked), ext);
        }
    }
}

static void
extension_state_toggled (CtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
	CtkTreeIter iter;
	CtkTreePath *path;
	CtkTreeModel *model;
    gboolean new_state;
    Extension *ext;

	path = ctk_tree_path_new_from_string (path_str);
	model = ctk_tree_view_get_model (CTK_TREE_VIEW (data));

    g_object_get (G_OBJECT (cell), "active", &new_state, NULL);
    ctk_tree_model_get_iter_from_string (model, &iter, path_str);

    new_state ^= 1;

	if (&iter != NULL)
    {
        ctk_tree_model_get (model, &iter, EXT_STRUCT_COLUMN, &ext, -1);

        if (baul_extension_set_state (ext, new_state))
        {
            ctk_list_store_set (CTK_LIST_STORE (model), &iter,
                                EXT_STATE_COLUMN, new_state, -1);
        }
    }
    ctk_tree_path_free (path);
}


static void
baul_file_management_properties_dialog_setup_media_page (CtkBuilder *builder)
{
    unsigned int n;
    GList *l;
    GList *content_types;
    CtkWidget *other_type_combo_box;
    CtkListStore *other_type_list_store;
    CtkCellRenderer *renderer;
    CtkTreeIter iter;
    const char *s[] = {"media_audio_cdda_combobox",   "x-content/audio-cdda",
                       "media_video_dvd_combobox",    "x-content/video-dvd",
                       "media_music_player_combobox", "x-content/audio-player",
                       "media_dcf_combobox",          "x-content/image-dcf",
                       "media_software_combobox",     "x-content/software",
                       NULL
                      };

    for (n = 0; s[n*2] != NULL; n++)
    {
        baul_autorun_prepare_combo_box (CTK_WIDGET (ctk_builder_get_object (builder, s[n*2])), s[n*2 + 1],
                                        TRUE, TRUE, TRUE, NULL, NULL);
    }

    other_type_combo_box = CTK_WIDGET (ctk_builder_get_object (builder, "media_other_type_combobox"));

    other_type_list_store = ctk_list_store_new (3,
                            CAIRO_GOBJECT_TYPE_SURFACE,
                            G_TYPE_STRING,
                            G_TYPE_STRING);

    ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (other_type_list_store),
                                          1, CTK_SORT_ASCENDING);


    content_types = g_content_types_get_registered ();

    for (l = content_types; l != NULL; l = l->next)
    {
        char *content_type = l->data;
        char *description;
        GIcon *icon;
        BaulIconInfo *icon_info;
        cairo_surface_t *surface;
        int icon_size, icon_scale;

        if (!g_str_has_prefix (content_type, "x-content/"))
            continue;
        for (n = 0; s[n*2] != NULL; n++)
        {
            if (strcmp (content_type, s[n*2 + 1]) == 0)
            {
                goto skip;
            }
        }

        icon_size = baul_get_icon_size_for_stock_size (CTK_ICON_SIZE_MENU);
        icon_scale = ctk_widget_get_scale_factor (other_type_combo_box);

        description = g_content_type_get_description (content_type);
        ctk_list_store_append (other_type_list_store, &iter);
        icon = g_content_type_get_icon (content_type);
        if (icon != NULL)
        {
            icon_info = baul_icon_info_lookup (icon, icon_size, icon_scale);
            g_object_unref (icon);
            surface = baul_icon_info_get_surface_nodefault_at_size (icon_info, icon_size);
            g_object_unref (icon_info);
        }
        else
        {
            surface = NULL;
        }

        ctk_list_store_set (other_type_list_store, &iter,
                            0, surface,
                            1, description,
                            2, content_type,
                            -1);
        if (surface != NULL)
            cairo_surface_destroy (surface);
        g_free (description);
skip:
        ;
    }
    g_list_foreach (content_types, (GFunc) g_free, NULL);
    g_list_free (content_types);

    ctk_combo_box_set_model (CTK_COMBO_BOX (other_type_combo_box), CTK_TREE_MODEL (other_type_list_store));

    renderer = ctk_cell_renderer_pixbuf_new ();
    ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (other_type_combo_box), renderer, FALSE);
    ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (other_type_combo_box), renderer,
                                    "surface", 0,
                                    NULL);
    renderer = ctk_cell_renderer_text_new ();
    ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (other_type_combo_box), renderer, TRUE);
    ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (other_type_combo_box), renderer,
                                    "text", 1,
                                    NULL);

    g_signal_connect (G_OBJECT (other_type_combo_box),
                      "changed",
                      G_CALLBACK (other_type_combo_box_changed),
                      ctk_builder_get_object (builder, "media_other_action_combobox"));

    ctk_combo_box_set_active (CTK_COMBO_BOX (other_type_combo_box), 0);

    baul_file_management_properties_dialog_update_media_sensitivity (builder);
}

static void
baul_file_management_properties_dialog_setup_extension_page (CtkBuilder *builder)
{
    CtkCellRendererToggle *toggle;
    CtkListStore *store;
    CtkTreeView *view;
    CtkTreeSelection *selection;
    CtkTreeIter iter;
    CtkIconTheme *icon_theme;
    cairo_surface_t *ext_surface_icon;
    CtkButton *about_button, *configure_button;
    gchar *ext_text_info;

    GList *extensions;
    int i;

    extensions = baul_extensions_get_list ();

    view = CTK_TREE_VIEW (
                    ctk_builder_get_object (builder, "extension_view"));
    store = CTK_LIST_STORE (
                    ctk_builder_get_object (builder, "extension_store"));

    toggle = CTK_CELL_RENDERER_TOGGLE (
                    ctk_builder_get_object (builder, "extension_toggle"));
    g_object_set (toggle, "xpad", 6, NULL);

    g_signal_connect (toggle, "toggled",
                      G_CALLBACK (extension_state_toggled), view);

    icon_theme = ctk_icon_theme_get_default();

    for (i = 0; i < g_list_length (extensions); i++)
    {
        Extension* ext = EXTENSION (g_list_nth_data (extensions, i));

        if (ext->icon != NULL)
        {
            ext_surface_icon = ctk_icon_theme_load_surface (icon_theme, ext->icon,
                                                            24, ctk_widget_get_scale_factor (CTK_WIDGET (view)),
                                                            NULL, CTK_ICON_LOOKUP_USE_BUILTIN, NULL);
        }
        else
        {
            ext_surface_icon = ctk_icon_theme_load_surface (icon_theme, "system-run",
                                                            24, ctk_widget_get_scale_factor (CTK_WIDGET (view)),
                                                            NULL, CTK_ICON_LOOKUP_USE_BUILTIN, NULL);
        }

        if (ext->description != NULL)
        {
            ext_text_info = g_markup_printf_escaped ("<b>%s</b>\n%s",
                                                     ext->name ? ext->name : ext->filename,
                                                     ext->description);
        }
        else
        {
            ext_text_info = g_markup_printf_escaped ("<b>%s</b>",
                                                     ext->name ? ext->name : ext->filename);
        }

        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            EXT_STATE_COLUMN, ext->state,
                            EXT_ICON_COLUMN, ext_surface_icon,
                            EXT_INFO_COLUMN, ext_text_info,
                            EXT_STRUCT_COLUMN, ext, -1);

        g_free (ext_text_info);
        if (ext_surface_icon)
            cairo_surface_destroy (ext_surface_icon);
    }

    about_button = CTK_BUTTON (ctk_builder_get_object (builder, "about_extension_button"));
    configure_button = CTK_BUTTON (ctk_builder_get_object (builder, "configure_extension_button"));

    selection = ctk_tree_view_get_selection (view);
    ctk_tree_selection_set_mode (selection, CTK_SELECTION_SINGLE);
    g_signal_connect (selection, "changed",
                      G_CALLBACK (extension_list_selection_changed_about),
                      about_button);
    g_signal_connect (selection, "changed",
                      G_CALLBACK (extension_list_selection_changed_configure),
                      configure_button);
}

static void
bind_builder_bool (CtkBuilder *builder,
                   GSettings *settings,
                   const char *widget_name,
                   const char *prefs)
{
    g_settings_bind (settings, prefs,
                     ctk_builder_get_object (builder, widget_name),
                     "active", G_SETTINGS_BIND_DEFAULT);
}

static void
bind_builder_bool_inverted (CtkBuilder *builder,
                            GSettings *settings,
                            const char *widget_name,
                            const char *prefs)
{
    g_settings_bind (settings, prefs,
                      ctk_builder_get_object (builder, widget_name),
                      "active", G_SETTINGS_BIND_INVERT_BOOLEAN);
}

static gboolean
enum_get_mapping (GValue             *value,
                  GVariant           *variant,
                  gpointer            user_data)
{
    const char **enum_values = user_data;
    const char *str;
    int i;

    str = g_variant_get_string (variant, NULL);
    for (i = 0; enum_values[i] != NULL; i++) {
        if (strcmp (enum_values[i], str) == 0) {
            g_value_set_int (value, i);
            return TRUE;
        }
    }

    return FALSE;
}

static GVariant *
enum_set_mapping (const GValue       *value,
                  const GVariantType *expected_type,
                  gpointer            user_data)
{
    const char **enum_values = user_data;

    return g_variant_new_string (enum_values[g_value_get_int (value)]);
}

static void
bind_builder_enum (CtkBuilder *builder,
                   GSettings *settings,
                   const char *widget_name,
                   const char *prefs,
                   const char **enum_values)
{
    g_settings_bind_with_mapping (settings, prefs,
                                  ctk_builder_get_object (builder, widget_name),
                                  "active", G_SETTINGS_BIND_DEFAULT,
                                  enum_get_mapping,
                                  enum_set_mapping,
                                  enum_values, NULL);
}

typedef struct {
    const guint64 *values;
    int n_values;
} UIntEnumBinding;

static gboolean
uint_enum_get_mapping (GValue             *value,
               GVariant           *variant,
               gpointer            user_data)
{
    UIntEnumBinding *binding = user_data;
    guint64 v;
    int i;

    v = g_variant_get_uint64 (variant);
    for (i = 0; i < binding->n_values; i++) {
        if (binding->values[i] >= v) {
            g_value_set_int (value, i);
            return TRUE;
        }
    }

    return FALSE;
}

static GVariant *
uint_enum_set_mapping (const GValue       *value,
               const GVariantType *expected_type,
               gpointer            user_data)
{
    UIntEnumBinding *binding = user_data;

    return g_variant_new_uint64 (binding->values[g_value_get_int (value)]);
}

static void
bind_builder_uint_enum (CtkBuilder *builder,
            GSettings *settings,
            const char *widget_name,
            const char *prefs,
            const guint64 *values,
            int n_values)
{
    UIntEnumBinding *binding;

    binding = g_new (UIntEnumBinding, 1);
    binding->values = values;
    binding->n_values = n_values;

    g_settings_bind_with_mapping (settings, prefs,
                      ctk_builder_get_object (builder, widget_name),
                      "active", G_SETTINGS_BIND_DEFAULT,
                      uint_enum_get_mapping,
                      uint_enum_set_mapping,
                      binding, g_free);
}

static GVariant *
radio_mapping_set (const GValue *gvalue,
                   const GVariantType *expected_type,
                   gpointer user_data)
{
    const gchar *widget_value = user_data;
    GVariant *retval = NULL;

    if (g_value_get_boolean (gvalue)) {
        retval = g_variant_new_string (widget_value);
    }
    return retval;
}

static gboolean
radio_mapping_get (GValue *gvalue,
                   GVariant *variant,
                   gpointer user_data)
{
    const gchar *widget_value = user_data;
    const gchar *value;
    value = g_variant_get_string (variant, NULL);

    if (g_strcmp0 (value, widget_value) == 0) {
        g_value_set_boolean (gvalue, TRUE);
    } else {
        g_value_set_boolean (gvalue, FALSE);
    }

    return TRUE;
 }

static void
bind_builder_radio (CtkBuilder *builder,
            GSettings *settings,
            const char **widget_names,
            const char *prefs,
            const char **values)
{
    int i;
    CtkWidget *button = NULL;

    for (i = 0; widget_names[i] != NULL; i++) {
        button = CTK_WIDGET (ctk_builder_get_object (builder, widget_names[i]));

        g_settings_bind_with_mapping (settings, prefs,
                                      button, "active",
                                      G_SETTINGS_BIND_DEFAULT,
                                      radio_mapping_get, radio_mapping_set,
                                      (gpointer) values[i], NULL);
    }
}

static  void
baul_file_management_properties_dialog_setup (CtkBuilder *builder, CtkWindow *window)
{
    CtkWidget *dialog;

    /* setup UI */
    baul_file_management_properties_size_group_create (builder,
            "views_label",
            5);
    baul_file_management_properties_size_group_create (builder,
            "captions_label",
            3);
    baul_file_management_properties_size_group_create (builder,
            "preview_label",
            5);
    create_date_format_menu (builder);

    /* setup preferences */
    bind_builder_bool (builder, baul_icon_view_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_COMPACT_LAYOUT_WIDGET,
                       BAUL_PREFERENCES_ICON_VIEW_DEFAULT_USE_TIGHTER_LAYOUT);
    bind_builder_bool (builder, baul_icon_view_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_LABELS_BESIDE_ICONS_WIDGET,
                       BAUL_PREFERENCES_ICON_VIEW_LABELS_BESIDE_ICONS);
    bind_builder_bool (builder, baul_compact_view_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_ALL_COLUMNS_SAME_WIDTH,
                       BAUL_PREFERENCES_COMPACT_VIEW_ALL_COLUMNS_SAME_WIDTH);
    bind_builder_bool (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_FOLDERS_FIRST_WIDGET,
                       BAUL_PREFERENCES_SORT_DIRECTORIES_FIRST);
    bind_builder_bool_inverted (builder, baul_preferences,
                                BAUL_FILE_MANAGEMENT_PROPERTIES_ALWAYS_USE_BROWSER_WIDGET,
                                BAUL_PREFERENCES_ALWAYS_USE_BROWSER);

    bind_builder_bool (builder, baul_media_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_MEDIA_AUTOMOUNT_OPEN,
                       BAUL_PREFERENCES_MEDIA_AUTOMOUNT_OPEN);
    bind_builder_bool (builder, baul_media_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_MEDIA_AUTORUN_NEVER,
                       BAUL_PREFERENCES_MEDIA_AUTORUN_NEVER);

    bind_builder_bool (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_TRASH_CONFIRM_WIDGET,
                       BAUL_PREFERENCES_CONFIRM_TRASH);
    bind_builder_bool (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_TRASH_CONFIRM_TRASH_WIDGET,
                       BAUL_PREFERENCES_CONFIRM_MOVE_TO_TRASH);

    bind_builder_bool (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_TRASH_DELETE_WIDGET,
                       BAUL_PREFERENCES_ENABLE_DELETE);
    bind_builder_bool (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_SHOW_HIDDEN_WIDGET,
                       BAUL_PREFERENCES_SHOW_HIDDEN_FILES);
    bind_builder_bool (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_SHOW_BACKUP_WIDGET,
                       BAUL_PREFERENCES_SHOW_BACKUP_FILES);
    bind_builder_bool (builder, baul_tree_sidebar_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_TREE_VIEW_FOLDERS_WIDGET,
                       BAUL_PREFERENCES_TREE_SHOW_ONLY_DIRECTORIES);

    bind_builder_bool (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_USE_IEC_UNITS_WIDGET,
                       BAUL_PREFERENCES_USE_IEC_UNITS);

    bind_builder_bool (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_SHOW_ICONS_IN_LIST_VIEW,
                       BAUL_PREFERENCES_SHOW_ICONS_IN_LIST_VIEW);

    bind_builder_enum (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_DEFAULT_VIEW_WIDGET,
                       BAUL_PREFERENCES_DEFAULT_FOLDER_VIEWER,
                       (const char **) default_view_values);
    bind_builder_enum (builder, baul_icon_view_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_ICON_VIEW_ZOOM_WIDGET,
                       BAUL_PREFERENCES_ICON_VIEW_DEFAULT_ZOOM_LEVEL,
                       (const char **) zoom_values);
    bind_builder_enum (builder, baul_compact_view_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_COMPACT_VIEW_ZOOM_WIDGET,
                       BAUL_PREFERENCES_COMPACT_VIEW_DEFAULT_ZOOM_LEVEL,
                       (const char **) zoom_values);
    bind_builder_enum (builder, baul_list_view_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_LIST_VIEW_ZOOM_WIDGET,
                       BAUL_PREFERENCES_LIST_VIEW_DEFAULT_ZOOM_LEVEL,
                       (const char **) zoom_values);
    bind_builder_enum (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_SORT_ORDER_WIDGET,
                       BAUL_PREFERENCES_DEFAULT_SORT_ORDER,
                       (const char **) sort_order_values);
    bind_builder_enum (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_PREVIEW_TEXT_WIDGET,
                       BAUL_PREFERENCES_SHOW_TEXT_IN_ICONS,
                       (const char **) preview_values);
    bind_builder_enum (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_PREVIEW_IMAGE_WIDGET,
                       BAUL_PREFERENCES_SHOW_IMAGE_FILE_THUMBNAILS,
                       (const char **) preview_values);
    bind_builder_enum (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_PREVIEW_SOUND_WIDGET,
                       BAUL_PREFERENCES_PREVIEW_SOUND,
                       (const char **) preview_values);
    bind_builder_enum (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_PREVIEW_FOLDER_WIDGET,
                       BAUL_PREFERENCES_SHOW_DIRECTORY_ITEM_COUNTS,
                       (const char **) preview_values);
    bind_builder_enum (builder, baul_preferences,
                       BAUL_FILE_MANAGEMENT_PROPERTIES_DATE_FORMAT_WIDGET,
                       BAUL_PREFERENCES_DATE_FORMAT,
                       (const char **) date_format_values);

    bind_builder_radio (builder, baul_preferences,
                        (const char **) click_behavior_components,
                        BAUL_PREFERENCES_CLICK_POLICY,
                        (const char **) click_behavior_values);
    bind_builder_radio (builder, baul_preferences,
                        (const char **) executable_text_components,
                        BAUL_PREFERENCES_EXECUTABLE_TEXT_ACTIVATION,
                        (const char **) executable_text_values);

    bind_builder_uint_enum (builder, baul_preferences,
                            BAUL_FILE_MANAGEMENT_PROPERTIES_THUMBNAIL_LIMIT_WIDGET,
                            BAUL_PREFERENCES_IMAGE_FILE_THUMBNAIL_LIMIT,
                            thumbnail_limit_values,
                            G_N_ELEMENTS (thumbnail_limit_values));

    baul_file_management_properties_dialog_setup_icon_caption_page (builder);
    baul_file_management_properties_dialog_setup_list_column_page (builder);
    baul_file_management_properties_dialog_setup_media_page (builder);
    baul_file_management_properties_dialog_setup_extension_page (builder);

    g_signal_connect_swapped (baul_media_preferences,
                              "changed::" BAUL_PREFERENCES_MEDIA_AUTORUN_NEVER,
                              G_CALLBACK(baul_file_management_properties_dialog_update_media_sensitivity),
                              builder);


    /* UI callbacks */
    dialog = CTK_WIDGET (ctk_builder_get_object (builder, "file_management_dialog"));
    g_signal_connect_data (G_OBJECT (dialog), "response",
                           G_CALLBACK (baul_file_management_properties_dialog_response_cb),
                           g_object_ref (builder),
                           (GClosureNotify)g_object_unref,
                           0);

    ctk_window_set_icon_name (CTK_WINDOW (dialog), "system-file-manager");

    if (window)
    {
        ctk_window_set_screen (CTK_WINDOW (dialog), ctk_window_get_screen(window));
    }

    CtkWidget *notebook = CTK_WIDGET (ctk_builder_get_object (builder, "notebook1"));
    ctk_widget_add_events (CTK_WIDGET (notebook), GDK_SCROLL_MASK);
    g_signal_connect (CTK_WIDGET (notebook), "scroll-event",
                      G_CALLBACK (eel_dialog_page_scroll_event_callback),
                      window);

    ctk_widget_show (dialog);
}

static gboolean
delete_event_callback (CtkWidget       *widget,
                       GdkEventAny     *event,
                       gpointer         data)
{
    void (*response_callback) (CtkDialog *dialog,
                               gint response_id);

    response_callback = data;

    response_callback (CTK_DIALOG (widget), CTK_RESPONSE_CLOSE);

    return TRUE;
}

void
baul_file_management_properties_dialog_show (GCallback close_callback, CtkWindow *window)
{
    CtkBuilder *builder;

    builder = ctk_builder_new ();

    ctk_builder_add_from_file (builder,
                               UIDIR "/baul-file-management-properties.ui",
                               NULL);

    g_signal_connect (G_OBJECT (ctk_builder_get_object (builder, "file_management_dialog")),
                      "response", close_callback, NULL);
    g_signal_connect (G_OBJECT (ctk_builder_get_object (builder, "file_management_dialog")),
                      "delete_event", G_CALLBACK (delete_event_callback), close_callback);

    baul_file_management_properties_dialog_setup (builder, window);

    g_object_unref (builder);
}
