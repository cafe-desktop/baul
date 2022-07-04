/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-window-slot-info.c: Interface for baul window slots

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
#include "baul-window-slot-info.h"

enum
{
    ACTIVE,
    INACTIVE,
    LAST_SIGNAL
};

static guint baul_window_slot_info_signals[LAST_SIGNAL] = { 0 };

static void
baul_window_slot_info_base_init (gpointer g_class)
{
    static gboolean initialized = FALSE;

    if (!initialized)
    {
        baul_window_slot_info_signals[ACTIVE] =
            g_signal_new ("active",
                          BAUL_TYPE_WINDOW_SLOT_INFO,
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (BaulWindowSlotInfoIface, active),
                          NULL, NULL,
                          g_cclosure_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

        baul_window_slot_info_signals[INACTIVE] =
            g_signal_new ("inactive",
                          BAUL_TYPE_WINDOW_SLOT_INFO,
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (BaulWindowSlotInfoIface, inactive),
                          NULL, NULL,
                          g_cclosure_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

        initialized = TRUE;
    }
}

GType
baul_window_slot_info_get_type (void)
{
    static GType type = 0;

    if (!type)
    {
        const GTypeInfo info =
        {
            sizeof (BaulWindowSlotInfoIface),
            baul_window_slot_info_base_init,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            NULL
        };

        type = g_type_register_static (G_TYPE_INTERFACE,
                                       "BaulWindowSlotInfo",
                                       &info, 0);
        g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }

    return type;
}

void
baul_window_slot_info_set_status (BaulWindowSlotInfo *slot,
                                  const char             *status)
{
    g_assert (BAUL_IS_WINDOW_SLOT_INFO (slot));

    (* BAUL_WINDOW_SLOT_INFO_GET_IFACE (slot)->set_status) (slot,
            status);
}

void
baul_window_slot_info_make_hosting_pane_active (BaulWindowSlotInfo *slot)
{
    g_assert (BAUL_IS_WINDOW_SLOT_INFO (slot));
    (* BAUL_WINDOW_SLOT_INFO_GET_IFACE (slot)->make_hosting_pane_active) (slot);
}

void
baul_window_slot_info_open_location_full (BaulWindowSlotInfo  *slot,
                                     GFile                   *location,
                                     BaulWindowOpenMode       mode,
                                     BaulWindowOpenFlags      flags,
                                     GList                   *selection,
                                     BaulWindowGoToCallback   callback,
                                     gpointer user_data)
{
    g_assert (BAUL_IS_WINDOW_SLOT_INFO (slot));

    (* BAUL_WINDOW_SLOT_INFO_GET_IFACE (slot)->open_location) (slot,
            location,
            mode,
            flags,
            selection,
            callback,
            user_data);
}

char *
baul_window_slot_info_get_title (BaulWindowSlotInfo *slot)
{
    g_assert (BAUL_IS_WINDOW_SLOT_INFO (slot));

    return (* BAUL_WINDOW_SLOT_INFO_GET_IFACE (slot)->get_title) (slot);
}

char *
baul_window_slot_info_get_current_location (BaulWindowSlotInfo *slot)
{
    g_assert (BAUL_IS_WINDOW_SLOT_INFO (slot));

    return (* BAUL_WINDOW_SLOT_INFO_GET_IFACE (slot)->get_current_location) (slot);
}

BaulView *
baul_window_slot_info_get_current_view (BaulWindowSlotInfo *slot)
{
    g_assert (BAUL_IS_WINDOW_SLOT_INFO (slot));

    return (* BAUL_WINDOW_SLOT_INFO_GET_IFACE (slot)->get_current_view) (slot);
}

BaulWindowInfo *
baul_window_slot_info_get_window (BaulWindowSlotInfo *slot)
{
    g_assert (BAUL_IS_WINDOW_SLOT_INFO (slot));

    return (* BAUL_WINDOW_SLOT_INFO_GET_IFACE (slot)->get_window) (slot);
}

