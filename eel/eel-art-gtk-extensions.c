/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-eel-ctk-extensions.c - Access ctk/gdk attributes as libeel rectangles.

   Copyright (C) 2000 Eazel, Inc.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PEELICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Cafe Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

#include <config.h>

#include "eel-art-ctk-extensions.h"

/**
 * eel_ctk_widget_get_bounds:
 * @ctk_widget: The source CtkWidget.
 *
 * Return value: An EelIRect representation of the given CtkWidget's geometry
 * relative to its parent.  In the Ctk universe this is known as "allocation."
 *
 */
EelIRect
eel_ctk_widget_get_bounds (CtkWidget *ctk_widget)
{
    CtkAllocation allocation;
    g_return_val_if_fail (GTK_IS_WIDGET (ctk_widget), eel_irect_empty);

    ctk_widget_get_allocation (ctk_widget, &allocation);
    return eel_irect_assign (allocation.x,
                             allocation.y,
                             (int) allocation.width,
                             (int) allocation.height);
}

/**
 * eel_ctk_widget_get_dimensions:
 * @ctk_widget: The source CtkWidget.
 *
 * Return value: The widget's dimensions.  The returned dimensions are only valid
 *               after the widget's geometry has been "allocated" by its container.
 */
EelDimensions
eel_ctk_widget_get_dimensions (CtkWidget *ctk_widget)
{
    EelDimensions dimensions;
    CtkAllocation allocation;

    g_return_val_if_fail (GTK_IS_WIDGET (ctk_widget), eel_dimensions_empty);

    ctk_widget_get_allocation (ctk_widget, &allocation);
    dimensions.width = (int) allocation.width;
    dimensions.height = (int) allocation.height;

    return dimensions;
}
