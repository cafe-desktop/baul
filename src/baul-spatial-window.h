/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/*
 *  Baul
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 1999, 2000, 2001 Eazel, Inc.
 *  Copyright (C) 2003 Ximian, Inc.
 *
 *  Baul is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Baul is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
/* baul-window.h: Interface of the main window object */

#ifndef BAUL_SPATIAL_WINDOW_H
#define BAUL_SPATIAL_WINDOW_H

#include "baul-window.h"
#include "baul-window-private.h"

#define BAUL_TYPE_SPATIAL_WINDOW baul_spatial_window_get_type()
#define BAUL_SPATIAL_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_SPATIAL_WINDOW, BaulSpatialWindow))
#define BAUL_SPATIAL_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_SPATIAL_WINDOW, BaulSpatialWindowClass))
#define BAUL_IS_SPATIAL_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_SPATIAL_WINDOW))
#define BAUL_IS_SPATIAL_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_SPATIAL_WINDOW))
#define BAUL_SPATIAL_WINDOW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_SPATIAL_WINDOW, BaulSpatialWindowClass))

#ifndef BAUL_SPATIAL_WINDOW_DEFINED
#define BAUL_SPATIAL_WINDOW_DEFINED
typedef struct _BaulSpatialWindow        BaulSpatialWindow;
#endif
typedef struct _BaulSpatialWindowClass   BaulSpatialWindowClass;
typedef struct _BaulSpatialWindowPrivate BaulSpatialWindowPrivate;

struct _BaulSpatialWindow
{
    BaulWindow parent_object;

    BaulSpatialWindowPrivate *details;
};

struct _BaulSpatialWindowClass
{
    BaulWindowClass parent_spot;
};


GType            baul_spatial_window_get_type			(void);
void             baul_spatial_window_set_location_button		(BaulSpatialWindow *window,
        GFile                 *location);

#endif
