/*
 * baul-freedesktop-dbus: Implementation for the org.freedesktop DBus file-management interfaces
 *
 * Baul is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Baul is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Authors: Akshay Gupta <kitallis@gmail.com>
 *          Federico Mena Quintero <federico@gnome.org>
 *          Stefano Karapetsas <stefano@karapetsas.com>
 */


#ifndef __BAUL_FREEDESKTOP_DBUS_H__
#define __BAUL_FREEDESKTOP_DBUS_H__

#include <glib-object.h>

#include "baul-application.h"

#define BAUL_FDO_DBUS_IFACE "org.freedesktop.FileManager1"
#define BAUL_FDO_DBUS_NAME  "org.freedesktop.FileManager1"
#define BAUL_FDO_DBUS_PATH  "/org/freedesktop/FileManager1"

typedef struct _BaulFreedesktopDBus BaulFreedesktopDBus;
typedef struct _BaulFreedesktopDBusClass BaulFreedesktopDBusClass;

GType baul_freedesktop_dbus_get_type (void);
BaulFreedesktopDBus * baul_freedesktop_dbus_new (BaulApplication *application);

#endif /* __BAUL_FREEDESKTOP_DBUS_H__ */
