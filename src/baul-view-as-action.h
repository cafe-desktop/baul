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

#ifndef BAUL_VIEW_AS_ACTION_H
#define BAUL_VIEW_AS_ACTION_H

#include <gtk/gtk.h>

#define BAUL_TYPE_VIEW_AS_ACTION            (baul_view_as_action_get_type ())
#define BAUL_VIEW_AS_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_VIEW_AS_ACTION, BaulViewAsAction))
#define BAUL_VIEW_AS_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_VIEW_AS_ACTION, BaulViewAsActionClass))
#define BAUL_IS_VIEW_AS_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_VIEW_AS_ACTION))
#define BAUL_IS_VIEW_AS_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BAUL_TYPE_VIEW_AS_ACTION))
#define BAUL_VIEW_AS_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BAUL_TYPE_VIEW_AS_ACTION, BaulViewAsActionClass))

typedef struct _BaulViewAsAction       BaulViewAsAction;
typedef struct _BaulViewAsActionClass  BaulViewAsActionClass;
typedef struct _BaulViewAsActionPrivate BaulViewAsActionPrivate;

struct _BaulViewAsAction
{
    GtkAction parent;

    /*< private >*/
    BaulViewAsActionPrivate *priv;
};

struct _BaulViewAsActionClass
{
    GtkActionClass parent_class;
};

GType    baul_view_as_action_get_type   (void);

#endif
