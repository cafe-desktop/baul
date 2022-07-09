/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-stock-dialogs.c: Various standard dialogs for Eel.

   Copyright (C) 2000 Eazel, Inc.

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

   Authors: Darin Adler <darin@eazel.com>
*/

#include <config.h>
#include "eel-stock-dialogs.h"

#include "eel-glib-extensions.h"
#include "eel-ctk-extensions.h"

#include <glib/gi18n-lib.h>
#include <ctk/ctk.h>

#define TIMED_WAIT_STANDARD_DURATION 2000
#define TIMED_WAIT_MIN_TIME_UP 3000

#define TIMED_WAIT_MINIMUM_DIALOG_WIDTH 300

#define RESPONSE_DETAILS 1000

typedef struct
{
    EelCancelCallback cancel_callback;
    gpointer callback_data;

    /* Parameters for creation of the window. */
    char *wait_message;
    CtkWindow *parent_window;

    /* Timer to determine when we need to create the window. */
    guint timeout_handler_id;

    /* Window, once it's created. */
    CtkDialog *dialog;

    /* system time (microseconds) when dialog was created */
    gint64 dialog_creation_time;

} TimedWait;

static GHashTable *timed_wait_hash_table;

static void timed_wait_dialog_destroy_callback (CtkWidget *object, gpointer callback_data);

static guint
timed_wait_hash (gconstpointer value)
{
    const TimedWait *wait;

    wait = value;

    return GPOINTER_TO_UINT (wait->cancel_callback)
           ^ GPOINTER_TO_UINT (wait->callback_data);
}

static gboolean
timed_wait_hash_equal (gconstpointer value1, gconstpointer value2)
{
    const TimedWait *wait1, *wait2;

    wait1 = value1;
    wait2 = value2;

    return wait1->cancel_callback == wait2->cancel_callback
           && wait1->callback_data == wait2->callback_data;
}

static void
timed_wait_delayed_close_destroy_dialog_callback (CtkWidget *object, gpointer callback_data)
{
    g_source_remove (GPOINTER_TO_UINT (callback_data));
}

static gboolean
timed_wait_delayed_close_timeout_callback (gpointer callback_data)
{
    guint handler_id;

    handler_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (callback_data),
                                   "eel-stock-dialogs/delayed_close_handler_timeout_id"));

    g_signal_handlers_disconnect_by_func (G_OBJECT (callback_data),
                                          G_CALLBACK (timed_wait_delayed_close_destroy_dialog_callback),
                                          GUINT_TO_POINTER (handler_id));

	ctk_widget_destroy (CTK_WIDGET (callback_data));

    return FALSE;
}

static void
timed_wait_free (TimedWait *wait)
{
    guint delayed_close_handler_id;
    guint64 time_up;

    g_assert (g_hash_table_lookup (timed_wait_hash_table, wait) != NULL);

    g_hash_table_remove (timed_wait_hash_table, wait);

    g_free (wait->wait_message);
    if (wait->parent_window != NULL)
    {
        g_object_unref (wait->parent_window);
    }
    if (wait->timeout_handler_id != 0)
    {
        g_source_remove (wait->timeout_handler_id);
    }
    if (wait->dialog != NULL)
    {
        /* Make sure to detach from the "destroy" signal, or we'll
         * double-free.
         */
        g_signal_handlers_disconnect_by_func (G_OBJECT (wait->dialog),
                                              G_CALLBACK (timed_wait_dialog_destroy_callback),
                                              wait);

        /* compute time up in milliseconds */
        time_up = (g_get_monotonic_time () - wait->dialog_creation_time) / 1000;

        if (time_up < TIMED_WAIT_MIN_TIME_UP)
        {
            delayed_close_handler_id = g_timeout_add (TIMED_WAIT_MIN_TIME_UP - time_up,
                                       timed_wait_delayed_close_timeout_callback,
                                       wait->dialog);
            g_object_set_data (G_OBJECT (wait->dialog),
                               "eel-stock-dialogs/delayed_close_handler_timeout_id",
                               GUINT_TO_POINTER (delayed_close_handler_id));
            g_signal_connect (wait->dialog, "destroy",
                              G_CALLBACK (timed_wait_delayed_close_destroy_dialog_callback),
                              GUINT_TO_POINTER (delayed_close_handler_id));
        }
        else
        {
            ctk_widget_destroy (CTK_WIDGET (wait->dialog));
        }
    }

    /* And the wait object itself. */
    g_free (wait);
}

