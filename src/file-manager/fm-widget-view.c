/*
 * fm-widget-view.c: This file is part of baul.
 *
 * Copyright (C) 2019 Wu Xiaotian <yetist@gmail.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * */

#include <config.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-ctk-macros.h>
#include <eel/eel-vfs-extensions.h>

#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-view.h>
#include <libbaul-private/baul-view-factory.h>
#include <libbaul-private/baul-extensions.h>
#include <libbaul-private/baul-module.h>
#include <libbaul-private/baul-metadata.h>
#include <libbaul-extension/baul-widget-view-provider.h>

#include "fm-widget-view.h"

struct _FMWidgetView
{
    FMDirectoryView         object;
    BaulWidgetViewProvider *provider;
};

static GList *fm_widget_view_get_selection                   (FMDirectoryView   *view);
static GList *fm_widget_view_get_selection_for_file_transfer (FMDirectoryView   *view);
static void   fm_widget_view_scroll_to_file                  (BaulView *view, const char *uri);
static void   fm_widget_view_iface_init                      (BaulViewIface *iface);

G_DEFINE_TYPE_WITH_CODE (FMWidgetView, fm_widget_view, FM_TYPE_DIRECTORY_VIEW,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_VIEW, fm_widget_view_iface_init));

static void
fm_widget_view_add_file (FMDirectoryView *view, BaulFile *file, BaulDirectory *directory)
{
    FMWidgetView *widget_view;
    BaulFile *file_dir;

    widget_view = FM_WIDGET_VIEW (view);
    g_return_if_fail (FM_IS_WIDGET_VIEW(view));
    g_return_if_fail (BAUL_IS_WIDGET_VIEW_PROVIDER (widget_view->provider));

    file_dir = baul_directory_get_corresponding_file (directory);
    baul_widget_view_provider_add_file (widget_view->provider, file, file_dir);
    baul_file_unref (file_dir);
}

static void
fm_widget_view_begin_loading (FMDirectoryView *view)
{
    CtkWindow *window;
    BaulFile *file;
    gchar *uri;
    GList *providers, *l;
    char *mimetype;
    CtkWidget *widget;
    FMWidgetView *widget_view;

    widget_view = FM_WIDGET_VIEW (view);

    uri = fm_directory_view_get_uri (view);
    file = baul_file_get_by_uri (uri);
    mimetype = baul_file_get_mime_type (file);

    providers = baul_extensions_get_for_type (BAUL_TYPE_WIDGET_VIEW_PROVIDER);
    for (l = providers; l != NULL; l = l->next)
    {
        BaulWidgetViewProvider *provider;

        provider = BAUL_WIDGET_VIEW_PROVIDER (l->data);
        if (baul_widget_view_provider_supports_uri (provider, uri,
                                                    baul_file_get_file_type (file),
                                                    mimetype)) {
            widget_view->provider = provider;
            break;
        }
    }
    baul_file_unref (file);
    g_free (mimetype);
    baul_module_extension_list_free (providers);

    if (widget_view->provider == NULL) {
        g_free (uri);
        return;
    }

    baul_widget_view_provider_set_location (widget_view->provider, uri);
    g_free (uri);

    widget = baul_widget_view_provider_get_widget (widget_view->provider);
    ctk_container_add (CTK_CONTAINER(widget_view), widget);

    window = fm_directory_view_get_containing_window (view);
    baul_widget_view_provider_set_window (widget_view->provider, window);
}

static void
fm_widget_view_clear (FMDirectoryView *view)
{
    FMWidgetView *widget_view;

    widget_view = FM_WIDGET_VIEW (view);
    g_return_if_fail (FM_IS_WIDGET_VIEW(view));
    g_return_if_fail (BAUL_IS_WIDGET_VIEW_PROVIDER (widget_view->provider));

    baul_widget_view_provider_clear (widget_view->provider);
}

