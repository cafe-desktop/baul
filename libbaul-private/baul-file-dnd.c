/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-file-drag.c - Drag & drop handling code that operated on
   CajaFile objects.

   Copyright (C) 2000 Eazel, Inc.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Pavel Cisler <pavel@eazel.com>,
*/

#include <config.h>
#include "baul-file-dnd.h"
#include "baul-desktop-icon-file.h"

#include "baul-dnd.h"
#include "baul-directory.h"
#include "baul-file-utilities.h"
#include <string.h>

static gboolean
baul_drag_can_accept_files (CajaFile *drop_target_item)
{
    if (baul_file_is_directory (drop_target_item))
    {
        CajaDirectory *directory;
        gboolean res;

        /* target is a directory, accept if editable */
        directory = baul_directory_get_for_file (drop_target_item);
        res = baul_directory_is_editable (directory);
        baul_directory_unref (directory);
        return res;
    }

    if (BAUL_IS_DESKTOP_ICON_FILE (drop_target_item))
    {
        return TRUE;
    }

    /* All Caja links are assumed to be links to directories.
     * Therefore, they all can accept drags, like all other
     * directories to. As with other directories, there can be
     * errors when the actual copy is attempted due to
     * permissions.
     */
    if (baul_file_is_baul_link (drop_target_item))
    {
        return TRUE;
    }

    if (baul_is_engrampa_installed () &&
            baul_file_is_archive (drop_target_item))
    {
        return TRUE;
    }

    return FALSE;
}

gboolean
baul_drag_can_accept_item (CajaFile *drop_target_item,
                           const char *item_uri)
{
    if (baul_file_matches_uri (drop_target_item, item_uri))
    {
        /* can't accept itself */
        return FALSE;
    }

    return baul_drag_can_accept_files (drop_target_item);
}

gboolean
baul_drag_can_accept_items (CajaFile *drop_target_item,
                            const GList *items)
{
    int max;

    if (drop_target_item == NULL)
        return FALSE;

    g_assert (BAUL_IS_FILE (drop_target_item));

    /* Iterate through selection checking if item will get accepted by the
     * drop target. If more than 100 items selected, return an over-optimisic
     * result
     */
    for (max = 100; items != NULL && max >= 0; items = items->next, max--)
    {
        if (!baul_drag_can_accept_item (drop_target_item,
                                        ((CajaDragSelectionItem *)items->data)->uri))
        {
            return FALSE;
        }
    }

    return TRUE;
}

gboolean
baul_drag_can_accept_info (CajaFile *drop_target_item,
                           CajaIconDndTargetType drag_type,
                           const GList *items)
{
    switch (drag_type)
    {
    case BAUL_ICON_DND_MATE_ICON_LIST:
        return baul_drag_can_accept_items (drop_target_item, items);

    case BAUL_ICON_DND_URI_LIST:
    case BAUL_ICON_DND_NETSCAPE_URL:
    case BAUL_ICON_DND_TEXT:
        return baul_drag_can_accept_files (drop_target_item);

    case BAUL_ICON_DND_XDNDDIRECTSAVE:
    case BAUL_ICON_DND_RAW:
        return baul_drag_can_accept_files (drop_target_item); /* Check if we can accept files at this location */

    case BAUL_ICON_DND_KEYWORD:
        return TRUE;

    case BAUL_ICON_DND_ROOTWINDOW_DROP:
        return FALSE;

        /* TODO return TRUE for folders as soon as drop handling is implemented */
    case BAUL_ICON_DND_COLOR:
    case BAUL_ICON_DND_BGIMAGE:
    case BAUL_ICON_DND_RESET_BACKGROUND:
        return FALSE;

    default:
        g_assert_not_reached ();
        return FALSE;
    }
}

void
baul_drag_file_receive_dropped_keyword (CajaFile *file,
                                        const char *keyword)
{
    GList *keywords;

    g_return_if_fail (BAUL_IS_FILE (file));
    g_return_if_fail (keyword != NULL);

    /* special case the erase emblem */
    if (strcmp (keyword, BAUL_FILE_DND_ERASE_KEYWORD) == 0)
    {
        keywords = NULL;
    }
    else
    {
        GList *word;

        keywords = baul_file_get_keywords (file);
        word = g_list_find_custom (keywords, keyword, (GCompareFunc) strcmp);
        if (word == NULL)
        {
            keywords = g_list_prepend (keywords, g_strdup (keyword));
        }
        else
        {
            keywords = g_list_remove_link (keywords, word);
            g_free (word->data);
            g_list_free_1 (word);
        }
    }

    baul_file_set_keywords (file, keywords);
    g_list_free_full (keywords, g_free);
}
