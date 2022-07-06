/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-file-conflict-dialog: dialog that handles file conflicts
   during transfer operations.

   Copyright (C) 2008-2010 Cosimo Cecchi

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Cosimo Cecchi <cosimoc@gnome.org>
*/

#include <config.h>
#include <string.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <pango/pango.h>

#include <eel/eel-vfs-extensions.h>
#include <eel/eel-stock-dialogs.h>

#include "baul-file-conflict-dialog.h"
#include "baul-file.h"
#include "baul-icon-info.h"

struct _BaulFileConflictDialogPrivate
{
    /* conflicting objects */
    BaulFile *source;
    BaulFile *destination;
    BaulFile *dest_dir;

    gchar *conflict_name;
    BaulFileListHandle *handle;
    gulong src_handler_id;
    gulong dest_handler_id;

    /* UI objects */
    GtkWidget *titles_vbox;
    GtkWidget *first_hbox;
    GtkWidget *second_hbox;
    GtkWidget *expander;
    GtkWidget *entry;
    GtkWidget *checkbox;
    GtkWidget *rename_button;
    GtkWidget *diff_button;
    GtkWidget *replace_button;
    GtkWidget *dest_image;
    GtkWidget *src_image;
};

G_DEFINE_TYPE_WITH_PRIVATE (BaulFileConflictDialog,
                            baul_file_conflict_dialog,
                            GTK_TYPE_DIALOG);

static void
file_icons_changed (BaulFile *file,
                    BaulFileConflictDialog *fcd)
{
    cairo_surface_t *surface;

    surface = baul_file_get_icon_surface (fcd->details->destination,
                                          BAUL_ICON_SIZE_LARGE,
                                          FALSE,
                                          ctk_widget_get_scale_factor (fcd->details->dest_image),
                                          BAUL_FILE_ICON_FLAGS_USE_THUMBNAILS);

    ctk_image_set_from_surface (GTK_IMAGE (fcd->details->dest_image), surface);
    cairo_surface_destroy (surface);

    surface = baul_file_get_icon_surface (fcd->details->source,
                                          BAUL_ICON_SIZE_LARGE,
                                          FALSE,
                                          ctk_widget_get_scale_factor (fcd->details->src_image),
                                          BAUL_FILE_ICON_FLAGS_USE_THUMBNAILS);

    ctk_image_set_from_surface (GTK_IMAGE (fcd->details->src_image), surface);
    cairo_surface_destroy (surface);
}

