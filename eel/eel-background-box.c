/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-background-box.c - an event box that renders an eel background

   Copyright (C) 2002 Sun Microsystems, Inc.

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

   Author: Dave Camp <dave@ximian.com>
*/

#include <config.h>
#include "eel-background-box.h"

#include "eel-background.h"

G_DEFINE_TYPE (EelBackgroundBox, eel_background_box, CTK_TYPE_EVENT_BOX)

static gboolean
eel_background_box_draw (CtkWidget *widget,
                         cairo_t *cr)
{
    eel_background_draw (widget, cr);
    ctk_container_propagate_draw (CTK_CONTAINER (widget),
                                  ctk_bin_get_child (CTK_BIN (widget)),
                                  cr);
    return TRUE;
}

static void
eel_background_box_init (EelBackgroundBox *box G_GNUC_UNUSED)
{
}

static void
eel_background_box_class_init (EelBackgroundBoxClass *klass)
{
    CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
    widget_class->draw = eel_background_box_draw;

}

CtkWidget*
eel_background_box_new (void)
{
    EelBackgroundBox *background_box;

    background_box = EEL_BACKGROUND_BOX (ctk_widget_new (eel_background_box_get_type (), NULL));

    return CTK_WIDGET (background_box);
}
