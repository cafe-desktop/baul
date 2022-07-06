/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Baul is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Baul is distributed in the hope that it will be useful,
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

#include <ctk/ctk-a11y.h>

#define BAUL_TYPE_DESKTOP_WINDOW baul_desktop_window_get_type()
#define BAUL_DESKTOP_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_DESKTOP_WINDOW, BaulDesktopWindow))
#define BAUL_DESKTOP_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_DESKTOP_WINDOW, BaulDesktopWindowClass))
#define BAUL_IS_DESKTOP_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_DESKTOP_WINDOW))
#define BAUL_IS_DESKTOP_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_DESKTOP_WINDOW))
#define BAUL_DESKTOP_WINDOW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_DESKTOP_WINDOW, BaulDesktopWindowClass))

typedef struct _BaulDesktopWindowPrivate BaulDesktopWindowPrivate;

typedef struct
{
    BaulSpatialWindow parent_spot;
    BaulDesktopWindowPrivate *details;
    gboolean affect_desktop_on_next_location_change;
} BaulDesktopWindow;

typedef struct
{
    BaulSpatialWindowClass parent_spot;
} BaulDesktopWindowClass;

GType                  baul_desktop_window_get_type            (void);
BaulDesktopWindow *baul_desktop_window_new                 (BaulApplication *application,
        CdkScreen           *screen);
void                   baul_desktop_window_update_directory    (BaulDesktopWindow *window);
gboolean               baul_desktop_window_loaded              (BaulDesktopWindow *window);

#define BAUL_TYPE_DESKTOP_WINDOW_ACCESSIBLE baul_desktop_window_accessible_get_type()

typedef struct
{
  CtkWindowAccessible parent_spot;
} BaulDesktopWindowAccessible;

typedef struct
{
  CtkWindowAccessibleClass parent_spot;
} BaulDesktopWindowAccessibleClass;

#endif /* BAUL_DESKTOP_WINDOW_H */