static void
file_list_ready_cb (GList *files,
                    gpointer user_data)
{
    BaulFileConflictDialog *fcd = user_data;
    BaulFile *src, *dest, *dest_dir;
    time_t src_mtime, dest_mtime;
    gboolean source_is_dir,	dest_is_dir, should_show_type;
    BaulFileConflictDialogPrivate *details;
    char *primary_text, *message, *secondary_text;
    const gchar *message_extra;
    char *dest_name, *dest_dir_name, *edit_name;
    char *label_text;
    char *size, *date, *type = NULL;
    cairo_surface_t *surface;
    GtkWidget *label;
    GString *str;
    PangoAttrList *attr_list;

    details = fcd->details;

    details->handle = NULL;

    dest_dir = g_list_nth_data (files, 0);
    dest = g_list_nth_data (files, 1);
    src = g_list_nth_data (files, 2);

    src_mtime = baul_file_get_mtime (src);
    dest_mtime = baul_file_get_mtime (dest);

    dest_name = baul_file_get_display_name (dest);
    dest_dir_name = baul_file_get_display_name (dest_dir);

    source_is_dir = baul_file_is_directory (src);
    dest_is_dir = baul_file_is_directory (dest);

    type = baul_file_get_mime_type (dest);
    should_show_type = !baul_file_is_mime_type (src, type);

    g_free (type);
    type = NULL;

    /* Set up the right labels */
    if (dest_is_dir)
    {
        if (source_is_dir)
        {
            primary_text = g_strdup_printf
                           (_("Merge folder \"%s\"?"),
                            dest_name);

            message_extra =
                _("Merging will ask for confirmation before replacing any files in "
                  "the folder that conflict with the files being copied.");

            if (src_mtime > dest_mtime)
            {
                message = g_strdup_printf (
                              _("An older folder with the same name already exists in \"%s\"."),
                              dest_dir_name);
            }
            else if (src_mtime < dest_mtime)
            {
                message = g_strdup_printf (
                              _("A newer folder with the same name already exists in \"%s\"."),
                              dest_dir_name);
            }
            else
            {
                message = g_strdup_printf (
                              _("Another folder with the same name already exists in \"%s\"."),
                              dest_dir_name);
            }
        }
        else
        {
            message_extra =
                _("Replacing it will remove all files in the folder.");
            primary_text = g_strdup_printf
                           (_("Replace folder \"%s\"?"), dest_name);
            message = g_strdup_printf
                      (_("A folder with the same name already exists in \"%s\"."),
                       dest_dir_name);
        }
    }
    else
    {
        primary_text = g_strdup_printf
                       (_("Replace file \"%s\"?"), dest_name);

        message_extra = _("Replacing it will overwrite its content.");

        if (src_mtime > dest_mtime)
        {
            message = g_strdup_printf (
                          _("An older file with the same name already exists in \"%s\"."),
                          dest_dir_name);
        }
        else if (src_mtime < dest_mtime)
        {
            message = g_strdup_printf (
                          _("A newer file with the same name already exists in \"%s\"."),
                          dest_dir_name);
        }
        else
        {
            message = g_strdup_printf (
                          _("Another file with the same name already exists in \"%s\"."),
                          dest_dir_name);
        }
    }

    secondary_text = g_strdup_printf ("%s\n%s", message, message_extra);
    g_free (message);

    label = ctk_label_new (primary_text);
    ctk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    ctk_label_set_line_wrap_mode (GTK_LABEL (label), PANGO_WRAP_WORD_CHAR);
    ctk_label_set_xalign (GTK_LABEL (label), 0.0);
    ctk_box_pack_start (GTK_BOX (details->titles_vbox),
                        label, FALSE, FALSE, 0);
    ctk_widget_show (label);

    attr_list = pango_attr_list_new ();
    pango_attr_list_insert (attr_list, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
    pango_attr_list_insert (attr_list, pango_attr_scale_new (PANGO_SCALE_LARGE));
    g_object_set (label,
                  "attributes", attr_list,
                  NULL);

    pango_attr_list_unref (attr_list);
    label = ctk_label_new (secondary_text);
    ctk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    ctk_label_set_max_width_chars (GTK_LABEL (label), 60);
    ctk_label_set_xalign (GTK_LABEL (label), 0.0);
    ctk_box_pack_start (GTK_BOX (details->titles_vbox),
                        label, FALSE, FALSE, 0);
    ctk_widget_show (label);
    g_free (primary_text);
    g_free (secondary_text);

    /* Set up file icons */
    surface = baul_file_get_icon_surface (dest,
                                          BAUL_ICON_SIZE_LARGE,
                                          TRUE,
                                          ctk_widget_get_scale_factor (fcd->details->titles_vbox),
                                          BAUL_FILE_ICON_FLAGS_USE_THUMBNAILS);
    details->dest_image = ctk_image_new_from_surface (surface);
    ctk_box_pack_start (GTK_BOX (details->first_hbox),
                        details->dest_image, FALSE, FALSE, 0);
    ctk_widget_show (details->dest_image);
    cairo_surface_destroy (surface);

    surface = baul_file_get_icon_surface (src,
                                          BAUL_ICON_SIZE_LARGE,
                                          TRUE,
                                          ctk_widget_get_scale_factor (fcd->details->titles_vbox),
                                          BAUL_FILE_ICON_FLAGS_USE_THUMBNAILS);
    details->src_image = ctk_image_new_from_surface (surface);
    ctk_box_pack_start (GTK_BOX (details->second_hbox),
                        details->src_image, FALSE, FALSE, 0);
    ctk_widget_show (details->src_image);
    cairo_surface_destroy (surface);

    /* Set up labels */
    label = ctk_label_new (NULL);
    date = baul_file_get_string_attribute (dest,
                                           "date_modified");
    size = baul_file_get_string_attribute (dest, "size");

    if (should_show_type)
    {
        type = baul_file_get_string_attribute (dest, "type");
    }

    str = g_string_new (NULL);
    if (dest_is_dir) {
        g_string_append_printf (str, "<b>%s</b>\n", _("Original folder"));
        g_string_append_printf (str, "%s %s\n", _("Items:"), size);
    }
    else {
        g_string_append_printf (str, "<b>%s</b>\n", _("Original file"));
        g_string_append_printf (str, "%s %s\n", _("Size:"), size);
    }

    if (should_show_type)
    {
        g_string_append_printf (str, "%s %s\n", _("Type:"), type);
    }

    g_string_append_printf (str, "%s %s", _("Last modified:"), date);

    label_text = str->str;
    ctk_label_set_markup (GTK_LABEL (label),
                          label_text);
    ctk_box_pack_start (GTK_BOX (details->first_hbox),
                        label, FALSE, FALSE, 0);
    ctk_widget_show (label);

    g_free (size);
    g_free (type);
    g_free (date);
    g_string_erase (str, 0, -1);

    /* Second label */
    label = ctk_label_new (NULL);
    date = baul_file_get_string_attribute (src,
                                           "date_modified");
    size = baul_file_get_string_attribute (src, "size");

    if (should_show_type)
    {
        type = baul_file_get_string_attribute (src, "type");
    }

    if (source_is_dir) {
        g_string_append_printf (str, "<b>%s</b>\n", dest_is_dir ? _("Merge with") : _("Replace with"));
        g_string_append_printf (str, "%s %s\n", _("Items:"), size);
    }
    else {
        g_string_append_printf (str, "<b>%s</b>\n", _("Replace with"));
        g_string_append_printf (str, "%s %s\n", _("Size:"), size);
    }

    if (should_show_type)
    {
        g_string_append_printf (str, "%s %s\n", _("Type:"), type);
    }

    g_string_append_printf (str, "%s %s", _("Last modified:"), date);
    label_text = g_string_free (str, FALSE);

    ctk_label_set_markup (GTK_LABEL (label),
                          label_text);
    ctk_box_pack_start (GTK_BOX (details->second_hbox),
                        label, FALSE, FALSE, 0);
    ctk_widget_show (label);

    g_free (size);
    g_free (date);
    g_free (type);
    g_free (label_text);

    /* Populate the entry */
    edit_name = baul_file_get_edit_name (dest);
    details->conflict_name = edit_name;

    ctk_entry_set_text (GTK_ENTRY (details->entry), edit_name);

    if (source_is_dir && dest_is_dir)
    {
        ctk_button_set_label (GTK_BUTTON (details->replace_button),
                              _("Merge"));
    }

    /* If meld is installed, and source and destination arent binary
     * files, show the diff button
     */
    ctk_widget_hide (details->diff_button);
    if (!source_is_dir && !dest_is_dir)
    {
        gchar *meld_found = g_find_program_in_path ("meld");
        if (meld_found) {
            g_free (meld_found);
            gboolean src_is_binary;
            gboolean dest_is_binary;

            src_is_binary = baul_file_is_binary (details->source);
            dest_is_binary = baul_file_is_binary (details->destination);

            if (!src_is_binary && !dest_is_binary)
                ctk_widget_show (details->diff_button);
        }
    }

    baul_file_monitor_add (src, fcd, BAUL_FILE_ATTRIBUTES_FOR_ICON);
    baul_file_monitor_add (dest, fcd, BAUL_FILE_ATTRIBUTES_FOR_ICON);

    details->src_handler_id = g_signal_connect (src, "changed",
                              G_CALLBACK (file_icons_changed), fcd);
    details->dest_handler_id = g_signal_connect (dest, "changed",
                               G_CALLBACK (file_icons_changed), fcd);
}

static void
build_dialog_appearance (BaulFileConflictDialog *fcd)
{
    GList *files = NULL;
    BaulFileConflictDialogPrivate *details = fcd->details;

    files = g_list_prepend (files, details->source);
    files = g_list_prepend (files, details->destination);
    files = g_list_prepend (files, details->dest_dir);

    baul_file_list_call_when_ready (files,
                                    BAUL_FILE_ATTRIBUTES_FOR_ICON,
                                    &details->handle, file_list_ready_cb, fcd);
    g_list_free (files);
}

static void
set_source_and_destination (GtkWidget *w,
                            GFile *source,
                            GFile *destination,
                            GFile *dest_dir)
{
    BaulFileConflictDialog *dialog;
    BaulFileConflictDialogPrivate *details;

    dialog = BAUL_FILE_CONFLICT_DIALOG (w);
    details = dialog->details;

    details->source = baul_file_get (source);
    details->destination = baul_file_get (destination);
    details->dest_dir = baul_file_get (dest_dir);

    build_dialog_appearance (dialog);
}

static void
entry_text_changed_cb (GtkEditable *entry,
                       BaulFileConflictDialog *dialog)
{
    BaulFileConflictDialogPrivate *details;

    details = dialog->details;

    /* The rename button is visible only if there's text
     * in the entry.
     */
    if  (g_strcmp0 (ctk_entry_get_text (GTK_ENTRY (entry)), "") != 0 &&
            g_strcmp0 (ctk_entry_get_text (GTK_ENTRY (entry)), details->conflict_name) != 0)
    {
        ctk_widget_hide (details->replace_button);
        ctk_widget_show (details->rename_button);

        ctk_widget_set_sensitive (details->checkbox, FALSE);

        ctk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         CONFLICT_RESPONSE_RENAME);
    }
    else
    {
        ctk_widget_hide (details->rename_button);
        ctk_widget_show (details->replace_button);

        ctk_widget_set_sensitive (details->checkbox, TRUE);

        ctk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         CONFLICT_RESPONSE_REPLACE);
    }
}

