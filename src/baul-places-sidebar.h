/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 *  Caja
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Author : Mr Jamie McCracken (jamiemcc at blueyonder dot co dot uk)
 *
 */
#ifndef _BAUL_PLACES_SIDEBAR_H
#define _BAUL_PLACES_SIDEBAR_H

#include <gtk/gtk.h>

#include <libbaul-private/baul-view.h>
#include <libbaul-private/baul-window-info.h>

#define BAUL_PLACES_SIDEBAR_ID    "places"

#define BAUL_TYPE_PLACES_SIDEBAR baul_places_sidebar_get_type()
#define BAUL_PLACES_SIDEBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_PLACES_SIDEBAR, CajaPlacesSidebar))
#define BAUL_PLACES_SIDEBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_PLACES_SIDEBAR, CajaPlacesSidebarClass))
#define BAUL_IS_PLACES_SIDEBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_PLACES_SIDEBAR))
#define BAUL_IS_PLACES_SIDEBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_PLACES_SIDEBAR))
#define BAUL_PLACES_SIDEBAR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_PLACES_SIDEBAR, CajaPlacesSidebarClass))


GType baul_places_sidebar_get_type (void);
void baul_places_sidebar_register (void);

#endif
