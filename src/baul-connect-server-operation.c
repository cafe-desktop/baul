/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Baul
 *
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
 *
 * Author: Cosimo Cecchi <cosimoc@gnome.org>
 */

#include <config.h>

#include "baul-connect-server-operation.h"

#include "baul-connect-server-dialog.h"

enum {
	PROP_DIALOG = 1,
	NUM_PROPERTIES
};

struct _BaulConnectServerOperationPrivate {
	BaulConnectServerDialog *dialog;
};

G_DEFINE_TYPE_WITH_PRIVATE (BaulConnectServerOperation,
	       baul_connect_server_operation, CTK_TYPE_MOUNT_OPERATION);

static void
fill_details_async_cb (GObject *source,
		       GAsyncResult *result,
		       gpointer user_data)
{
	BaulConnectServerDialog *dialog;
	BaulConnectServerOperation *self;
	gboolean res;

	self = user_data;
	dialog = BAUL_CONNECT_SERVER_DIALOG (source);

	res = baul_connect_server_dialog_fill_details_finish (dialog, result);

	if (!res) {
		g_mount_operation_reply (G_MOUNT_OPERATION (self), G_MOUNT_OPERATION_ABORTED);
	} else {
		g_mount_operation_reply (G_MOUNT_OPERATION (self), G_MOUNT_OPERATION_HANDLED);
	}
}

static void
baul_connect_server_operation_ask_password (GMountOperation  *op,
					    const gchar      *message G_GNUC_UNUSED,
					    const gchar      *default_user,
					    const gchar      *default_domain,
					    GAskPasswordFlags flags)
{
	BaulConnectServerOperation *self;

	self = BAUL_CONNECT_SERVER_OPERATION (op);

	baul_connect_server_dialog_fill_details_async (self->details->dialog,
							   G_MOUNT_OPERATION (self),
							   default_user,
							   default_domain,
							   flags,
							   fill_details_async_cb,
							   self);
}

static void
baul_connect_server_operation_set_property (GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec)
{
	BaulConnectServerOperation *self;

	self = BAUL_CONNECT_SERVER_OPERATION (object);

	switch (property_id) {
	case PROP_DIALOG:
		self->details->dialog = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
baul_connect_server_operation_class_init (BaulConnectServerOperationClass *klass)
{
	GMountOperationClass *mount_op_class;
	GObjectClass *object_class;
	GParamSpec *pspec;

	object_class = G_OBJECT_CLASS (klass);
	object_class->set_property = baul_connect_server_operation_set_property;

	mount_op_class = G_MOUNT_OPERATION_CLASS (klass);
	mount_op_class->ask_password = baul_connect_server_operation_ask_password;

	pspec = g_param_spec_object ("dialog", "The connect dialog",
				     "The connect to server dialog",
				     BAUL_TYPE_CONNECT_SERVER_DIALOG,
				     G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_DIALOG, pspec);
}

static void
baul_connect_server_operation_init (BaulConnectServerOperation *self)
{
	self->details = baul_connect_server_operation_get_instance_private (self);
}

GMountOperation *
baul_connect_server_operation_new (BaulConnectServerDialog *dialog)
{
	return g_object_new (BAUL_TYPE_CONNECT_SERVER_OPERATION,
			     "dialog", dialog,
			     NULL);
}
