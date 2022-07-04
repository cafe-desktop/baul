/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/*
 *  Caja
 *
 *  Copyright (C) 2004 Red Hat, Inc.
 *  Copyright (C) 2003 Marco Pesenti Gritti
 *
 *  Caja is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Caja is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 *  Based on ephy-navigation-action.h from Epiphany
 *
 *  Authors: Alexander Larsson <alexl@redhat.com>
 *           Marco Pesenti Gritti
 *
 */

#ifndef BAUL_NAVIGATION_ACTION_H
#define BAUL_NAVIGATION_ACTION_H

#include <gtk/gtk.h>

#define BAUL_TYPE_NAVIGATION_ACTION            (baul_navigation_action_get_type ())
#define BAUL_NAVIGATION_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_NAVIGATION_ACTION, CajaNavigationAction))
#define BAUL_NAVIGATION_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_NAVIGATION_ACTION, CajaNavigationActionClass))
#define BAUL_IS_NAVIGATION_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_NAVIGATION_ACTION))
#define BAUL_IS_NAVIGATION_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BAUL_TYPE_NAVIGATION_ACTION))
#define BAUL_NAVIGATION_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BAUL_TYPE_NAVIGATION_ACTION, CajaNavigationActionClass))

typedef struct _CajaNavigationAction       CajaNavigationAction;
typedef struct _CajaNavigationActionClass  CajaNavigationActionClass;
typedef struct _CajaNavigationActionPrivate CajaNavigationActionPrivate;

typedef enum
{
    BAUL_NAVIGATION_DIRECTION_BACK,
    BAUL_NAVIGATION_DIRECTION_FORWARD
} CajaNavigationDirection;

struct _CajaNavigationAction
{
    GtkAction parent;

    /*< private >*/
    CajaNavigationActionPrivate *priv;
};

struct _CajaNavigationActionClass
{
    GtkActionClass parent_class;
};

GType    baul_navigation_action_get_type   (void);

#endif
