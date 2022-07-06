/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-keep-last-vertical-box.h: Subclass of GtkBox that clips off
 				      items that don't fit, except the last one.

   Copyright (C) 2000 Eazel, Inc.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Cafe Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Author: John Sullivan <sullivan@eazel.com>,
 */

#ifndef BAUL_KEEP_LAST_VERTICAL_BOX_H
#define BAUL_KEEP_LAST_VERTICAL_BOX_H

#include <ctk/ctk.h>

#define BAUL_TYPE_KEEP_LAST_VERTICAL_BOX baul_keep_last_vertical_box_get_type()
#define BAUL_KEEP_LAST_VERTICAL_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_KEEP_LAST_VERTICAL_BOX, BaulKeepLastVerticalBox))
#define BAUL_KEEP_LAST_VERTICAL_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_KEEP_LAST_VERTICAL_BOX, BaulKeepLastVerticalBoxClass))
#define BAUL_IS_KEEP_LAST_VERTICAL_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_KEEP_LAST_VERTICAL_BOX))
#define BAUL_IS_KEEP_LAST_VERTICAL_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_KEEP_LAST_VERTICAL_BOX))
#define BAUL_KEEP_LAST_VERTICAL_BOX_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_KEEP_LAST_VERTICAL_BOX, BaulKeepLastVerticalBoxClass))

typedef struct BaulKeepLastVerticalBox BaulKeepLastVerticalBox;
typedef struct BaulKeepLastVerticalBoxClass BaulKeepLastVerticalBoxClass;

struct BaulKeepLastVerticalBox
{
    GtkBox parent;
};

struct BaulKeepLastVerticalBoxClass
{
    GtkBoxClass parent_class;
};

GType      baul_keep_last_vertical_box_get_type  (void);
GtkWidget *baul_keep_last_vertical_box_new       (gint spacing);

#endif /* BAUL_KEEP_LAST_VERTICAL_BOX_H */
