/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* fm-empty-view.c - implementation of empty view of directory.

   Copyright (C) 2006 Free Software Foundation, Inc.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Cafe Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Christian Neumair <chris@cafe-de.org>
*/

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>

#include <eel/eel-glib-extensions.h>
#include <eel/eel-ctk-macros.h>
#include <eel/eel-vfs-extensions.h>

#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-view.h>
#include <libbaul-private/baul-view-factory.h>

#include "fm-empty-view.h"

struct FMEmptyViewDetails
{
    int number_of_files;
};

static GList *fm_empty_view_get_selection                   (FMDirectoryView   *view);
static GList *fm_empty_view_get_selection_for_file_transfer (FMDirectoryView   *view);
static void   fm_empty_view_scroll_to_file                  (BaulView      *view,
        const char        *uri);
static void   fm_empty_view_iface_init                      (BaulViewIface *iface);

G_DEFINE_TYPE_WITH_CODE (FMEmptyView, fm_empty_view, FM_TYPE_DIRECTORY_VIEW,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_VIEW,
                                 fm_empty_view_iface_init));

/* for EEL_CALL_PARENT */
#define parent_class fm_empty_view_parent_class

static void
fm_empty_view_add_file (FMDirectoryView *view,
			BaulFile        *file,
			BaulDirectory   *directory G_GNUC_UNUSED)
{
    static GTimer *timer = NULL;
    static gdouble cumu = 0, elaps;
    FM_EMPTY_VIEW (view)->details->number_of_files++;
    cairo_surface_t *icon;

    if (!timer) timer = g_timer_new ();

    g_timer_start (timer);
    icon = baul_file_get_icon_surface (file, baul_get_icon_size_for_zoom_level (BAUL_ZOOM_LEVEL_STANDARD),
                                       TRUE, ctk_widget_get_scale_factor (CTK_WIDGET(view)), 0);

    elaps = g_timer_elapsed (timer, NULL);
    g_timer_stop (timer);

    cairo_surface_destroy (icon);

    cumu += elaps;
    g_message ("entire loading: %.3f, cumulative %.3f", elaps, cumu);
}


static void
fm_empty_view_begin_loading (FMDirectoryView *view G_GNUC_UNUSED)
{
}

static void
fm_empty_view_clear (FMDirectoryView *view G_GNUC_UNUSED)
{
}


static void
fm_empty_view_file_changed (FMDirectoryView *view G_GNUC_UNUSED,
			    BaulFile        *file G_GNUC_UNUSED,
			    BaulDirectory   *directory G_GNUC_UNUSED)
{
}

static CtkWidget *
fm_empty_view_get_background_widget (FMDirectoryView *view)
{
    return CTK_WIDGET (view);
}

static GList *
fm_empty_view_get_selection (FMDirectoryView *view G_GNUC_UNUSED)
{
    return NULL;
}


static GList *
fm_empty_view_get_selection_for_file_transfer (FMDirectoryView *view G_GNUC_UNUSED)
{
    return NULL;
}

static guint
fm_empty_view_get_item_count (FMDirectoryView *view)
{
    return FM_EMPTY_VIEW (view)->details->number_of_files;
}

static gboolean
fm_empty_view_is_empty (FMDirectoryView *view)
{
    return FM_EMPTY_VIEW (view)->details->number_of_files == 0;
}

static void
fm_empty_view_end_file_changes (FMDirectoryView *view G_GNUC_UNUSED)
{
}

static void
fm_empty_view_remove_file (FMDirectoryView *view,
			   BaulFile        *file G_GNUC_UNUSED,
			   BaulDirectory   *directory G_GNUC_UNUSED)
{
    FM_EMPTY_VIEW (view)->details->number_of_files--;
    g_assert (FM_EMPTY_VIEW (view)->details->number_of_files >= 0);
}

static void
fm_empty_view_set_selection (FMDirectoryView *view,
			     GList           *selection G_GNUC_UNUSED)
{
    fm_directory_view_notify_selection_changed (view);
}

static void
fm_empty_view_select_all (FMDirectoryView *view G_GNUC_UNUSED)
{
}

static void
fm_empty_view_reveal_selection (FMDirectoryView *view G_GNUC_UNUSED)
{
}

static void
fm_empty_view_merge_menus (FMDirectoryView *view)
{
    EEL_CALL_PARENT (FM_DIRECTORY_VIEW_CLASS, merge_menus, (view));
}

