/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-navigation-window-pane.h: Baul navigation window pane

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

#ifndef BAUL_NAVIGATION_WINDOW_PANE_H
#define BAUL_NAVIGATION_WINDOW_PANE_H

#include "baul-window-pane.h"
#include "baul-navigation-window-slot.h"

#define BAUL_TYPE_NAVIGATION_WINDOW_PANE     (baul_navigation_window_pane_get_type())
#define BAUL_NAVIGATION_WINDOW_PANE_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), BAUL_TYPE_NAVIGATION_WINDOW_PANE, BaulNavigationWindowPaneClass))
#define BAUL_NAVIGATION_WINDOW_PANE(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_NAVIGATION_WINDOW_PANE, BaulNavigationWindowPane))
#define BAUL_IS_NAVIGATION_WINDOW_PANE(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_NAVIGATION_WINDOW_PANE))
#define BAUL_IS_NAVIGATION_WINDOW_PANE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BAUL_TYPE_NAVIGATION_WINDOW_PANE))
#define BAUL_NAVIGATION_WINDOW_PANE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BAUL_TYPE_NAVIGATION_WINDOW_PANE, BaulNavigationWindowPaneClass))

typedef struct _BaulNavigationWindowPaneClass BaulNavigationWindowPaneClass;
typedef struct _BaulNavigationWindowPane      BaulNavigationWindowPane;

struct _BaulNavigationWindowPaneClass
{
    BaulWindowPaneClass parent_class;
};

struct _BaulNavigationWindowPane
{
    BaulWindowPane parent;

    GtkWidget *widget;

    /* location bar */
    GtkWidget *location_bar;
    GtkWidget *location_button;
    GtkWidget *navigation_bar;
    GtkWidget *path_bar;
    GtkWidget *search_bar;

    gboolean temporary_navigation_bar;
    gboolean temporary_location_bar;
    gboolean temporary_search_bar;

    /* notebook */
    GtkWidget *notebook;

    /* split view */
    GtkWidget *split_view_hpane;
};

GType    baul_navigation_window_pane_get_type (void);

BaulNavigationWindowPane* baul_navigation_window_pane_new (BaulWindow *window);

/* location bar */
void     baul_navigation_window_pane_setup             (BaulNavigationWindowPane *pane);

void     baul_navigation_window_pane_hide_location_bar (BaulNavigationWindowPane *pane, gboolean save_preference);
void     baul_navigation_window_pane_show_location_bar (BaulNavigationWindowPane *pane, gboolean save_preference);
gboolean baul_navigation_window_pane_location_bar_showing (BaulNavigationWindowPane *pane);
void     baul_navigation_window_pane_hide_path_bar (BaulNavigationWindowPane *pane);
void     baul_navigation_window_pane_show_path_bar (BaulNavigationWindowPane *pane);
gboolean baul_navigation_window_pane_path_bar_showing (BaulNavigationWindowPane *pane);
gboolean baul_navigation_window_pane_search_bar_showing (BaulNavigationWindowPane *pane);
void     baul_navigation_window_pane_set_bar_mode  (BaulNavigationWindowPane *pane, BaulBarMode mode);
void     baul_navigation_window_pane_show_location_bar_temporarily (BaulNavigationWindowPane *pane);
void     baul_navigation_window_pane_show_navigation_bar_temporarily (BaulNavigationWindowPane *pane);
void     baul_navigation_window_pane_always_use_location_entry (BaulNavigationWindowPane *pane, gboolean use_entry);
gboolean baul_navigation_window_pane_hide_temporary_bars (BaulNavigationWindowPane *pane);
/* notebook */
void     baul_navigation_window_pane_add_slot_in_tab (BaulNavigationWindowPane *pane, BaulWindowSlot *slot, BaulWindowOpenSlotFlags flags);
void     baul_navigation_window_pane_remove_page (BaulNavigationWindowPane *pane, int page_num);

#endif /* BAUL_NAVIGATION_WINDOW_PANE_H */
