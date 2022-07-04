/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/*
 *  Baul
 *
 *  Copyright (C) 2004 Red Hat, Inc.
 *  Copyright (C) 2003 Marco Pesenti Gritti
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
#define BAUL_NAVIGATION_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_NAVIGATION_ACTION, BaulNavigationAction))
#define BAUL_NAVIGATION_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_NAVIGATION_ACTION, BaulNavigationActionClass))
#define BAUL_IS_NAVIGATION_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_NAVIGATION_ACTION))
#define BAUL_IS_NAVIGATION_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BAUL_TYPE_NAVIGATION_ACTION))
#define BAUL_NAVIGATION_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BAUL_TYPE_NAVIGATION_ACTION, BaulNavigationActionClass))

typedef struct _BaulNavigationAction       BaulNavigationAction;
typedef struct _BaulNavigationActionClass  BaulNavigationActionClass;
typedef struct _BaulNavigationActionPrivate BaulNavigationActionPrivate;

typedef enum
{
    BAUL_NAVIGATION_DIRECTION_BACK,
    BAUL_NAVIGATION_DIRECTION_FORWARD
} BaulNavigationDirection;

struct _BaulNavigationAction
{
    GtkAction parent;

    /*< private >*/
    BaulNavigationActionPrivate *priv;
};

struct _BaulNavigationActionClass
{
    GtkActionClass parent_class;
};

GType    baul_navigation_action_get_type   (void);

#endif
