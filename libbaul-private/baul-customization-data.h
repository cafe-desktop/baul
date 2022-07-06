/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Baul is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Baul is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Rebecca Schulman <rebecka@eazel.com>
 */

/* baul-customization-data.h - functions to collect and load property
   names and imges */



#ifndef BAUL_CUSTOMIZATION_DATA_H
#define BAUL_CUSTOMIZATION_DATA_H

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <ctk/ctk.h>

#define RESET_IMAGE_NAME "reset.png"

typedef struct BaulCustomizationData BaulCustomizationData;



BaulCustomizationData* baul_customization_data_new                          (const char *customization_name,
        gboolean show_public_customizations,
        int maximum_icon_height,
        int maximum_icon_width);

/* Returns the following attrbiutes for a customization object (pattern, emblem)
 *
 * object_name   - The name of the object.  Usually what is used to represent it in storage.
 * object_pixbuf - Pixbuf for graphical display of the object.
 * object_label  - Textual label display of the object.
 */
gboolean                   baul_customization_data_get_next_element_for_display (BaulCustomizationData *data,
        char **object_name,
        CdkPixbuf **object_pixbuf,
        char **object_label);
gboolean                   baul_customization_data_private_data_was_displayed   (BaulCustomizationData *data);

void                       baul_customization_data_destroy                      (BaulCustomizationData *data);



CdkPixbuf*                 baul_customization_make_pattern_chit                 (CdkPixbuf *pattern_tile,
        CdkPixbuf *frame,
        gboolean dragging,
        gboolean is_reset);

#endif /* BAUL_CUSTOMIZATION_DATA_H */
