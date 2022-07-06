/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/*
 *  Baul
 *
 *  Copyright (C) 2000, 2001 Eazel, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Author: Andy Hertzfeld <andy@eazel.com>
 *
 */

/* notes sidebar panel -- allows editing per-directory notes */

#include <config.h>

#include <ctk/ctk.h>
#include <glib/gi18n.h>

#include <eel/eel-debug.h>
#include <eel/eel-ctk-extensions.h>

#include <libbaul-private/baul-file-attributes.h>
#include <libbaul-private/baul-file.h>
#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-metadata.h>
#include <libbaul-private/baul-clipboard.h>
#include <libbaul-private/baul-module.h>
#include <libbaul-private/baul-sidebar-provider.h>
#include <libbaul-private/baul-window-info.h>
#include <libbaul-private/baul-window-slot-info.h>
#include <libbaul-extension/baul-property-page-provider.h>

#include "baul-notes-viewer.h"

#define SAVE_TIMEOUT 3

static void load_note_text_from_metadata             (BaulFile                      *file,
        BaulNotesViewer               *notes);
static void notes_save_metainfo                      (BaulNotesViewer               *notes);
static void baul_notes_viewer_sidebar_iface_init (BaulSidebarIface              *iface);
static void on_changed                               (CtkEditable                       *editable,
        BaulNotesViewer               *notes);
static void property_page_provider_iface_init        (BaulPropertyPageProviderIface *iface);
static void sidebar_provider_iface_init              (BaulSidebarProviderIface       *iface);

typedef struct
{
    CtkScrolledWindowClass parent;
} BaulNotesViewerClass;

typedef struct
{
    GObject parent;
} BaulNotesViewerProvider;

typedef struct
{
    GObjectClass parent;
} BaulNotesViewerProviderClass;


G_DEFINE_TYPE_WITH_CODE (BaulNotesViewer, baul_notes_viewer, CTK_TYPE_SCROLLED_WINDOW,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_SIDEBAR,
                                 baul_notes_viewer_sidebar_iface_init));

static GType baul_notes_viewer_provider_get_type (void);

G_DEFINE_TYPE_WITH_CODE (BaulNotesViewerProvider, baul_notes_viewer_provider, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_PROPERTY_PAGE_PROVIDER,
                                 property_page_provider_iface_init);
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_SIDEBAR_PROVIDER,
                                 sidebar_provider_iface_init));


struct _BaulNotesViewerDetails
{
    CtkWidget *note_text_field;
    CtkTextBuffer *text_buffer;
    char *uri;
    BaulFile *file;
    guint save_timeout_id;
    char *previous_saved_text;
    GdkPixbuf *icon;
};

static gboolean
schedule_save_callback (gpointer data)
{
    BaulNotesViewer *notes;

    notes = data;

    /* Zero out save_timeout_id so no one will try to cancel our
     * in-progress timeout callback.
         */
    notes->details->save_timeout_id = 0;

    notes_save_metainfo (notes);

    return FALSE;
}

static void
cancel_pending_save (BaulNotesViewer *notes)
{
    if (notes->details->save_timeout_id != 0)
    {
        g_source_remove (notes->details->save_timeout_id);
        notes->details->save_timeout_id = 0;
    }
}

static void
schedule_save (BaulNotesViewer *notes)
{
    cancel_pending_save (notes);

    notes->details->save_timeout_id = g_timeout_add_seconds (SAVE_TIMEOUT, schedule_save_callback, notes);
}

/* notifies event listeners if the notes data actually changed */
static void
set_saved_text (BaulNotesViewer *notes, char *new_notes)
{
    char *old_text;

    old_text = notes->details->previous_saved_text;
    notes->details->previous_saved_text = new_notes;

    if (g_strcmp0 (old_text, new_notes) != 0)
    {
        g_signal_emit_by_name (BAUL_SIDEBAR (notes),
                               "tab_icon_changed");
    }

    g_free (old_text);
}

