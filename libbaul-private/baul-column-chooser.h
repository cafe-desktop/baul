/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-column-choose.h - A column chooser widget

   Copyright (C) 2004 Novell, Inc.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Cafe Library; see the column COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Dave Camp <dave@ximian.com>
*/

#ifndef BAUL_COLUMN_CHOOSER_H
#define BAUL_COLUMN_CHOOSER_H

#include <gtk/gtk.h>

#include "baul-file.h"

#define BAUL_TYPE_COLUMN_CHOOSER baul_column_chooser_get_type()
#define BAUL_COLUMN_CHOOSER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_COLUMN_CHOOSER, BaulColumnChooser))
#define BAUL_COLUMN_CHOOSER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_COLUMN_CHOOSER, BaulColumnChooserClass))
#define BAUL_IS_COLUMN_CHOOSER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_COLUMN_CHOOSER))
#define BAUL_IS_COLUMN_CHOOSER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_COLUMN_CHOOSER))
#define BAUL_COLUMN_CHOOSER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_COLUMN_CHOOSER, BaulColumnChooserClass))

typedef struct _BaulColumnChooserPrivate BaulColumnChooserPrivate;

typedef struct
{
    GtkBox parent;

    BaulColumnChooserPrivate *details;
} BaulColumnChooser;

typedef struct
{
    GtkBoxClass parent_slot;

    void (*changed) (BaulColumnChooser *chooser);
    void (*use_default) (BaulColumnChooser *chooser);
} BaulColumnChooserClass;

GType      baul_column_chooser_get_type            (void);
GtkWidget *baul_column_chooser_new                 (BaulFile *file);
void       baul_column_chooser_set_settings    (BaulColumnChooser   *chooser,
        char                   **visible_columns,
        char                   **column_order);
void       baul_column_chooser_get_settings    (BaulColumnChooser *chooser,
        char                  ***visible_columns,
        char                  ***column_order);

#endif /* BAUL_COLUMN_CHOOSER_H */
