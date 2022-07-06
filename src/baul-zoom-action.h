/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/*
 *  Baul
 *
 *  Copyright (C) 2009 Red Hat, Inc.
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
 *  Authors: Alexander Larsson <alexl@redhat.com>
 *
 */

#ifndef BAUL_ZOOM_ACTION_H
#define BAUL_ZOOM_ACTION_H

#include <ctk/ctk.h>

#define BAUL_TYPE_ZOOM_ACTION            (baul_zoom_action_get_type ())
#define BAUL_ZOOM_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_ZOOM_ACTION, BaulZoomAction))
#define BAUL_ZOOM_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_ZOOM_ACTION, BaulZoomActionClass))
#define BAUL_IS_ZOOM_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_ZOOM_ACTION))
#define BAUL_IS_ZOOM_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BAUL_TYPE_ZOOM_ACTION))
#define BAUL_ZOOM_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BAUL_TYPE_ZOOM_ACTION, BaulZoomActionClass))

typedef struct _BaulZoomAction       BaulZoomAction;
typedef struct _BaulZoomActionClass  BaulZoomActionClass;
typedef struct _BaulZoomActionPrivate BaulZoomActionPrivate;

struct _BaulZoomAction
{
    GtkAction parent;

    /*< private >*/
    BaulZoomActionPrivate *priv;
};

struct _BaulZoomActionClass
{
    GtkActionClass parent_class;
};

GType    baul_zoom_action_get_type   (void);

#endif