static void
expander_activated_cb (GtkExpander *w,
                       BaulFileConflictDialog *dialog)
{
    BaulFileConflictDialogPrivate *details;
    int start_pos, end_pos;

    details = dialog->details;

    if (!ctk_expander_get_expanded (w))
    {
        if (g_strcmp0 (ctk_entry_get_text (GTK_ENTRY (details->entry)),
                       details->conflict_name) == 0)
        {
            ctk_widget_grab_focus (details->entry);

            eel_filename_get_rename_region (details->conflict_name,
                                            &start_pos, &end_pos);
            ctk_editable_select_region (GTK_EDITABLE (details->entry),
                                        start_pos, end_pos);
        }
    }
}

static void
checkbox_toggled_cb (GtkToggleButton *t,
                     BaulFileConflictDialog *dialog)
{
    BaulFileConflictDialogPrivate *details;

    details = dialog->details;

    ctk_widget_set_sensitive (details->expander,
                              !ctk_toggle_button_get_active (t));
    ctk_widget_set_sensitive (details->rename_button,
                              !ctk_toggle_button_get_active (t));

    if  (!ctk_toggle_button_get_active (t) &&
            g_strcmp0 (ctk_entry_get_text (GTK_ENTRY (details->entry)),
                       "") != 0 &&
            g_strcmp0 (ctk_entry_get_text (GTK_ENTRY (details->entry)),
                       details->conflict_name) != 0)
    {
        ctk_widget_hide (details->replace_button);
        ctk_widget_show (details->rename_button);
    }
    else
    {
        ctk_widget_hide (details->rename_button);
        ctk_widget_show (details->replace_button);
    }
}