static void
fm_widget_view_file_changed (FMDirectoryView *view G_GNUC_UNUSED,
			     BaulFile        *file G_GNUC_UNUSED,
			     BaulDirectory   *directory G_GNUC_UNUSED)
{
}

static CtkWidget *
fm_widget_view_get_background_widget (FMDirectoryView *view)
{
    return CTK_WIDGET (view);
}

static GList *
fm_widget_view_get_selection (FMDirectoryView *view G_GNUC_UNUSED)
{
    return NULL;
}

static GList *
fm_widget_view_get_selection_for_file_transfer (FMDirectoryView *view G_GNUC_UNUSED)
{
    return NULL;
}

static guint
fm_widget_view_get_item_count (FMDirectoryView *view)
{
    FMWidgetView *widget_view;

    widget_view = FM_WIDGET_VIEW (view);
    g_return_val_if_fail (FM_IS_WIDGET_VIEW(view), 0);
    g_return_val_if_fail (BAUL_IS_WIDGET_VIEW_PROVIDER (widget_view->provider), 0);

    return baul_widget_view_provider_get_item_count (widget_view->provider);
}

static gboolean
fm_widget_view_is_empty (FMDirectoryView *view)
{
    FMWidgetView *widget_view;

    widget_view = FM_WIDGET_VIEW (view);
    g_return_val_if_fail (FM_IS_WIDGET_VIEW(view), TRUE);
    g_return_val_if_fail (BAUL_IS_WIDGET_VIEW_PROVIDER (widget_view->provider), TRUE);

    return baul_widget_view_provider_get_item_count (widget_view->provider) == 0;
}

static void
fm_widget_view_end_file_changes (FMDirectoryView *view G_GNUC_UNUSED)
{
}

static void
fm_widget_view_remove_file (FMDirectoryView *view G_GNUC_UNUSED,
			    BaulFile        *file G_GNUC_UNUSED,
			    BaulDirectory   *directory G_GNUC_UNUSED)
{
}

static void
fm_widget_view_set_selection (FMDirectoryView *view,
			      GList           *selection G_GNUC_UNUSED)
{
    fm_directory_view_notify_selection_changed (view);
}

static void
fm_widget_view_select_all (FMDirectoryView *view G_GNUC_UNUSED)
{
}

static void
fm_widget_view_reveal_selection (FMDirectoryView *view G_GNUC_UNUSED)
{
}

static void
fm_widget_view_merge_menus (FMDirectoryView *view)
{
    FM_DIRECTORY_VIEW_CLASS (fm_widget_view_parent_class)->merge_menus(view);
}

static void
fm_widget_view_update_menus (FMDirectoryView *view)
{
    FM_DIRECTORY_VIEW_CLASS (fm_widget_view_parent_class)->update_menus(view);
}

/* Reset sort criteria and zoom level to match defaults */
static void
fm_widget_view_reset_to_defaults (FMDirectoryView *view G_GNUC_UNUSED)
{
}

static void
fm_widget_view_bump_zoom_level (FMDirectoryView *view G_GNUC_UNUSED,
				int              zoom_increment G_GNUC_UNUSED)
{
}

static BaulZoomLevel
fm_widget_view_get_zoom_level (FMDirectoryView *view G_GNUC_UNUSED)
{
    return BAUL_ZOOM_LEVEL_STANDARD;
}

static void
fm_widget_view_zoom_to_level (FMDirectoryView *view G_GNUC_UNUSED,
			      BaulZoomLevel    zoom_level G_GNUC_UNUSED)
{
}

static void
fm_widget_view_restore_default_zoom_level (FMDirectoryView *view G_GNUC_UNUSED)
{
}

static gboolean
fm_widget_view_can_zoom_in (FMDirectoryView *view G_GNUC_UNUSED)
{
    return FALSE;
}

static gboolean
fm_widget_view_can_zoom_out (FMDirectoryView *view G_GNUC_UNUSED)
{
    return FALSE;
}

