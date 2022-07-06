/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-window-slot.c: Baul window slot

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
#include <eel/eel-string.h>

#include <libbaul-private/baul-file.h>
#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-window-slot-info.h>

#include "baul-window-slot.h"
#include "baul-navigation-window-slot.h"
#include "baul-desktop-window.h"
#include "baul-window-private.h"
#include "baul-window-manage-views.h"

static void baul_window_slot_dispose    (GObject *object);

static void baul_window_slot_info_iface_init (BaulWindowSlotInfoIface *iface);

G_DEFINE_TYPE_WITH_CODE (BaulWindowSlot,
                         baul_window_slot,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_WINDOW_SLOT_INFO,
                                 baul_window_slot_info_iface_init))

#define parent_class baul_window_slot_parent_class

static void
query_editor_changed_callback (BaulSearchBar *bar,
                               BaulQuery *query,
                               gboolean reload,
                               BaulWindowSlot *slot)
{
    BaulDirectory *directory;

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
real_update_query_editor (BaulWindowSlot *slot)
{
    BaulDirectory *directory;

    directory = baul_directory_get (slot->location);

    if (BAUL_IS_SEARCH_DIRECTORY (directory))
    {
        GtkWidget *query_editor;
        BaulQuery *query;
        BaulSearchDirectory *search_directory;

        search_directory = BAUL_SEARCH_DIRECTORY (directory);

        query_editor = baul_query_editor_new (baul_search_directory_is_saved_search (search_directory),
                                              baul_search_directory_is_indexed (search_directory));

        slot->query_editor = BAUL_QUERY_EDITOR (query_editor);

        baul_window_slot_add_extra_location_widget (slot, query_editor);
        ctk_widget_show (query_editor);
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
    }

    baul_directory_unref (directory);
}


static void
real_active (BaulWindowSlot *slot)
{
    BaulWindow *window;

    window = slot->pane->window;

    /* sync window to new slot */
    baul_window_sync_status (window);
    baul_window_sync_allow_stop (window, slot);
    baul_window_sync_title (window, slot);
    baul_window_sync_zoom_widgets (window);
    baul_window_pane_sync_location_widgets (slot->pane);
    baul_window_pane_sync_search_widgets (slot->pane);

    if (slot->viewed_file != NULL)
    {
        baul_window_load_view_as_menus (window);
        baul_window_load_extension_menus (window);
    }
}

static void
baul_window_slot_active (BaulWindowSlot *slot)
{
    BaulWindow *window;
    BaulWindowPane *pane;

    g_assert (BAUL_IS_WINDOW_SLOT (slot));

    pane = BAUL_WINDOW_PANE (slot->pane);
    window = BAUL_WINDOW (slot->pane->window);
    g_assert (g_list_find (pane->slots, slot) != NULL);
    g_assert (slot == window->details->active_pane->active_slot);

    EEL_CALL_METHOD (BAUL_WINDOW_SLOT_CLASS, slot,
                     active, (slot));
}

static void
real_inactive (BaulWindowSlot *slot)
{
    BaulWindow *window;

    window = BAUL_WINDOW (slot->pane->window);
    g_assert (slot == window->details->active_pane->active_slot);
}

static void
baul_window_slot_inactive (BaulWindowSlot *slot)
{
    BaulWindow *window;
    BaulWindowPane *pane;

    g_assert (BAUL_IS_WINDOW_SLOT (slot));

    pane = BAUL_WINDOW_PANE (slot->pane);
    window = BAUL_WINDOW (pane->window);

    g_assert (g_list_find (pane->slots, slot) != NULL);
    g_assert (slot == window->details->active_pane->active_slot);

    EEL_CALL_METHOD (BAUL_WINDOW_SLOT_CLASS, slot,
                     inactive, (slot));
}


static void
baul_window_slot_init (BaulWindowSlot *slot)
{
    GtkWidget *content_box, *eventbox, *extras_vbox, *frame;

    content_box = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    slot->content_box = content_box;
    ctk_widget_show (content_box);

    frame = ctk_frame_new (NULL);
    ctk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    ctk_box_pack_start (GTK_BOX (content_box), frame, FALSE, FALSE, 0);
    slot->extra_location_frame = frame;

    eventbox = ctk_event_box_new ();
    ctk_widget_set_name (eventbox, "baul-extra-view-widget");
    ctk_container_add (GTK_CONTAINER (frame), eventbox);
    ctk_widget_show (eventbox);

    extras_vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    ctk_container_set_border_width (GTK_CONTAINER (extras_vbox), 6);
    slot->extra_location_widgets = extras_vbox;
    ctk_container_add (GTK_CONTAINER (eventbox), extras_vbox);
    ctk_widget_show (extras_vbox);

    slot->view_box = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    ctk_box_pack_start (GTK_BOX (content_box), slot->view_box, TRUE, TRUE, 0);
    ctk_widget_show (slot->view_box);

    slot->title = g_strdup (_("Loading..."));
}

static void
baul_window_slot_class_init (BaulWindowSlotClass *class)
{
    class->active = real_active;
    class->inactive = real_inactive;
    class->update_query_editor = real_update_query_editor;

    G_OBJECT_CLASS (class)->dispose = baul_window_slot_dispose;
}

static int
baul_window_slot_get_selection_count (BaulWindowSlot *slot)
{
    g_assert (BAUL_IS_WINDOW_SLOT (slot));

    if (slot->content_view != NULL)
    {
        return baul_view_get_selection_count (slot->content_view);
    }
    return 0;
}

GFile *
baul_window_slot_get_location (BaulWindowSlot *slot)
{
    g_assert (slot != NULL);
    g_assert (BAUL_IS_WINDOW (slot->pane->window));

    if (slot->location != NULL)
    {
        return g_object_ref (slot->location);
    }
    return NULL;
}

char *
baul_window_slot_get_location_uri (BaulWindowSlotInfo *slot)
{
    g_assert (BAUL_IS_WINDOW_SLOT (slot));

    if (slot->location)
    {
        return g_file_get_uri (slot->location);
    }
    return NULL;
}

static void
baul_window_slot_make_hosting_pane_active (BaulWindowSlot *slot)
{
    g_assert (BAUL_IS_WINDOW_SLOT (slot));
    g_assert (BAUL_IS_WINDOW_PANE (slot->pane));

    baul_window_set_active_slot (slot->pane->window, slot);
}

char *
baul_window_slot_get_title (BaulWindowSlot *slot)
{
    char *title;

    g_assert (BAUL_IS_WINDOW_SLOT (slot));

    title = NULL;
    if (slot->new_content_view != NULL)
    {
        title = baul_view_get_title (slot->new_content_view);
    }
    else if (slot->content_view != NULL)
    {
        title = baul_view_get_title (slot->content_view);
    }

    if (title == NULL)
    {
        title = baul_compute_title_for_location (slot->location);
    }

    return title;
}

static BaulWindow *
baul_window_slot_get_window (BaulWindowSlot *slot)
{
    g_assert (BAUL_IS_WINDOW_SLOT (slot));
    return slot->pane->window;
}

/* baul_window_slot_set_title:
 *
 * Sets slot->title, and if it changed
 * synchronizes the actual GtkWindow title which
 * might look a bit different (e.g. with "file browser:" added)
 */
static void
baul_window_slot_set_title (BaulWindowSlot *slot,
                            const char *title)
{
    BaulWindow *window;
    gboolean changed;

    g_assert (BAUL_IS_WINDOW_SLOT (slot));

    window = BAUL_WINDOW (slot->pane->window);

    changed = FALSE;

    if (eel_strcmp (title, slot->title) != 0)
    {
        changed = TRUE;

        g_free (slot->title);
        slot->title = g_strdup (title);
    }

    if (eel_strlen (slot->title) > 0 && slot->current_location_bookmark &&
            baul_bookmark_set_name (slot->current_location_bookmark,
                                    slot->title))
    {
        changed = TRUE;

        /* Name of item in history list changed, tell listeners. */
        baul_send_history_list_changed ();
    }

    if (changed)
    {
        baul_window_sync_title (window, slot);
    }
}


/* baul_window_slot_update_title:
 *
 * Re-calculate the slot title.
 * Called when the location or view has changed.
 * @slot: The BaulWindowSlot in question.
 *
 */
void
baul_window_slot_update_title (BaulWindowSlot *slot)
{
    char *title;

    title = baul_window_slot_get_title (slot);
    baul_window_slot_set_title (slot, title);
    g_free (title);
}

/* baul_window_slot_update_icon:
 *
 * Re-calculate the slot icon
 * Called when the location or view or icon set has changed.
 * @slot: The BaulWindowSlot in question.
 */
void
baul_window_slot_update_icon (BaulWindowSlot *slot)
{
    BaulWindow *window;
    BaulIconInfo *info;
    const char *icon_name;

    window = slot->pane->window;

    g_return_if_fail (BAUL_IS_WINDOW (window));

    info = EEL_CALL_METHOD_WITH_RETURN_VALUE (BAUL_WINDOW_CLASS, window,
            get_icon, (window, slot));

    if (slot != slot->pane->active_slot)
        return;

    icon_name = NULL;
    if (info)
    {
        icon_name = baul_icon_info_get_used_name (info);
        if (icon_name != NULL)
        {
            /* Gtk+ doesn't short circuit this (yet), so avoid lots of work
             * if we're setting to the same icon. This happens a lot e.g. when
             * the trash directory changes due to the file count changing.
             */
            if (g_strcmp0 (icon_name, ctk_window_get_icon_name (GTK_WINDOW (window))) != 0)
            {
                if (g_strcmp0 (icon_name, "text-x-generic") == 0)
                    ctk_window_set_icon_name (GTK_WINDOW (window), "folder-saved-search");
                else
                    ctk_window_set_icon_name (GTK_WINDOW (window), icon_name);
            }
        }
        else
        {
            GdkPixbuf *pixbuf;

            pixbuf = baul_icon_info_get_pixbuf_nodefault (info);

            if (pixbuf)
            {
                ctk_window_set_icon (GTK_WINDOW (window), pixbuf);
                g_object_unref (pixbuf);
            }
        }

        g_object_unref (info);
    }
}

void
baul_window_slot_is_in_active_pane (BaulWindowSlot *slot,
                                    gboolean is_active)
{
    /* NULL is valid, and happens during init */
    if (!slot)
    {
        return;
    }

    /* it may also be that the content is not a valid directory view during init */
    if (slot->content_view != NULL)
    {
        baul_view_set_is_active (slot->content_view, is_active);
    }

    if (slot->new_content_view != NULL)
    {
        baul_view_set_is_active (slot->new_content_view, is_active);
    }
}

void
baul_window_slot_connect_content_view (BaulWindowSlot *slot,
                                       BaulView *view)
{
    BaulWindow *window;

    window = slot->pane->window;
    if (window != NULL && slot == baul_window_get_active_slot (window))
    {
        baul_window_connect_content_view (window, view);
    }
}

void
baul_window_slot_disconnect_content_view (BaulWindowSlot *slot,
        BaulView *view)
{
    BaulWindow *window;

    window = slot->pane->window;
    if (window != NULL && window->details->active_pane && window->details->active_pane->active_slot == slot)
    {
        baul_window_disconnect_content_view (window, view);
    }
}

void
baul_window_slot_set_content_view_widget (BaulWindowSlot *slot,
        BaulView *new_view)
{
    BaulWindow *window;
    GtkWidget *widget;

    window = slot->pane->window;
    g_assert (BAUL_IS_WINDOW (window));

    if (slot->content_view != NULL)
    {
        /* disconnect old view */
        baul_window_slot_disconnect_content_view (slot, slot->content_view);

        widget = baul_view_get_widget (slot->content_view);
        ctk_widget_destroy (widget);
        g_object_unref (slot->content_view);
        slot->content_view = NULL;
    }

    if (new_view != NULL)
    {
        widget = baul_view_get_widget (new_view);
        ctk_box_pack_start (GTK_BOX (slot->view_box), widget,
                            TRUE, TRUE, 0);

        ctk_widget_show (widget);

        slot->content_view = new_view;
        g_object_ref (slot->content_view);

        /* connect new view */
        baul_window_slot_connect_content_view (slot, new_view);
    }
}

void
baul_window_slot_set_allow_stop (BaulWindowSlot *slot,
                                 gboolean allow)
{
    BaulWindow *window;

    g_assert (BAUL_IS_WINDOW_SLOT (slot));

    slot->allow_stop = allow;

    window = BAUL_WINDOW (slot->pane->window);
    baul_window_sync_allow_stop (window, slot);
}

void
baul_window_slot_set_status (BaulWindowSlot *slot,
                             const char *status)
{
    BaulWindow *window;

    g_assert (BAUL_IS_WINDOW_SLOT (slot));

    g_free (slot->status_text);
    slot->status_text = g_strdup (status);

    window = BAUL_WINDOW (slot->pane->window);
    if (slot == window->details->active_pane->active_slot)
    {
        baul_window_sync_status (window);
    }
}

/* baul_window_slot_update_query_editor:
 *
 * Update the query editor.
 * Called when the location has changed.
 *
 * @slot: The BaulWindowSlot in question.
 */
void
baul_window_slot_update_query_editor (BaulWindowSlot *slot)
{
    if (slot->query_editor != NULL)
    {
        ctk_widget_destroy (GTK_WIDGET (slot->query_editor));
        g_assert (slot->query_editor == NULL);
    }

    EEL_CALL_METHOD (BAUL_WINDOW_SLOT_CLASS, slot,
                     update_query_editor, (slot));

    eel_add_weak_pointer (&slot->query_editor);
}

static void
remove_all (GtkWidget *widget,
            gpointer data)
{
    GtkContainer *container;
    container = GTK_CONTAINER (data);

    ctk_container_remove (container, widget);
}

void
baul_window_slot_remove_extra_location_widgets (BaulWindowSlot *slot)
{
    ctk_container_foreach (GTK_CONTAINER (slot->extra_location_widgets),
                           remove_all,
                           slot->extra_location_widgets);
    ctk_widget_hide (slot->extra_location_frame);
}

void
baul_window_slot_add_extra_location_widget (BaulWindowSlot *slot,
        GtkWidget *widget)
{
    ctk_box_pack_start (GTK_BOX (slot->extra_location_widgets),
                        widget, TRUE, TRUE, 0);
    ctk_widget_show (slot->extra_location_frame);
}

void
baul_window_slot_add_current_location_to_history_list (BaulWindowSlot *slot)
{

    if ((slot->pane->window == NULL || !BAUL_IS_DESKTOP_WINDOW (slot->pane->window)) &&
            baul_add_bookmark_to_history_list (slot->current_location_bookmark))
    {
        baul_send_history_list_changed ();
    }
}

/* returns either the pending or the actual current location - used by side panes. */
static char *
real_slot_info_get_current_location (BaulWindowSlotInfo *info)
{
    BaulWindowSlot *slot;

    slot = BAUL_WINDOW_SLOT (info);

    if (slot->pending_location != NULL)
    {
        return g_file_get_uri (slot->pending_location);
    }

    if (slot->location != NULL)
    {
        return g_file_get_uri (slot->location);
    }

    g_assert_not_reached ();
    return NULL;
}

static BaulView *
real_slot_info_get_current_view (BaulWindowSlotInfo *info)
{
    BaulWindowSlot *slot;

    slot = BAUL_WINDOW_SLOT (info);

    if (slot->content_view != NULL)
    {
        return g_object_ref (slot->content_view);
    }
    else if (slot->new_content_view)
    {
        return g_object_ref (slot->new_content_view);
    }

    return NULL;
}

static void
baul_window_slot_dispose (GObject *object)
{
    BaulWindowSlot *slot;
    GtkWidget *widget;

    slot = BAUL_WINDOW_SLOT (object);

    if (slot->content_view)
    {
        widget = baul_view_get_widget (slot->content_view);
        ctk_widget_destroy (widget);
        g_object_unref (slot->content_view);
        slot->content_view = NULL;
    }

    if (slot->new_content_view)
    {
        widget = baul_view_get_widget (slot->new_content_view);
        ctk_widget_destroy (widget);
        g_object_unref (slot->new_content_view);
        slot->new_content_view = NULL;
    }

    baul_window_slot_set_viewed_file (slot, NULL);

    g_clear_object (&slot->location);

    g_list_free_full (slot->pending_selection, g_free);
    slot->pending_selection = NULL;

    if (slot->current_location_bookmark != NULL)
    {
        g_object_unref (slot->current_location_bookmark);
        slot->current_location_bookmark = NULL;
    }
    if (slot->last_location_bookmark != NULL)
    {
        g_object_unref (slot->last_location_bookmark);
        slot->last_location_bookmark = NULL;
    }

    if (slot->find_mount_cancellable != NULL)
    {
        g_cancellable_cancel (slot->find_mount_cancellable);
        slot->find_mount_cancellable = NULL;
    }

    slot->pane = NULL;

    g_free (slot->title);
    slot->title = NULL;

    g_free (slot->status_text);
    slot->status_text = NULL;

    G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
baul_window_slot_info_iface_init (BaulWindowSlotInfoIface *iface)
{
    iface->active = baul_window_slot_active;
    iface->inactive = baul_window_slot_inactive;
    iface->get_window = baul_window_slot_get_window;
    iface->get_selection_count = baul_window_slot_get_selection_count;
    iface->get_current_location = real_slot_info_get_current_location;
    iface->get_current_view = real_slot_info_get_current_view;
    iface->set_status = baul_window_slot_set_status;
    iface->get_title = baul_window_slot_get_title;
    iface->open_location = baul_window_slot_open_location_full;
    iface->make_hosting_pane_active = baul_window_slot_make_hosting_pane_active;
}

