/*
 *  baul-info-provider.h - Interface for Caja extensions that
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

/* This interface is implemented by Caja extensions that want to
 * provide information about files.  Extensions are called when Caja
 * needs information about a file.  They are passed a CajaFileInfo
 * object which should be filled with relevant information */

#ifndef BAUL_INFO_PROVIDER_H
#define BAUL_INFO_PROVIDER_H

#include <glib-object.h>
#include "baul-extension-types.h"
#include "baul-file-info.h"

G_BEGIN_DECLS

#define BAUL_TYPE_INFO_PROVIDER           (baul_info_provider_get_type ())
#define BAUL_INFO_PROVIDER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_INFO_PROVIDER, CajaInfoProvider))
#define BAUL_IS_INFO_PROVIDER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_INFO_PROVIDER))
#define BAUL_INFO_PROVIDER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAUL_TYPE_INFO_PROVIDER, CajaInfoProviderIface))

typedef struct _CajaInfoProvider       CajaInfoProvider;
typedef struct _CajaInfoProviderIface  CajaInfoProviderIface;

typedef void (*CajaInfoProviderUpdateComplete) (CajaInfoProvider    *provider,
                                                CajaOperationHandle *handle,
                                                CajaOperationResult  result,
                                                gpointer             user_data);

/**
 * CajaInfoProviderIface:
 * @g_iface: The parent interface.
 * @update_file_info: Returns a #CajaOperationResult.
 *   See baul_info_provider_update_file_info() for details.
 * @cancel_update: Cancels a previous call to baul_info_provider_update_file_info().
 *   See baul_info_provider_cancel_update() for details.
 *
 * Interface for extensions to provide additional information about files.
 */

struct _CajaInfoProviderIface {
    GTypeInterface g_iface;

    CajaOperationResult (*update_file_info) (CajaInfoProvider     *provider,
                                             CajaFileInfo         *file,
                                             GClosure             *update_complete,
                                             CajaOperationHandle **handle);
    void                (*cancel_update)    (CajaInfoProvider     *provider,
                                             CajaOperationHandle  *handle);
};

/* Interface Functions */
GType               baul_info_provider_get_type               (void);
CajaOperationResult baul_info_provider_update_file_info       (CajaInfoProvider     *provider,
                                                               CajaFileInfo         *file,
                                                               GClosure             *update_complete,
                                                               CajaOperationHandle **handle);
void                baul_info_provider_cancel_update          (CajaInfoProvider     *provider,
                                                               CajaOperationHandle  *handle);



/* Helper functions for implementations */
void                baul_info_provider_update_complete_invoke (GClosure             *update_complete,
                                                               CajaInfoProvider     *provider,
                                                               CajaOperationHandle  *handle,
                                                               CajaOperationResult   result);

G_END_DECLS

#endif