static void
timed_wait_dialog_destroy_callback (CtkWidget *object, gpointer callback_data)
{
    TimedWait *wait;

    wait = callback_data;

    g_assert (CTK_DIALOG (object) == wait->dialog);

    wait->dialog = NULL;

    /* When there's no cancel_callback, the originator will/must
     * call eel_timed_wait_stop which will call timed_wait_free.
     */

    if (wait->cancel_callback != NULL)
    {
        (* wait->cancel_callback) (wait->callback_data);
        timed_wait_free (wait);
    }
}

static void
trash_dialog_response_callback (CtkDialog *dialog,
                                int response_id,
                                TimedWait *wait)
{
    ctk_widget_destroy (CTK_WIDGET (dialog));
}

CtkWidget*
eel_dialog_add_button (CtkDialog   *dialog,
                       const gchar *button_text,
                       const gchar *icon_name,
                             gint   response_id)
{
    CtkWidget *button;

    button = ctk_button_new_with_mnemonic (button_text);
    ctk_button_set_image (CTK_BUTTON (button), ctk_image_new_from_icon_name (icon_name, CTK_ICON_SIZE_BUTTON));

    ctk_button_set_use_underline (CTK_BUTTON (button), TRUE);
    ctk_style_context_add_class (ctk_widget_get_style_context (button), "text-button");
    ctk_widget_set_can_default (button, TRUE);
    ctk_widget_show (button);
    ctk_dialog_add_action_widget (CTK_DIALOG (dialog), button, response_id);

    return button;
}

static CtkWidget *
eel_file_chooser_dialog_new_valist (const gchar          *title,
                                    CtkWindow            *parent,
                                    CtkFileChooserAction  action,
                                    const gchar          *first_button_text,
                                    va_list               varargs)
{
    CtkWidget *result;
    const char *button_text = first_button_text;
    gint response_id;

    result = g_object_new (CTK_TYPE_FILE_CHOOSER_DIALOG,
                           "title", title,
                           "action", action,
                           NULL);

    if (parent)
        ctk_window_set_transient_for (CTK_WINDOW (result), parent);

    while (button_text)
        {
            response_id = va_arg (varargs, gint);

            if (g_strcmp0 (button_text, "process-stop") == 0)
                eel_dialog_add_button (CTK_DIALOG (result), _("_Cancel"), button_text, response_id);
            else if (g_strcmp0 (button_text, "document-open") == 0)
                eel_dialog_add_button (CTK_DIALOG (result), _("_Open"), button_text, response_id);
            else if (g_strcmp0 (button_text, "document-revert") == 0)
                eel_dialog_add_button (CTK_DIALOG (result), _("_Revert"), button_text, response_id);
            else
                ctk_dialog_add_button (CTK_DIALOG (result), button_text, response_id);

            button_text = va_arg (varargs, const gchar *);
        }

    return result;
}

CtkWidget *
eel_file_chooser_dialog_new (const gchar          *title,
                             CtkWindow            *parent,
                             CtkFileChooserAction  action,
                             const gchar          *first_button_text,
                             ...)
{
    CtkWidget *result;
    va_list varargs;

    va_start (varargs, first_button_text);
    result = eel_file_chooser_dialog_new_valist (title, parent, action,
                                                 first_button_text,
                                                 varargs);
    va_end (varargs);

    return result;
}

