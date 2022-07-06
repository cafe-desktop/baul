/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-cell-renderer-text-ellipsized.c: Cell renderer for text which
   will use pango ellipsization but deactivate it temporarily for the size
   calculation to get the size based on the actual text length.

   Copyright (C) 2007 Martin Wehner

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Author: Martin Wehner <martin.wehner@gmail.com>
*/

#include "baul-cell-renderer-text-ellipsized.h"

G_DEFINE_TYPE (BaulCellRendererTextEllipsized, baul_cell_renderer_text_ellipsized,
               CTK_TYPE_CELL_RENDERER_TEXT);

static void
baul_cell_renderer_text_ellipsized_init (BaulCellRendererTextEllipsized *cell)
{
    g_object_set (cell,
                  "ellipsize", PANGO_ELLIPSIZE_END,
                  "ellipsize-set", TRUE,
                  NULL);
}

static void
baul_cell_renderer_text_ellipsized_get_preferred_width (CtkCellRenderer *cell,
        						    CtkWidget       *widget,
        						    gint            *minimum_size,
        						    gint            *natural_size)
{
    g_object_set (cell,
                  "ellipsize", PANGO_ELLIPSIZE_NONE,
                  "ellipsize-set", FALSE,
                  NULL);

    CTK_CELL_RENDERER_CLASS
            (baul_cell_renderer_text_ellipsized_parent_class)->get_preferred_width (cell, widget,
        										minimum_size, natural_size);

    g_object_set (cell,
                  "ellipsize", PANGO_ELLIPSIZE_END,
                  "ellipsize-set", TRUE,
                  NULL);
}

static void
baul_cell_renderer_text_ellipsized_class_init (BaulCellRendererTextEllipsizedClass *klass)
{
    CtkCellRendererClass *cell_class = CTK_CELL_RENDERER_CLASS (klass);
    cell_class->get_preferred_width = baul_cell_renderer_text_ellipsized_get_preferred_width;

}

CtkCellRenderer *
baul_cell_renderer_text_ellipsized_new (void)
{
    return g_object_new (BAUL_TYPE_CELL_RENDERER_TEXT_ELLIPSIZED, NULL);
}
