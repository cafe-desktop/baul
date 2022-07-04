/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-sidebar.h: Interface for baul sidebar plugins

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

#ifndef BAUL_SIDEBAR_H
#define BAUL_SIDEBAR_H

#include <glib-object.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BAUL_TYPE_SIDEBAR           (baul_sidebar_get_type ())
#define BAUL_SIDEBAR(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_SIDEBAR, BaulSidebar))
#define BAUL_IS_SIDEBAR(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_SIDEBAR))
#define BAUL_SIDEBAR_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAUL_TYPE_SIDEBAR, BaulSidebarIface))

    typedef struct _BaulSidebar BaulSidebar; /* dummy typedef */
    typedef struct _BaulSidebarIface BaulSidebarIface;

    /* Must also be a GtkWidget */
    struct _BaulSidebarIface
    {
        GTypeInterface g_iface;

        /* Signals: */
        void           (* tab_icon_changed)       (BaulSidebar *sidebar);

        /* VTable: */
        const char *   (* get_sidebar_id)         (BaulSidebar *sidebar);
        char *         (* get_tab_label)          (BaulSidebar *sidebar);
        char *         (* get_tab_tooltip)        (BaulSidebar *sidebar);
        GdkPixbuf *    (* get_tab_icon)           (BaulSidebar *sidebar);
        void           (* is_visible_changed)     (BaulSidebar *sidebar,
                gboolean         is_visible);


        /* Padding for future expansion */
        void (*_reserved1) (void);
        void (*_reserved2) (void);
        void (*_reserved3) (void);
        void (*_reserved4) (void);
        void (*_reserved5) (void);
        void (*_reserved6) (void);
        void (*_reserved7) (void);
        void (*_reserved8) (void);
    };

    GType             baul_sidebar_get_type             (void);

    const char *baul_sidebar_get_sidebar_id     (BaulSidebar *sidebar);
    char *      baul_sidebar_get_tab_label      (BaulSidebar *sidebar);
    char *      baul_sidebar_get_tab_tooltip    (BaulSidebar *sidebar);
    GdkPixbuf * baul_sidebar_get_tab_icon       (BaulSidebar *sidebar);
    void        baul_sidebar_is_visible_changed (BaulSidebar *sidebar,
            gboolean         is_visible);

#ifdef __cplusplus
}
#endif

#endif /* BAUL_VIEW_H */