/* save the metainfo corresponding to the current uri, if any, into the text field */
static void
notes_save_metainfo (BaulNotesViewer *notes)
{
    char *notes_text;
    CtkTextIter start_iter;
    CtkTextIter end_iter;

    if (notes->details->file == NULL)
    {
        return;
    }

    cancel_pending_save (notes);

    /* Block the handler, so we don't respond to our own change.
     */
    g_signal_handlers_block_matched (notes->details->file,
                                     G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                     0, 0, NULL,
                                     G_CALLBACK (load_note_text_from_metadata),
                                     notes);

    ctk_text_buffer_get_start_iter (notes->details->text_buffer, &start_iter);
    ctk_text_buffer_get_end_iter (notes->details->text_buffer, &end_iter);
    notes_text = ctk_text_buffer_get_text (notes->details->text_buffer,
                                           &start_iter,
                                           &end_iter,
                                           FALSE);

    baul_file_set_metadata (notes->details->file,
                            BAUL_METADATA_KEY_ANNOTATION,
                            NULL, notes_text);

    g_signal_handlers_unblock_matched (notes->details->file,
                                       G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                       0, 0, NULL,
                                       G_CALLBACK (load_note_text_from_metadata),
                                       notes);

    set_saved_text (notes, notes_text);
}

static void
load_note_text_from_metadata (BaulFile *file,
                              BaulNotesViewer *notes)
{
    char *saved_text;

    g_assert (BAUL_IS_FILE (file));
    g_assert (notes->details->file == file);

    saved_text = baul_file_get_metadata (file, BAUL_METADATA_KEY_ANNOTATION, "");

    /* This fn is called for any change signal on the file, so make sure that the
     * metadata has actually changed.
     */
    if (g_strcmp0 (saved_text, notes->details->previous_saved_text) != 0)
    {
        set_saved_text (notes, saved_text);
        cancel_pending_save (notes);

        /* Block the handler, so we don't respond to our own change.
         */
        g_signal_handlers_block_matched (notes->details->text_buffer,
                                         G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                         0, 0, NULL,
                                         G_CALLBACK (on_changed),
                                         notes);
        ctk_text_buffer_set_text (notes->details->text_buffer, saved_text, -1);
        g_signal_handlers_unblock_matched (notes->details->text_buffer,
                                           G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                           0, 0, NULL,
                                           G_CALLBACK (on_changed),
                                           notes);
    }
    else
    {
        g_free (saved_text);
    }
}

static void
done_with_file (BaulNotesViewer *notes)
{
    cancel_pending_save (notes);

    if (notes->details->file != NULL)
    {
        baul_file_monitor_remove (notes->details->file, notes);
        g_signal_handlers_disconnect_matched (notes->details->file,
                                              G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                              0, 0, NULL,
                                              G_CALLBACK (load_note_text_from_metadata),
                                              notes);
        baul_file_unref (notes->details->file);
    }
}

static void
notes_load_metainfo (BaulNotesViewer *notes)
{
    BaulFileAttributes attributes;

    done_with_file (notes);
    notes->details->file = baul_file_get_by_uri (notes->details->uri);

    /* Block the handler, so we don't respond to our own change.
     */
    g_signal_handlers_block_matched (notes->details->text_buffer,
                                     G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                     0, 0, NULL,
                                     G_CALLBACK (on_changed),
                                     notes);
    ctk_text_buffer_set_text (notes->details->text_buffer, "", -1);
    g_signal_handlers_unblock_matched (notes->details->text_buffer,
                                       G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                       0, 0, NULL,
                                       G_CALLBACK (on_changed),
                                       notes);

    if (notes->details->file == NULL)
    {
        return;
    }

    attributes = BAUL_FILE_ATTRIBUTE_INFO;
    baul_file_monitor_add (notes->details->file, notes, attributes);

    if (baul_file_check_if_ready (notes->details->file, attributes))
    {
        load_note_text_from_metadata (notes->details->file, notes);
    }

    g_signal_connect (notes->details->file, "changed",
                      G_CALLBACK (load_note_text_from_metadata), notes);
}

