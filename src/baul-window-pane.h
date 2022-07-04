/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-window-pane.h: Baul window pane

   Copyright (C) 2008 Free Software Foundation, Inc.

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

   Author: Holger Berndt <berndth@gmx.de>
*/

#ifndef BAUL_WINDOW_PANE_H
#define BAUL_WINDOW_PANE_H

#include "baul-window.h"

#define BAUL_TYPE_WINDOW_PANE	 (baul_window_pane_get_type())
#define BAUL_WINDOW_PANE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BAUL_TYPE_WINDOW_PANE, BaulWindowPaneClass))
#define BAUL_WINDOW_PANE(obj)	 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_WINDOW_PANE, BaulWindowPane))
#define BAUL_IS_WINDOW_PANE(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_WINDOW_PANE))
#define BAUL_IS_WINDOW_PANE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BAUL_TYPE_WINDOW_PANE))
#define BAUL_WINDOW_PANE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BAUL_TYPE_WINDOW_PANE, BaulWindowPaneClass))

typedef struct _BaulWindowPaneClass BaulWindowPaneClass;

struct _BaulWindowPaneClass
{
    GObjectClass parent_class;

    void (*show) (BaulWindowPane *pane);
    void (*set_active) (BaulWindowPane *pane,
                        gboolean is_active);
    void (*sync_search_widgets) (BaulWindowPane *pane);
    void (*sync_location_widgets) (BaulWindowPane *pane);
};

/* A BaulWindowPane is a layer between a slot and a window.
 * Each slot is contained in one pane, and each pane can contain
 * one or more slots. It also supports the notion of an "active slot".
 * On the other hand, each pane is contained in a window, while each
 * window can contain one or multiple panes. Likewise, the window has
 * the notion of an "active pane".
 *
 * A spatial window has only one pane, which contains a single slot.
 * A navigation window may have one or more panes.
 */
struct _BaulWindowPane
{
    GObject parent;

    /* hosting window */
    BaulWindow *window;
    gboolean visible;

    /* available slots, and active slot.
     * Both of them may never be NULL. */
    GList *slots;
    GList *active_slots;
    BaulWindowSlot *active_slot;

    /* whether or not this pane is active */
    gboolean is_active;
};

GType baul_window_pane_get_type (void);
BaulWindowPane *baul_window_pane_new (BaulWindow *window);


void baul_window_pane_show (BaulWindowPane *pane);
void baul_window_pane_zoom_in (BaulWindowPane *pane);
void baul_window_pane_zoom_to_level (BaulWindowPane *pane, BaulZoomLevel level);
void baul_window_pane_zoom_out (BaulWindowPane *pane);
void baul_window_pane_zoom_to_default (BaulWindowPane *pane);
void baul_window_pane_sync_location_widgets (BaulWindowPane *pane);
void baul_window_pane_sync_search_widgets  (BaulWindowPane *pane);
void baul_window_pane_set_active (BaulWindowPane *pane, gboolean is_active);
void baul_window_pane_slot_close (BaulWindowPane *pane, BaulWindowSlot *slot);

BaulWindowSlot* baul_window_pane_get_slot_for_content_box (BaulWindowPane *pane, GtkWidget *content_box);
void baul_window_pane_switch_to (BaulWindowPane *pane);
void baul_window_pane_grab_focus (BaulWindowPane *pane);


#endif /* BAUL_WINDOW_PANE_H */