static void
reset_button_clicked_cb (GtkButton *w,
                         BaulFileConflictDialog *dialog)
{
    BaulFileConflictDialogPrivate *details;
    int start_pos, end_pos;

    details = dialog->details;

    ctk_entry_set_text (GTK_ENTRY (details->entry),
                        details->conflict_name);
    ctk_widget_grab_focus (details->entry);
    eel_filename_get_rename_region (details->conflict_name,
                                    &start_pos, &end_pos);
    ctk_editable_select_region (GTK_EDITABLE (details->entry),
                                start_pos, end_pos);

}

static void
diff_button_clicked_cb (GtkButton *w,
                        BaulFileConflictDialog *dialog)
{
    BaulFileConflictDialogPrivate *details;
    details = dialog->details;

    GError *error;
    char *command;

    command = g_find_program_in_path ("meld");
    if (command)
    {
        char **argv;

        argv = g_new (char *, 4);
        argv[0] = command;
        argv[1] = g_file_get_path (baul_file_get_location (details->source));
        argv[2] = g_file_get_path (baul_file_get_location (details->destination));
        argv[3] = NULL;

        error = NULL;
        if (!g_spawn_async_with_pipes (NULL,
                                       argv,
                                       NULL,
                                       G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                                       NULL,
                                       NULL /* user_data */,
                                       NULL,
                                       NULL, NULL, NULL,
                                       &error))
        {
            g_warning ("Error opening meld to show differences: %s\n", error->message);
            g_error_free (error);
        }
        g_strfreev (argv);
    }
}

