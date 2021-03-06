/*
 *  baul-extension.c - extension management functions
 *
 *  Copyright (C) 2014 CAFE Desktop.
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
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 *  Author: Alexander van der Meij <alexandervdm@gliese.me>
 */

#ifndef BAUL_EXTENSIONS_H
#define BAUL_EXTENSIONS_H

#include <glib.h>
#include <glib-object.h>

typedef struct _Extension {
    gchar *filename;
    gchar *name;
    gchar *description;
    gchar *icon;
    gchar **author;
    gchar *copyright;
    gchar *version;
    gchar *website;
    gboolean state;
    GObject *module;
} Extension;

#define EXTENSION(x) ((Extension*)x)

void baul_extension_register (gchar *filename, GObject *module);

GList *baul_extensions_get_list (void);
GList *baul_extensions_get_for_type (GType type);

gboolean baul_extension_set_state (Extension *ext, gboolean new_state);

#endif // BAUL_EXTENSIONS_H

