/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Red Hat, Inc.
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
 * Authors: David Zeuthen <davidz@redhat.com>
 *          Paolo Borelli <pborelli@katamail.com>
 *
 */

#ifndef __BAUL_X_CONTENT_BAR_H
#define __BAUL_X_CONTENT_BAR_H

#include <ctk/ctk.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define BAUL_TYPE_X_CONTENT_BAR         (baul_x_content_bar_get_type ())
#define BAUL_X_CONTENT_BAR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BAUL_TYPE_X_CONTENT_BAR, BaulXContentBar))
#define BAUL_X_CONTENT_BAR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BAUL_TYPE_X_CONTENT_BAR, BaulXContentBarClass))
#define BAUL_IS_X_CONTENT_BAR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BAUL_TYPE_X_CONTENT_BAR))
#define BAUL_IS_X_CONTENT_BAR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BAUL_TYPE_X_CONTENT_BAR))
#define BAUL_X_CONTENT_BAR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BAUL_TYPE_X_CONTENT_BAR, BaulXContentBarClass))

    typedef struct _BaulXContentBarPrivate BaulXContentBarPrivate;

    typedef struct
    {
        CtkBox	box;

        BaulXContentBarPrivate *priv;
    } BaulXContentBar;

    typedef struct
    {
        CtkBoxClass	    parent_class;
    } BaulXContentBarClass;

    GType		 baul_x_content_bar_get_type	(void) G_GNUC_CONST;

    CtkWidget	*baul_x_content_bar_new		   (GMount              *mount,
            const char          *x_content_type);
    const char      *baul_x_content_bar_get_x_content_type (BaulXContentBar *bar);
    void             baul_x_content_bar_set_x_content_type (BaulXContentBar *bar,
            const char          *x_content_type);
    void             baul_x_content_bar_set_mount          (BaulXContentBar *bar,
            GMount              *mount);
    GMount          *baul_x_content_bar_get_mount          (BaulXContentBar *bar);

G_END_DECLS

#endif /* __BAUL_X_CONTENT_BAR_H */
