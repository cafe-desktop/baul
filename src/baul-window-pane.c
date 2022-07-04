/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-window-pane.c: Caja window pane

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

#include <eel/eel-gtk-macros.h>

#include "baul-window-pane.h"
#include "baul-window-private.h"
#include "baul-navigation-window-pane.h"
#include "baul-window-manage-views.h"

static void baul_window_pane_dispose    (GObject *object);

G_DEFINE_TYPE (CajaWindowPane,
               baul_window_pane,
               G_TYPE_OBJECT)
#define parent_class baul_window_pane_parent_class


static inline CajaWindowSlot *
get_first_inactive_slot (CajaWindowPane *pane)
{
    GList *l;
    CajaWindowSlot *slot = NULL;

    for (l = pane->slots; l != NULL; l = l->next)
    {
        slot = BAUL_WINDOW_SLOT (l->data);
        if (slot != pane->active_slot)
        {
            return slot;
        }
    }

    return NULL;
}

void
baul_window_pane_show (CajaWindowPane *pane)
{
    pane->visible = TRUE;
    EEL_CALL_METHOD (BAUL_WINDOW_PANE_CLASS, pane,
                     show, (pane));
}

void
baul_window_pane_zoom_in (CajaWindowPane *pane)
{
    CajaWindowSlot *slot;

    g_assert (pane != NULL);

    baul_window_set_active_pane (pane->window, pane);

    slot = pane->active_slot;
    if (slot->content_view != NULL)
    {
        baul_view_bump_zoom_level (slot->content_view, 1);
    }
}

void
baul_window_pane_zoom_to_level (CajaWindowPane *pane,
                                CajaZoomLevel level)
{
    CajaWindowSlot *slot;

    g_assert (pane != NULL);

    baul_window_set_active_pane (pane->window, pane);

    slot = pane->active_slot;
    if (slot->content_view != NULL)
    {
        baul_view_zoom_to_level (slot->content_view, level);
    }
}

void
baul_window_pane_zoom_out (CajaWindowPane *pane)
{
    CajaWindowSlot *slot;

    g_assert (pane != NULL);

    baul_window_set_active_pane (pane->window, pane);

    slot = pane->active_slot;
    if (slot->content_view != NULL)
    {
        baul_view_bump_zoom_level (slot->content_view, -1);
    }
}

void
baul_window_pane_zoom_to_default (CajaWindowPane *pane)
{
    CajaWindowSlot *slot;

    g_assert (pane != NULL);

    baul_window_set_active_pane (pane->window, pane);

    slot = pane->active_slot;
    if (slot->content_view != NULL)
    {
        baul_view_restore_default_zoom_level (slot->content_view);
    }
}

void
baul_window_pane_slot_close (CajaWindowPane *pane, CajaWindowSlot *slot)
{
    if (pane->window)
    {
        CajaWindow *window;
        window = pane->window;
        if (pane->active_slot == slot)
        {
            g_assert (pane->active_slots != NULL);
            g_assert (pane->active_slots->data == slot);

            CajaWindowSlot *next_slot;

            next_slot = NULL;
            if (pane->active_slots->next != NULL)
            {
                next_slot = BAUL_WINDOW_SLOT (pane->active_slots->next->data);
            }

            if (next_slot == NULL)
            {
                next_slot = get_first_inactive_slot (BAUL_WINDOW_PANE (pane));
            }

            baul_window_set_active_slot (window, next_slot);
        }
        baul_window_close_slot (slot);

        /* If that was the last slot in the active pane, close the pane or even the whole window. */
        if (window->details->active_pane->slots == NULL)
        {
            CajaWindowPane *next_pane;
            next_pane = baul_window_get_next_pane (window);

            /* If next_pane is non-NULL, we have more than one pane available. In this
             * case, close the current pane and switch to the next one. If there is
             * no next pane, close the window. */
            if(next_pane)
            {
                baul_window_close_pane (pane);
                baul_window_pane_switch_to (next_pane);
                if (BAUL_IS_NAVIGATION_WINDOW (window))
                {
                    baul_navigation_window_update_show_hide_menu_items (BAUL_NAVIGATION_WINDOW (window));
                }
            }
            else
            {
                baul_window_close (window);
            }
        }
    }
}

static void
real_sync_location_widgets (CajaWindowPane *pane)
{
    CajaWindowSlot *slot;

    /* TODO: Would be nice with a real subclass for spatial panes */
    g_assert (BAUL_IS_SPATIAL_WINDOW (pane->window));

    slot = pane->active_slot;

    /* Change the location button to match the current location. */
    baul_spatial_window_set_location_button (BAUL_SPATIAL_WINDOW (pane->window),
            slot->location);
}


void
baul_window_pane_sync_location_widgets (CajaWindowPane *pane)
{
    EEL_CALL_METHOD (BAUL_WINDOW_PANE_CLASS, pane,
                     sync_location_widgets, (pane));
}

void
baul_window_pane_sync_search_widgets (CajaWindowPane *pane)
{
    g_assert (BAUL_IS_WINDOW_PANE (pane));

    EEL_CALL_METHOD (BAUL_WINDOW_PANE_CLASS, pane,
                     sync_search_widgets, (pane));
}

void
baul_window_pane_grab_focus (CajaWindowPane *pane)
{
    if (BAUL_IS_WINDOW_PANE (pane) && pane->active_slot)
    {
        baul_view_grab_focus (pane->active_slot->content_view);
    }
}

void
baul_window_pane_switch_to (CajaWindowPane *pane)
{
    baul_window_pane_grab_focus (pane);
}

static void
baul_window_pane_init (CajaWindowPane *pane)
{
    pane->slots = NULL;
    pane->active_slots = NULL;
    pane->active_slot = NULL;
    pane->is_active = FALSE;
}

void
baul_window_pane_set_active (CajaWindowPane *pane, gboolean is_active)
{
    if (is_active == pane->is_active)
    {
        return;
    }

    pane->is_active = is_active;

    /* notify the current slot about its activity state (so that it can e.g. modify the bg color) */
    baul_window_slot_is_in_active_pane (pane->active_slot, is_active);

    EEL_CALL_METHOD (BAUL_WINDOW_PANE_CLASS, pane,
                     set_active, (pane, is_active));
}

static void
baul_window_pane_class_init (CajaWindowPaneClass *class)
{
    G_OBJECT_CLASS (class)->dispose = baul_window_pane_dispose;
    BAUL_WINDOW_PANE_CLASS (class)->sync_location_widgets = real_sync_location_widgets;
}

static void
baul_window_pane_dispose (GObject *object)
{
    CajaWindowPane *pane = BAUL_WINDOW_PANE (object);

    g_assert (pane->slots == NULL);

    pane->window = NULL;
    G_OBJECT_CLASS (parent_class)->dispose (object);
}

CajaWindowPane *
baul_window_pane_new (CajaWindow *window)
{
    CajaWindowPane *pane;

    pane = g_object_new (BAUL_TYPE_WINDOW_PANE, NULL);
    pane->window = window;
    return pane;
}

CajaWindowSlot *
baul_window_pane_get_slot_for_content_box (CajaWindowPane *pane,
        GtkWidget *content_box)
{
    GList *l;
    CajaWindowSlot *slot = NULL;

    for (l = pane->slots; l != NULL; l = l->next)
    {
        slot = BAUL_WINDOW_SLOT (l->data);

        if (slot->content_box == content_box)
        {
            return slot;
        }
    }
    return NULL;
}