static void
loading_uri_callback (BaulWindowInfo *window,
                      const char *location,
                      BaulNotesViewer *notes)
{
    if (strcmp (notes->details->uri, location) != 0)
    {
        notes_save_metainfo (notes);
        g_free (notes->details->uri);
        notes->details->uri = g_strdup (location);
        notes_load_metainfo (notes);
    }
}

static gboolean
on_text_field_focus_out_event (CtkWidget *widget,
                               GdkEventFocus *event,
                               gpointer callback_data)
{
    BaulNotesViewer *notes;

    notes = callback_data;
    notes_save_metainfo (notes);
    return FALSE;
}

static void
on_changed (CtkEditable *editable, BaulNotesViewer *notes)
{
    schedule_save (notes);
}

static void
baul_notes_viewer_init (BaulNotesViewer *sidebar)
{
    BaulNotesViewerDetails *details;
    BaulIconInfo *info;
    gint scale;

    details = g_new0 (BaulNotesViewerDetails, 1);
    sidebar->details = details;

    details->uri = g_strdup ("");

    scale = cdk_window_get_scale_factor (cdk_get_default_root_window ());
    info = baul_icon_info_lookup_from_name ("emblem-note", 16, scale);
    details->icon = baul_icon_info_get_pixbuf (info);

    /* create the text container */
    details->text_buffer = ctk_text_buffer_new (NULL);
    details->note_text_field = ctk_text_view_new_with_buffer (details->text_buffer);

    ctk_text_view_set_editable (CTK_TEXT_VIEW (details->note_text_field), TRUE);
    ctk_text_view_set_wrap_mode (CTK_TEXT_VIEW (details->note_text_field),
                                 CTK_WRAP_WORD);
    ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sidebar),
                                    CTK_POLICY_AUTOMATIC,
                                    CTK_POLICY_AUTOMATIC);
    ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (sidebar),
                                         CTK_SHADOW_IN);
    ctk_scrolled_window_set_hadjustment (CTK_SCROLLED_WINDOW (sidebar), NULL);
    ctk_scrolled_window_set_vadjustment (CTK_SCROLLED_WINDOW (sidebar), NULL);
    ctk_scrolled_window_set_overlay_scrolling (CTK_SCROLLED_WINDOW (sidebar), FALSE);

    ctk_container_add (CTK_CONTAINER (sidebar), details->note_text_field);

    g_signal_connect (details->note_text_field, "focus_out_event",
                      G_CALLBACK (on_text_field_focus_out_event), sidebar);
    g_signal_connect (details->text_buffer, "changed",
                      G_CALLBACK (on_changed), sidebar);

    ctk_widget_show_all (CTK_WIDGET (sidebar));

}

static void
baul_notes_viewer_finalize (GObject *object)
{
    BaulNotesViewer *sidebar;

    sidebar = BAUL_NOTES_VIEWER (object);

    done_with_file (sidebar);
    if (sidebar->details->icon != NULL)
    {
        g_object_unref (sidebar->details->icon);
    }
    g_free (sidebar->details->uri);
    g_free (sidebar->details->previous_saved_text);
    g_free (sidebar->details);

    G_OBJECT_CLASS (baul_notes_viewer_parent_class)->finalize (object);
}


static void
baul_notes_viewer_class_init (BaulNotesViewerClass *class)
{
    G_OBJECT_CLASS (class)->finalize = baul_notes_viewer_finalize;
}

static const char *
baul_notes_viewer_get_sidebar_id (BaulSidebar *sidebar)
{
    return BAUL_NOTES_SIDEBAR_ID;
}

static char *
baul_notes_viewer_get_tab_label (BaulSidebar *sidebar)
{
    return g_strdup (_("Notes"));
}

