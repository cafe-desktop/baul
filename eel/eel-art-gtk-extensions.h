/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-art-ctk-extensions.h - Access ctk/gdk attributes as libart rectangles.

   Copyright (C) 2000 Eazel, Inc.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Cafe Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

/* The following functions accept ctk/gdk structures and
 * return their bounds and dimensions, where:
 *
 * bounds: The (x,y) and (width, height) of something.
 * dimensions: The (width, height) of something.
 *
 * These are very useful in code that uses libart functions
 * to do operations on ArtIRects (such as intersection)
 */

#ifndef EEL_ART_GTK_EXTENSIONS_H
#define EEL_ART_GTK_EXTENSIONS_H

#include "eel-ctk-extensions.h"
#include "eel-art-extensions.h"

#ifdef __cplusplus
extern "C" {
#endif

    /* GtkWidget bounds and dimensions */
    EelIRect      eel_ctk_widget_get_bounds                 (GtkWidget    *widget);
    EelDimensions eel_ctk_widget_get_dimensions             (GtkWidget    *widget);

#ifdef __cplusplus
}
#endif

#endif /* EEL_ART_GTK_EXTENSIONS_H */
