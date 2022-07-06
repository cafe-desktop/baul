/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-view.c: Interface for baul views

   Copyright (C) 2004 Red Hat Inc.

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

   Author: Alexander Larsson <alexl@redhat.com>
*/

#include <config.h>
#include "baul-view.h"

enum
{
    TITLE_CHANGED,
    ZOOM_LEVEL_CHANGED,
    LAST_SIGNAL
};

static guint baul_view_signals[LAST_SIGNAL] = { 0 };

static void
baul_view_base_init (gpointer g_class)
{
    static gboolean initialized = FALSE;

    if (! initialized)
    {
        baul_view_signals[TITLE_CHANGED] =
            g_signal_new ("title_changed",
                          BAUL_TYPE_VIEW,
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (BaulViewIface, title_changed),
                          NULL, NULL,
                          g_cclosure_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

        baul_view_signals[ZOOM_LEVEL_CHANGED] =
            g_signal_new ("zoom_level_changed",
                          BAUL_TYPE_VIEW,
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (BaulViewIface, zoom_level_changed),
                          NULL, NULL,
                          g_cclosure_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

        initialized = TRUE;
    }
}

GType
baul_view_get_type (void)
{
    static GType type = 0;

    if (!type)
    {
        const GTypeInfo info =
        {
            sizeof (BaulViewIface),
            baul_view_base_init,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            NULL
        };

        type = g_type_register_static (G_TYPE_INTERFACE,
                                       "BaulView",
                                       &info, 0);
        g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }

    return type;
}

const char *
baul_view_get_view_id (BaulView *view)
{
    g_return_val_if_fail (BAUL_IS_VIEW (view), NULL);

    return (* BAUL_VIEW_GET_IFACE (view)->get_view_id) (view);
}

CtkWidget *
baul_view_get_widget (BaulView *view)
{
    g_return_val_if_fail (BAUL_IS_VIEW (view), NULL);

    return (* BAUL_VIEW_GET_IFACE (view)->get_widget) (view);
}

void
baul_view_load_location (BaulView *view,
                         const char   *location_uri)
{
    g_return_if_fail (BAUL_IS_VIEW (view));
    g_return_if_fail (location_uri != NULL);

    (* BAUL_VIEW_GET_IFACE (view)->load_location) (view,
            location_uri);
}

void
baul_view_stop_loading (BaulView *view)
{
    g_return_if_fail (BAUL_IS_VIEW (view));

    (* BAUL_VIEW_GET_IFACE (view)->stop_loading) (view);
}

int
baul_view_get_selection_count (BaulView *view)
{
    g_return_val_if_fail (BAUL_IS_VIEW (view), 0);

    return (* BAUL_VIEW_GET_IFACE (view)->get_selection_count) (view);
}

GList *
baul_view_get_selection (BaulView *view)
{
    g_return_val_if_fail (BAUL_IS_VIEW (view), NULL);

    return (* BAUL_VIEW_GET_IFACE (view)->get_selection) (view);
}

void
baul_view_set_selection (BaulView *view,
                         GList        *list)
{
    g_return_if_fail (BAUL_IS_VIEW (view));

    (* BAUL_VIEW_GET_IFACE (view)->set_selection) (view,
            list);
}

void
baul_view_set_is_active (BaulView *view,
                         gboolean is_active)
{
    g_return_if_fail (BAUL_IS_VIEW (view));

    (* BAUL_VIEW_GET_IFACE (view)->set_is_active) (view,
            is_active);
}

void
baul_view_invert_selection (BaulView *view)
{
    g_return_if_fail (BAUL_IS_VIEW (view));

    (* BAUL_VIEW_GET_IFACE (view)->invert_selection) (view);
}

char *
baul_view_get_first_visible_file (BaulView *view)
{
    g_return_val_if_fail (BAUL_IS_VIEW (view), NULL);

    return (* BAUL_VIEW_GET_IFACE (view)->get_first_visible_file) (view);
}

void
baul_view_scroll_to_file (BaulView *view,
                          const char   *uri)
{
    g_return_if_fail (BAUL_IS_VIEW (view));

    (* BAUL_VIEW_GET_IFACE (view)->scroll_to_file) (view, uri);
}

char *
baul_view_get_title (BaulView *view)
{
    g_return_val_if_fail (BAUL_IS_VIEW (view), NULL);

    if (BAUL_VIEW_GET_IFACE (view)->get_title != NULL)
    {
        return (* BAUL_VIEW_GET_IFACE (view)->get_title) (view);
    }
    else
    {
        return NULL;
    }
}


gboolean
baul_view_supports_zooming (BaulView *view)
{
    g_return_val_if_fail (BAUL_IS_VIEW (view), FALSE);

    return (* BAUL_VIEW_GET_IFACE (view)->supports_zooming) (view);
}

void
baul_view_bump_zoom_level (BaulView *view,
                           int zoom_increment)
{
    g_return_if_fail (BAUL_IS_VIEW (view));

    (* BAUL_VIEW_GET_IFACE (view)->bump_zoom_level) (view,
            zoom_increment);
}

void
baul_view_zoom_to_level (BaulView      *view,
                         BaulZoomLevel  level)
{
    g_return_if_fail (BAUL_IS_VIEW (view));

    (* BAUL_VIEW_GET_IFACE (view)->zoom_to_level) (view,
            level);
}

void
baul_view_restore_default_zoom_level (BaulView *view)
{
    g_return_if_fail (BAUL_IS_VIEW (view));

    (* BAUL_VIEW_GET_IFACE (view)->restore_default_zoom_level) (view);
}

gboolean
baul_view_can_zoom_in (BaulView *view)
{
    g_return_val_if_fail (BAUL_IS_VIEW (view), FALSE);

    return (* BAUL_VIEW_GET_IFACE (view)->can_zoom_in) (view);
}

gboolean
baul_view_can_zoom_out (BaulView *view)
{
    g_return_val_if_fail (BAUL_IS_VIEW (view), FALSE);

    return (* BAUL_VIEW_GET_IFACE (view)->can_zoom_out) (view);
}

BaulZoomLevel
baul_view_get_zoom_level (BaulView *view)
{
    g_return_val_if_fail (BAUL_IS_VIEW (view), BAUL_ZOOM_LEVEL_STANDARD);

    return (* BAUL_VIEW_GET_IFACE (view)->get_zoom_level) (view);
}

void
baul_view_grab_focus (BaulView   *view)
{
    g_return_if_fail (BAUL_IS_VIEW (view));

    if (BAUL_VIEW_GET_IFACE (view)->grab_focus != NULL)
    {
        (* BAUL_VIEW_GET_IFACE (view)->grab_focus) (view);
    }
}

void
baul_view_update_menus (BaulView *view)
{
    g_return_if_fail (BAUL_IS_VIEW (view));

    if (BAUL_VIEW_GET_IFACE (view)->update_menus != NULL)
    {
        (* BAUL_VIEW_GET_IFACE (view)->update_menus) (view);
    }
}

void
baul_view_pop_up_location_context_menu (BaulView   *view,
                                        CdkEventButton *event,
                                        const char     *location)
{
    g_return_if_fail (BAUL_IS_VIEW (view));

    if (BAUL_VIEW_GET_IFACE (view)->pop_up_location_context_menu != NULL)
    {
        (* BAUL_VIEW_GET_IFACE (view)->pop_up_location_context_menu) (view, event, location);
    }
}

void
baul_view_drop_proxy_received_uris   (BaulView         *view,
                                      GList                *uris,
                                      const char           *target_location,
                                      CdkDragAction         action)
{
    g_return_if_fail (BAUL_IS_VIEW (view));

    if (BAUL_VIEW_GET_IFACE (view)->drop_proxy_received_uris != NULL)
    {
        (* BAUL_VIEW_GET_IFACE (view)->drop_proxy_received_uris) (view, uris, target_location, action);
    }
}

void
baul_view_drop_proxy_received_netscape_url (BaulView         *view,
        const char           *source_url,
        const char           *target_location,
        CdkDragAction         action)
{
    g_return_if_fail (BAUL_IS_VIEW (view));

    if (BAUL_VIEW_GET_IFACE (view)->drop_proxy_received_netscape_url != NULL)
    {
        (* BAUL_VIEW_GET_IFACE (view)->drop_proxy_received_netscape_url) (view, source_url, target_location, action);
    }
}


