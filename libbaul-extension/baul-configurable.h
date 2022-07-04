/*
 *  baul-configurable.c - Interface for configuration
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
 *  Author: 20kdc <gamemanj@hotmail.co.uk>
 *  Based on baul-menu-provider.h by Dave Camp <dave@ximian.com>
 *
 */

/* This interface is implemented by Baul extensions that want to
 * have configuration screens (this is particularly important for open-terminal)
 */

#ifndef BAUL_CONFIGURABLE_H
#define BAUL_CONFIGURABLE_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "baul-extension-types.h"
#include "baul-file-info.h"
#include "baul-menu.h"

G_BEGIN_DECLS

#define BAUL_TYPE_CONFIGURABLE           (baul_configurable_get_type ())
#define BAUL_CONFIGURABLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_CONFIGURABLE, BaulConfigurable))
#define BAUL_IS_CONFIGURABLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_CONFIGURABLE))
#define BAUL_CONFIGURABLE_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAUL_TYPE_CONFIGURABLE, BaulConfigurableIface))

typedef struct _BaulConfigurable       BaulConfigurable;
typedef struct _BaulConfigurableIface  BaulConfigurableIface;

/**
 * BaulConfigurableIface:
 * @g_iface: The parent interface.
 * @run: Starts the configuration panel (should use g_dialog_run)
 *
 * Interface for extensions to provide additional menu items.
 */

struct _BaulConfigurableIface {
    GTypeInterface g_iface;

    void (*run_config) (BaulConfigurable *provider);
};

/* Interface Functions */
GType baul_configurable_get_type   (void);
void  baul_configurable_run_config (BaulConfigurable *provider);

G_END_DECLS

#endif
