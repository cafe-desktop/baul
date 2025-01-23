/*
 *  baul-location-widget-provider.c - Interface for Baul
                 extensions that provide extra widgets for a location
 *
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
 *  Author:  Alexander Larsson <alexl@redhat.com>
 *
 */

#include <config.h>
#include "baul-location-widget-provider.h"

#include <glib-object.h>

/**
 * SECTION:baul-location-widget-provider
 * @title: BaulLocationWidgetProvider
 * @short_description: Interface to provide additional location widgets
 * @include: libbaul-extension/baul-location-widget-provider.h
 *
 * #BaulLocationWidgetProvider allows extension to provide additional location
 * widgets in the file manager views.
 */

static void
baul_location_widget_provider_base_init (gpointer g_class G_GNUC_UNUSED)
{
}

GType
baul_location_widget_provider_get_type (void)
{
    static GType type = 0;

    if (!type) {
        const GTypeInfo info = {
            sizeof (BaulLocationWidgetProviderIface),
            baul_location_widget_provider_base_init,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            NULL
        };

        type = g_type_register_static (G_TYPE_INTERFACE,
                                       "BaulLocationWidgetProvider",
                                       &info, 0);
        g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }

    return type;
}

/**
 * baul_location_widget_provider_get_widget:
 * @provider: a #BaulLocationWidgetProvider
 * @uri: the URI of the location
 * @window: parent #CtkWindow
 *
 * Returns: (transfer none): the location widget for @provider at @uri
 */
CtkWidget *
baul_location_widget_provider_get_widget (BaulLocationWidgetProvider *provider,
                                          const char                 *uri,
                                          CtkWidget                  *window)
{
    g_return_val_if_fail (BAUL_IS_LOCATION_WIDGET_PROVIDER (provider), NULL);
    g_return_val_if_fail (BAUL_LOCATION_WIDGET_PROVIDER_GET_IFACE (provider)->get_widget != NULL, NULL);

    return BAUL_LOCATION_WIDGET_PROVIDER_GET_IFACE (provider)->get_widget
           (provider, uri, window);

}