static gboolean
timed_wait_callback (gpointer callback_data)
{
    TimedWait *wait;
    CtkDialog *dialog;
    const char *button;

    wait = callback_data;

    /* Put up the timed wait window. */
    button = wait->cancel_callback != NULL ? "process-stop" : "ctk-ok";
	dialog = CTK_DIALOG (ctk_message_dialog_new (wait->parent_window,
						     0,
						     CTK_MESSAGE_INFO,
						     CTK_BUTTONS_NONE,
						     NULL));

	g_object_set (dialog,
		      "text", wait->wait_message,
		      "secondary-text", _("You can stop this operation by clicking cancel."),
		      NULL);

    if (g_strcmp0 (button, "process-stop") == 0)
        eel_dialog_add_button (CTK_DIALOG (dialog), _("_Cancel"), button, CTK_RESPONSE_OK);
    else
        eel_dialog_add_button (CTK_DIALOG (dialog), _("_OK"), button, CTK_RESPONSE_OK);

    ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);

    /* The contents are often very small, causing tiny little
     * dialogs with their titles clipped if you just let ctk
     * sizing do its thing. This enforces a minimum width to
     * make it more likely that the title won't be clipped.
     */
    ctk_window_set_default_size (CTK_WINDOW (dialog),
                                 TIMED_WAIT_MINIMUM_DIALOG_WIDTH,
                                 -1);
    wait->dialog_creation_time = g_get_monotonic_time ();
    ctk_widget_show (CTK_WIDGET (dialog));

    /* FIXME bugzilla.eazel.com 2441:
     * Could parent here, but it's complicated because we
     * don't want this window to go away just because the parent
     * would go away first.
     */

    /* Make the dialog cancel the timed wait when it goes away.
     * Connect to "destroy" instead of "response" since we want
     * to be called no matter how the dialog goes away.
     */
    g_signal_connect (dialog, "destroy",
                      G_CALLBACK (timed_wait_dialog_destroy_callback),
                      wait);
    g_signal_connect (dialog, "response",
                      G_CALLBACK (trash_dialog_response_callback),
                      wait);

    wait->timeout_handler_id = 0;
    wait->dialog = dialog;

    return FALSE;
}

void
eel_timed_wait_start_with_duration (int duration,
                                    EelCancelCallback cancel_callback,
                                    gpointer callback_data,
                                    const char *wait_message,
                                    CtkWindow *parent_window)
{
    TimedWait *wait;

    g_return_if_fail (callback_data != NULL);
    g_return_if_fail (wait_message != NULL);
    g_return_if_fail (parent_window == NULL || CTK_IS_WINDOW (parent_window));

    /* Create the timed wait record. */
    wait = g_new0 (TimedWait, 1);
    wait->wait_message = g_strdup (wait_message);
    wait->cancel_callback = cancel_callback;
    wait->callback_data = callback_data;
    wait->parent_window = parent_window;

    if (parent_window != NULL)
    {
        g_object_ref (parent_window);
    }

    /* Start the timer. */
    wait->timeout_handler_id = g_timeout_add (duration, timed_wait_callback, wait);

    /* Put in the hash table so we can find it later. */
    if (timed_wait_hash_table == NULL)
    {
        timed_wait_hash_table = eel_g_hash_table_new_free_at_exit
                                (timed_wait_hash, timed_wait_hash_equal, __FILE__ ": timed wait");
    }
    g_assert (g_hash_table_lookup (timed_wait_hash_table, wait) == NULL);
    g_hash_table_insert (timed_wait_hash_table, wait, wait);
    g_assert (g_hash_table_lookup (timed_wait_hash_table, wait) == wait);
}

void
eel_timed_wait_start (EelCancelCallback cancel_callback,
                      gpointer callback_data,
                      const char *wait_message,
                      CtkWindow *parent_window)
{
    eel_timed_wait_start_with_duration
    (TIMED_WAIT_STANDARD_DURATION,
     cancel_callback, callback_data,
     wait_message, parent_window);
}

