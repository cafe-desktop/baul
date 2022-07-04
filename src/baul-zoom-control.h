/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2000 Eazel, Inc.
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
 * Author: Andy Hertzfeld <andy@eazel.com>
 *
 * This is the header file for the zoom control on the location bar
 *
 */

#ifndef BAUL_ZOOM_CONTROL_H
#define BAUL_ZOOM_CONTROL_H

#include <gtk/gtk.h>

#include <libbaul-private/baul-icon-info.h> /* For BaulZoomLevel */

#define BAUL_TYPE_ZOOM_CONTROL baul_zoom_control_get_type()
#define BAUL_ZOOM_CONTROL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_ZOOM_CONTROL, BaulZoomControl))
#define BAUL_ZOOM_CONTROL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_ZOOM_CONTROL, BaulZoomControlClass))
#define BAUL_IS_ZOOM_CONTROL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_ZOOM_CONTROL))
#define BAUL_IS_ZOOM_CONTROL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_ZOOM_CONTROL))
#define BAUL_ZOOM_CONTROL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_ZOOM_CONTROL, BaulZoomControlClass))

typedef struct BaulZoomControl BaulZoomControl;
typedef struct BaulZoomControlClass BaulZoomControlClass;
typedef struct _BaulZoomControlPrivate BaulZoomControlPrivate;

struct BaulZoomControl
{
    GtkBox parent;
    BaulZoomControlPrivate *details;
};

struct BaulZoomControlClass
{
    GtkBoxClass parent_class;

    void (*zoom_in)		(BaulZoomControl *control);
    void (*zoom_out) 	(BaulZoomControl *control);
    void (*zoom_to_level) 	(BaulZoomControl *control,
                             BaulZoomLevel zoom_level);
    void (*zoom_to_default)	(BaulZoomControl *control);

    /* Action signal for keybindings, do not connect to this */
    void (*change_value)    (BaulZoomControl *control,
                             GtkScrollType scroll);
};

GType             baul_zoom_control_get_type           (void);
GtkWidget *       baul_zoom_control_new                (void);
void              baul_zoom_control_set_zoom_level     (BaulZoomControl *zoom_control,
        BaulZoomLevel    zoom_level);
void              baul_zoom_control_set_parameters     (BaulZoomControl *zoom_control,
        BaulZoomLevel    min_zoom_level,
        BaulZoomLevel    max_zoom_level,
        gboolean             has_min_zoom_level,
        gboolean             has_max_zoom_level,
        GList               *zoom_levels);
BaulZoomLevel baul_zoom_control_get_zoom_level     (BaulZoomControl *zoom_control);
BaulZoomLevel baul_zoom_control_get_min_zoom_level (BaulZoomControl *zoom_control);
BaulZoomLevel baul_zoom_control_get_max_zoom_level (BaulZoomControl *zoom_control);
gboolean          baul_zoom_control_has_min_zoom_level (BaulZoomControl *zoom_control);
gboolean          baul_zoom_control_has_max_zoom_level (BaulZoomControl *zoom_control);
gboolean          baul_zoom_control_can_zoom_in        (BaulZoomControl *zoom_control);
gboolean          baul_zoom_control_can_zoom_out       (BaulZoomControl *zoom_control);

void              baul_zoom_control_set_active_appearance (BaulZoomControl *zoom_control, gboolean is_active);

#endif /* BAUL_ZOOM_CONTROL_H */