static void
fm_widget_view_start_renaming_file (FMDirectoryView *view G_GNUC_UNUSED,
				    BaulFile        *file G_GNUC_UNUSED,
				    gboolean         select_all G_GNUC_UNUSED)
{
}

static void
fm_widget_view_click_policy_changed (FMDirectoryView *directory_view G_GNUC_UNUSED)
{
}

static int
fm_widget_view_compare_files (FMDirectoryView *view G_GNUC_UNUSED,
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
fm_widget_view_using_manual_layout (FMDirectoryView *view G_GNUC_UNUSED)
{
    return FALSE;
}

static void
fm_widget_view_end_loading (FMDirectoryView *view G_GNUC_UNUSED,
			    gboolean         all_files_seen G_GNUC_UNUSED)
{
}

static void
fm_widget_view_finalize (GObject *object)
{
    G_OBJECT_CLASS (fm_widget_view_parent_class)->finalize (object);
}

static void
fm_widget_view_emblems_changed (FMDirectoryView *directory_view G_GNUC_UNUSED)
{
}

static char *
fm_widget_view_get_first_visible_file (BaulView *view)
{
    FMWidgetView *widget_view;

    widget_view = FM_WIDGET_VIEW (view);
    g_return_val_if_fail (FM_IS_WIDGET_VIEW(view), NULL);
    g_return_val_if_fail (BAUL_IS_WIDGET_VIEW_PROVIDER (widget_view->provider), NULL);

    return baul_widget_view_provider_get_first_visible_file (widget_view->provider);
}

static void
fm_widget_view_scroll_to_file (BaulView   *view G_GNUC_UNUSED,
			       const char *uri G_GNUC_UNUSED)
{
}

static void
fm_widget_view_grab_focus (BaulView *view)
{
    ctk_widget_grab_focus (CTK_WIDGET (view));
}

static void
fm_widget_view_sort_directories_first_changed (FMDirectoryView *view G_GNUC_UNUSED)
{
}

static void
fm_widget_view_class_init (FMWidgetViewClass *class)
{
    FMDirectoryViewClass *fm_directory_view_class;

    fm_directory_view_class = FM_DIRECTORY_VIEW_CLASS (class);

    G_OBJECT_CLASS (class)->finalize = fm_widget_view_finalize;

    fm_directory_view_class->add_file = fm_widget_view_add_file;
    fm_directory_view_class->begin_loading = fm_widget_view_begin_loading;
    fm_directory_view_class->bump_zoom_level = fm_widget_view_bump_zoom_level;
    fm_directory_view_class->can_zoom_in = fm_widget_view_can_zoom_in;
    fm_directory_view_class->can_zoom_out = fm_widget_view_can_zoom_out;
    fm_directory_view_class->click_policy_changed = fm_widget_view_click_policy_changed;
    fm_directory_view_class->clear = fm_widget_view_clear;
    fm_directory_view_class->file_changed = fm_widget_view_file_changed;
    fm_directory_view_class->get_background_widget = fm_widget_view_get_background_widget;
    fm_directory_view_class->get_selection = fm_widget_view_get_selection;
    fm_directory_view_class->get_selection_for_file_transfer = fm_widget_view_get_selection_for_file_transfer;
    fm_directory_view_class->get_item_count = fm_widget_view_get_item_count;
    fm_directory_view_class->is_empty = fm_widget_view_is_empty;
    fm_directory_view_class->remove_file = fm_widget_view_remove_file;
    fm_directory_view_class->merge_menus = fm_widget_view_merge_menus;
    fm_directory_view_class->update_menus = fm_widget_view_update_menus;
    fm_directory_view_class->reset_to_defaults = fm_widget_view_reset_to_defaults;
    fm_directory_view_class->restore_default_zoom_level = fm_widget_view_restore_default_zoom_level;
    fm_directory_view_class->reveal_selection = fm_widget_view_reveal_selection;
    fm_directory_view_class->select_all = fm_widget_view_select_all;
    fm_directory_view_class->set_selection = fm_widget_view_set_selection;
    fm_directory_view_class->compare_files = fm_widget_view_compare_files;
    fm_directory_view_class->sort_directories_first_changed = fm_widget_view_sort_directories_first_changed;
    fm_directory_view_class->start_renaming_file = fm_widget_view_start_renaming_file;
    fm_directory_view_class->get_zoom_level = fm_widget_view_get_zoom_level;
    fm_directory_view_class->zoom_to_level = fm_widget_view_zoom_to_level;
    fm_directory_view_class->emblems_changed = fm_widget_view_emblems_changed;
    fm_directory_view_class->end_file_changes = fm_widget_view_end_file_changes;
    fm_directory_view_class->using_manual_layout = fm_widget_view_using_manual_layout;
    fm_directory_view_class->end_loading = fm_widget_view_end_loading;
}

static const char *
fm_widget_view_get_id (BaulView *view G_GNUC_UNUSED)
{
    return FM_WIDGET_VIEW_ID;
}


static void
fm_widget_view_iface_init (BaulViewIface *iface)
{
    fm_directory_view_init_view_iface (iface);

    iface->get_view_id = fm_widget_view_get_id;
    iface->get_first_visible_file = fm_widget_view_get_first_visible_file;
    iface->scroll_to_file = fm_widget_view_scroll_to_file;
    iface->get_title = NULL;
    iface->grab_focus = fm_widget_view_grab_focus;
}


static void
fm_widget_view_init (FMWidgetView *widget_view)
{
    widget_view->provider = NULL;
}

static BaulView *
fm_widget_view_create (BaulWindowSlotInfo *slot)
{
    FMWidgetView *view;
    g_assert (BAUL_IS_WINDOW_SLOT_INFO (slot));

    view = g_object_new (FM_TYPE_WIDGET_VIEW,
                         "window-slot", slot,
                         NULL);

    return BAUL_VIEW (view);
}

static gboolean
fm_widget_view_supports_uri (const char *uri,
                            GFileType file_type,
                            const char *mime_type)
{
    GList *providers, *l;
    gboolean result = FALSE;

    providers = baul_extensions_get_for_type (BAUL_TYPE_WIDGET_VIEW_PROVIDER);

    for (l = providers; l != NULL; l = l->next)
    {
        BaulWidgetViewProvider *provider;

        provider = BAUL_WIDGET_VIEW_PROVIDER (l->data);
        if (baul_widget_view_provider_supports_uri (provider, uri, file_type, mime_type)) {
            result = TRUE;
        }
    }
    baul_module_extension_list_free (providers);

    return result;
}

static BaulViewInfo fm_widget_view =
{
    .id = FM_WIDGET_VIEW_ID,
    .view_combo_label = N_("Widget View"),
    .view_menu_label_with_mnemonic = N_("_Widget View"),
    .error_label = N_("The widget view encountered an error."),
    .startup_error_label = N_("The widget view encountered an error while starting up."),
    .display_location_label = N_("Display this location with the widget view."),
    .create = fm_widget_view_create,
    .supports_uri = fm_widget_view_supports_uri
};

void
fm_widget_view_register (void)
{
    fm_widget_view.view_combo_label = _(fm_widget_view.view_combo_label);
    fm_widget_view.view_menu_label_with_mnemonic = _(fm_widget_view.view_menu_label_with_mnemonic);
    fm_widget_view.error_label = _(fm_widget_view.error_label);
    fm_widget_view.startup_error_label = _(fm_widget_view.startup_error_label);
    fm_widget_view.display_location_label = _(fm_widget_view.display_location_label);
    fm_widget_view.single_view = TRUE;
    baul_view_factory_register (&fm_widget_view);
}