static char *
baul_notes_viewer_get_tab_tooltip (BaulSidebar *sidebar)
{
    return g_strdup (_("Show Notes"));
}

static GdkPixbuf *
baul_notes_viewer_get_tab_icon (BaulSidebar *sidebar)
{
    BaulNotesViewer *notes;

    notes = BAUL_NOTES_VIEWER (sidebar);

    if (notes->details->previous_saved_text != NULL &&
            notes->details->previous_saved_text[0] != '\0')
    {
        return g_object_ref (notes->details->icon);
    }

    return NULL;
}

static void
baul_notes_viewer_is_visible_changed (BaulSidebar *sidebar,
                                      gboolean         is_visible)
{
    /* Do nothing */
}

static void
baul_notes_viewer_sidebar_iface_init (BaulSidebarIface *iface)
{
    iface->get_sidebar_id = baul_notes_viewer_get_sidebar_id;
    iface->get_tab_label = baul_notes_viewer_get_tab_label;
    iface->get_tab_tooltip = baul_notes_viewer_get_tab_tooltip;
    iface->get_tab_icon = baul_notes_viewer_get_tab_icon;
    iface->is_visible_changed = baul_notes_viewer_is_visible_changed;
}

static void
baul_notes_viewer_set_parent_window (BaulNotesViewer *sidebar,
                                     BaulWindowInfo *window)
{
    BaulWindowSlotInfo *slot;

    slot = baul_window_info_get_active_slot (window);

    g_signal_connect_object (window, "loading_uri",
                             G_CALLBACK (loading_uri_callback), sidebar, 0);

    g_free (sidebar->details->uri);
    sidebar->details->uri = baul_window_slot_info_get_current_location (slot);
    notes_load_metainfo (sidebar);

    baul_clipboard_set_up_text_view
    (CTK_TEXT_VIEW (sidebar->details->note_text_field),
     baul_window_info_get_ui_manager (window));
}

static BaulSidebar *
baul_notes_viewer_create_sidebar (BaulSidebarProvider *provider,
                                  BaulWindowInfo *window)
{
    BaulNotesViewer *sidebar;

    sidebar = g_object_new (baul_notes_viewer_get_type (), NULL);
    baul_notes_viewer_set_parent_window (sidebar, window);
    g_object_ref_sink (sidebar);

    return BAUL_SIDEBAR (sidebar);
}

static GList *
get_property_pages (BaulPropertyPageProvider *provider,
                    GList *files)
{
    GList *pages;
    BaulPropertyPage *page;
    BaulFileInfo *file;
    char *uri;
    BaulNotesViewer *viewer;


    /* Only show the property page if 1 file is selected */
    if (!files || files->next != NULL)
    {
        return NULL;
    }

    pages = NULL;

    file = BAUL_FILE_INFO (files->data);
    uri = baul_file_info_get_uri (file);

    viewer = g_object_new (baul_notes_viewer_get_type (), NULL);
    g_free (viewer->details->uri);
    viewer->details->uri = uri;
    notes_load_metainfo (viewer);

    page = baul_property_page_new
           ("BaulNotesViewer::property_page",
            ctk_label_new (_("Notes")),
            CTK_WIDGET (viewer));
    pages = g_list_append (pages, page);

    return pages;
}

static void
property_page_provider_iface_init (BaulPropertyPageProviderIface *iface)
{
    iface->get_pages = get_property_pages;
}

static void
sidebar_provider_iface_init (BaulSidebarProviderIface *iface)
{
    iface->create = baul_notes_viewer_create_sidebar;
}

static void
baul_notes_viewer_provider_init (BaulNotesViewerProvider *sidebar)
{
}

static void
baul_notes_viewer_provider_class_init (BaulNotesViewerProviderClass *class)
{
}

void
baul_notes_viewer_register (void)
{
    baul_module_add_type (baul_notes_viewer_provider_get_type ());
}

