/*
 *  baul-extension-types.c - Type definitions for Baul extensions
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

#include <config.h>
#include "baul-extension-types.h"

GType
baul_operation_result_get_type (void)
{
    static GType type = 0;
    if (type == 0) {
        static const GEnumValue values[] = {
            {
                BAUL_OPERATION_COMPLETE,
                "BAUL_OPERATION_COMPLETE",
                "complete",
            },
            {
                BAUL_OPERATION_FAILED,
                "BAUL_OPERATION_FAILED",
                "failed",
            },
            {
                BAUL_OPERATION_IN_PROGRESS,
                "BAUL_OPERATION_IN_PROGRESS",
                "in_progress",
            },
            { 0, NULL, NULL }
        };

        type = g_enum_register_static ("BaulOperationResult",
                                       values);
    }

    return type;
}
