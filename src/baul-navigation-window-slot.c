/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-navigation-window-slot.c: Baul navigation window slot

   Copyright (C) 2008 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Author: Christian Neumair <cneumair@gnome.org>
*/

#include <eel/eel-ctk-macros.h>

#include <libbaul-private/baul-window-slot-info.h>
#include <libbaul-private/baul-file.h>

#include "baul-window-slot.h"
#include "baul-navigation-window-slot.h"
#include "baul-window-private.h"
#include "baul-search-bar.h"
#include "baul-navigation-window-pane.h"

G_DEFINE_TYPE (BaulNavigationWindowSlot, baul_navigation_window_slot, BAUL_TYPE_WINDOW_SLOT)

#define parent_class baul_navigation_window_slot_parent_class

gboolean
baul_navigation_window_slot_should_close_with_mount (BaulNavigationWindowSlot *slot,
        GMount *mount)
{
    BaulBookmark *bookmark;
    GFile *mount_location, *bookmark_location;
    GList *l;
    gboolean close_with_mount;

    if (slot->parent.pane->window->details->initiated_unmount)
    {
        return FALSE;
    }

    mount_location = g_mount_get_root (mount);

    close_with_mount = TRUE;

    for (l = slot->back_list; l != NULL; l = l->next)
    {
        bookmark = BAUL_BOOKMARK (l->data);

        bookmark_location = baul_bookmark_get_location (bookmark);
        close_with_mount &= g_file_has_prefix (bookmark_location, mount_location) ||
                            g_file_equal (bookmark_location, mount_location);
        g_object_unref (bookmark_location);

        if (!close_with_mount)
        {
            break;
        }
    }

    close_with_mount &= g_file_has_prefix (BAUL_WINDOW_SLOT (slot)->location, mount_location) ||
                        g_file_equal (BAUL_WINDOW_SLOT (slot)->location, mount_location);

    /* we could also consider the forward list here, but since the “go home” request
     * in baul-window-manager-views.c:mount_removed_callback() would discard those
     * anyway, we don't consider them.
     */

    g_object_unref (mount_location);

    return close_with_mount;
}

void
baul_navigation_window_slot_clear_forward_list (BaulNavigationWindowSlot *slot)
{
    g_assert (BAUL_IS_NAVIGATION_WINDOW_SLOT (slot));

    g_list_free_full (slot->forward_list, g_object_unref);
    slot->forward_list = NULL;
}

void
baul_navigation_window_slot_clear_back_list (BaulNavigationWindowSlot *slot)
{
    g_assert (BAUL_IS_NAVIGATION_WINDOW_SLOT (slot));

    g_list_free_full (slot->back_list, g_object_unref);
    slot->back_list = NULL;
}

static void
query_editor_changed_callback (BaulSearchBar *bar,
                               BaulQuery *query,
                               gboolean reload,
                               BaulWindowSlot *slot)
{
    BaulDirectory *directory;

    g_assert (BAUL_IS_FILE (slot->viewed_file));

    directory = baul_directory_get_for_file (slot->viewed_file);
    g_assert (BAUL_IS_SEARCH_DIRECTORY (directory));

    baul_search_directory_set_query (BAUL_SEARCH_DIRECTORY (directory),
                                     query);
    if (reload)
    {
        baul_window_slot_reload (slot);
    }

    baul_directory_unref (directory);
}


static void
baul_navigation_window_slot_update_query_editor (BaulWindowSlot *slot)
{
    BaulDirectory *directory;
    BaulSearchDirectory *search_directory;
    CtkWidget *query_editor;

    g_assert (slot->pane->window != NULL);

    query_editor = NULL;

    directory = baul_directory_get (slot->location);
    if (BAUL_IS_SEARCH_DIRECTORY (directory))
    {
        search_directory = BAUL_SEARCH_DIRECTORY (directory);

        if (baul_search_directory_is_saved_search (search_directory))
        {
            query_editor = baul_query_editor_new (TRUE,
                                                  baul_search_directory_is_indexed (search_directory));
        }
        else
        {
            query_editor = baul_query_editor_new_with_bar (FALSE,
                           baul_search_directory_is_indexed (search_directory),
                           slot->pane->window->details->active_pane->active_slot == slot,
                           BAUL_SEARCH_BAR (BAUL_NAVIGATION_WINDOW_PANE (slot->pane)->search_bar),
                           slot);
        }
    }

    slot->query_editor = BAUL_QUERY_EDITOR (query_editor);

    if (query_editor != NULL)
    {
        BaulQuery *query;

        g_signal_connect_object (query_editor, "changed",
                                 G_CALLBACK (query_editor_changed_callback), slot, 0);

        query = baul_search_directory_get_query (search_directory);
        if (query != NULL)
        {
            baul_query_editor_set_query (BAUL_QUERY_EDITOR (query_editor),
                                         query);
            g_object_unref (query);
        }
        else
        {
            baul_query_editor_set_default_query (BAUL_QUERY_EDITOR (query_editor));
        }

        baul_window_slot_add_extra_location_widget (slot, query_editor);
        ctk_widget_show (query_editor);
        baul_query_editor_grab_focus (BAUL_QUERY_EDITOR (query_editor));
    }

    baul_directory_unref (directory);
}

static void
baul_navigation_window_slot_active (BaulWindowSlot *slot)
{
    BaulNavigationWindow *window;
    BaulNavigationWindowPane *pane;
    int page_num;

    pane = BAUL_NAVIGATION_WINDOW_PANE (slot->pane);
    window = BAUL_NAVIGATION_WINDOW (slot->pane->window);

    page_num = ctk_notebook_page_num (GTK_NOTEBOOK (pane->notebook),
                                      slot->content_box);
    g_assert (page_num >= 0);

    ctk_notebook_set_current_page (GTK_NOTEBOOK (pane->notebook), page_num);

    EEL_CALL_PARENT (BAUL_WINDOW_SLOT_CLASS, active, (slot));

    if (slot->viewed_file != NULL)
    {
        baul_navigation_window_load_extension_toolbar_items (window);
    }
}

static void
baul_navigation_window_slot_dispose (GObject *object)
{
    BaulNavigationWindowSlot *slot;

    slot = BAUL_NAVIGATION_WINDOW_SLOT (object);

    baul_navigation_window_slot_clear_forward_list (slot);
    baul_navigation_window_slot_clear_back_list (slot);

    G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
baul_navigation_window_slot_init (BaulNavigationWindowSlot *slot)
{
}

static void
baul_navigation_window_slot_class_init (BaulNavigationWindowSlotClass *class)
{
    BAUL_WINDOW_SLOT_CLASS (class)->active = baul_navigation_window_slot_active;
    BAUL_WINDOW_SLOT_CLASS (class)->update_query_editor = baul_navigation_window_slot_update_query_editor;

    G_OBJECT_CLASS (class)->dispose = baul_navigation_window_slot_dispose;
}

