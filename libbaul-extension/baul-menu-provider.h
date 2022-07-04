/*
 *  baul-menu-provider.h - Interface for Baul extensions that
 *                             provide context menu items.
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

/* This interface is implemented by Baul extensions that want to
 * add context menu entries to files.  Extensions are called when
 * Baul constructs the context menu for a file.  They are passed a
 * list of BaulFileInfo objects which holds the current selection */

#ifndef BAUL_MENU_PROVIDER_H
#define BAUL_MENU_PROVIDER_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "baul-extension-types.h"
#include "baul-file-info.h"
#include "baul-menu.h"

G_BEGIN_DECLS

#define BAUL_TYPE_MENU_PROVIDER           (baul_menu_provider_get_type ())
#define BAUL_MENU_PROVIDER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_MENU_PROVIDER, BaulMenuProvider))
#define BAUL_IS_MENU_PROVIDER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_MENU_PROVIDER))
#define BAUL_MENU_PROVIDER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAUL_TYPE_MENU_PROVIDER, BaulMenuProviderIface))

typedef struct _BaulMenuProvider       BaulMenuProvider;
typedef struct _BaulMenuProviderIface  BaulMenuProviderIface;

/**
 * BaulMenuProviderIface:
 * @g_iface: The parent interface.
 * @get_file_items: Returns a #GList of #BaulMenuItem.
 *   See baul_menu_provider_get_file_items() for details.
 * @get_background_items: Returns a #GList of #BaulMenuItem.
 *   See baul_menu_provider_get_background_items() for details.
 * @get_toolbar_items: Returns a #GList of #BaulMenuItem.
 *   See baul_menu_provider_get_toolbar_items() for details.
 *
 * Interface for extensions to provide additional menu items.
 */

struct _BaulMenuProviderIface {
    GTypeInterface g_iface;

    GList *(*get_file_items)       (BaulMenuProvider *provider,
                                    GtkWidget        *window,
                                    GList            *files);
    GList *(*get_background_items) (BaulMenuProvider *provider,
                                    GtkWidget        *window,
                                    BaulFileInfo     *current_folder);
    GList *(*get_toolbar_items)    (BaulMenuProvider *provider,
                                    GtkWidget        *window,
                                    BaulFileInfo     *current_folder);
};

/* Interface Functions */
GType  baul_menu_provider_get_type             (void);
GList *baul_menu_provider_get_file_items       (BaulMenuProvider *provider,
                                                GtkWidget        *window,
                                                GList            *files);
GList *baul_menu_provider_get_background_items (BaulMenuProvider *provider,
                                                GtkWidget        *window,
                                                BaulFileInfo     *current_folder);
GList *baul_menu_provider_get_toolbar_items    (BaulMenuProvider *provider,
                                                GtkWidget        *window,
                                                BaulFileInfo     *current_folder);

/* This function emit a signal to inform baul that its item list has changed. */
void   baul_menu_provider_emit_items_updated_signal (BaulMenuProvider *provider);

G_END_DECLS

#endif