static void
fm_empty_view_update_menus (FMDirectoryView *view)
{
    EEL_CALL_PARENT (FM_DIRECTORY_VIEW_CLASS, update_menus, (view));
}

/* Reset sort criteria and zoom level to match defaults */
static void
fm_empty_view_reset_to_defaults (FMDirectoryView *view G_GNUC_UNUSED)
{
}

static void
fm_empty_view_bump_zoom_level (FMDirectoryView *view G_GNUC_UNUSED,
			       int              zoom_increment G_GNUC_UNUSED)
{
}

static BaulZoomLevel
fm_empty_view_get_zoom_level (FMDirectoryView *view G_GNUC_UNUSED)
{
    return BAUL_ZOOM_LEVEL_STANDARD;
}

static void
fm_empty_view_zoom_to_level (FMDirectoryView *view G_GNUC_UNUSED,
			     BaulZoomLevel    zoom_level G_GNUC_UNUSED)
{
}

static void
fm_empty_view_restore_default_zoom_level (FMDirectoryView *view G_GNUC_UNUSED)
{
}

static gboolean
fm_empty_view_can_zoom_in (FMDirectoryView *view G_GNUC_UNUSED)
{
    return FALSE;
}

static gboolean
fm_empty_view_can_zoom_out (FMDirectoryView *view G_GNUC_UNUSED)
{
    return FALSE;
}

static void
fm_empty_view_start_renaming_file (FMDirectoryView *view G_GNUC_UNUSED,
				   BaulFile        *file G_GNUC_UNUSED,
				   gboolean         select_all G_GNUC_UNUSED)
{
}

static void
fm_empty_view_click_policy_changed (FMDirectoryView *directory_view G_GNUC_UNUSED)
{
}


static int
fm_empty_view_compare_files (FMDirectoryView *view G_GNUC_UNUSED,
			     BaulFile        *file1,
			     BaulFile        *file2)
{
    if (file1 < file2)
    {
        return -1;
    }

    if (file1 > file2)
    {
        return +1;
    }

    return 0;
}

static gboolean
fm_empty_view_using_manual_layout (FMDirectoryView *view G_GNUC_UNUSED)
{
    return FALSE;
}

static void
fm_empty_view_end_loading (FMDirectoryView *view G_GNUC_UNUSED,
			   gboolean         all_files_seen G_GNUC_UNUSED)
{
}

