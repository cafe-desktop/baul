/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-column-utilities.h - Utilities related to column specifications

   Copyright (C) 2004 Novell, Inc.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the column COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Dave Camp <dave@ximian.com>
*/

#ifndef BAUL_COLUMN_UTILITIES_H
#define BAUL_COLUMN_UTILITIES_H

#include <libbaul-extension/baul-column.h>

#include "baul-file.h"

GList *baul_get_all_columns       (void);
GList *baul_get_common_columns    (void);
GList *baul_get_columns_for_file (BaulFile *file);
GList *baul_column_list_copy      (GList       *columns);
void   baul_column_list_free      (GList       *columns);

GList *baul_sort_columns          (GList       *columns,
                                   char       **column_order);


#endif /* BAUL_COLUMN_UTILITIES_H */
