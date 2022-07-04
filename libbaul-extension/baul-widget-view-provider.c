/* vi: set sw=4 ts=4 wrap ai: */
/*
 * baul-widget-view-provider.c: This file is part of baul.
 *
 * Copyright (C) 2019 Wu Xiaotian <yetist@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * */

#include <config.h>
#include "baul-widget-view-provider.h"

#include <glib-object.h>

/**
 * SECTION:baul-widget-view-provider
 * @title: CajaWidgetViewProvider
 * @short_description: Interface to provide widgets view.
 * @include: libbaul-extension/baul-widget-view-provider.h
 *
 * #CajaWidgetViewProvider allows extension to provide widgets view
 * in the file manager.
 */

static void
baul_widget_view_provider_base_init (gpointer g_class)
{
}

GType
baul_widget_view_provider_get_type (void)
{
    static GType type = 0;

    if (!type) {
        const GTypeInfo info = {
            sizeof (CajaWidgetViewProviderIface),
            baul_widget_view_provider_base_init,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            NULL
        };

        type = g_type_register_static (G_TYPE_INTERFACE,
                                       "CajaWidgetViewProvider",
                                       &info, 0);
        g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }

    return type;
}

/**
 * baul_widget_view_provider_get_widget:
 * @provider: a #CajaWidgetViewProvider
 *
 * Return a #GtkWidget to show the current location content.
 *
 * Returns: The #GtkWidget used to show the contents.
 **/
GtkWidget *
baul_widget_view_provider_get_widget (CajaWidgetViewProvider *provider)
{
    g_return_val_if_fail (CAJA_IS_WIDGET_VIEW_PROVIDER (provider), NULL);
    g_return_val_if_fail (CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->get_widget != NULL, NULL);

    return CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->get_widget (provider);
}

/**
 * baul_widget_view_provider_add_file:
 * @provider: a #CajaWidgetViewProvider
 * @file: add a #CajaFile into the widget view.
 * @directory: the directory of the file.
 *
 * Add a file of this location into the widget view.
 **/
void baul_widget_view_provider_add_file (CajaWidgetViewProvider *provider, CajaFile *file, CajaFile *directory)
{
    g_return_if_fail (CAJA_IS_WIDGET_VIEW_PROVIDER (provider));
    g_return_if_fail (CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->add_file != NULL);

    CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->add_file (provider, file, directory);
}

/**
 * baul_widget_view_provider_set_location:
 * @provider: a #CajaWidgetViewProvider
 * @uri: the URI of the location
 *
 * Set the location of this #CajaWidgetViewProvider.
 **/
void baul_widget_view_provider_set_location (CajaWidgetViewProvider *provider, const char *location)
{
    g_return_if_fail (CAJA_IS_WIDGET_VIEW_PROVIDER (provider));
    g_return_if_fail (CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->set_location != NULL);

    CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->set_location (provider, location);
}

/**
 * baul_widget_view_provider_set_window:
 * @provider: a #CajaWidgetViewProvider
 * @window: parent #GtkWindow
 *
 * Set parent #GtkWindow of this #CajaWidgetViewProvider.
 **/
void baul_widget_view_provider_set_window (CajaWidgetViewProvider *provider, GtkWindow *window)
{
    g_return_if_fail (CAJA_IS_WIDGET_VIEW_PROVIDER (provider));
    g_return_if_fail (CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->set_window != NULL);

    CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->set_window (provider, window);
}

/**
 * baul_widget_view_provider_get_item_count:
 * @provider: a #CajaWidgetViewProvider
 *
 * Return value: The item count of this #CajaWidgetViewProvider
 **/
guint baul_widget_view_provider_get_item_count (CajaWidgetViewProvider *provider)
{
    g_return_val_if_fail (CAJA_IS_WIDGET_VIEW_PROVIDER (provider), 0);
    g_return_val_if_fail (CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->get_item_count != NULL, 0);

    return CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->get_item_count (provider);
}

/**
 * baul_widget_view_provider_get_first_visible_file:
 * @provider: a #CajaWidgetViewProvider
 *
 * Return the first visible file. When use start visit the location, the baul's status is waiting, until
 * get the first visible file.
 *
 * Return value: the first visible file.
 **/
gchar* baul_widget_view_provider_get_first_visible_file (CajaWidgetViewProvider *provider)
{
    g_return_val_if_fail (CAJA_IS_WIDGET_VIEW_PROVIDER (provider), NULL);
    g_return_val_if_fail (CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->get_first_visible_file != NULL, NULL);

    return CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->get_first_visible_file (provider);
}

/**
 * baul_widget_view_provider_clear:
 * @provider: a #CajaWidgetViewProvider
 *
 * Clear the content of this widget view.
 **/
void baul_widget_view_provider_clear (CajaWidgetViewProvider *provider)
{
    g_return_if_fail (CAJA_IS_WIDGET_VIEW_PROVIDER (provider));
    g_return_if_fail (CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->clear != NULL);

    CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->clear (provider);
}

/**
 * baul_widget_view_provider_supports_uri:
 * @provider: a #CajaWidgetViewProvider
 * @uri: the location to visit.
 * @file_type: The #GFileType for the uri
 * @mime_type: The mimetype for the uri
 *
 * Whether this widget view works for the uri.
 *
 * Return value: True to use custom widget view, False to ignore, and baul use normal view.
 **/
gboolean baul_widget_view_provider_supports_uri (CajaWidgetViewProvider *provider,
                                                 const char *uri,
                                                 GFileType file_type,
                                                 const char *mime_type)
{
    g_return_val_if_fail (CAJA_IS_WIDGET_VIEW_PROVIDER (provider), FALSE);
    g_return_val_if_fail (CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->supports_uri!= NULL, FALSE);

    return CAJA_WIDGET_VIEW_PROVIDER_GET_IFACE (provider)->supports_uri (provider, uri, file_type, mime_type);
}
