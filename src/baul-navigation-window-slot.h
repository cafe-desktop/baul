/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-navigation-window-slot.h: Baul navigation window slot

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

   Author: Christian Neumair <cneumair@gnome.org>
*/

#ifndef BAUL_NAVIGATION_WINDOW_SLOT_H
#define BAUL_NAVIGATION_WINDOW_SLOT_H

#include "baul-window-slot.h"

typedef struct BaulNavigationWindowSlot BaulNavigationWindowSlot;
typedef struct BaulNavigationWindowSlotClass BaulNavigationWindowSlotClass;


#define BAUL_TYPE_NAVIGATION_WINDOW_SLOT         (baul_navigation_window_slot_get_type())
#define BAUL_NAVIGATION_WINDOW_SLOT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BAUL_NAVIGATION_WINDOW_SLOT_CLASS, BaulNavigationWindowSlotClass))
#define BAUL_NAVIGATION_WINDOW_SLOT(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_NAVIGATION_WINDOW_SLOT, BaulNavigationWindowSlot))
#define BAUL_IS_NAVIGATION_WINDOW_SLOT(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_NAVIGATION_WINDOW_SLOT))
#define BAUL_IS_NAVIGATION_WINDOW_SLOT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BAUL_TYPE_NAVIGATION_WINDOW_SLOT))
#define BAUL_NAVIGATION_WINDOW_SLOT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BAUL_TYPE_NAVIGATION_WINDOW_SLOT, BaulNavigationWindowSlotClass))

typedef enum
{
    BAUL_BAR_PATH,
    BAUL_BAR_NAVIGATION,
    BAUL_BAR_SEARCH
} BaulBarMode;

struct BaulNavigationWindowSlot
{
    BaulWindowSlot parent;

    BaulBarMode bar_mode;
    CtkTreeModel *viewer_model;
    int num_viewers;

    /* Back/Forward chain, and history list.
     * The data in these lists are BaulBookmark pointers.
     */
    GList *back_list, *forward_list;

    /* Current views stuff */
    GList *sidebar_panels;
};

struct BaulNavigationWindowSlotClass
{
    BaulWindowSlotClass parent;
};

GType baul_navigation_window_slot_get_type (void);

gboolean baul_navigation_window_slot_should_close_with_mount (BaulNavigationWindowSlot *slot,
        GMount *mount);

void baul_navigation_window_slot_clear_forward_list (BaulNavigationWindowSlot *slot);
void baul_navigation_window_slot_clear_back_list    (BaulNavigationWindowSlot *slot);

#endif /* BAUL_NAVIGATION_WINDOW_SLOT_H */
