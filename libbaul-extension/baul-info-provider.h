/*
 *  baul-info-provider.h - Interface for Baul extensions that
 *                             provide info about files.
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
 * provide information about files.  Extensions are called when Baul
 * needs information about a file.  They are passed a BaulFileInfo
 * object which should be filled with relevant information */

#ifndef BAUL_INFO_PROVIDER_H
#define BAUL_INFO_PROVIDER_H

#include <glib-object.h>
#include "baul-extension-types.h"
#include "baul-file-info.h"

G_BEGIN_DECLS

#define BAUL_TYPE_INFO_PROVIDER           (baul_info_provider_get_type ())
#define BAUL_INFO_PROVIDER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_INFO_PROVIDER, BaulInfoProvider))
#define BAUL_IS_INFO_PROVIDER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_INFO_PROVIDER))
#define BAUL_INFO_PROVIDER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAUL_TYPE_INFO_PROVIDER, BaulInfoProviderIface))

typedef struct _BaulInfoProvider       BaulInfoProvider;
typedef struct _BaulInfoProviderIface  BaulInfoProviderIface;

typedef void (*BaulInfoProviderUpdateComplete) (BaulInfoProvider    *provider,
                                                BaulOperationHandle *handle,
                                                BaulOperationResult  result,
                                                gpointer             user_data);

/**
 * BaulInfoProviderIface:
 * @g_iface: The parent interface.
 * @update_file_info: Returns a #BaulOperationResult.
 *   See baul_info_provider_update_file_info() for details.
 * @cancel_update: Cancels a previous call to baul_info_provider_update_file_info().
 *   See baul_info_provider_cancel_update() for details.
 *
 * Interface for extensions to provide additional information about files.
 */

struct _BaulInfoProviderIface {
    GTypeInterface g_iface;

    BaulOperationResult (*update_file_info) (BaulInfoProvider     *provider,
                                             BaulFileInfo         *file,
                                             GClosure             *update_complete,
                                             BaulOperationHandle **handle);
    void                (*cancel_update)    (BaulInfoProvider     *provider,
                                             BaulOperationHandle  *handle);
};

/* Interface Functions */
GType               baul_info_provider_get_type               (void);
BaulOperationResult baul_info_provider_update_file_info       (BaulInfoProvider     *provider,
                                                               BaulFileInfo         *file,
                                                               GClosure             *update_complete,
                                                               BaulOperationHandle **handle);
void                baul_info_provider_cancel_update          (BaulInfoProvider     *provider,
                                                               BaulOperationHandle  *handle);



/* Helper functions for implementations */
void                baul_info_provider_update_complete_invoke (GClosure             *update_complete,
                                                               BaulInfoProvider     *provider,
                                                               BaulOperationHandle  *handle,
                                                               BaulOperationResult   result);

G_END_DECLS

#endif
