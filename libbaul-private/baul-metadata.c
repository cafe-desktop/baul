/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-*/

/* baul-metadata.c - metadata utils
 *
 * Copyright (C) 2009 Red Hatl, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include "baul-metadata.h"
#include <glib.h>

static char *used_metadata_names[] =
{
    BAUL_METADATA_KEY_DEFAULT_VIEW,
    BAUL_METADATA_KEY_LOCATION_BACKGROUND_COLOR,
    BAUL_METADATA_KEY_LOCATION_BACKGROUND_IMAGE,
    BAUL_METADATA_KEY_ICON_VIEW_ZOOM_LEVEL,
    BAUL_METADATA_KEY_ICON_VIEW_AUTO_LAYOUT,
    BAUL_METADATA_KEY_ICON_VIEW_TIGHTER_LAYOUT,
    BAUL_METADATA_KEY_ICON_VIEW_SORT_BY,
    BAUL_METADATA_KEY_ICON_VIEW_SORT_REVERSED,
    BAUL_METADATA_KEY_ICON_VIEW_KEEP_ALIGNED,
    BAUL_METADATA_KEY_ICON_VIEW_LAYOUT_TIMESTAMP,
    BAUL_METADATA_KEY_LIST_VIEW_ZOOM_LEVEL,
    BAUL_METADATA_KEY_LIST_VIEW_SORT_COLUMN,
    BAUL_METADATA_KEY_LIST_VIEW_SORT_REVERSED,
    BAUL_METADATA_KEY_LIST_VIEW_VISIBLE_COLUMNS,
    BAUL_METADATA_KEY_LIST_VIEW_COLUMN_ORDER,
    BAUL_METADATA_KEY_COMPACT_VIEW_ZOOM_LEVEL,
    BAUL_METADATA_KEY_WINDOW_GEOMETRY,
    BAUL_METADATA_KEY_WINDOW_SCROLL_POSITION,
    BAUL_METADATA_KEY_WINDOW_SHOW_HIDDEN_FILES,
    BAUL_METADATA_KEY_WINDOW_SHOW_BACKUP_FILES,
    BAUL_METADATA_KEY_WINDOW_MAXIMIZED,
    BAUL_METADATA_KEY_WINDOW_STICKY,
    BAUL_METADATA_KEY_WINDOW_KEEP_ABOVE,
    BAUL_METADATA_KEY_SIDEBAR_BACKGROUND_COLOR,
    BAUL_METADATA_KEY_SIDEBAR_BACKGROUND_IMAGE,
    BAUL_METADATA_KEY_SIDEBAR_BUTTONS,
    BAUL_METADATA_KEY_ANNOTATION,
    BAUL_METADATA_KEY_ICON_POSITION,
    BAUL_METADATA_KEY_ICON_POSITION_TIMESTAMP,
    BAUL_METADATA_KEY_ICON_SCALE,
    BAUL_METADATA_KEY_CUSTOM_ICON,
    BAUL_METADATA_KEY_SCREEN,
    BAUL_METADATA_KEY_EMBLEMS,
    NULL
};

guint
baul_metadata_get_id (const char *metadata)
{
    static GHashTable *hash;

    if (hash == NULL)
    {
        int i;

        hash = g_hash_table_new (g_str_hash, g_str_equal);
        for (i = 0; used_metadata_names[i] != NULL; i++)
            g_hash_table_insert (hash,
                                 used_metadata_names[i],
                                 GINT_TO_POINTER (i + 1));
    }

    return GPOINTER_TO_INT (g_hash_table_lookup (hash, metadata));
}
