/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2005 Red Hat, Inc.
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

#include <gio/gio.h>

#include <libbaul-private/baul-global-preferences.h>

#include "baul-connect-server-dialog.h"

/* This file contains the glue for the calls from the connect to server dialog
 * to the main baul binary. A different version of this glue is in
 * baul-connect-server-dialog-main.c for the standalone version.
 */

static GSimpleAsyncResult *display_location_res = NULL;

static void
window_go_to_cb (BaulWindow *window G_GNUC_UNUSED,
		 GError     *error,
		 gpointer    user_data G_GNUC_UNUSED)
{
    if (error != NULL) {
    	g_simple_async_result_set_from_error (display_location_res, error);
    }

    g_simple_async_result_complete (display_location_res);

    g_object_unref (display_location_res);
    display_location_res = NULL;
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
    						   BaulApplication *application,
    						   GFile *location,
    						   GAsyncReadyCallback callback,
    						   gpointer user_data)
{
    BaulWindow *window;
    CtkWidget *widget;

    widget = CTK_WIDGET (self);

    display_location_res =
        g_simple_async_result_new (G_OBJECT (self),
        			   callback, user_data,
        			   baul_connect_server_dialog_display_location_async);

    if (g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_ALWAYS_USE_BROWSER)) {
        window = baul_application_create_navigation_window (application,
        						    ctk_widget_get_screen (widget));
    } else {
    	window = baul_application_get_spatial_window (application,
    							  NULL,
    							  NULL,
    							  location,
    							  ctk_widget_get_screen (widget),
    							  NULL);
    }

    baul_window_go_to_full (window, location,
    			    window_go_to_cb, self);
}
