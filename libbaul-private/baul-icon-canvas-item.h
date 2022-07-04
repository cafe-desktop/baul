/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* Caja - Icon canvas item class for icon container.
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Author: Andy Hertzfeld <andy@eazel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef BAUL_ICON_CANVAS_ITEM_H
#define BAUL_ICON_CANVAS_ITEM_H

#include <eel/eel-canvas.h>
#include <eel/eel-art-extensions.h>

G_BEGIN_DECLS

#define BAUL_TYPE_ICON_CANVAS_ITEM baul_icon_canvas_item_get_type()
#define BAUL_ICON_CANVAS_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_ICON_CANVAS_ITEM, CajaIconCanvasItem))
#define BAUL_ICON_CANVAS_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_ICON_CANVAS_ITEM, CajaIconCanvasItemClass))
#define BAUL_IS_ICON_CANVAS_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_ICON_CANVAS_ITEM))
#define BAUL_IS_ICON_CANVAS_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_ICON_CANVAS_ITEM))
#define BAUL_ICON_CANVAS_ITEM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_ICON_CANVAS_ITEM, CajaIconCanvasItemClass))

    typedef struct CajaIconCanvasItem CajaIconCanvasItem;
    typedef struct CajaIconCanvasItemClass CajaIconCanvasItemClass;
    typedef struct _CajaIconCanvasItemPrivate CajaIconCanvasItemPrivate;

    struct CajaIconCanvasItem
    {
        EelCanvasItem item;
        CajaIconCanvasItemPrivate *details;
        gpointer user_data;
    };

    struct CajaIconCanvasItemClass
    {
        EelCanvasItemClass parent_class;
    };

    /* not namespaced due to their length */
    typedef enum
    {
        BOUNDS_USAGE_FOR_LAYOUT,
        BOUNDS_USAGE_FOR_ENTIRE_ITEM,
        BOUNDS_USAGE_FOR_DISPLAY
    } CajaIconCanvasItemBoundsUsage;

    /* GObject */
    GType       baul_icon_canvas_item_get_type                 (void);

    /* attributes */
    void        baul_icon_canvas_item_set_image                (CajaIconCanvasItem       *item,
            GdkPixbuf                    *image);

    cairo_surface_t* baul_icon_canvas_item_get_drag_surface    (CajaIconCanvasItem       *item);

    void        baul_icon_canvas_item_set_emblems              (CajaIconCanvasItem       *item,
            GList                        *emblem_pixbufs);
    void        baul_icon_canvas_item_set_show_stretch_handles (CajaIconCanvasItem       *item,
            gboolean                      show_stretch_handles);
    void        baul_icon_canvas_item_set_attach_points        (CajaIconCanvasItem       *item,
            GdkPoint                     *attach_points,
            int                           n_attach_points);
    void        baul_icon_canvas_item_set_embedded_text_rect   (CajaIconCanvasItem       *item,
            const GdkRectangle           *text_rect);
    void        baul_icon_canvas_item_set_embedded_text        (CajaIconCanvasItem       *item,
            const char                   *text);
    double      baul_icon_canvas_item_get_max_text_width       (CajaIconCanvasItem       *item);
    const char *baul_icon_canvas_item_get_editable_text        (CajaIconCanvasItem       *icon_item);
    void        baul_icon_canvas_item_set_renaming             (CajaIconCanvasItem       *icon_item,
            gboolean                      state);

    /* geometry and hit testing */
    gboolean    baul_icon_canvas_item_hit_test_rectangle       (CajaIconCanvasItem       *item,
            EelIRect                      canvas_rect);
    gboolean    baul_icon_canvas_item_hit_test_stretch_handles (CajaIconCanvasItem       *item,
            EelDPoint                     world_point,
            GtkCornerType                *corner);
    void        baul_icon_canvas_item_invalidate_label         (CajaIconCanvasItem       *item);
    void        baul_icon_canvas_item_invalidate_label_size    (CajaIconCanvasItem       *item);
    EelDRect    baul_icon_canvas_item_get_icon_rectangle       (const CajaIconCanvasItem *item);
    EelDRect    baul_icon_canvas_item_get_text_rectangle       (CajaIconCanvasItem       *item,
            gboolean                      for_layout);
    void        baul_icon_canvas_item_get_bounds_for_layout    (CajaIconCanvasItem       *item,
            double *x1, double *y1, double *x2, double *y2);
    void        baul_icon_canvas_item_get_bounds_for_entire_item (CajaIconCanvasItem       *item,
            double *x1, double *y1, double *x2, double *y2);
    void        baul_icon_canvas_item_update_bounds            (CajaIconCanvasItem       *item,
            double i2w_dx, double i2w_dy);
    void        baul_icon_canvas_item_set_is_visible           (CajaIconCanvasItem       *item,
            gboolean                      visible);
    /* whether the entire label text must be visible at all times */
    void        baul_icon_canvas_item_set_entire_text          (CajaIconCanvasItem       *icon_item,
            gboolean                      entire_text);

G_END_DECLS

#endif /* BAUL_ICON_CANVAS_ITEM_H */
