/*
 *  baul-property-page-provider.c - Interface for Baul extensions
 *                                      that provide property pages for
 *                                      files.
 *
 *  Copyright (C) 2003 Novell, Inc.
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
 *
 */

#include <config.h>
#include "baul-property-page-provider.h"

#include <glib-object.h>

/**
 * SECTION:baul-property-page-provider
 * @title: BaulPropertyPageProvider
 * @short_description: Interface to provide additional property pages
 * @include: libbaul-extension/baul-property-page-provider.h
 *
 * #BaulPropertyPageProvider allows extension to provide additional pages
 * for the file properties dialog.
 */

static void
baul_property_page_provider_base_init (gpointer g_class G_GNUC_UNUSED)
{
}

GType
baul_property_page_provider_get_type (void)
{
    static GType type = 0;

    if (!type) {
        const GTypeInfo info = {
            sizeof (BaulPropertyPageProviderIface),
            baul_property_page_provider_base_init,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            NULL
        };

        type = g_type_register_static (G_TYPE_INTERFACE,
                                       "BaulPropertyPageProvider",
                                       &info, 0);
        g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }

    return type;
}

/**
 * baul_property_page_provider_get_pages:
 * @provider: a #BaulPropertyPageProvider
 * @files: (element-type BaulFileInfo): a #GList of #BaulFileInfo
 *
 * This function is called by Baul when it wants property page
 * items from the extension.
 *
 * This function is called in the main thread before a property page
 * is shown, so it should return quickly.
 *
 * Returns: (element-type BaulPropertyPage) (transfer full): A #GList of allocated #BaulPropertyPage items.
 */
GList *
baul_property_page_provider_get_pages (BaulPropertyPageProvider *provider,
                                       GList *files)
{
    g_return_val_if_fail (BAUL_IS_PROPERTY_PAGE_PROVIDER (provider), NULL);
    g_return_val_if_fail (BAUL_PROPERTY_PAGE_PROVIDER_GET_IFACE (provider)->get_pages != NULL, NULL);

    return BAUL_PROPERTY_PAGE_PROVIDER_GET_IFACE (provider)->get_pages
           (provider, files);
}


