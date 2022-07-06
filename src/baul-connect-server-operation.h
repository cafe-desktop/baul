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

#ifndef __BAUL_CONNECT_SERVER_OPERATION_H__
#define __BAUL_CONNECT_SERVER_OPERATION_H__

#include <gio/gio.h>
#include <ctk/ctk.h>

#include "baul-connect-server-dialog.h"

#define BAUL_TYPE_CONNECT_SERVER_OPERATION\
	(baul_connect_server_operation_get_type ())
#define BAUL_CONNECT_SERVER_OPERATION(obj)\
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
			       BAUL_TYPE_CONNECT_SERVER_OPERATION,\
			       BaulConnectServerOperation))
#define BAUL_CONNECT_SERVER_OPERATION_CLASS(klass)\
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_CONNECT_SERVER_OPERATION,\
			    BaulConnectServerOperationClass))
#define BAUL_IS_CONNECT_SERVER_OPERATION(obj)\
  (G_TYPE_INSTANCE_CHECK_TYPE ((obj), BAUL_TYPE_CONNECT_SERVER_OPERATION)

typedef struct _BaulConnectServerOperationPrivate
  BaulConnectServerOperationPrivate;

typedef struct {
	GtkMountOperation parent;
	BaulConnectServerOperationPrivate *details;
} BaulConnectServerOperation;

typedef struct {
	GtkMountOperationClass parent_class;
} BaulConnectServerOperationClass;

GType baul_connect_server_operation_get_type (void);

GMountOperation *
baul_connect_server_operation_new (BaulConnectServerDialog *dialog);


#endif /* __BAUL_CONNECT_SERVER_OPERATION_H__ */
