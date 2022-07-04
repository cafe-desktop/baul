/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 1999, 2000 Eazel, Inc.
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
 *
 *  This is the header file for the index panel widget, which displays oversidebar information
 *  in a vertical panel and hosts the meta-sidebars.
 */

#ifndef BAUL_EMBLEM_SIDEBAR_H
#define BAUL_EMBLEM_SIDEBAR_H

#include <gtk/gtk.h>

#define BAUL_TYPE_EMBLEM_SIDEBAR baul_emblem_sidebar_get_type()
#define BAUL_EMBLEM_SIDEBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_EMBLEM_SIDEBAR, BaulEmblemSidebar))
#define BAUL_EMBLEM_SIDEBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_EMBLEM_SIDEBAR, BaulEmblemSidebarClass))
#define BAUL_IS_EMBLEM_SIDEBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_EMBLEM_SIDEBAR))
#define BAUL_IS_EMBLEM_SIDEBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_EMBLEM_SIDEBAR))
#define BAUL_EMBLEM_SIDEBAR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_EMBLEM_SIDEBAR, BaulEmblemSidebarClass))

#define BAUL_EMBLEM_SIDEBAR_ID "emblems"

typedef struct BaulEmblemSidebarDetails BaulEmblemSidebarDetails;

typedef struct
{
    GtkBox parent_slot;
    BaulEmblemSidebarDetails *details;
} BaulEmblemSidebar;

typedef struct
{
    GtkBoxClass parent_slot;
} BaulEmblemSidebarClass;

GType	baul_emblem_sidebar_get_type     (void);
void    baul_emblem_sidebar_register     (void);

#endif /* BAUL_EMBLEM_SIDEBAR_H */