void
eel_timed_wait_stop (EelCancelCallback cancel_callback,
                     gpointer callback_data)
{
    TimedWait key;
    TimedWait *wait;

    g_return_if_fail (callback_data != NULL);

    key.cancel_callback = cancel_callback;
    key.callback_data = callback_data;
    wait = g_hash_table_lookup (timed_wait_hash_table, &key);

    g_return_if_fail (wait != NULL);

    timed_wait_free (wait);
}

int
eel_run_simple_dialog (CtkWidget *parent, gboolean ignore_close_box,
                       CtkMessageType message_type, const char *primary_text,
                       const char *secondary_text, ...)
{
    va_list button_title_args;
    CtkWidget *dialog;
    CtkWidget *top_widget, *chosen_parent;
    int result;
    int response_id;

    /* Parent it if asked to. */
    chosen_parent = NULL;
    if (parent != NULL)
    {
        top_widget = ctk_widget_get_toplevel (parent);
        if (CTK_IS_WINDOW (top_widget))
        {
            chosen_parent = top_widget;
        }
    }

    /* Create the dialog. */
	dialog = ctk_message_dialog_new (CTK_WINDOW (chosen_parent),
					 0,
					 message_type,
					 CTK_BUTTONS_NONE,
					 NULL);

	g_object_set (dialog,
		      "text", primary_text,
		      "secondary-text", secondary_text,
		      NULL);

    va_start (button_title_args, secondary_text);
    response_id = 0;
    while (1)
    {
        const char *button_title;

        button_title = va_arg (button_title_args, const char *);
        if (button_title == NULL)
        {
            break;
        }

        if (g_strcmp0 (button_title, "process-stop") == 0)
            eel_dialog_add_button (CTK_DIALOG (dialog), _("_Cancel"), button_title, response_id);
        else
            eel_dialog_add_button (CTK_DIALOG (dialog), _("_OK"), button_title, response_id);

        ctk_dialog_set_default_response (CTK_DIALOG (dialog), response_id);
        response_id++;
    }
    va_end (button_title_args);

    /* Run it. */
    ctk_widget_show (dialog);
    result = ctk_dialog_run (CTK_DIALOG (dialog));
    while ((result == CTK_RESPONSE_NONE || result == CTK_RESPONSE_DELETE_EVENT) && ignore_close_box)
    {
        ctk_widget_show (CTK_WIDGET (dialog));
        result = ctk_dialog_run (CTK_DIALOG (dialog));
    }
	ctk_widget_destroy (dialog);

    return result;
}

static CtkDialog *
create_message_dialog (const char *primary_text,
                       const char *secondary_text,
                       CtkMessageType type,
                       CtkButtonsType buttons_type,
                       CtkWindow *parent)
{
    CtkWidget *dialog;

	dialog = ctk_message_dialog_new (parent,
					 0,
					 type,
					 buttons_type,
					 NULL);

	g_object_set (dialog,
		      "text", primary_text,
		      "secondary-text", secondary_text,
		      NULL);

    return CTK_DIALOG (dialog);
}

static CtkDialog *
show_message_dialog (const char *primary_text,
                     const char *secondary_text,
                     CtkMessageType type,
                     CtkButtonsType buttons_type,
                     const char *details_text,
                     CtkWindow *parent)
{
    CtkDialog *dialog;

    dialog = create_message_dialog (primary_text, secondary_text, type,
                                    buttons_type, parent);
    if (details_text != NULL) {
        eel_ctk_message_dialog_set_details_label (CTK_MESSAGE_DIALOG (dialog),
						  details_text);
    }
    ctk_widget_show (CTK_WIDGET (dialog));

    g_signal_connect (dialog, "response",
			  G_CALLBACK (ctk_widget_destroy), NULL);

    return dialog;
}

static CtkDialog *
show_ok_dialog (const char *primary_text,
                const char *secondary_text,
                CtkMessageType type,
                CtkWindow *parent)
{
    CtkDialog *dialog;

    dialog = show_message_dialog (primary_text, secondary_text, type,
                                  CTK_BUTTONS_OK, NULL, parent);
    ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);

    return dialog;
}