static void
baul_file_conflict_dialog_init (BaulFileConflictDialog *fcd)
{
    GtkWidget *hbox, *vbox, *vbox2;
    GtkWidget *widget, *dialog_area;
    BaulFileConflictDialogPrivate *details;
    GtkDialog *dialog;

    details = fcd->details = baul_file_conflict_dialog_get_instance_private (fcd);
    dialog = GTK_DIALOG (fcd);

    /* Setup the main hbox */
    hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    dialog_area = ctk_dialog_get_content_area (dialog);
    ctk_box_pack_start (GTK_BOX (dialog_area), hbox, FALSE, FALSE, 0);
    ctk_container_set_border_width (GTK_CONTAINER (hbox), 6);

    /* Setup the dialog image */
    widget = ctk_image_new_from_icon_name ("dialog-warning",
                                       GTK_ICON_SIZE_DIALOG);
    ctk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    ctk_widget_set_halign (widget, GTK_ALIGN_CENTER);
    ctk_widget_set_valign (widget, GTK_ALIGN_START);

    /* Setup the vbox containing the dialog body */
    vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 12);
    ctk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

    /* Setup the vbox for the dialog labels */
    widget = ctk_box_new (GTK_ORIENTATION_VERTICAL, 12);
    ctk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
    details->titles_vbox = widget;

    /* Setup the hboxes to pack file infos into */
    vbox2 = ctk_box_new (GTK_ORIENTATION_VERTICAL, 12);
    ctk_widget_set_halign (vbox2, GTK_ALIGN_START);
    ctk_widget_set_valign (vbox2, GTK_ALIGN_START);
    ctk_widget_set_margin_start (vbox2, 12);
    ctk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);

    hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    ctk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    details->first_hbox = hbox;

    hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
    ctk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    details->second_hbox = hbox;

    /* Setup the expander for the rename action */
    details->expander = ctk_expander_new_with_mnemonic (_("Select a new name for the _destination"));
    ctk_box_pack_start (GTK_BOX (vbox2), details->expander, FALSE, FALSE, 0);
    g_signal_connect (details->expander, "activate",
                      G_CALLBACK (expander_activated_cb), dialog);

    hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    ctk_container_add (GTK_CONTAINER (details->expander), hbox);

    widget = ctk_entry_new ();
    ctk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 6);
    details->entry = widget;
    g_signal_connect (widget, "changed",
                      G_CALLBACK (entry_text_changed_cb), dialog);

    widget = ctk_button_new_with_label (_("Reset"));
    ctk_button_set_image (GTK_BUTTON (widget),
                          ctk_image_new_from_icon_name ("edit-undo",
                                  GTK_ICON_SIZE_MENU));
    ctk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 6);
    g_signal_connect (widget, "clicked",
                      G_CALLBACK (reset_button_clicked_cb), dialog);

    ctk_widget_show_all (vbox2);

    /* Setup the diff button for text files */
    details->diff_button = ctk_button_new_with_label (_("Differences..."));
    ctk_button_set_image (GTK_BUTTON (details->diff_button),
                          ctk_image_new_from_icon_name ("edit-find",
                                  GTK_ICON_SIZE_MENU));
    ctk_box_pack_start (GTK_BOX (vbox), details->diff_button, FALSE, FALSE, 6);
    g_signal_connect (details->diff_button, "clicked",
                      G_CALLBACK (diff_button_clicked_cb), dialog);
    ctk_widget_hide (details->diff_button);

    /* Setup the checkbox to apply the action to all files */
    widget = ctk_check_button_new_with_mnemonic (_("Apply this action to all files and folders"));

    ctk_box_pack_start (GTK_BOX (vbox),
                        widget, FALSE, FALSE, 0);
    details->checkbox = widget;
    g_signal_connect (widget, "toggled",
                      G_CALLBACK (checkbox_toggled_cb), dialog);

    /* Add buttons */
    eel_dialog_add_button (dialog,
                           _("_Cancel"),
                           "process-stop",
                           GTK_RESPONSE_CANCEL);

    ctk_dialog_add_button (dialog,
                           _("_Skip"),
                           CONFLICT_RESPONSE_SKIP);

    details->rename_button =
        ctk_dialog_add_button (dialog,
                               _("Re_name"),
                               CONFLICT_RESPONSE_RENAME);
    ctk_widget_hide (details->rename_button);

    details->replace_button =
        ctk_dialog_add_button (dialog,
                               _("Replace"),
                               CONFLICT_RESPONSE_REPLACE);
    ctk_widget_grab_focus (details->replace_button);

    /* Setup HIG properties */
    ctk_container_set_border_width (GTK_CONTAINER (dialog), 5);
    ctk_box_set_spacing (GTK_BOX (ctk_dialog_get_content_area (dialog)), 14);
    ctk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

    ctk_widget_show_all (dialog_area);
}

