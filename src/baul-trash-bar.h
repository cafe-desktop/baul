/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006 Paolo Borelli <pborelli@katamail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Authors: Paolo Borelli <pborelli@katamail.com>
 *
 */

#ifndef __BAUL_TRASH_BAR_H
#define __BAUL_TRASH_BAR_H

#include "baul-window.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BAUL_TYPE_TRASH_BAR         (baul_trash_bar_get_type ())
#define BAUL_TRASH_BAR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BAUL_TYPE_TRASH_BAR, CajaTrashBar))
#define BAUL_TRASH_BAR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BAUL_TYPE_TRASH_BAR, CajaTrashBarClass))
#define BAUL_IS_TRASH_BAR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BAUL_TYPE_TRASH_BAR))
#define BAUL_IS_TRASH_BAR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BAUL_TYPE_TRASH_BAR))
#define BAUL_TRASH_BAR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BAUL_TYPE_TRASH_BAR, CajaTrashBarClass))

typedef struct _CajaTrashBarPrivate CajaTrashBarPrivate;

typedef struct
{
        GtkBox	box;
        CajaTrashBarPrivate *priv;
} CajaTrashBar;

typedef struct
{
GtkBoxClass	    parent_class;

} CajaTrashBarClass;

GType		 baul_trash_bar_get_type	(void) G_GNUC_CONST;

GtkWidget       *baul_trash_bar_new         (CajaWindow *window);

G_END_DECLS

#endif /* __GS_TRASH_BAR_H */