CtkDialog *
eel_show_info_dialog (const char *primary_text,
                      const char *secondary_text,
                      CtkWindow *parent)
{
    return show_ok_dialog (primary_text,
                           secondary_text,
                           CTK_MESSAGE_INFO, parent);
}

CtkDialog *
eel_show_info_dialog_with_details (const char *primary_text,
                                   const char *secondary_text,
                                   const char *detailed_info,
                                   CtkWindow *parent)
{
    CtkDialog *dialog;

    if (detailed_info == NULL
            || strcmp (primary_text, detailed_info) == 0)
    {
        return eel_show_info_dialog (primary_text, secondary_text, parent);
    }

    dialog = show_message_dialog (primary_text,
                                  secondary_text,
                                  CTK_MESSAGE_INFO,
                                  CTK_BUTTONS_OK,
                                  detailed_info,
                                  parent);

    return dialog;

}


CtkDialog *
eel_show_warning_dialog (const char *primary_text,
                         const char *secondary_text,
                         CtkWindow *parent)
{
    return show_ok_dialog (primary_text,
                           secondary_text,
                           CTK_MESSAGE_WARNING, parent);
}


CtkDialog *
eel_show_error_dialog (const char *primary_text,
                       const char *secondary_text,
                       CtkWindow *parent)
{
    return show_ok_dialog (primary_text,
                           secondary_text,
                           CTK_MESSAGE_ERROR, parent);
}

/**
 * eel_show_yes_no_dialog:
 *
 * Create and show a dialog asking a question with two choices.
 * The caller needs to set up any necessary callbacks
 * for the buttons. Use eel_create_question_dialog instead
 * if any visual changes need to be made, to avoid flashiness.
 * @question: The text of the question.
 * @yes_label: The label of the "yes" button.
 * @no_label: The label of the "no" button.
 * @parent: The parent window for this dialog.
 */
CtkDialog *
eel_show_yes_no_dialog (const char *primary_text,
                        const char *secondary_text,
                        const char *yes_label,
                        const char *no_label,
                        CtkWindow *parent)
{
    CtkDialog *dialog = NULL;
    dialog = eel_create_question_dialog (primary_text,
                                         secondary_text,
                                         no_label, CTK_RESPONSE_CANCEL,
                                         yes_label, CTK_RESPONSE_YES,
                                         CTK_WINDOW (parent));
    ctk_widget_show (CTK_WIDGET (dialog));
    return dialog;
}

/**
 * eel_create_question_dialog:
 *
 * Create a dialog asking a question with at least two choices.
 * The caller needs to set up any necessary callbacks
 * for the buttons. The dialog is not yet shown, so that the
 * caller can add additional buttons or make other visual changes
 * without causing flashiness.
 * @question: The text of the question.
 * @answer_0: The label of the leftmost button (index 0)
 * @answer_1: The label of the 2nd-to-leftmost button (index 1)
 * @parent: The parent window for this dialog.
 */
CtkDialog *
eel_create_question_dialog (const char *primary_text,
                            const char *secondary_text,
                            const char *answer_1,
                            int response_1,
                            const char *answer_2,
                            int response_2,
                            CtkWindow *parent)
{
    CtkDialog *dialog;

    dialog = create_message_dialog (primary_text,
                                    secondary_text,
                                    CTK_MESSAGE_QUESTION,
                                    CTK_BUTTONS_NONE,
                                    parent);

    if (g_strcmp0 (answer_1, "process-stop") == 0)
        eel_dialog_add_button (dialog, _("_Cancel"), answer_1, response_1);
    else
        ctk_dialog_add_button (dialog, answer_1, response_1);

    if (g_strcmp0 (answer_2, "ctk-ok") == 0)
        eel_dialog_add_button (dialog, _("_OK"), answer_2, response_2);
    else if (g_strcmp0 (answer_2, "edit-clear") == 0)
        eel_dialog_add_button (dialog, _("_Clear"), answer_2, response_2);
    else
        ctk_dialog_add_button (dialog, answer_2, response_2);

    return dialog;
}