static void
do_finalize (GObject *self)
{
    BaulFileConflictDialogPrivate *details =
        BAUL_FILE_CONFLICT_DIALOG (self)->details;

    g_free (details->conflict_name);

    if (details->handle != NULL)
    {
        baul_file_list_cancel_call_when_ready (details->handle);
    }

    if (details->src_handler_id)
    {
        g_signal_handler_disconnect (details->source, details->src_handler_id);
        baul_file_monitor_remove (details->source, self);
    }

    if (details->dest_handler_id)
    {
        g_signal_handler_disconnect (details->destination, details->dest_handler_id);
        baul_file_monitor_remove (details->destination, self);
    }

    baul_file_unref (details->source);
    baul_file_unref (details->destination);
    baul_file_unref (details->dest_dir);

    G_OBJECT_CLASS (baul_file_conflict_dialog_parent_class)->finalize (self);
}

static void
baul_file_conflict_dialog_class_init (BaulFileConflictDialogClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = do_finalize;
}

char *
baul_file_conflict_dialog_get_new_name (BaulFileConflictDialog *dialog)
{
    return g_strdup (ctk_entry_get_text
                     (GTK_ENTRY (dialog->details->entry)));
}

gboolean
baul_file_conflict_dialog_get_apply_to_all (BaulFileConflictDialog *dialog)
{
    return ctk_toggle_button_get_active
           (GTK_TOGGLE_BUTTON (dialog->details->checkbox));
}

GtkWidget *
baul_file_conflict_dialog_new (GtkWindow *parent,
                               GFile *source,
                               GFile *destination,
                               GFile *dest_dir)
{
    GtkWidget *dialog;
    BaulFile *src, *dest;
    gboolean source_is_dir, dest_is_dir;

    src = baul_file_get (source);
    dest = baul_file_get (destination);

    source_is_dir = baul_file_is_directory (src);
    dest_is_dir = baul_file_is_directory (dest);

    if (source_is_dir) {
        dialog = GTK_WIDGET (g_object_new (BAUL_TYPE_FILE_CONFLICT_DIALOG,
                                           "title", dest_is_dir ? _("Merge Folder") : _("File and Folder conflict"),
                                           NULL));
    }
    else {
        dialog = GTK_WIDGET (g_object_new (BAUL_TYPE_FILE_CONFLICT_DIALOG,
                                           "title", dest_is_dir ? _("File and Folder conflict") : _("File conflict"),
                                           NULL));
    }

    set_source_and_destination (dialog,
                                source,
                                destination,
                                dest_dir);
    ctk_window_set_transient_for (GTK_WINDOW (dialog),
                                  parent);
    return dialog;
}
