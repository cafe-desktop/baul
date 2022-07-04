/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Caja
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Caja is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Caja is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Authors: Darin Adler <darin@bentspoon.com>
 */

/* baul-desktop-window.h
 */

#ifndef BAUL_DESKTOP_WINDOW_H
#define BAUL_DESKTOP_WINDOW_H

#include "baul-window.h"
#include "baul-application.h"
#include "baul-spatial-window.h"

#include <gtk/gtk-a11y.h>

#define BAUL_TYPE_DESKTOP_WINDOW baul_desktop_window_get_type()
#define BAUL_DESKTOP_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_DESKTOP_WINDOW, CajaDesktopWindow))
#define BAUL_DESKTOP_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_DESKTOP_WINDOW, CajaDesktopWindowClass))
#define BAUL_IS_DESKTOP_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_DESKTOP_WINDOW))
#define BAUL_IS_DESKTOP_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_DESKTOP_WINDOW))
#define BAUL_DESKTOP_WINDOW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_DESKTOP_WINDOW, CajaDesktopWindowClass))

typedef struct _CajaDesktopWindowPrivate CajaDesktopWindowPrivate;

typedef struct
{
    CajaSpatialWindow parent_spot;
    CajaDesktopWindowPrivate *details;
    gboolean affect_desktop_on_next_location_change;
} CajaDesktopWindow;

typedef struct
{
    CajaSpatialWindowClass parent_spot;
} CajaDesktopWindowClass;

GType                  baul_desktop_window_get_type            (void);
CajaDesktopWindow *baul_desktop_window_new                 (CajaApplication *application,
        GdkScreen           *screen);
void                   baul_desktop_window_update_directory    (CajaDesktopWindow *window);
gboolean               baul_desktop_window_loaded              (CajaDesktopWindow *window);

#define BAUL_TYPE_DESKTOP_WINDOW_ACCESSIBLE baul_desktop_window_accessible_get_type()

typedef struct
{
  GtkWindowAccessible parent_spot;
} CajaDesktopWindowAccessible;

typedef struct
{
  GtkWindowAccessibleClass parent_spot;
} CajaDesktopWindowAccessibleClass;

#endif /* BAUL_DESKTOP_WINDOW_H */
