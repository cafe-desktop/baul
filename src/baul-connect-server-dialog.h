/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Baul
 *
 * Copyright (C) 2003 Red Hat, Inc.
 * Copyright (C) 2010 Cosimo Cecchi <cosimoc@gnome.org>
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

#ifndef BAUL_CONNECT_SERVER_DIALOG_H
#define BAUL_CONNECT_SERVER_DIALOG_H

#include <gio/gio.h>
#include <ctk/ctk.h>

#include "baul-window.h"

#define BAUL_TYPE_CONNECT_SERVER_DIALOG\
	(baul_connect_server_dialog_get_type ())
#define BAUL_CONNECT_SERVER_DIALOG(obj)\
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_CONNECT_SERVER_DIALOG,\
				     BaulConnectServerDialog))
#define BAUL_CONNECT_SERVER_DIALOG_CLASS(klass)\
	(G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_CONNECT_SERVER_DIALOG,\
				  BaulConnectServerDialogClass))
#define BAUL_IS_CONNECT_SERVER_DIALOG(obj)\
	(G_TYPE_INSTANCE_CHECK_TYPE ((obj), BAUL_TYPE_CONNECT_SERVER_DIALOG)

typedef struct _BaulConnectServerDialog BaulConnectServerDialog;
typedef struct _BaulConnectServerDialogClass BaulConnectServerDialogClass;
typedef struct _BaulConnectServerDialogPrivate BaulConnectServerDialogPrivate;

struct _BaulConnectServerDialog
{
    GtkDialog parent;
    BaulConnectServerDialogPrivate *details;
};

struct _BaulConnectServerDialogClass
{
    GtkDialogClass parent_class;
};

GType baul_connect_server_dialog_get_type (void);

GtkWidget* baul_connect_server_dialog_new (BaulWindow *window);

void baul_connect_server_dialog_display_location_async (BaulConnectServerDialog *self,
							    BaulApplication *application,
							    GFile *location,
							    GAsyncReadyCallback callback,
							    gpointer user_data);
gboolean baul_connect_server_dialog_display_location_finish (BaulConnectServerDialog *self,
								 GAsyncResult *result,
								 GError **error);

void baul_connect_server_dialog_fill_details_async (BaulConnectServerDialog *self,
							GMountOperation *operation,
							const gchar *default_user,
							const gchar *default_domain,
							GAskPasswordFlags flags,
							GAsyncReadyCallback callback,
							gpointer user_data);
gboolean baul_connect_server_dialog_fill_details_finish (BaulConnectServerDialog *self,
							     GAsyncResult *result);

#endif /* BAUL_CONNECT_SERVER_DIALOG_H */
