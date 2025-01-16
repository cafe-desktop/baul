/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2003 Ximian, Inc.
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
 * You should have received a copy of the GNU General Public
 * License along with this program; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>

#include <ctk/ctk.h>
#include <glib/gi18n.h>

#include <eel/eel-ctk-macros.h>
#include <eel/eel-stock-dialogs.h>

#include <libbaul-private/baul-file-utilities.h>

#include "baul-location-dialog.h"
#include "baul-location-entry.h"
#include "baul-desktop-window.h"

struct _BaulLocationDialogDetails
{
    CtkWidget *entry;
    BaulWindow *window;
};

static void  baul_location_dialog_class_init       (BaulLocationDialogClass *class);
static void  baul_location_dialog_init             (BaulLocationDialog      *dialog);

EEL_CLASS_BOILERPLATE (BaulLocationDialog,
                       baul_location_dialog,
                       CTK_TYPE_DIALOG)
enum
{
    RESPONSE_OPEN
};

static void
baul_location_dialog_finalize (GObject *object)
{
    BaulLocationDialog *dialog;

    dialog = BAUL_LOCATION_DIALOG (object);

    g_free (dialog->details);

    EEL_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
open_current_location (BaulLocationDialog *dialog)
{
    GFile *location;
    char *user_location;

    user_location = ctk_editable_get_chars (CTK_EDITABLE (dialog->details->entry), 0, -1);
    location = g_file_parse_name (user_location);
    baul_window_go_to (dialog->details->window, location);
    g_object_unref (location);
    g_free (user_location);
}

static void
response_callback (BaulLocationDialog *dialog,
		   int                 response_id,
		   gpointer            data G_GNUC_UNUSED)
{
    GError *error;

    switch (response_id)
    {
    case RESPONSE_OPEN :
        open_current_location (dialog);
        ctk_widget_destroy (CTK_WIDGET (dialog));
        break;
    case CTK_RESPONSE_NONE :
    case CTK_RESPONSE_DELETE_EVENT :
    case CTK_RESPONSE_CANCEL :
        ctk_widget_destroy (CTK_WIDGET (dialog));
        break;
    case CTK_RESPONSE_HELP :
        error = NULL;
        ctk_show_uri_on_window (CTK_WINDOW (dialog),
                                "help:cafe-user-guide/baul-open-location",
                                ctk_get_current_event_time (), &error);
        if (error)
        {
            eel_show_error_dialog (_("There was an error displaying help."), error->message,
                                   CTK_WINDOW (dialog));
            g_error_free (error);
        }
        break;
    default :
        g_assert_not_reached ();
    }
}

static void
entry_activate_callback (CtkEntry *entry G_GNUC_UNUSED,
			 gpointer  user_data)
{
    BaulLocationDialog *dialog;

    dialog = BAUL_LOCATION_DIALOG (user_data);

    if (ctk_entry_get_text_length (CTK_ENTRY (dialog->details->entry)) != 0)
    {
        ctk_dialog_response (CTK_DIALOG (dialog), RESPONSE_OPEN);
    }
}

static void
baul_location_dialog_class_init (BaulLocationDialogClass *class)
{
    G_OBJECT_CLASS (class)->finalize = baul_location_dialog_finalize;
}

static void
entry_text_changed (GObject    *object G_GNUC_UNUSED,
		    GParamSpec *spec G_GNUC_UNUSED,
		    gpointer    user_data)
{
    BaulLocationDialog *dialog;

    dialog = BAUL_LOCATION_DIALOG (user_data);

    if (ctk_entry_get_text_length (CTK_ENTRY (dialog->details->entry)) != 0)
    {
        ctk_dialog_set_response_sensitive (CTK_DIALOG (dialog), RESPONSE_OPEN, TRUE);
    }
    else
    {
        ctk_dialog_set_response_sensitive (CTK_DIALOG (dialog), RESPONSE_OPEN, FALSE);
    }
}

static void
baul_location_dialog_init (BaulLocationDialog *dialog)
{
    CtkWidget *box;
    CtkWidget *label;

    dialog->details = g_new0 (BaulLocationDialogDetails, 1);

    ctk_window_set_title (CTK_WINDOW (dialog), _("Open Location"));
    ctk_window_set_default_size (CTK_WINDOW (dialog), 300, -1);
    ctk_window_set_destroy_with_parent (CTK_WINDOW (dialog), TRUE);
    ctk_container_set_border_width (CTK_CONTAINER (dialog), 5);
    ctk_box_set_spacing (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dialog))), 2);

    box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
    ctk_container_set_border_width (CTK_CONTAINER (box), 5);
    ctk_widget_show (box);

    label = ctk_label_new_with_mnemonic (_("_Location:"));
    ctk_widget_show (label);
    ctk_box_pack_start (CTK_BOX (box), label, FALSE, FALSE, 0);

    dialog->details->entry = baul_location_entry_new ();
    ctk_entry_set_width_chars (CTK_ENTRY (dialog->details->entry), 30);
    g_signal_connect_after (dialog->details->entry,
                            "activate",
                            G_CALLBACK (entry_activate_callback),
                            dialog);

    ctk_widget_show (dialog->details->entry);

    ctk_label_set_mnemonic_widget (CTK_LABEL (label), dialog->details->entry);

    ctk_box_pack_start (CTK_BOX (box), dialog->details->entry,
                        TRUE, TRUE, 0);

    ctk_box_pack_start (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dialog))),
                        box, FALSE, TRUE, 0);

    eel_dialog_add_button (CTK_DIALOG (dialog),
                           _("_Help"),
                           "help-browser",
                           CTK_RESPONSE_HELP);

    eel_dialog_add_button (CTK_DIALOG (dialog),
                           _("_Cancel"),
                           "process-stop",
                           CTK_RESPONSE_CANCEL);

    eel_dialog_add_button (CTK_DIALOG (dialog),
                           _("_Open"),
                           "document-open",
                           RESPONSE_OPEN);

    ctk_dialog_set_default_response (CTK_DIALOG (dialog),
                                     RESPONSE_OPEN);

    g_signal_connect (dialog->details->entry, "notify::text",
                      G_CALLBACK (entry_text_changed), dialog);

    g_signal_connect (dialog, "response",
                      G_CALLBACK (response_callback),
                      dialog);
}

