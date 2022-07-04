/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-sidebar-provider.h: register and create BaulSidebars

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

#ifndef BAUL_SIDEBAR_PROVIDER_H
#define BAUL_SIDEBAR_PROVIDER_H

#include "baul-sidebar.h"
#include "baul-window-info.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BAUL_TYPE_SIDEBAR_PROVIDER           (baul_sidebar_provider_get_type ())
#define BAUL_SIDEBAR_PROVIDER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_SIDEBAR_PROVIDER, BaulSidebarProvider))
#define BAUL_IS_SIDEBAR_PROVIDER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_SIDEBAR_PROVIDER))
#define BAUL_SIDEBAR_PROVIDER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAUL_TYPE_SIDEBAR_PROVIDER, BaulSidebarProviderIface))

    typedef struct _BaulSidebarProvider       BaulSidebarProvider;
    typedef struct _BaulSidebarProviderIface  BaulSidebarProviderIface;

    struct _BaulSidebarProviderIface
    {
        GTypeInterface g_iface;

        BaulSidebar * (*create) (BaulSidebarProvider *provider,
                                 BaulWindowInfo *window);
    };

    /* Interface Functions */
    GType                   baul_sidebar_provider_get_type  (void);
    BaulSidebar *       baul_sidebar_provider_create (BaulSidebarProvider *provider,
            BaulWindowInfo  *window);
    GList *                 baul_list_sidebar_providers (void);

#ifdef __cplusplus
}
#endif

#endif /* BAUL_SIDEBAR_PROVIDER_H */
