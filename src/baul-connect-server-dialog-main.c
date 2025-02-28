/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-connect-server-main.c - Start the "Connect to Server" dialog.
 * Baul
 *
 * Copyright (C) 2005 Vincent Untz
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
 *
 * Authors:
 *   Vincent Untz <vincent@vuntz.net>
 *   Cosimo Cecchi <cosimoc@gnome.org>
 */

#include <config.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <ctk/ctk.h>
#include <cdk/cdk.h>

#include <eel/eel-stock-dialogs.h>

#include <libbaul-private/baul-icon-names.h>
#include <libbaul-private/baul-global-preferences.h>

#include "baul-connect-server-dialog.h"

static GSimpleAsyncResult *display_location_res = NULL;

static void
main_dialog_destroyed (CtkWidget *widget G_GNUC_UNUSED,
		       gpointer   user_data G_GNUC_UNUSED)
{
    /* this only happens when user clicks "cancel"
     * on the main dialog or when we are all done.
     */
    ctk_main_quit ();
}

gboolean
baul_connect_server_dialog_display_location_finish (BaulConnectServerDialog *self G_GNUC_UNUSED,
						    GAsyncResult            *res,
						    GError                 **error)
{
    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error)) {
    	return FALSE;
    }

    return TRUE;
}

void
baul_connect_server_dialog_display_location_async (BaulConnectServerDialog *self,
						   BaulApplication         *application G_GNUC_UNUSED,
						   GFile                   *location,
						   GAsyncReadyCallback      callback,
						   gpointer                 user_data)
{
    GError *error;
    CdkAppLaunchContext *launch_context;
    gchar *uri;

    display_location_res = g_simple_async_result_new (G_OBJECT (self),
    			    callback, user_data,
    			    baul_connect_server_dialog_display_location_async);

    error = NULL;
    uri = g_file_get_uri (location);

    launch_context = cdk_display_get_app_launch_context (ctk_widget_get_display (CTK_WIDGET (self)));

    cdk_app_launch_context_set_screen (launch_context,
                                       ctk_widget_get_screen (CTK_WIDGET (self)));

    g_app_info_launch_default_for_uri (uri,
                                       G_APP_LAUNCH_CONTEXT (launch_context),
                                       &error);

    g_object_unref (launch_context);

    if (error != NULL) {
    	g_simple_async_result_set_from_error (display_location_res, error);
        g_error_free (error);
    }
    g_simple_async_result_complete_in_idle (display_location_res);

    g_object_unref (display_location_res);
    display_location_res = NULL;
}

int
main (int argc, char *argv[])
{
    CtkWidget *dialog;
    GOptionContext *context;
    GError *error;

    bindtextdomain (GETTEXT_PACKAGE, CAFELOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    error = NULL;
    /* Translators: This is the --help description for the connect to server app,
       the initial newlines are between the command line arg and the description */
    context = g_option_context_new (N_("\n\nAdd connect to server mount"));
    g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
    g_option_context_add_group (context, ctk_get_option_group (TRUE));

    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
        g_critical ("Failed to parse arguments: %s", error->message);
        g_error_free (error);
        g_option_context_free (context);
        exit (1);
    }

    g_option_context_free (context);

    baul_global_preferences_init ();

    ctk_window_set_default_icon_name (BAUL_ICON_FOLDER);

    dialog = baul_connect_server_dialog_new (NULL);

    g_signal_connect (dialog, "destroy",
                      G_CALLBACK (main_dialog_destroyed), NULL);

    ctk_widget_show (dialog);

    ctk_main ();

    return 0;
}
