/*
 *  baul-info-provider.h - Interface for Baul extensions that
 *                             provide info about files.
 *
 *  Copyright (C) 2003 Novell, Inc.
 *  Copyright (C) 2005 Red Hat, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Author:  Dave Camp <dave@ximian.com>
 *           Alexander Larsson <alexl@redhat.com>
 *
 */

/* This interface is implemented by Baul extensions that want to
 * provide extra location widgets for a particular location.
 * Extensions are called when Baul displays a location.
 */

#ifndef BAUL_LOCATION_WIDGET_PROVIDER_H
#define BAUL_LOCATION_WIDGET_PROVIDER_H

#include <glib-object.h>
#include <ctk/ctk.h>
#include "baul-extension-types.h"

G_BEGIN_DECLS

#define BAUL_TYPE_LOCATION_WIDGET_PROVIDER           (baul_location_widget_provider_get_type ())
#define BAUL_LOCATION_WIDGET_PROVIDER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_LOCATION_WIDGET_PROVIDER, BaulLocationWidgetProvider))
#define BAUL_IS_LOCATION_WIDGET_PROVIDER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_LOCATION_WIDGET_PROVIDER))
#define BAUL_LOCATION_WIDGET_PROVIDER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAUL_TYPE_LOCATION_WIDGET_PROVIDER, BaulLocationWidgetProviderIface))

typedef struct _BaulLocationWidgetProvider       BaulLocationWidgetProvider;
typedef struct _BaulLocationWidgetProviderIface  BaulLocationWidgetProviderIface;

/**
 * BaulLocationWidgetProviderIface:
 * @g_iface: The parent interface.
 * @get_widget: Returns a #CtkWidget.
 *   See baul_location_widget_provider_get_widget() for details.
 *
 * Interface for extensions to provide additional location widgets.
 */
struct _BaulLocationWidgetProviderIface {
    GTypeInterface g_iface;

    CtkWidget *(*get_widget) (BaulLocationWidgetProvider *provider,
                              const char                 *uri,
                              CtkWidget                  *window);
};

/* Interface Functions */
GType      baul_location_widget_provider_get_type   (void);
CtkWidget *baul_location_widget_provider_get_widget (BaulLocationWidgetProvider *provider,
                                                     const char                 *uri,
                                                     CtkWidget                  *window);
G_END_DECLS

#endif
