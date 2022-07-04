/*
 *  baul-column-provider.h - Interface for Baul extensions that
 *                               provide column descriptions.
 *
 *  Copyright (C) 2003 Novell, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Author:  Dave Camp <dave@ximian.com>
 *
 */

/* This interface is implemented by Baul extensions that want to
 * add columns to the list view and details to the icon view.
 * Extensions are asked for a list of columns to display.  Each
 * returned column refers to a string attribute which can be filled in
 * by BaulInfoProvider */

#ifndef BAUL_COLUMN_PROVIDER_H
#define BAUL_COLUMN_PROVIDER_H

#include <glib-object.h>
#include "baul-extension-types.h"
#include "baul-column.h"

G_BEGIN_DECLS

#define BAUL_TYPE_COLUMN_PROVIDER           (baul_column_provider_get_type ())
#define BAUL_COLUMN_PROVIDER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_COLUMN_PROVIDER, BaulColumnProvider))
#define BAUL_IS_COLUMN_PROVIDER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_COLUMN_PROVIDER))
#define BAUL_COLUMN_PROVIDER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAUL_TYPE_COLUMN_PROVIDER, BaulColumnProviderIface))

typedef struct _BaulColumnProvider       BaulColumnProvider;
typedef struct _BaulColumnProviderIface  BaulColumnProviderIface;

/**
 * BaulColumnProviderIface:
 * @g_iface: The parent interface.
 * @get_columns: Returns a #GList of #BaulColumn.
 *   See baul_column_provider_get_columns() for details.
 *
 * Interface for extensions to provide additional list view columns.
 */

struct _BaulColumnProviderIface {
    GTypeInterface g_iface;

    GList *(*get_columns) (BaulColumnProvider *provider);
};

/* Interface Functions */
GType  baul_column_provider_get_type    (void);
GList *baul_column_provider_get_columns (BaulColumnProvider *provider);

G_END_DECLS

#endif