static void
fm_empty_view_finalize (GObject *object)
{
    FMEmptyView *empty_view;

    empty_view = FM_EMPTY_VIEW (object);
    g_free (empty_view->details);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
fm_empty_view_emblems_changed (FMDirectoryView *directory_view G_GNUC_UNUSED)
{
}

static char *
fm_empty_view_get_first_visible_file (BaulView *view G_GNUC_UNUSED)
{
    return NULL;
}

static void
fm_empty_view_scroll_to_file (BaulView   *view G_GNUC_UNUSED,
			      const char *uri G_GNUC_UNUSED)
{
}

static void
fm_empty_view_grab_focus (BaulView *view)
{
    ctk_widget_grab_focus (CTK_WIDGET (view));
}

static void
fm_empty_view_sort_directories_first_changed (FMDirectoryView *view G_GNUC_UNUSED)
{
}

static void
fm_empty_view_class_init (FMEmptyViewClass *class)
{
    FMDirectoryViewClass *fm_directory_view_class;

    fm_directory_view_class = FM_DIRECTORY_VIEW_CLASS (class);

    G_OBJECT_CLASS (class)->finalize = fm_empty_view_finalize;

    fm_directory_view_class->add_file = fm_empty_view_add_file;
    fm_directory_view_class->begin_loading = fm_empty_view_begin_loading;
    fm_directory_view_class->bump_zoom_level = fm_empty_view_bump_zoom_level;
    fm_directory_view_class->can_zoom_in = fm_empty_view_can_zoom_in;
    fm_directory_view_class->can_zoom_out = fm_empty_view_can_zoom_out;
    fm_directory_view_class->click_policy_changed = fm_empty_view_click_policy_changed;
    fm_directory_view_class->clear = fm_empty_view_clear;
    fm_directory_view_class->file_changed = fm_empty_view_file_changed;
    fm_directory_view_class->get_background_widget = fm_empty_view_get_background_widget;
    fm_directory_view_class->get_selection = fm_empty_view_get_selection;
    fm_directory_view_class->get_selection_for_file_transfer = fm_empty_view_get_selection_for_file_transfer;
    fm_directory_view_class->get_item_count = fm_empty_view_get_item_count;
    fm_directory_view_class->is_empty = fm_empty_view_is_empty;
    fm_directory_view_class->remove_file = fm_empty_view_remove_file;
    fm_directory_view_class->merge_menus = fm_empty_view_merge_menus;
    fm_directory_view_class->update_menus = fm_empty_view_update_menus;
    fm_directory_view_class->reset_to_defaults = fm_empty_view_reset_to_defaults;
    fm_directory_view_class->restore_default_zoom_level = fm_empty_view_restore_default_zoom_level;
    fm_directory_view_class->reveal_selection = fm_empty_view_reveal_selection;
    fm_directory_view_class->select_all = fm_empty_view_select_all;
    fm_directory_view_class->set_selection = fm_empty_view_set_selection;
    fm_directory_view_class->compare_files = fm_empty_view_compare_files;
    fm_directory_view_class->sort_directories_first_changed = fm_empty_view_sort_directories_first_changed;
    fm_directory_view_class->start_renaming_file = fm_empty_view_start_renaming_file;
    fm_directory_view_class->get_zoom_level = fm_empty_view_get_zoom_level;
    fm_directory_view_class->zoom_to_level = fm_empty_view_zoom_to_level;
    fm_directory_view_class->emblems_changed = fm_empty_view_emblems_changed;
    fm_directory_view_class->end_file_changes = fm_empty_view_end_file_changes;
    fm_directory_view_class->using_manual_layout = fm_empty_view_using_manual_layout;
    fm_directory_view_class->end_loading = fm_empty_view_end_loading;
}

static const char *
fm_empty_view_get_id (BaulView *view G_GNUC_UNUSED)
{
    return FM_EMPTY_VIEW_ID;
}


static void
fm_empty_view_iface_init (BaulViewIface *iface)
{
    fm_directory_view_init_view_iface (iface);

    iface->get_view_id = fm_empty_view_get_id;
    iface->get_first_visible_file = fm_empty_view_get_first_visible_file;
    iface->scroll_to_file = fm_empty_view_scroll_to_file;
    iface->get_title = NULL;
    iface->grab_focus = fm_empty_view_grab_focus;
}


static void
fm_empty_view_init (FMEmptyView *empty_view)
{
    empty_view->details = g_new0 (FMEmptyViewDetails, 1);
}

static BaulView *
fm_empty_view_create (BaulWindowSlotInfo *slot)
{
    FMEmptyView *view;

    g_assert (BAUL_IS_WINDOW_SLOT_INFO (slot));

    view = g_object_new (FM_TYPE_EMPTY_VIEW,
                         "window-slot", slot,
                         NULL);

    return BAUL_VIEW (view);
}

static gboolean
fm_empty_view_supports_uri (const char *uri,
                            GFileType file_type,
                            const char *mime_type)
{
    if (file_type == G_FILE_TYPE_DIRECTORY)
    {
        return TRUE;
    }
    if (strcmp (mime_type, BAUL_SAVED_SEARCH_MIMETYPE) == 0)
    {
        return TRUE;
    }
    if (g_str_has_prefix (uri, "trash:"))
    {
        return TRUE;
    }
    if (g_str_has_prefix (uri, EEL_SEARCH_URI))
    {
        return TRUE;
    }

    return FALSE;
}

static BaulViewInfo fm_empty_view =
{
    .id = FM_EMPTY_VIEW_ID,
    .view_combo_label = N_("Empty View"),
    .view_menu_label_with_mnemonic = N_("_Empty"),
    .error_label = N_("Empty View"),
    .startup_error_label = N_("The empty view encountered an error."),
    .display_location_label = N_("Display this location with the empty view."),
    .create = fm_empty_view_create,
    .supports_uri =fm_empty_view_supports_uri
};

void
fm_empty_view_register (void)
{
    fm_empty_view.id = fm_empty_view.id;
    fm_empty_view.view_combo_label = fm_empty_view.view_combo_label;
    fm_empty_view.view_menu_label_with_mnemonic = fm_empty_view.view_menu_label_with_mnemonic;
    fm_empty_view.error_label = fm_empty_view.error_label;
    fm_empty_view.display_location_label = fm_empty_view.display_location_label;

    baul_view_factory_register (&fm_empty_view);
}