CtkWidget *
baul_location_dialog_new (BaulWindow *window)
{
    BaulLocationDialog *loc_dialog;
    CtkWidget *dialog;
    GFile *location;

    dialog = ctk_widget_new (BAUL_TYPE_LOCATION_DIALOG, NULL);
    loc_dialog = BAUL_LOCATION_DIALOG (dialog);

    if (window)
    {
        ctk_window_set_transient_for (CTK_WINDOW (dialog), CTK_WINDOW (window));
        ctk_window_set_screen (CTK_WINDOW (dialog),
                               ctk_window_get_screen (CTK_WINDOW (window)));
        loc_dialog->details->window = window;
        location = window->details->active_pane->active_slot->location;
    }
    else
        location = NULL;

    if (location != NULL)
    {
        char *formatted_location;

        if (BAUL_IS_DESKTOP_WINDOW (window))
        {
            formatted_location = g_strdup_printf ("%s/", g_get_home_dir ());
        }
        else
        {
            formatted_location = g_file_get_parse_name (location);
        }
        baul_location_entry_update_current_location (BAUL_LOCATION_ENTRY (loc_dialog->details->entry),
                formatted_location);
        g_free (formatted_location);
    }

    ctk_widget_grab_focus (loc_dialog->details->entry);

    return dialog;
}

void
baul_location_dialog_set_location (BaulLocationDialog *dialog,
                                   const char *location)
{
    baul_location_entry_update_current_location (BAUL_LOCATION_ENTRY (dialog->details->entry),
            location);
}
