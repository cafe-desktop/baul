/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Copyright (C) 2004 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef BAUL_IMAGE_PROPERTIES_PAGE_H
#define BAUL_IMAGE_PROPERTIES_PAGE_H

#include <ctk/ctk.h>

G_BEGIN_DECLS

#define BAUL_TYPE_IMAGE_PROPERTIES_PAGE baul_image_properties_page_get_type()
#define BAUL_IMAGE_PROPERTIES_PAGE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_IMAGE_PROPERTIES_PAGE, BaulImagePropertiesPage))
#define BAUL_IMAGE_PROPERTIES_PAGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_IMAGE_PROPERTIES_PAGE, BaulImagePropertiesPageClass))
#define BAUL_IS_IMAGE_PROPERTIES_PAGE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_IMAGE_PROPERTIES_PAGE))
#define BAUL_IS_IMAGE_PROPERTIES_PAGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_IMAGE_PROPERTIES_PAGE))
#define BAUL_IMAGE_PROPERTIES_PAGE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_IMAGE_PROPERTIES_PAGE, BaulImagePropertiesPageClass))

typedef struct _BaulImagePropertiesPagePrivate BaulImagePropertiesPagePrivate;

typedef struct
{
    GtkBox parent;
    BaulImagePropertiesPagePrivate *details;
} BaulImagePropertiesPage;

typedef struct
{
    GtkBoxClass parent;
} BaulImagePropertiesPageClass;

GType baul_image_properties_page_get_type (void);
void  baul_image_properties_page_register (void);

G_END_DECLS

#endif /* BAUL_IMAGE_PROPERTIES_PAGE_H */
