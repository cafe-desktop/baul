/*
 *  baul-column.h - Info columns exported by
 *                      CajaColumnProvider objects.
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

#ifndef BAUL_COLUMN_H
#define BAUL_COLUMN_H

#include <glib-object.h>
#include "baul-extension-types.h"

G_BEGIN_DECLS

#define BAUL_TYPE_COLUMN            (baul_column_get_type())
#define BAUL_COLUMN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_COLUMN, CajaColumn))
#define BAUL_COLUMN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_COLUMN, CajaColumnClass))
#define BAUL_INFO_IS_COLUMN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_COLUMN))
#define BAUL_INFO_IS_COLUMN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BAUL_TYPE_COLUMN))
#define BAUL_COLUMN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BAUL_TYPE_COLUMN, CajaColumnClass))

typedef struct _CajaColumn        CajaColumn;
typedef struct _CajaColumnDetails CajaColumnDetails;
typedef struct _CajaColumnClass   CajaColumnClass;

struct _CajaColumn {
    GObject parent;

    CajaColumnDetails *details;
};

struct _CajaColumnClass {
    GObjectClass parent;
};

GType       baul_column_get_type  (void);
CajaColumn *baul_column_new       (const char *name,
                                   const char *attribute,
                                   const char *label,
                                   const char *description);

/* CajaColumn has the following properties:
 *   name (string)        - the identifier for the column
 *   attribute (string)   - the file attribute to be displayed in the
 *                          column
 *   label (string)       - the user-visible label for the column
 *   description (string) - a user-visible description of the column
 *   xalign (float)       - x-alignment of the column
 */

G_END_DECLS

#endif
