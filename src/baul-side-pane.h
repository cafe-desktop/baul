/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*  baul-side-pane.c
 *
 *  Copyright (C) 2002 Ximian, Inc.
 *
 *  Baul is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Baul is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Author: Dave Camp <dave@ximian.com>
 */

#ifndef BAUL_SIDE_PANE_H
#define BAUL_SIDE_PANE_H

#include <ctk/ctk.h>

G_BEGIN_DECLS

#define BAUL_TYPE_SIDE_PANE baul_side_pane_get_type()
#define BAUL_SIDE_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_SIDE_PANE, BaulSidePane))
#define BAUL_SIDE_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_SIDE_PANE, BaulSidePaneClass))
#define BAUL_IS_SIDE_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_SIDE_PANE))
#define BAUL_IS_SIDE_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_SIDE_PANE))
#define BAUL_SIDE_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_SIDE_PANE, BaulSidePaneClass))

    typedef struct _BaulSidePanePrivate BaulSidePanePrivate;

    typedef struct
    {
        GtkBox parent;
        BaulSidePanePrivate *details;
    } BaulSidePane;

    typedef struct
    {
        GtkBoxClass parent_slot;

        void (*close_requested) (BaulSidePane *side_pane);
        void (*switch_page) (BaulSidePane *side_pane,
                             GtkWidget *child);
    } BaulSidePaneClass;

    GType                  baul_side_pane_get_type        (void);
    BaulSidePane      *baul_side_pane_new             (void);
    void                   baul_side_pane_add_panel       (BaulSidePane *side_pane,
            GtkWidget        *widget,
            const char       *title,
            const char       *tooltip);
    void                   baul_side_pane_remove_panel    (BaulSidePane *side_pane,
            GtkWidget        *widget);
    void                   baul_side_pane_show_panel      (BaulSidePane *side_pane,
            GtkWidget        *widget);
    void                   baul_side_pane_set_panel_image (BaulSidePane *side_pane,
            GtkWidget        *widget,
            GdkPixbuf        *pixbuf);
    GtkWidget             *baul_side_pane_get_current_panel (BaulSidePane *side_pane);
    GtkWidget             *baul_side_pane_get_title        (BaulSidePane *side_pane);

G_END_DECLS

#endif /* BAUL_SIDE_PANE_H */
