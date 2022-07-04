/*
 *  baul-info-provider.h - Type definitions for Baul extensions
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
 *  Author: Dave Camp <dave@ximian.com>
 *
 */

/* This interface is implemented by Baul extensions that want to
 * provide information about files.  Extensions are called when Baul
 * needs information about a file.  They are passed a BaulFileInfo
 * object which should be filled with relevant information */

#ifndef BAUL_EXTENSION_TYPES_H
#define BAUL_EXTENSION_TYPES_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BAUL_TYPE_OPERATION_RESULT (baul_operation_result_get_type ())

/**
 * BaulOperationHandle:
 *
 * Handle for asynchronous interfaces. These are opaque handles that must
 * be unique within an extension object. These are returned by operations
 * that return BAUL_OPERATION_IN_PROGRESS.
 */
typedef struct _BaulOperationHandle BaulOperationHandle;

/**
 * BaulOperationResult:
 * @BAUL_OPERATION_COMPLETE: the operation succeeded, and the extension
 *  is done with the request.
 * @BAUL_OPERATION_FAILED: the operation failed.
 * @BAUL_OPERATION_IN_PROGRESS: the extension has begin an async operation.
 *  When this value is returned, the extension must set the handle parameter
 *  and call the callback closure when the operation is complete.
 */
typedef enum {
    /* Returned if the call succeeded, and the extension is done
     * with the request */
    BAUL_OPERATION_COMPLETE,

    /* Returned if the call failed */
    BAUL_OPERATION_FAILED,

    /* Returned if the extension has begun an async operation.
     * If this is returned, the extension must set the handle
     * parameter and call the callback closure when the
     * operation is complete. */
    BAUL_OPERATION_IN_PROGRESS
} BaulOperationResult;

GType baul_operation_result_get_type (void);

/**
 * SECTION:baul-extension-types
 * @title: BaulModule
 * @short_description: Initialize an extension
 * @include: libbaul-extension/baul-extension-types.h
 *
 * Methods that each extension implements.
 */

void baul_module_initialize  (GTypeModule  *module);
void baul_module_shutdown    (void);
void baul_module_list_types  (const GType **types,
                              int          *num_types);
void baul_module_list_pyfiles (GList      **pyfiles);

G_END_DECLS

#endif
