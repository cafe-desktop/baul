/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 *  Baul
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
 *  Authors : Mr Jamie McCracken (jamiemcc at blueyonder dot co dot uk)
 *            Cosimo Cecchi <cosimoc@gnome.org>
 *
 */

#include <config.h>
#include <cairo-gobject.h>

#include <cdk/cdkkeysyms.h>
#include <ctk/ctk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <libnotify/notify.h>

#include <eel/eel-debug.h>
#include <eel/eel-ctk-extensions.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-graphic-effects.h>
#include <eel/eel-string.h>
#include <eel/eel-stock-dialogs.h>

#include <libbaul-private/baul-debug-log.h>
#include <libbaul-private/baul-dnd.h>
#include <libbaul-private/baul-bookmark.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-sidebar-provider.h>
#include <libbaul-private/baul-module.h>
#include <libbaul-private/baul-file.h>
#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-file-operations.h>
#include <libbaul-private/baul-trash-monitor.h>
#include <libbaul-private/baul-icon-names.h>
#include <libbaul-private/baul-autorun.h>
#include <libbaul-private/baul-window-info.h>
#include <libbaul-private/baul-window-slot-info.h>

#include "baul-bookmark-list.h"
#include "baul-places-sidebar.h"
#include "baul-window.h"

#define EJECT_BUTTON_XPAD 6
#define ICON_CELL_XPAD 6

typedef struct
{
    CtkScrolledWindow  parent;
    CtkTreeView        *tree_view;
    CtkCellRenderer    *eject_icon_cell_renderer;
    char               *uri;
    CtkListStore       *store;
    CtkTreeModel       *filter_model;
    BaulWindowInfo *window;
    BaulBookmarkList *bookmarks;
    GVolumeMonitor *volume_monitor;

    gboolean devices_header_added;
    gboolean bookmarks_header_added;

    /* DnD */
    GList     *drag_list;
    gboolean  drag_data_received;
    int       drag_data_info;
    gboolean  drop_occured;

    CtkWidget *popup_menu;
    CtkWidget *popup_menu_open_in_new_tab_item;
    CtkWidget *popup_menu_remove_item;
    CtkWidget *popup_menu_rename_item;
    CtkWidget *popup_menu_separator_item;
    CtkWidget *popup_menu_mount_item;
    CtkWidget *popup_menu_unmount_item;
    CtkWidget *popup_menu_eject_item;
    CtkWidget *popup_menu_rescan_item;
    CtkWidget *popup_menu_format_item;
    CtkWidget *popup_menu_empty_trash_item;
    CtkWidget *popup_menu_start_item;
    CtkWidget *popup_menu_stop_item;

    /* volume mounting - delayed open process */
    gboolean mounting;
    BaulWindowSlotInfo *go_to_after_mount_slot;
    BaulWindowOpenFlags go_to_after_mount_flags;

    CtkTreePath *eject_highlight_path;
} BaulPlacesSidebar;

typedef struct
{
    CtkScrolledWindowClass parent;
} BaulPlacesSidebarClass;

typedef struct
{
    GObject parent;
} BaulPlacesSidebarProvider;

typedef struct
{
    GObjectClass parent;
} BaulPlacesSidebarProviderClass;

enum
{
    PLACES_SIDEBAR_COLUMN_ROW_TYPE,
    PLACES_SIDEBAR_COLUMN_URI,
    PLACES_SIDEBAR_COLUMN_DRIVE,
    PLACES_SIDEBAR_COLUMN_VOLUME,
    PLACES_SIDEBAR_COLUMN_MOUNT,
    PLACES_SIDEBAR_COLUMN_NAME,
    PLACES_SIDEBAR_COLUMN_ICON,
    PLACES_SIDEBAR_COLUMN_INDEX,
    PLACES_SIDEBAR_COLUMN_EJECT,
    PLACES_SIDEBAR_COLUMN_NO_EJECT,
    PLACES_SIDEBAR_COLUMN_BOOKMARK,
    PLACES_SIDEBAR_COLUMN_TOOLTIP,
    PLACES_SIDEBAR_COLUMN_EJECT_ICON,
    PLACES_SIDEBAR_COLUMN_SECTION_TYPE,
    PLACES_SIDEBAR_COLUMN_HEADING_TEXT,

    PLACES_SIDEBAR_COLUMN_COUNT
};

typedef enum
{
    PLACES_BUILT_IN,
    PLACES_MOUNTED_VOLUME,
    PLACES_BOOKMARK,
    PLACES_HEADING,
} PlaceType;

typedef enum {
    SECTION_COMPUTER,
    SECTION_DEVICES,
    SECTION_BOOKMARKS,
    SECTION_NETWORK,
} SectionType;

static void  baul_places_sidebar_iface_init        (BaulSidebarIface         *iface);
static void  sidebar_provider_iface_init               (BaulSidebarProviderIface *iface);
static GType baul_places_sidebar_provider_get_type (void);
static void  open_selected_bookmark                    (BaulPlacesSidebar        *sidebar,
        CtkTreeModel                 *model,
        CtkTreePath                  *path,
        BaulWindowOpenFlags flags);

static void  baul_places_sidebar_style_updated         (CtkWidget                    *widget);

static gboolean eject_or_unmount_bookmark              (BaulPlacesSidebar *sidebar,
        CtkTreePath *path);
static gboolean eject_or_unmount_selection             (BaulPlacesSidebar *sidebar);
static void  check_unmount_and_eject                   (GMount *mount,
        GVolume *volume,
        GDrive *drive,
        gboolean *show_unmount,
        gboolean *show_eject);

static void bookmarks_check_popup_sensitivity          (BaulPlacesSidebar *sidebar);

/* Identifiers for target types */
enum
{
    CTK_TREE_MODEL_ROW,
    TEXT_URI_LIST
};

/* Target types for dragging from the shortcuts list */
static const CtkTargetEntry baul_shortcuts_source_targets[] =
{
    { "CTK_TREE_MODEL_ROW", CTK_TARGET_SAME_WIDGET, CTK_TREE_MODEL_ROW }
};

/* Target types for dropping into the shortcuts list */
static const CtkTargetEntry baul_shortcuts_drop_targets [] =
{
    { "CTK_TREE_MODEL_ROW", CTK_TARGET_SAME_WIDGET, CTK_TREE_MODEL_ROW },
    { "text/uri-list", 0, TEXT_URI_LIST }
};

/* Drag and drop interface declarations */
typedef struct
{
    CtkTreeModelFilter parent;

    BaulPlacesSidebar *sidebar;
} BaulShortcutsModelFilter;

typedef struct
{
    CtkTreeModelFilterClass parent_class;
} BaulShortcutsModelFilterClass;

#define BAUL_SHORTCUTS_MODEL_FILTER_TYPE (_baul_shortcuts_model_filter_get_type ())
#define BAUL_SHORTCUTS_MODEL_FILTER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_SHORTCUTS_MODEL_FILTER_TYPE, BaulShortcutsModelFilter))

GType _baul_shortcuts_model_filter_get_type (void);
static void baul_shortcuts_model_filter_drag_source_iface_init (CtkTreeDragSourceIface *iface);

G_DEFINE_TYPE_WITH_CODE (BaulShortcutsModelFilter,
                         _baul_shortcuts_model_filter,
                         CTK_TYPE_TREE_MODEL_FILTER,
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_TREE_DRAG_SOURCE,
                                 baul_shortcuts_model_filter_drag_source_iface_init));

static CtkTreeModel *baul_shortcuts_model_filter_new (BaulPlacesSidebar *sidebar,
        CtkTreeModel          *child_model,
        CtkTreePath           *root);

G_DEFINE_TYPE_WITH_CODE (BaulPlacesSidebar, baul_places_sidebar, CTK_TYPE_SCROLLED_WINDOW,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_SIDEBAR,
                                 baul_places_sidebar_iface_init));

G_DEFINE_TYPE_WITH_CODE (BaulPlacesSidebarProvider, baul_places_sidebar_provider, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_SIDEBAR_PROVIDER,
                                 sidebar_provider_iface_init));

static cairo_surface_t *
get_eject_icon (gboolean highlighted)
{
    GdkPixbuf *eject;
    cairo_surface_t *eject_surface;
    BaulIconInfo *eject_icon_info;
    int icon_size, icon_scale;

    icon_size = baul_get_icon_size_for_stock_size (CTK_ICON_SIZE_MENU);
    icon_scale = cdk_window_get_scale_factor (cdk_get_default_root_window ());

    eject_icon_info = baul_icon_info_lookup_from_name ("media-eject", icon_size, icon_scale);
    eject = baul_icon_info_get_pixbuf_at_size (eject_icon_info, icon_size);

    if (highlighted) {
        GdkPixbuf *high;
        high = eel_create_spotlight_pixbuf (eject);
        g_object_unref (eject);
        eject = high;
    }

    eject_surface = cdk_cairo_surface_create_from_pixbuf (eject, icon_scale, NULL);

    g_object_unref (eject_icon_info);
    g_object_unref (eject);

    return eject_surface;
}

static gboolean
is_built_in_bookmark (BaulFile *file)
{
    gboolean built_in;
    gint idx;

    built_in = FALSE;

    for (idx = 0; idx < G_USER_N_DIRECTORIES; idx++) {
        /* PUBLIC_SHARE and TEMPLATES are not in our built-in list */
        if (baul_file_is_user_special_directory (file, idx)) {
            if (idx != G_USER_DIRECTORY_PUBLIC_SHARE &&  idx != G_USER_DIRECTORY_TEMPLATES) {
                built_in = TRUE;
            }

            break;
        }
    }

    return built_in;
}

static CtkTreeIter
add_heading (BaulPlacesSidebar *sidebar,
         SectionType section_type,
         const gchar *title)
{
    CtkTreeIter iter, child_iter;

    ctk_list_store_append (sidebar->store, &iter);
    ctk_list_store_set (sidebar->store, &iter,
                PLACES_SIDEBAR_COLUMN_ROW_TYPE, PLACES_HEADING,
                PLACES_SIDEBAR_COLUMN_SECTION_TYPE, section_type,
                PLACES_SIDEBAR_COLUMN_HEADING_TEXT, title,
                PLACES_SIDEBAR_COLUMN_EJECT, FALSE,
                PLACES_SIDEBAR_COLUMN_NO_EJECT, TRUE,
                -1);

    ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (sidebar->filter_model));
    ctk_tree_model_filter_convert_child_iter_to_iter (CTK_TREE_MODEL_FILTER (sidebar->filter_model),
                              &child_iter,
                              &iter);

    return child_iter;
}

static void
check_heading_for_section (BaulPlacesSidebar *sidebar,
               SectionType section_type)
{
    switch (section_type) {
    case SECTION_DEVICES:
        if (!sidebar->devices_header_added) {
            add_heading (sidebar, SECTION_DEVICES,
                     _("Devices"));
            sidebar->devices_header_added = TRUE;
        }

        break;
    case SECTION_BOOKMARKS:
        if (!sidebar->bookmarks_header_added) {
            add_heading (sidebar, SECTION_BOOKMARKS,
                     _("Bookmarks"));
            sidebar->bookmarks_header_added = TRUE;
        }

        break;
    default:
        break;
    }
}

static CtkTreeIter
add_place (BaulPlacesSidebar *sidebar,
           PlaceType place_type,
           SectionType section_type,
           const char *name,
           GIcon *icon,
           const char *uri,
           GDrive *drive,
           GVolume *volume,
           GMount *mount,
           const int index,
           const char *tooltip)
{
    GdkPixbuf       *pixbuf;
    cairo_surface_t *surface;
    CtkTreeIter      iter, child_iter;
    cairo_surface_t *eject;
    BaulIconInfo    *icon_info;
    int              icon_size;
    int              icon_scale;
    gboolean         show_eject;
    gboolean         show_unmount;
    gboolean         show_eject_button;

    check_heading_for_section (sidebar, section_type);

    icon_size = baul_get_icon_size_for_stock_size (CTK_ICON_SIZE_MENU);
    icon_scale = ctk_widget_get_scale_factor (CTK_WIDGET (sidebar));
    icon_info = baul_icon_info_lookup (icon, icon_size, icon_scale);

    pixbuf = baul_icon_info_get_pixbuf_at_size (icon_info, icon_size);
    g_object_unref (icon_info);

    if (pixbuf != NULL)
    {
       surface = cdk_cairo_surface_create_from_pixbuf (pixbuf, icon_scale, NULL);
       g_object_unref (pixbuf);
    }
    else
    {
       surface = NULL;
    }

    check_unmount_and_eject (mount, volume, drive,
                             &show_unmount, &show_eject);

    if (show_unmount || show_eject)
    {
        g_assert (place_type != PLACES_BOOKMARK);
    }

    if (mount == NULL)
    {
        show_eject_button = FALSE;
    }
    else
    {
        show_eject_button = (show_unmount || show_eject);
    }

    if (show_eject_button) {
        eject = get_eject_icon (FALSE);
    } else {
        eject = NULL;
    }

    ctk_list_store_append (sidebar->store, &iter);
    ctk_list_store_set (sidebar->store, &iter,
                        PLACES_SIDEBAR_COLUMN_ICON, surface,
                        PLACES_SIDEBAR_COLUMN_NAME, name,
                        PLACES_SIDEBAR_COLUMN_URI, uri,
                        PLACES_SIDEBAR_COLUMN_DRIVE, drive,
                        PLACES_SIDEBAR_COLUMN_VOLUME, volume,
                        PLACES_SIDEBAR_COLUMN_MOUNT, mount,
                        PLACES_SIDEBAR_COLUMN_ROW_TYPE, place_type,
                        PLACES_SIDEBAR_COLUMN_INDEX, index,
                        PLACES_SIDEBAR_COLUMN_EJECT, show_eject_button,
                        PLACES_SIDEBAR_COLUMN_NO_EJECT, !show_eject_button,
                        PLACES_SIDEBAR_COLUMN_BOOKMARK, place_type != PLACES_BOOKMARK,
                        PLACES_SIDEBAR_COLUMN_TOOLTIP, tooltip,
                        PLACES_SIDEBAR_COLUMN_EJECT_ICON, eject,
                        PLACES_SIDEBAR_COLUMN_SECTION_TYPE, section_type,
                        -1);

    if (surface != NULL)
    {
       cairo_surface_destroy (surface);
    }
    ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (sidebar->filter_model));
    ctk_tree_model_filter_convert_child_iter_to_iter (CTK_TREE_MODEL_FILTER (sidebar->filter_model),
            &child_iter,
            &iter);
    return child_iter;
}

static void
compare_for_selection (BaulPlacesSidebar *sidebar,
                       const gchar *location,
                       const gchar *added_uri,
                       const gchar *last_uri,
                       CtkTreeIter *iter,
                       CtkTreePath **path)
{
    int res;

    res = g_strcmp0 (added_uri, last_uri);

    if (res == 0)
    {
        /* last_uri always comes first */
        if (*path != NULL)
        {
            ctk_tree_path_free (*path);
        }
        *path = ctk_tree_model_get_path (sidebar->filter_model,
                                         iter);
    }
    else if (g_strcmp0 (location, added_uri) == 0)
    {
        if (*path == NULL)
        {
            *path = ctk_tree_model_get_path (sidebar->filter_model,
                                             iter);
        }
    }
}

static void
update_places (BaulPlacesSidebar *sidebar)
{
    BaulBookmark *bookmark;
    CtkTreeSelection *selection;
    CtkTreeIter last_iter;
    CtkTreePath *select_path;
    CtkTreeModel *model;
    GVolumeMonitor *volume_monitor;
    GList *mounts, *l, *ll;
    GMount *mount;
    GList *drives;
    GDrive *drive;
    GList *volumes;
    GVolume *volume;
    int bookmark_count, index;
    char *location, *mount_uri, *name, *desktop_path, *last_uri;
    const gchar *path;
    GIcon *icon;
    GFile *root;
    BaulWindowSlotInfo *slot;
    char *tooltip;
    GList *network_mounts;
    GList *xdg_dirs;
    BaulFile *file;

    model = NULL;
    last_uri = NULL;
    select_path = NULL;
    bookmark = NULL;

    selection = ctk_tree_view_get_selection (sidebar->tree_view);
    if (ctk_tree_selection_get_selected (selection, &model, &last_iter))
    {
        ctk_tree_model_get (model,
                            &last_iter,
                            PLACES_SIDEBAR_COLUMN_URI, &last_uri, -1);
    }
    ctk_list_store_clear (sidebar->store);

    sidebar->devices_header_added = FALSE;
    sidebar->bookmarks_header_added = FALSE;

    slot = baul_window_info_get_active_slot (sidebar->window);
    location = baul_window_slot_info_get_current_location (slot);

    volume_monitor = sidebar->volume_monitor;

    /* COMPUTER */
    last_iter = add_heading (sidebar, SECTION_COMPUTER,
                             _("Computer"));

    /* add built in bookmarks */

    /* home folder */
    char *display_name;

    mount_uri = baul_get_home_directory_uri ();
    display_name = g_filename_display_basename (g_get_home_dir ());
    icon = g_themed_icon_new (BAUL_ICON_HOME);
    last_iter = add_place (sidebar, PLACES_BUILT_IN,
                           SECTION_COMPUTER,
                           display_name, icon,
                           mount_uri, NULL, NULL, NULL, 0,
                           _("Open your personal folder"));
    g_object_unref (icon);
    g_free (display_name);
    compare_for_selection (sidebar,
                           location, mount_uri, last_uri,
                           &last_iter, &select_path);
    g_free (mount_uri);

    /* desktop */
    desktop_path = baul_get_desktop_directory ();
    if (strcmp (g_get_home_dir(), desktop_path) != 0) {
	    mount_uri = g_filename_to_uri (desktop_path, NULL, NULL);
	    icon = g_themed_icon_new (BAUL_ICON_DESKTOP);
	    last_iter = add_place (sidebar, PLACES_BUILT_IN,
	                           SECTION_COMPUTER,
	                           _("Desktop"), icon,
	                           mount_uri, NULL, NULL, NULL, 0,
	                           _("Open the contents of your desktop in a folder"));
	    g_object_unref (icon);
	    compare_for_selection (sidebar,
	                           location, mount_uri, last_uri,
	                           &last_iter, &select_path);
	    g_free (mount_uri);
    }
	g_free (desktop_path);

    /* file system root */
    mount_uri = "file:///"; /* No need to strdup */
    icon = g_themed_icon_new (BAUL_ICON_FILESYSTEM);
    last_iter = add_place (sidebar, PLACES_BUILT_IN,
                           SECTION_COMPUTER,
                           _("File System"), icon,
                           mount_uri, NULL, NULL, NULL, 0,
                           _("Open the contents of the File System"));
    g_object_unref (icon);
    compare_for_selection (sidebar,
                           location, mount_uri, last_uri,
                           &last_iter, &select_path);


    /* XDG directories */
    xdg_dirs = NULL;
    for (index = 0; index < G_USER_N_DIRECTORIES; index++) {

        if (index == G_USER_DIRECTORY_DESKTOP ||
            index == G_USER_DIRECTORY_TEMPLATES ||
            index == G_USER_DIRECTORY_PUBLIC_SHARE) {
            continue;
        }

        path = g_get_user_special_dir (index);

        /* xdg resets special dirs to the home directory in case
         * it's not finiding what it expects. We don't want the home
         * to be added multiple times in that weird configuration.
         */
        if (path == NULL
            || g_strcmp0 (path, g_get_home_dir ()) == 0
            || g_list_find_custom (xdg_dirs, path, (GCompareFunc) g_strcmp0) != NULL) {
            continue;
        }

        root = g_file_new_for_path (path);
        name = g_file_get_basename (root);
        icon = baul_user_special_directory_get_gicon (index);
        mount_uri = g_file_get_uri (root);
        tooltip = g_file_get_parse_name (root);

        last_iter = add_place (sidebar, PLACES_BUILT_IN,
                               SECTION_COMPUTER,
                               name, icon, mount_uri,
                               NULL, NULL, NULL, 0,
                               tooltip);
        compare_for_selection (sidebar,
                               location, mount_uri, last_uri,
                               &last_iter, &select_path);
        g_free (name);
        g_object_unref (root);
        g_object_unref (icon);
        g_free (mount_uri);
        g_free (tooltip);

        xdg_dirs = g_list_prepend (xdg_dirs, (char *)path);
    }
    g_list_free (xdg_dirs);

    mount_uri = "trash:///"; /* No need to strdup */
    icon = baul_trash_monitor_get_icon ();
    last_iter = add_place (sidebar, PLACES_BUILT_IN,
                           SECTION_COMPUTER,
                           _("Trash"), icon, mount_uri,
                           NULL, NULL, NULL, 0,
                           _("Open the trash"));
    compare_for_selection (sidebar,
                           location, mount_uri, last_uri,
                           &last_iter, &select_path);
    g_object_unref (icon);

    /* first go through all connected drives */
    drives = g_volume_monitor_get_connected_drives (volume_monitor);

    for (l = drives; l != NULL; l = l->next)
    {
        drive = l->data;

        volumes = g_drive_get_volumes (drive);
        if (volumes != NULL)
        {
            for (ll = volumes; ll != NULL; ll = ll->next)
            {
                volume = ll->data;
                mount = g_volume_get_mount (volume);
                if (mount != NULL)
                {
                    /* Show mounted volume in the sidebar */
                    icon = g_mount_get_icon (mount);
                    root = g_mount_get_default_location (mount);
                    mount_uri = g_file_get_uri (root);
                    name = g_mount_get_name (mount);
                    tooltip = g_file_get_parse_name (root);

                    last_iter = add_place (sidebar, PLACES_MOUNTED_VOLUME,
                                           SECTION_DEVICES,
                                           name, icon, mount_uri,
                                           drive, volume, mount, 0, tooltip);
                    compare_for_selection (sidebar,
                                           location, mount_uri, last_uri,
                                           &last_iter, &select_path);
                    g_object_unref (root);
                    g_object_unref (mount);
                    g_object_unref (icon);
                    g_free (tooltip);
                    g_free (name);
                    g_free (mount_uri);
                }
                else
                {
                    /* Do show the unmounted volumes in the sidebar;
                     * this is so the user can mount it (in case automounting
                     * is off).
                     *
                     * Also, even if automounting is enabled, this gives a visual
                     * cue that the user should remember to yank out the media if
                     * he just unmounted it.
                     */
                    icon = g_volume_get_icon (volume);
                    name = g_volume_get_name (volume);
                    tooltip = g_strdup_printf (_("Mount and open %s"), name);

                    last_iter = add_place (sidebar, PLACES_MOUNTED_VOLUME,
                                           SECTION_DEVICES,
                                           name, icon, NULL,
                                           drive, volume, NULL, 0, tooltip);
                    g_object_unref (icon);
                    g_free (name);
                    g_free (tooltip);
                }
                g_object_unref (volume);
            }
            g_list_free (volumes);
        }
        else
        {
            if (g_drive_is_media_removable (drive) && !g_drive_is_media_check_automatic (drive))
            {
                /* If the drive has no mountable volumes and we cannot detect media change.. we
                 * display the drive in the sidebar so the user can manually poll the drive by
                 * right clicking and selecting "Rescan..."
                 *
                 * This is mainly for drives like floppies where media detection doesn't
                 * work.. but it's also for human beings who like to turn off media detection
                 * in the OS to save battery juice.
                 */
                icon = g_drive_get_icon (drive);
                name = g_drive_get_name (drive);
                tooltip = g_strdup_printf (_("Mount and open %s"), name);

                last_iter = add_place (sidebar, PLACES_BUILT_IN,
                                       SECTION_DEVICES,
                                       name, icon, NULL,
                                       drive, NULL, NULL, 0, tooltip);
                g_object_unref (icon);
                g_free (tooltip);
                g_free (name);
            }
        }
        g_object_unref (drive);
    }
    g_list_free (drives);

    /* add all volumes that is not associated with a drive */
    volumes = g_volume_monitor_get_volumes (volume_monitor);
    for (l = volumes; l != NULL; l = l->next)
    {
        volume = l->data;
        drive = g_volume_get_drive (volume);
        if (drive != NULL)
        {
            g_object_unref (volume);
            g_object_unref (drive);
            continue;
        }
        mount = g_volume_get_mount (volume);
        if (mount != NULL)
        {
            icon = g_mount_get_icon (mount);
            root = g_mount_get_default_location (mount);
            mount_uri = g_file_get_uri (root);
            tooltip = g_file_get_parse_name (root);
            g_object_unref (root);
            name = g_mount_get_name (mount);
            last_iter = add_place (sidebar, PLACES_MOUNTED_VOLUME,
                                   SECTION_DEVICES,
                                   name, icon, mount_uri,
                                   NULL, volume, mount, 0, tooltip);
            compare_for_selection (sidebar,
                                   location, mount_uri, last_uri,
                                   &last_iter, &select_path);
            g_object_unref (mount);
            g_object_unref (icon);
            g_free (name);
            g_free (tooltip);
            g_free (mount_uri);
        }
        else
        {
            /* see comment above in why we add an icon for an unmounted mountable volume */
            icon = g_volume_get_icon (volume);
            name = g_volume_get_name (volume);
            last_iter = add_place (sidebar, PLACES_MOUNTED_VOLUME,
                                   SECTION_DEVICES,
                                   name, icon, NULL,
                                   NULL, volume, NULL, 0, name);
            g_object_unref (icon);
            g_free (name);
        }
        g_object_unref (volume);
    }
    g_list_free (volumes);

    /* add mounts that has no volume (/etc/mtab mounts, ftp, sftp,...) */
    network_mounts = NULL;
    mounts = g_volume_monitor_get_mounts (volume_monitor);

    for (l = mounts; l != NULL; l = l->next)
    {
        mount = l->data;
        if (g_mount_is_shadowed (mount))
        {
            g_object_unref (mount);
            continue;
        }
        volume = g_mount_get_volume (mount);
        if (volume != NULL)
        {
            g_object_unref (volume);
            g_object_unref (mount);
            continue;
        }
        root = g_mount_get_default_location (mount);

        if (!g_file_is_native (root)) {
            network_mounts = g_list_prepend (network_mounts, g_object_ref (mount));
            continue;
        }

        icon = g_mount_get_icon (mount);
        mount_uri = g_file_get_uri (root);
        name = g_mount_get_name (mount);
        tooltip = g_file_get_parse_name (root);
        last_iter = add_place (sidebar, PLACES_MOUNTED_VOLUME,
                               SECTION_COMPUTER,
                               name, icon, mount_uri,
                               NULL, NULL, mount, 0, tooltip);
        compare_for_selection (sidebar,
                               location, mount_uri, last_uri,
                               &last_iter, &select_path);
        g_object_unref (root);
        g_object_unref (mount);
        g_object_unref (icon);
        g_free (name);
        g_free (mount_uri);
        g_free (tooltip);
    }
    g_list_free (mounts);


    /* add bookmarks */
    bookmark_count = baul_bookmark_list_length (sidebar->bookmarks);

    for (index = 0; index < bookmark_count; ++index) {
        bookmark = baul_bookmark_list_item_at (sidebar->bookmarks, index);

        if (baul_bookmark_uri_known_not_to_exist (bookmark)) {
            continue;
        }

        root = baul_bookmark_get_location (bookmark);
        file = baul_file_get (root);

        if (is_built_in_bookmark (file)) {
            g_object_unref (root);
            baul_file_unref (file);
            continue;
        }

        name = baul_bookmark_get_name (bookmark);
        icon = baul_bookmark_get_icon (bookmark);
        mount_uri = baul_bookmark_get_uri (bookmark);
        tooltip = g_file_get_parse_name (root);

        last_iter = add_place (sidebar, PLACES_BOOKMARK,
                               SECTION_BOOKMARKS,
                               name, icon, mount_uri,
                               NULL, NULL, NULL, index,
                               tooltip);
        compare_for_selection (sidebar,
                               location, mount_uri, last_uri,
                               &last_iter, &select_path);
        g_free (name);
        g_object_unref (root);
        g_object_unref (icon);
        g_free (mount_uri);
        g_free (tooltip);
    }

    /* network */
    last_iter = add_heading (sidebar, SECTION_NETWORK,
                             _("Network"));

    network_mounts = g_list_reverse (network_mounts);
    for (l = network_mounts; l != NULL; l = l->next) {
        mount = l->data;
        root = g_mount_get_default_location (mount);
        icon = g_mount_get_icon (mount);
        mount_uri = g_file_get_uri (root);
        name = g_mount_get_name (mount);
        tooltip = g_file_get_parse_name (root);
        last_iter = add_place (sidebar, PLACES_MOUNTED_VOLUME,
                               SECTION_NETWORK,
                               name, icon, mount_uri,
                               NULL, NULL, mount, 0, tooltip);
        compare_for_selection (sidebar,
                               location, mount_uri, last_uri,
                               &last_iter, &select_path);
        g_object_unref (root);
        g_object_unref (mount);
        g_object_unref (icon);
        g_free (name);
        g_free (mount_uri);
        g_free (tooltip);
    }

    g_list_free_full (network_mounts, g_object_unref);

    /* network:// */
    mount_uri = "network:///"; /* No need to strdup */
    icon = g_themed_icon_new (BAUL_ICON_NETWORK);
    last_iter = add_place (sidebar, PLACES_BUILT_IN,
                           SECTION_NETWORK,
                           _("Browse Network"), icon,
                           mount_uri, NULL, NULL, NULL, 0,
                           _("Browse the contents of the network"));
    g_object_unref (icon);
    compare_for_selection (sidebar,
                           location, mount_uri, last_uri,
                           &last_iter, &select_path);

    g_free (location);

    if (select_path != NULL) {
        ctk_tree_selection_select_path (selection, select_path);
    }

    if (select_path != NULL) {
        ctk_tree_path_free (select_path);
    }

    g_free (last_uri);
}

static void
mount_added_callback (GVolumeMonitor    *volume_monitor G_GNUC_UNUSED,
		      GMount            *mount G_GNUC_UNUSED,
		      BaulPlacesSidebar *sidebar)
{
    update_places (sidebar);
}

static void
mount_removed_callback (GVolumeMonitor    *volume_monitor G_GNUC_UNUSED,
			GMount            *mount G_GNUC_UNUSED,
			BaulPlacesSidebar *sidebar)
{
    update_places (sidebar);
}

static void
mount_changed_callback (GVolumeMonitor    *volume_monitor G_GNUC_UNUSED,
			GMount            *mount G_GNUC_UNUSED,
			BaulPlacesSidebar *sidebar)
{
    update_places (sidebar);
}

static void
volume_added_callback (GVolumeMonitor    *volume_monitor G_GNUC_UNUSED,
		       GVolume           *volume G_GNUC_UNUSED,
		       BaulPlacesSidebar *sidebar)
{
    update_places (sidebar);
}

static void
volume_removed_callback (GVolumeMonitor    *volume_monitor G_GNUC_UNUSED,
			 GVolume           *volume G_GNUC_UNUSED,
			 BaulPlacesSidebar *sidebar)
{
    update_places (sidebar);
}

static void
volume_changed_callback (GVolumeMonitor    *volume_monitor G_GNUC_UNUSED,
			 GVolume           *volume G_GNUC_UNUSED,
			 BaulPlacesSidebar *sidebar)
{
    update_places (sidebar);
}

static void
drive_disconnected_callback (GVolumeMonitor    *volume_monitor G_GNUC_UNUSED,
			     GDrive            *drive G_GNUC_UNUSED,
			     BaulPlacesSidebar *sidebar)
{
    update_places (sidebar);
}

static void
drive_connected_callback (GVolumeMonitor    *volume_monitor G_GNUC_UNUSED,
			  GDrive            *drive G_GNUC_UNUSED,
			  BaulPlacesSidebar *sidebar)
{
    update_places (sidebar);
}

static void
drive_changed_callback (GVolumeMonitor    *volume_monitor G_GNUC_UNUSED,
			GDrive            *drive G_GNUC_UNUSED,
			BaulPlacesSidebar *sidebar)
{
    update_places (sidebar);
}

static gboolean
over_eject_button (BaulPlacesSidebar *sidebar,
                   gint x,
                   gint y,
                   CtkTreePath **path)
{
    CtkTreeViewColumn *column;
    int width, x_offset, hseparator;
    int eject_button_size;
    gboolean show_eject;
    CtkTreeIter iter;
    CtkTreeModel *model;

    *path = NULL;
    model = ctk_tree_view_get_model (sidebar->tree_view);

   if (ctk_tree_view_get_path_at_pos (sidebar->tree_view,
                                      x, y,
                                      path, &column, NULL, NULL)) {

        ctk_tree_model_get_iter (model, &iter, *path);
        ctk_tree_model_get (model, &iter,
                            PLACES_SIDEBAR_COLUMN_EJECT, &show_eject,
                            -1);

        if (!show_eject) {
            goto out;
        }

        ctk_widget_style_get (CTK_WIDGET (sidebar->tree_view),
                              "horizontal-separator",&hseparator,
                              NULL);
        /* Reload cell attributes for this particular row */
        ctk_tree_view_column_cell_set_cell_data (column,
                                                 model, &iter, FALSE, FALSE);

        ctk_tree_view_column_cell_get_position (column,
                                                sidebar->eject_icon_cell_renderer,
                                                &x_offset, &width);

        eject_button_size = baul_get_icon_size_for_stock_size (CTK_ICON_SIZE_MENU);

       /* This is kinda weird, but we have to do it to workaround ctk+ expanding
       * the eject cell renderer (even thought we told it not to) and we then
       * had to set it right-aligned */
        x_offset += width - hseparator - EJECT_BUTTON_XPAD - eject_button_size;

        if (x - x_offset >= 0 &&
        x - x_offset <= eject_button_size) {
            return TRUE;
        }
    }

out:
    if (*path != NULL) {
        ctk_tree_path_free (*path);
        *path = NULL;
    }

    return FALSE;
}

static gboolean
clicked_eject_button (BaulPlacesSidebar *sidebar,
                      CtkTreePath **path)
{
    CdkEvent *event = ctk_get_current_event ();
    CdkEventButton *button_event = (CdkEventButton *) event;

    if ((event->type == CDK_BUTTON_PRESS || event->type == CDK_BUTTON_RELEASE) &&
         over_eject_button (sidebar, button_event->x, button_event->y, path)) {
        return TRUE;
    }

    return FALSE;
}

static void
desktop_location_changed_callback (gpointer user_data)
{
    BaulPlacesSidebar *sidebar;

    sidebar = BAUL_PLACES_SIDEBAR (user_data);

    update_places (sidebar);
}

static void
loading_uri_callback (BaulWindowInfo    *window G_GNUC_UNUSED,
		      char              *location,
		      BaulPlacesSidebar *sidebar)
{
    CtkTreeIter       iter;
    gboolean          valid;
    char             *uri;

    if (strcmp (sidebar->uri, location) != 0)
    {
        CtkTreeSelection *selection;

        g_free (sidebar->uri);
        sidebar->uri = g_strdup (location);

        /* set selection if any place matches location */
        selection = ctk_tree_view_get_selection (sidebar->tree_view);
        ctk_tree_selection_unselect_all (selection);
        valid = ctk_tree_model_get_iter_first (sidebar->filter_model, &iter);

        while (valid)
        {
            ctk_tree_model_get (sidebar->filter_model, &iter,
                                PLACES_SIDEBAR_COLUMN_URI, &uri,
                                -1);
            if (uri != NULL)
            {
                if (strcmp (uri, location) == 0)
                {
                    g_free (uri);
                    ctk_tree_selection_select_iter (selection, &iter);
                    break;
                }
                g_free (uri);
            }
            valid = ctk_tree_model_iter_next (sidebar->filter_model, &iter);
        }
    }
}

/* Computes the appropriate row and position for dropping */
static gboolean
compute_drop_position (CtkTreeView *tree_view,
                       int                      x,
                       int                      y,
                       CtkTreePath            **path,
                       CtkTreeViewDropPosition *pos,
                       BaulPlacesSidebar *sidebar)
{
    CtkTreeModel *model;
    CtkTreeIter iter;
    PlaceType place_type;
    SectionType section_type;

    if (!ctk_tree_view_get_dest_row_at_pos (tree_view,
                                            x,
                                            y,
                                            path,
                                            pos)) {
        return FALSE;
    }

    model = ctk_tree_view_get_model (tree_view);

    ctk_tree_model_get_iter (model, &iter, *path);
    ctk_tree_model_get (model, &iter,
                        PLACES_SIDEBAR_COLUMN_ROW_TYPE, &place_type,
                        PLACES_SIDEBAR_COLUMN_SECTION_TYPE, &section_type,
                        -1);

    if (place_type == PLACES_HEADING &&
        section_type != SECTION_BOOKMARKS &&
        section_type != SECTION_NETWORK) {
        /* never drop on headings, but the bookmarks or network heading
         * is a special case, so we can create new bookmarks by dragging
         * at the beginning or end of the bookmark list.
         */
        ctk_tree_path_free (*path);
        *path = NULL;

        return FALSE;
    }

    if (section_type != SECTION_BOOKMARKS &&
        sidebar->drag_data_received &&
        sidebar->drag_data_info == CTK_TREE_MODEL_ROW) {
        /* don't allow dropping bookmarks into non-bookmark areas */
        ctk_tree_path_free (*path);
        *path = NULL;

        return FALSE;
    }

    /* drag to top or bottom of bookmark list to add a bookmark */
    if (place_type == PLACES_HEADING && section_type == SECTION_BOOKMARKS) {
        *pos = CTK_TREE_VIEW_DROP_AFTER;
    } else if (place_type == PLACES_HEADING && section_type == SECTION_NETWORK) {
        *pos = CTK_TREE_VIEW_DROP_BEFORE;
    } else {
        /* or else you want to drag items INTO the existing bookmarks */
        *pos = CTK_TREE_VIEW_DROP_INTO_OR_BEFORE;
    }

    if (*pos != CTK_TREE_VIEW_DROP_BEFORE &&
        sidebar->drag_data_received &&
        sidebar->drag_data_info == CTK_TREE_MODEL_ROW) {
        /* bookmark rows are never dragged into other bookmark rows */
        *pos = CTK_TREE_VIEW_DROP_AFTER;
    }

    return TRUE;
}

static gboolean
get_drag_data (CtkTreeView *tree_view,
               CdkDragContext *context,
               unsigned int time)
{
    CdkAtom target;

    target = ctk_drag_dest_find_target (CTK_WIDGET (tree_view),
                                        context,
                                        NULL);

    if (target == CDK_NONE)
    {
        return FALSE;
    }

    ctk_drag_get_data (CTK_WIDGET (tree_view),
                       context, target, time);

    return TRUE;
}

static void
free_drag_data (BaulPlacesSidebar *sidebar)
{
    sidebar->drag_data_received = FALSE;

    if (sidebar->drag_list)
    {
        baul_drag_destroy_selection_list (sidebar->drag_list);
        sidebar->drag_list = NULL;
    }
}

static gboolean
can_accept_file_as_bookmark (BaulFile *file)
{
    return (baul_file_is_directory (file) &&
            !is_built_in_bookmark (file));
}

static gboolean
can_accept_items_as_bookmarks (const GList *items)
{
    int max;
    BaulFile *file = NULL;

    /* Iterate through selection checking if item will get accepted as a bookmark.
     * If more than 100 items selected, return an over-optimistic result.
     */
    for (max = 100; items != NULL && max >= 0; items = items->next, max--)
    {
        char *uri;

        uri = ((BaulDragSelectionItem *)items->data)->uri;
        file = baul_file_get_by_uri (uri);
        if (!can_accept_file_as_bookmark (file))
        {
            baul_file_unref (file);
            return FALSE;
        }
        baul_file_unref (file);
    }

    return TRUE;
}

static gboolean
drag_motion_callback (CtkTreeView *tree_view,
                      CdkDragContext *context,
                      int x,
                      int y,
                      unsigned int time,
                      BaulPlacesSidebar *sidebar)
{
    CtkTreePath *path;
    CtkTreeViewDropPosition pos;
    int action = 0;
    CtkTreeIter iter;
    char *uri;
    gboolean res;

    if (!sidebar->drag_data_received)
    {
        if (!get_drag_data (tree_view, context, time))
        {
            return FALSE;
        }
    }

    path = NULL;
    res = compute_drop_position (tree_view, x, y, &path, &pos, sidebar);

    if (!res) {
        goto out;
    }

    if (pos == CTK_TREE_VIEW_DROP_BEFORE ||
        pos == CTK_TREE_VIEW_DROP_AFTER )
    {
        if (sidebar->drag_data_received &&
            sidebar->drag_data_info == CTK_TREE_MODEL_ROW)
        {
            action = CDK_ACTION_MOVE;
        }
        else if (can_accept_items_as_bookmarks (sidebar->drag_list))
        {
            action = CDK_ACTION_COPY;
        }
        else
        {
            action = 0;
        }
    }
    else
    {
        if (sidebar->drag_list == NULL)
        {
            action = 0;
        }
        else
        {
            ctk_tree_model_get_iter (sidebar->filter_model,
                                     &iter, path);
            ctk_tree_model_get (CTK_TREE_MODEL (sidebar->filter_model),
                                &iter,
                                PLACES_SIDEBAR_COLUMN_URI, &uri,
                                -1);
            baul_drag_default_drop_action_for_icons (context, uri,
                    sidebar->drag_list,
                    &action);
            g_free (uri);
        }
    }

    if (action != 0) {
        ctk_tree_view_set_drag_dest_row (tree_view, path, pos);
    }

    if (path != NULL) {
        ctk_tree_path_free (path);
    }

 out:
    g_signal_stop_emission_by_name (tree_view, "drag-motion");

    if (action != 0)
    {
        cdk_drag_status (context, action, time);
    }
    else
    {
        cdk_drag_status (context, 0, time);
    }

    return TRUE;
}

static void
drag_leave_callback (CtkTreeView       *tree_view,
		     CdkDragContext    *context G_GNUC_UNUSED,
		     unsigned int       time G_GNUC_UNUSED,
		     BaulPlacesSidebar *sidebar)
{
    free_drag_data (sidebar);
    ctk_tree_view_set_drag_dest_row (tree_view, NULL, CTK_TREE_VIEW_DROP_BEFORE);
    g_signal_stop_emission_by_name (tree_view, "drag-leave");
}

/* Parses a "text/uri-list" string and inserts its URIs as bookmarks */
static void
bookmarks_drop_uris (BaulPlacesSidebar *sidebar,
                     CtkSelectionData      *selection_data,
                     int                    position)
{
    BaulBookmark *bookmark;
    char *name;
    char **uris;
    int i;
    GFile *location;
    GIcon *icon;
    BaulFile *file = NULL;

    uris = ctk_selection_data_get_uris (selection_data);
    if (!uris)
        return;

    for (i = 0; uris[i]; i++)
    {
        char *uri;

        uri = uris[i];
        file = baul_file_get_by_uri (uri);

        if (!can_accept_file_as_bookmark (file))
        {
            baul_file_unref (file);
            continue;
        }

        uri = baul_file_get_drop_target_uri (file);
        location = g_file_new_for_uri (uri);
        baul_file_unref (file);

        name = baul_compute_title_for_location (location);
        icon = g_themed_icon_new (BAUL_ICON_FOLDER);
        bookmark = baul_bookmark_new (location, name, TRUE, icon);

        if (!baul_bookmark_list_contains (sidebar->bookmarks, bookmark))
        {
            baul_bookmark_list_insert_item (sidebar->bookmarks, bookmark, position++);
        }

        g_object_unref (location);
        g_object_unref (bookmark);
        g_object_unref (icon);
        g_free (name);
        g_free (uri);
    }

    g_strfreev (uris);
}

static GList *
uri_list_from_selection (GList *selection)
{
    GList *ret;
    GList *l;
    BaulDragSelectionItem *item = NULL;

    ret = NULL;
    for (l = selection; l != NULL; l = l->next)
    {
        item = l->data;
        ret = g_list_prepend (ret, item->uri);
    }

    return g_list_reverse (ret);
}

static GList*
build_selection_list (const char *data)
{
    GList *result;
    char **uris;
    int i;
    BaulDragSelectionItem *item = NULL;

    uris = g_uri_list_extract_uris (data);

    result = NULL;
    for (i = 0; uris[i]; i++)
    {
        char *uri;

        uri = uris[i];
        item = baul_drag_selection_item_new ();
        item->uri = g_strdup (uri);
        item->got_icon_position = FALSE;
        result = g_list_prepend (result, item);
    }

    g_strfreev (uris);

    return g_list_reverse (result);
}

static gboolean
get_selected_iter (BaulPlacesSidebar *sidebar,
                   CtkTreeIter *iter)
{
    CtkTreeSelection *selection;

    selection = ctk_tree_view_get_selection (sidebar->tree_view);

    return ctk_tree_selection_get_selected (selection, NULL, iter);
}

/* Reorders the selected bookmark to the specified position */
static void
reorder_bookmarks (BaulPlacesSidebar *sidebar,
                   int                new_position)
{
    CtkTreeIter iter;
    PlaceType type;
    int old_position;

    /* Get the selected path */

    if (!get_selected_iter (sidebar, &iter))
        return;

    ctk_tree_model_get (CTK_TREE_MODEL (sidebar->filter_model), &iter,
                        PLACES_SIDEBAR_COLUMN_ROW_TYPE, &type,
                        PLACES_SIDEBAR_COLUMN_INDEX, &old_position,
                        -1);

    if (type != PLACES_BOOKMARK ||
            old_position < 0 ||
            old_position >= baul_bookmark_list_length (sidebar->bookmarks))
    {
        return;
    }

    baul_bookmark_list_move_item (sidebar->bookmarks, old_position,
                                  new_position);
}

static void
drag_data_received_callback (CtkWidget *widget,
                             CdkDragContext *context,
                             int x,
                             int y,
                             CtkSelectionData *selection_data,
                             unsigned int info,
                             unsigned int time,
                             BaulPlacesSidebar *sidebar)
{
    CtkTreeView *tree_view;
    CtkTreePath *tree_path;
    CtkTreeViewDropPosition tree_pos;
    CtkTreeIter iter;
    int position;
    CtkTreeModel *model;
    char *drop_uri;
    GList *selection_list, *uris;
    PlaceType place_type;
    SectionType section_type;
    gboolean success;

    tree_view = CTK_TREE_VIEW (widget);

    if (!sidebar->drag_data_received)
    {
        if (ctk_selection_data_get_target (selection_data) != CDK_NONE &&
                info == TEXT_URI_LIST)
        {
            sidebar->drag_list = build_selection_list (ctk_selection_data_get_data (selection_data));
        }
        else
        {
            sidebar->drag_list = NULL;
        }
        sidebar->drag_data_received = TRUE;
        sidebar->drag_data_info = info;
    }

    g_signal_stop_emission_by_name (widget, "drag-data-received");

    if (!sidebar->drop_occured)
    {
        return;
    }

    /* Compute position */
    success = compute_drop_position (tree_view, x, y, &tree_path, &tree_pos, sidebar);
    if (!success)
        goto out;

    success = FALSE;

    if (tree_pos == CTK_TREE_VIEW_DROP_BEFORE ||
        tree_pos == CTK_TREE_VIEW_DROP_AFTER)
    {
        model = ctk_tree_view_get_model (tree_view);

        if (!ctk_tree_model_get_iter (model, &iter, tree_path))
        {
            goto out;
        }

        ctk_tree_model_get (model, &iter,
                            PLACES_SIDEBAR_COLUMN_SECTION_TYPE, &section_type,
                            PLACES_SIDEBAR_COLUMN_ROW_TYPE, &place_type,
                            PLACES_SIDEBAR_COLUMN_INDEX, &position,
                            -1);

        if (section_type != SECTION_BOOKMARKS &&
            !(section_type == SECTION_NETWORK && place_type == PLACES_HEADING)) {
            goto out;
        }

        if (section_type == SECTION_NETWORK && place_type == PLACES_HEADING &&
            tree_pos == CTK_TREE_VIEW_DROP_BEFORE) {
            position = baul_bookmark_list_length (sidebar->bookmarks);
        }

        if (tree_pos == CTK_TREE_VIEW_DROP_AFTER && place_type != PLACES_HEADING) {
            /* heading already has position 0 */
            position++;
        }

        switch (info)
        {
        case TEXT_URI_LIST:
            bookmarks_drop_uris (sidebar, selection_data, position);
            success = TRUE;
            break;
        case CTK_TREE_MODEL_ROW:
            reorder_bookmarks (sidebar, position);
            success = TRUE;
            break;
        default:
            g_assert_not_reached ();
            break;
        }
    }
    else
    {
        CdkDragAction real_action;

        /* file transfer requested */
        real_action = cdk_drag_context_get_selected_action (context);

        if (real_action == CDK_ACTION_ASK)
        {
            real_action =
                baul_drag_drop_action_ask (CTK_WIDGET (tree_view),
                                           cdk_drag_context_get_actions (context));
        }

        if (real_action > 0)
        {
            model = ctk_tree_view_get_model (tree_view);

            ctk_tree_model_get_iter (model, &iter, tree_path);
            ctk_tree_model_get (model, &iter,
                                PLACES_SIDEBAR_COLUMN_URI, &drop_uri,
                                -1);

            switch (info)
            {
            case TEXT_URI_LIST:
                selection_list = build_selection_list (ctk_selection_data_get_data (selection_data));
                uris = uri_list_from_selection (selection_list);
                baul_file_operations_copy_move (uris, NULL, drop_uri,
                                                real_action, CTK_WIDGET (tree_view),
                                                NULL, NULL);
                baul_drag_destroy_selection_list (selection_list);
                g_list_free (uris);
                success = TRUE;
                break;
            case CTK_TREE_MODEL_ROW:
                success = FALSE;
                break;
            default:
                g_assert_not_reached ();
                break;
            }

            g_free (drop_uri);
        }
    }

out:
    sidebar->drop_occured = FALSE;
    free_drag_data (sidebar);
    ctk_drag_finish (context, success, FALSE, time);

    ctk_tree_path_free (tree_path);
}

static gboolean
drag_drop_callback (CtkTreeView       *tree_view,
		    CdkDragContext    *context,
		    int                x G_GNUC_UNUSED,
		    int                y G_GNUC_UNUSED,
		    unsigned int       time,
		    BaulPlacesSidebar *sidebar)
{
    gboolean retval = FALSE;
    sidebar->drop_occured = TRUE;
    retval = get_drag_data (tree_view, context, time);
    g_signal_stop_emission_by_name (tree_view, "drag-drop");
    return retval;
}

/* Callback used when the file list's popup menu is detached */
static void
bookmarks_popup_menu_detach_cb (CtkWidget *attach_widget,
				CtkMenu   *menu G_GNUC_UNUSED)
{
    BaulPlacesSidebar *sidebar;

    sidebar = BAUL_PLACES_SIDEBAR (attach_widget);
    g_assert (BAUL_IS_PLACES_SIDEBAR (sidebar));

    sidebar->popup_menu = NULL;
    sidebar->popup_menu_remove_item = NULL;
    sidebar->popup_menu_rename_item = NULL;
    sidebar->popup_menu_separator_item = NULL;
    sidebar->popup_menu_mount_item = NULL;
    sidebar->popup_menu_unmount_item = NULL;
    sidebar->popup_menu_eject_item = NULL;
    sidebar->popup_menu_rescan_item = NULL;
    sidebar->popup_menu_format_item = NULL;
    sidebar->popup_menu_start_item = NULL;
    sidebar->popup_menu_stop_item = NULL;
    sidebar->popup_menu_empty_trash_item = NULL;
}

static void
check_unmount_and_eject (GMount *mount,
                         GVolume *volume,
                         GDrive *drive,
                         gboolean *show_unmount,
                         gboolean *show_eject)
{
    *show_unmount = FALSE;
    *show_eject = FALSE;

    if (drive != NULL)
    {
        *show_eject = g_drive_can_eject (drive);
    }

    if (volume != NULL)
    {
        *show_eject |= g_volume_can_eject (volume);
    }
    if (mount != NULL)
    {
        *show_eject |= g_mount_can_eject (mount);
        *show_unmount = g_mount_can_unmount (mount) && !*show_eject;
    }
}

static void
check_visibility (GMount           *mount,
                  GVolume          *volume,
                  GDrive           *drive,
                  gboolean         *show_mount,
                  gboolean         *show_unmount,
                  gboolean         *show_eject,
                  gboolean         *show_rescan,
                  gboolean         *show_format,
                  gboolean         *show_start,
                  gboolean         *show_stop)
{
    *show_mount = FALSE;
    *show_format = FALSE;
    *show_rescan = FALSE;
    *show_start = FALSE;
    *show_stop = FALSE;

    check_unmount_and_eject (mount, volume, drive, show_unmount, show_eject);

    if (drive != NULL)
    {
        if (g_drive_is_media_removable (drive) &&
                !g_drive_is_media_check_automatic (drive) &&
                g_drive_can_poll_for_media (drive))
            *show_rescan = TRUE;

        *show_start = g_drive_can_start (drive) || g_drive_can_start_degraded (drive);
        *show_stop  = g_drive_can_stop (drive);

        if (*show_stop)
            *show_unmount = FALSE;
    }

    if (volume != NULL)
    {
        if (mount == NULL)
            *show_mount = g_volume_can_mount (volume);
    }
}

static void
bookmarks_check_popup_sensitivity (BaulPlacesSidebar *sidebar)
{
    CtkTreeIter iter;
    PlaceType type;
    GDrive *drive = NULL;
    GVolume *volume = NULL;
    GMount *mount = NULL;
    gboolean show_mount;
    gboolean show_unmount;
    gboolean show_eject;
    gboolean show_rescan;
    gboolean show_format;
    gboolean show_start;
    gboolean show_stop;
    gboolean show_empty_trash;
    char *uri = NULL;

    type = PLACES_BUILT_IN;

    if (sidebar->popup_menu == NULL)
    {
        return;
    }

    if (get_selected_iter (sidebar, &iter))
    {
        ctk_tree_model_get (CTK_TREE_MODEL (sidebar->filter_model), &iter,
                            PLACES_SIDEBAR_COLUMN_ROW_TYPE, &type,
                            PLACES_SIDEBAR_COLUMN_DRIVE, &drive,
                            PLACES_SIDEBAR_COLUMN_VOLUME, &volume,
                            PLACES_SIDEBAR_COLUMN_MOUNT, &mount,
                            PLACES_SIDEBAR_COLUMN_URI, &uri,
                            -1);
    }

    ctk_widget_show (sidebar->popup_menu_open_in_new_tab_item);

    ctk_widget_set_sensitive (sidebar->popup_menu_remove_item, (type == PLACES_BOOKMARK));
    ctk_widget_set_sensitive (sidebar->popup_menu_rename_item, (type == PLACES_BOOKMARK));
    ctk_widget_set_sensitive (sidebar->popup_menu_empty_trash_item, !baul_trash_monitor_is_empty ());

    check_visibility (mount, volume, drive,
                      &show_mount, &show_unmount, &show_eject, &show_rescan, &show_format, &show_start, &show_stop);

    /* We actually want both eject and unmount since eject will unmount all volumes.
     * TODO: hide unmount if the drive only has a single mountable volume
     */

    show_empty_trash = (uri != NULL) &&
                       (!strcmp (uri, "trash:///"));

    ctk_widget_set_visible (sidebar->popup_menu_separator_item,
                              show_mount || show_unmount || show_eject || show_format || show_empty_trash);
    ctk_widget_set_visible (sidebar->popup_menu_mount_item, show_mount);
    ctk_widget_set_visible (sidebar->popup_menu_unmount_item, show_unmount);
    ctk_widget_set_visible (sidebar->popup_menu_eject_item, show_eject);
    ctk_widget_set_visible (sidebar->popup_menu_rescan_item, show_rescan);
    ctk_widget_set_visible (sidebar->popup_menu_format_item, show_format);
    ctk_widget_set_visible (sidebar->popup_menu_start_item, show_start);
    ctk_widget_set_visible (sidebar->popup_menu_stop_item, show_stop);
    ctk_widget_set_visible (sidebar->popup_menu_empty_trash_item, show_empty_trash);

    /* Adjust start/stop items to reflect the type of the drive */
    ctk_menu_item_set_label (CTK_MENU_ITEM (sidebar->popup_menu_start_item), _("_Start"));
    ctk_menu_item_set_label (CTK_MENU_ITEM (sidebar->popup_menu_stop_item), _("_Stop"));
    if ((show_start || show_stop) && drive != NULL)
    {
        switch (g_drive_get_start_stop_type (drive))
        {
        case G_DRIVE_START_STOP_TYPE_SHUTDOWN:
            /* start() for type G_DRIVE_START_STOP_TYPE_SHUTDOWN is normally not used */
            ctk_menu_item_set_label (CTK_MENU_ITEM (sidebar->popup_menu_start_item), _("_Power On"));
            ctk_menu_item_set_label (CTK_MENU_ITEM (sidebar->popup_menu_stop_item), _("_Safely Remove Drive"));
            break;
        case G_DRIVE_START_STOP_TYPE_NETWORK:
            ctk_menu_item_set_label (CTK_MENU_ITEM (sidebar->popup_menu_start_item), _("_Connect Drive"));
            ctk_menu_item_set_label (CTK_MENU_ITEM (sidebar->popup_menu_stop_item), _("_Disconnect Drive"));
            break;
        case G_DRIVE_START_STOP_TYPE_MULTIDISK:
            ctk_menu_item_set_label (CTK_MENU_ITEM (sidebar->popup_menu_start_item), _("_Start Multi-disk Device"));
            ctk_menu_item_set_label (CTK_MENU_ITEM (sidebar->popup_menu_stop_item), _("_Stop Multi-disk Device"));
            break;
        case G_DRIVE_START_STOP_TYPE_PASSWORD:
            /* stop() for type G_DRIVE_START_STOP_TYPE_PASSWORD is normally not used */
            ctk_menu_item_set_label (CTK_MENU_ITEM (sidebar->popup_menu_start_item), _("_Unlock Drive"));
            ctk_menu_item_set_label (CTK_MENU_ITEM (sidebar->popup_menu_stop_item), _("_Lock Drive"));
            break;

        default:
        case G_DRIVE_START_STOP_TYPE_UNKNOWN:
            /* uses defaults set above */
            break;
        }
    }


    g_free (uri);
}

/* Callback used when the selection in the shortcuts tree changes */
static void
bookmarks_selection_changed_cb (CtkTreeSelection  *selection G_GNUC_UNUSED,
				BaulPlacesSidebar *sidebar)
{
    bookmarks_check_popup_sensitivity (sidebar);
}

static void
volume_mounted_cb (GVolume *volume,
                   GObject *user_data)
{
    GMount *mount;
    BaulPlacesSidebar *sidebar;

    sidebar = BAUL_PLACES_SIDEBAR (user_data);

    sidebar->mounting = FALSE;

    mount = g_volume_get_mount (volume);
    if (mount != NULL)
    {
        GFile *location;

        location = g_mount_get_default_location (mount);

        if (sidebar->go_to_after_mount_slot != NULL)
        {
            if ((sidebar->go_to_after_mount_flags & BAUL_WINDOW_OPEN_FLAG_NEW_WINDOW) == 0)
            {
                baul_window_slot_info_open_location (sidebar->go_to_after_mount_slot, location,
                                                     BAUL_WINDOW_OPEN_ACCORDING_TO_MODE,
                                                     sidebar->go_to_after_mount_flags, NULL);
            }
            else
            {
                BaulWindow *cur, *new;

                cur = BAUL_WINDOW (sidebar->window);
                new = baul_application_create_navigation_window (cur->application,
                        ctk_window_get_screen (CTK_WINDOW (cur)));
                baul_window_go_to (new, location);
            }
        }

        g_object_unref (G_OBJECT (location));
        g_object_unref (G_OBJECT (mount));
    }


    eel_remove_weak_pointer (&(sidebar->go_to_after_mount_slot));
}

static void
drive_start_from_bookmark_cb (GObject      *source_object,
			      GAsyncResult *res,
			      gpointer      user_data G_GNUC_UNUSED)
{
    GError *error;

    error = NULL;
    if (!g_drive_start_finish (G_DRIVE (source_object), res, &error))
    {
        if (error->code != G_IO_ERROR_FAILED_HANDLED)
        {
            char *primary;
            char *name;

            name = g_drive_get_name (G_DRIVE (source_object));
            primary = g_strdup_printf (_("Unable to start %s"), name);
            g_free (name);
            eel_show_error_dialog (primary,
                                   error->message,
                                   NULL);
            g_free (primary);
        }
        g_error_free (error);
    }
}

static void
open_selected_bookmark (BaulPlacesSidebar   *sidebar,
                        CtkTreeModel        *model,
                        CtkTreePath         *path,
                        BaulWindowOpenFlags  flags)
{
    BaulWindowSlotInfo *slot;
    CtkTreeIter iter;
    char *uri;

    if (!path)
    {
        return;
    }

    if (!ctk_tree_model_get_iter (model, &iter, path))
    {
        return;
    }

    ctk_tree_model_get (model, &iter, PLACES_SIDEBAR_COLUMN_URI, &uri, -1);

    if (uri != NULL)
    {
        GFile *location;

        baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                        "activate from places sidebar window=%p: %s",
                        sidebar->window, uri);
        location = g_file_new_for_uri (uri);
        /* Navigate to the clicked location */
        if ((flags & BAUL_WINDOW_OPEN_FLAG_NEW_WINDOW) == 0)
        {
            slot = baul_window_info_get_active_slot (sidebar->window);
            baul_window_slot_info_open_location (slot, location,
                                                 BAUL_WINDOW_OPEN_ACCORDING_TO_MODE,
                                                 flags, NULL);
        }
        else
        {
            BaulWindow *cur, *new;

            cur = BAUL_WINDOW (sidebar->window);
            new = baul_application_create_navigation_window (cur->application,
                    ctk_window_get_screen (CTK_WINDOW (cur)));
            baul_window_go_to (new, location);
        }
        g_object_unref (location);
        g_free (uri);

    }
    else
    {
        GDrive *drive;
        GVolume *volume;

        ctk_tree_model_get (model, &iter,
                            PLACES_SIDEBAR_COLUMN_DRIVE, &drive,
                            PLACES_SIDEBAR_COLUMN_VOLUME, &volume,
                            -1);

        if (volume != NULL && !sidebar->mounting)
        {
            sidebar->mounting = TRUE;

            g_assert (sidebar->go_to_after_mount_slot == NULL);

            slot = baul_window_info_get_active_slot (sidebar->window);
            sidebar->go_to_after_mount_slot = slot;
            eel_add_weak_pointer (&(sidebar->go_to_after_mount_slot));

            sidebar->go_to_after_mount_flags = flags;

            baul_file_operations_mount_volume_full (NULL, volume, FALSE,
                                                    volume_mounted_cb,
                                                    G_OBJECT (sidebar));
        }
        else if (volume == NULL && drive != NULL &&
                 (g_drive_can_start (drive) || g_drive_can_start_degraded (drive)))
        {
            GMountOperation *mount_op;

            mount_op = ctk_mount_operation_new (CTK_WINDOW (ctk_widget_get_toplevel (CTK_WIDGET (sidebar))));
            g_drive_start (drive, G_DRIVE_START_NONE, mount_op, NULL, drive_start_from_bookmark_cb, NULL);
            g_object_unref (mount_op);
        }

        if (drive != NULL)
            g_object_unref (drive);
        if (volume != NULL)
            g_object_unref (volume);
    }
}

static void
open_shortcut_from_menu (BaulPlacesSidebar   *sidebar,
                         BaulWindowOpenFlags  flags)
{
    CtkTreeModel *model;
    CtkTreePath *path;

    model = ctk_tree_view_get_model (sidebar->tree_view);
    ctk_tree_view_get_cursor (sidebar->tree_view, &path, NULL);

    open_selected_bookmark (sidebar, model, path, flags);

    ctk_tree_path_free (path);
}

static void
open_shortcut_cb (CtkMenuItem       *item G_GNUC_UNUSED,
		  BaulPlacesSidebar *sidebar)
{
    open_shortcut_from_menu (sidebar, 0);
}

static void
open_shortcut_in_new_window_cb (CtkMenuItem       *item G_GNUC_UNUSED,
				BaulPlacesSidebar *sidebar)
{
    open_shortcut_from_menu (sidebar, BAUL_WINDOW_OPEN_FLAG_NEW_WINDOW);
}

static void
open_shortcut_in_new_tab_cb (CtkMenuItem       *item G_GNUC_UNUSED,
			     BaulPlacesSidebar *sidebar)
{
    open_shortcut_from_menu (sidebar, BAUL_WINDOW_OPEN_FLAG_NEW_TAB);
}

/* Rename the selected bookmark */
static void
rename_selected_bookmark (BaulPlacesSidebar *sidebar)
{
    CtkTreeIter iter;

    if (get_selected_iter (sidebar, &iter))
    {
        CtkTreePath *path;
        CtkTreeViewColumn *column;
        CtkCellRenderer *cell;
        GList *renderers;

        path = ctk_tree_model_get_path (CTK_TREE_MODEL (sidebar->filter_model), &iter);
        column = ctk_tree_view_get_column (CTK_TREE_VIEW (sidebar->tree_view), 0);
        renderers = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (column));
        cell = g_list_nth_data (renderers, 6);
        g_list_free (renderers);
        g_object_set (cell, "editable", TRUE, NULL);
        ctk_tree_view_set_cursor_on_cell (CTK_TREE_VIEW (sidebar->tree_view),
                                          path, column, cell, TRUE);
        ctk_tree_path_free (path);
    }
}

static void
rename_shortcut_cb (CtkMenuItem       *item G_GNUC_UNUSED,
		    BaulPlacesSidebar *sidebar)
{
    rename_selected_bookmark (sidebar);
}

/* Removes the selected bookmarks */
static void
remove_selected_bookmarks (BaulPlacesSidebar *sidebar)
{
    CtkTreeIter iter;
    PlaceType type;
    int index;

    if (!get_selected_iter (sidebar, &iter))
    {
        return;
    }

    ctk_tree_model_get (CTK_TREE_MODEL (sidebar->filter_model), &iter,
                        PLACES_SIDEBAR_COLUMN_ROW_TYPE, &type,
                        -1);

    if (type != PLACES_BOOKMARK)
    {
        return;
    }

    ctk_tree_model_get (CTK_TREE_MODEL (sidebar->filter_model), &iter,
                        PLACES_SIDEBAR_COLUMN_INDEX, &index,
                        -1);

    baul_bookmark_list_delete_item_at (sidebar->bookmarks, index);
}

static void
remove_shortcut_cb (CtkMenuItem       *item G_GNUC_UNUSED,
		    BaulPlacesSidebar *sidebar)
{
    remove_selected_bookmarks (sidebar);
}

static void
mount_shortcut_cb (CtkMenuItem       *item G_GNUC_UNUSED,
		   BaulPlacesSidebar *sidebar)
{
    CtkTreeIter iter;
    GVolume *volume;

    if (!get_selected_iter (sidebar, &iter))
    {
        return;
    }

    ctk_tree_model_get (CTK_TREE_MODEL (sidebar->filter_model), &iter,
                        PLACES_SIDEBAR_COLUMN_VOLUME, &volume,
                        -1);

    if (volume != NULL)
    {
        baul_file_operations_mount_volume (NULL, volume, FALSE);
        g_object_unref (volume);
    }
}

static void
unmount_done (gpointer data)
{
    BaulWindow *window;

    window = data;
    baul_window_info_set_initiated_unmount (window, FALSE);
    g_object_unref (window);
}

static void
do_unmount (GMount *mount,
            BaulPlacesSidebar *sidebar)
{
    if (mount != NULL)
    {
        baul_window_info_set_initiated_unmount (sidebar->window, TRUE);
        baul_file_operations_unmount_mount_full (NULL, mount, FALSE, TRUE,
                unmount_done,
                g_object_ref (sidebar->window));
    }
}

static void
do_unmount_selection (BaulPlacesSidebar *sidebar)
{
    CtkTreeIter iter;
    GMount *mount;

    if (!get_selected_iter (sidebar, &iter))
    {
        return;
    }

    ctk_tree_model_get (CTK_TREE_MODEL (sidebar->filter_model), &iter,
                        PLACES_SIDEBAR_COLUMN_MOUNT, &mount,
                        -1);

    if (mount != NULL)
    {
        do_unmount (mount, sidebar);
        g_object_unref (mount);
    }
}

static void
unmount_shortcut_cb (CtkMenuItem       *item G_GNUC_UNUSED,
		     BaulPlacesSidebar *sidebar)
{
    do_unmount_selection (sidebar);
}

static void
drive_eject_cb (GObject *source_object,
                GAsyncResult *res,
                gpointer user_data)
{
    BaulWindow *window;
    GError *error;

    window = user_data;
    baul_window_info_set_initiated_unmount (window, FALSE);
    g_object_unref (window);

    error = NULL;
    if (!g_drive_eject_with_operation_finish (G_DRIVE (source_object), res, &error))
    {
        if (error->code != G_IO_ERROR_FAILED_HANDLED)
        {
            char *primary;
            char *name;

            name = g_drive_get_name (G_DRIVE (source_object));
            primary = g_strdup_printf (_("Unable to eject %s"), name);
            g_free (name);
            eel_show_error_dialog (primary,
                                   error->message,
                                   NULL);
            g_free (primary);
        }
        g_error_free (error);
    }
}

static void
volume_eject_cb (GObject *source_object,
                 GAsyncResult *res,
                 gpointer user_data)
{
    BaulWindow *window;
    GError *error;

    window = user_data;
    baul_window_info_set_initiated_unmount (window, FALSE);
    g_object_unref (window);

    error = NULL;
    if (!g_volume_eject_with_operation_finish (G_VOLUME (source_object), res, &error))
    {
        if (error->code != G_IO_ERROR_FAILED_HANDLED)
        {
            char *primary;
            char *name;

            name = g_volume_get_name (G_VOLUME (source_object));
            primary = g_strdup_printf (_("Unable to eject %s"), name);
            g_free (name);
            eel_show_error_dialog (primary,
                                   error->message,
                                   NULL);
            g_free (primary);
        }
        g_error_free (error);
    }

    else {
        baul_application_notify_unmount_show (_("It is now safe to remove the drive"));
    }
}

static void
mount_eject_cb (GObject *source_object,
                GAsyncResult *res,
                gpointer user_data)
{
    BaulWindow *window;
    GError *error;

    window = user_data;
    baul_window_info_set_initiated_unmount (window, FALSE);
    g_object_unref (window);

    error = NULL;
    if (!g_mount_eject_with_operation_finish (G_MOUNT (source_object), res, &error))
    {
        if (error->code != G_IO_ERROR_FAILED_HANDLED)
        {
            char *primary;
            char *name;

            name = g_mount_get_name (G_MOUNT (source_object));
            primary = g_strdup_printf (_("Unable to eject %s"), name);
            g_free (name);
            eel_show_error_dialog (primary,
                                   error->message,
                                   NULL);
            g_free (primary);
        }
        g_error_free (error);
    }

    else {
        baul_application_notify_unmount_show (_("It is now safe to remove the drive"));
    }
}

static void
do_eject (GMount *mount,
          GVolume *volume,
          GDrive *drive,
          BaulPlacesSidebar *sidebar)
{

    GMountOperation *mount_op;

    mount_op = ctk_mount_operation_new (CTK_WINDOW (ctk_widget_get_toplevel (CTK_WIDGET (sidebar))));

    if (mount != NULL)
    {
        baul_window_info_set_initiated_unmount (sidebar->window, TRUE);
        g_mount_eject_with_operation (mount, 0, mount_op, NULL, mount_eject_cb,
                                      g_object_ref (sidebar->window));
    }
    else if (volume != NULL)
    {
        baul_window_info_set_initiated_unmount (sidebar->window, TRUE);
        g_volume_eject_with_operation (volume, 0, mount_op, NULL, volume_eject_cb,
                                       g_object_ref (sidebar->window));
    }
    else if (drive != NULL)
    {
        baul_window_info_set_initiated_unmount (sidebar->window, TRUE);
        g_drive_eject_with_operation (drive, 0, mount_op, NULL, drive_eject_cb,
                                      g_object_ref (sidebar->window));
    }

    baul_application_notify_unmount_show (_("Writing data to the drive -- do not unplug"));
    g_object_unref (mount_op);
}

static void
eject_shortcut_cb (CtkMenuItem       *item G_GNUC_UNUSED,
		   BaulPlacesSidebar *sidebar)
{
    CtkTreeIter iter;
    GMount *mount;
    GVolume *volume;
    GDrive *drive;

    if (!get_selected_iter (sidebar, &iter))
    {
        return;
    }

    ctk_tree_model_get (CTK_TREE_MODEL (sidebar->filter_model), &iter,
                        PLACES_SIDEBAR_COLUMN_MOUNT, &mount,
                        PLACES_SIDEBAR_COLUMN_VOLUME, &volume,
                        PLACES_SIDEBAR_COLUMN_DRIVE, &drive,
                        -1);

    do_eject (mount, volume, drive, sidebar);
}

static gboolean
eject_or_unmount_bookmark (BaulPlacesSidebar *sidebar,
                           CtkTreePath *path)
{
    CtkTreeModel *model;
    CtkTreeIter iter;
    gboolean can_unmount, can_eject;
    GMount *mount;
    GVolume *volume;
    GDrive *drive;
    gboolean ret;

    model = CTK_TREE_MODEL (sidebar->filter_model);

    if (!path)
    {
        return FALSE;
    }
    if (!ctk_tree_model_get_iter (model, &iter, path))
    {
        return FALSE;
    }

    ctk_tree_model_get (model, &iter,
                        PLACES_SIDEBAR_COLUMN_MOUNT, &mount,
                        PLACES_SIDEBAR_COLUMN_VOLUME, &volume,
                        PLACES_SIDEBAR_COLUMN_DRIVE, &drive,
                        -1);

    ret = FALSE;

    check_unmount_and_eject (mount, volume, drive, &can_unmount, &can_eject);
    /* if we can eject, it has priority over unmount */
    if (can_eject)
    {
        do_eject (mount, volume, drive, sidebar);
        ret = TRUE;
    }
    else if (can_unmount)
    {
        do_unmount (mount, sidebar);
        ret = TRUE;
    }

    if (mount != NULL)
        g_object_unref (mount);
    if (volume != NULL)
        g_object_unref (volume);
    if (drive != NULL)
        g_object_unref (drive);

    return ret;
}

static gboolean
eject_or_unmount_selection (BaulPlacesSidebar *sidebar)
{
    CtkTreeIter iter;
    CtkTreePath *path;
    gboolean ret;

    if (!get_selected_iter (sidebar, &iter)) {
        return FALSE;
    }

    path = ctk_tree_model_get_path (CTK_TREE_MODEL (sidebar->filter_model), &iter);
    if (path == NULL) {
        return FALSE;
    }

    ret = eject_or_unmount_bookmark (sidebar, path);

    ctk_tree_path_free (path);

    return ret;
}

static void
drive_poll_for_media_cb (GObject      *source_object,
			 GAsyncResult *res,
			 gpointer      user_data G_GNUC_UNUSED)
{
    GError *error;

    error = NULL;
    if (!g_drive_poll_for_media_finish (G_DRIVE (source_object), res, &error))
    {
        if (error->code != G_IO_ERROR_FAILED_HANDLED)
        {
            char *primary;
            char *name;

            name = g_drive_get_name (G_DRIVE (source_object));
            primary = g_strdup_printf (_("Unable to poll %s for media changes"), name);
            g_free (name);
            eel_show_error_dialog (primary,
                                   error->message,
                                   NULL);
            g_free (primary);
        }
        g_error_free (error);
    }
}

static void
rescan_shortcut_cb (CtkMenuItem       *item G_GNUC_UNUSED,
		    BaulPlacesSidebar *sidebar)
{
    CtkTreeIter iter;
    GDrive  *drive;

    if (!get_selected_iter (sidebar, &iter))
    {
        return;
    }

    ctk_tree_model_get (CTK_TREE_MODEL (sidebar->filter_model), &iter,
                        PLACES_SIDEBAR_COLUMN_DRIVE, &drive,
                        -1);

    if (drive != NULL)
    {
        g_drive_poll_for_media (drive, NULL, drive_poll_for_media_cb, NULL);
    }
    g_object_unref (drive);
}

static void
format_shortcut_cb (CtkMenuItem       *item G_GNUC_UNUSED,
		    BaulPlacesSidebar *sidebar G_GNUC_UNUSED)
{
    g_spawn_command_line_async ("gfloppy", NULL);
}

static void
drive_start_cb (GObject      *source_object,
		GAsyncResult *res,
		gpointer      user_data G_GNUC_UNUSED)
{
    GError *error;

    error = NULL;
    if (!g_drive_start_finish (G_DRIVE (source_object), res, &error))
    {
        if (error->code != G_IO_ERROR_FAILED_HANDLED)
        {
            char *primary;
            char *name;

            name = g_drive_get_name (G_DRIVE (source_object));
            primary = g_strdup_printf (_("Unable to start %s"), name);
            g_free (name);
            eel_show_error_dialog (primary,
                                   error->message,
                                   NULL);
            g_free (primary);
        }
        g_error_free (error);
    }
}

static void
start_shortcut_cb (CtkMenuItem       *item G_GNUC_UNUSED,
		   BaulPlacesSidebar *sidebar)
{
    CtkTreeIter iter;
    GDrive  *drive;

    if (!get_selected_iter (sidebar, &iter))
    {
        return;
    }

    ctk_tree_model_get (CTK_TREE_MODEL (sidebar->filter_model), &iter,
                        PLACES_SIDEBAR_COLUMN_DRIVE, &drive,
                        -1);

    if (drive != NULL)
    {
        GMountOperation *mount_op;

        mount_op = ctk_mount_operation_new (CTK_WINDOW (ctk_widget_get_toplevel (CTK_WIDGET (sidebar))));

        g_drive_start (drive, G_DRIVE_START_NONE, mount_op, NULL, drive_start_cb, NULL);

        g_object_unref (mount_op);
    }
    g_object_unref (drive);
}

static void
drive_stop_cb (GObject *source_object,
               GAsyncResult *res,
               gpointer user_data)
{
    BaulWindow *window;
    GError *error;

    window = user_data;
    baul_window_info_set_initiated_unmount (window, FALSE);
    g_object_unref (window);

    error = NULL;
    if (!g_drive_stop_finish(G_DRIVE (source_object), res, &error))
    {
        if (error->code != G_IO_ERROR_FAILED_HANDLED)
        {
            char *primary;
            char *name;

            name = g_drive_get_name (G_DRIVE (source_object));
            primary = g_strdup_printf (_("Unable to stop %s"), name);
            g_free (name);
            eel_show_error_dialog (primary,
                                   error->message,
                                   NULL);
            g_free (primary);
        }
        g_error_free (error);
    }
}

static void
stop_shortcut_cb (CtkMenuItem       *item G_GNUC_UNUSED,
		  BaulPlacesSidebar *sidebar)
{
    CtkTreeIter iter;
    GDrive  *drive;

    if (!get_selected_iter (sidebar, &iter))
    {
        return;
    }

    ctk_tree_model_get (CTK_TREE_MODEL (sidebar->filter_model), &iter,
                        PLACES_SIDEBAR_COLUMN_DRIVE, &drive,
                        -1);

    if (drive != NULL)
    {
        GMountOperation *mount_op;

        mount_op = ctk_mount_operation_new (CTK_WINDOW (ctk_widget_get_toplevel (CTK_WIDGET (sidebar))));
        baul_window_info_set_initiated_unmount (sidebar->window, TRUE);
        g_drive_stop (drive, G_MOUNT_UNMOUNT_NONE, mount_op, NULL, drive_stop_cb,
                      g_object_ref (sidebar->window));
        g_object_unref (mount_op);
    }
    g_object_unref (drive);
}

static void
empty_trash_cb (CtkMenuItem       *item G_GNUC_UNUSED,
		BaulPlacesSidebar *sidebar)
{
    baul_file_operations_empty_trash (CTK_WIDGET (sidebar->window));
}

/* Handler for CtkWidget::key-press-event on the shortcuts list */
static gboolean
bookmarks_key_press_event_cb (CtkWidget         *widget G_GNUC_UNUSED,
			      CdkEventKey       *event,
			      BaulPlacesSidebar *sidebar)
{
    guint modifiers;
    CtkTreePath *path;
    BaulWindowOpenFlags flags = 0;

    modifiers = ctk_accelerator_get_default_mod_mask ();

    if (event->keyval == CDK_KEY_Return ||
        event->keyval == CDK_KEY_KP_Enter ||
        event->keyval == CDK_KEY_ISO_Enter ||
        event->keyval == CDK_KEY_space)
    {
        CtkTreeModel *model;

        if ((event->state & modifiers) == CDK_SHIFT_MASK)
            flags = BAUL_WINDOW_OPEN_FLAG_NEW_TAB;
        else if ((event->state & modifiers) == CDK_CONTROL_MASK)
            flags = BAUL_WINDOW_OPEN_FLAG_NEW_WINDOW;

        model = ctk_tree_view_get_model(sidebar->tree_view);
        ctk_tree_view_get_cursor(sidebar->tree_view, &path, NULL);

        open_selected_bookmark(sidebar, model, path, flags);

        ctk_tree_path_free(path);
        return TRUE;
    }

    if (event->keyval == CDK_KEY_Down &&
            (event->state & modifiers) == CDK_MOD1_MASK)
    {
        return eject_or_unmount_selection (sidebar);
    }

    if ((event->keyval == CDK_KEY_Delete
            || event->keyval == CDK_KEY_KP_Delete)
            && (event->state & modifiers) == 0)
    {
        remove_selected_bookmarks (sidebar);
        return TRUE;
    }

    if ((event->keyval == CDK_KEY_F2)
            && (event->state & modifiers) == 0)
    {
        rename_selected_bookmark (sidebar);
        return TRUE;
    }

    return FALSE;
}

/* Constructs the popup menu for the file list if needed */
static void
bookmarks_build_popup_menu (BaulPlacesSidebar *sidebar)
{
    CtkWidget *item;

    if (sidebar->popup_menu)
    {
        return;
    }

    sidebar->popup_menu = ctk_menu_new ();

    ctk_menu_set_reserve_toggle_size (CTK_MENU (sidebar->popup_menu), FALSE);

    ctk_menu_attach_to_widget (CTK_MENU (sidebar->popup_menu),
                               CTK_WIDGET (sidebar),
                               bookmarks_popup_menu_detach_cb);

    item = eel_image_menu_item_new_from_icon ("document-open", _("_Open"));

    g_signal_connect (item, "activate",
                      G_CALLBACK (open_shortcut_cb), sidebar);
    ctk_widget_show (item);
    ctk_menu_shell_append (CTK_MENU_SHELL (sidebar->popup_menu), item);

    item = eel_image_menu_item_new_from_icon (NULL, _("Open in New _Tab"));
    sidebar->popup_menu_open_in_new_tab_item = item;
    g_signal_connect (item, "activate",
                      G_CALLBACK (open_shortcut_in_new_tab_cb), sidebar);
    ctk_widget_show (item);
    ctk_menu_shell_append (CTK_MENU_SHELL (sidebar->popup_menu), item);

    item = eel_image_menu_item_new_from_icon (NULL, _("Open in New _Window"));
    g_signal_connect (item, "activate",
                      G_CALLBACK (open_shortcut_in_new_window_cb), sidebar);
    ctk_widget_show (item);
    ctk_menu_shell_append (CTK_MENU_SHELL (sidebar->popup_menu), item);

    eel_ctk_menu_append_separator (CTK_MENU (sidebar->popup_menu));

    item = eel_image_menu_item_new_from_icon ("list-remove", _("Remove"));

    sidebar->popup_menu_remove_item = item;

    g_signal_connect (item, "activate",
                      G_CALLBACK (remove_shortcut_cb), sidebar);
    ctk_widget_show (item);
    ctk_menu_shell_append (CTK_MENU_SHELL (sidebar->popup_menu), item);

    item = eel_image_menu_item_new_from_icon (NULL, _("Rename..."));
    sidebar->popup_menu_rename_item = item;
    g_signal_connect (item, "activate",
                      G_CALLBACK (rename_shortcut_cb), sidebar);
    ctk_widget_show (item);
    ctk_menu_shell_append (CTK_MENU_SHELL (sidebar->popup_menu), item);

    /* Mount/Unmount/Eject menu items */

    sidebar->popup_menu_separator_item =
        CTK_WIDGET (eel_ctk_menu_append_separator (CTK_MENU (sidebar->popup_menu)));

    item = eel_image_menu_item_new_from_icon (NULL, _("_Mount"));
    sidebar->popup_menu_mount_item = item;
    g_signal_connect (item, "activate",
                      G_CALLBACK (mount_shortcut_cb), sidebar);
    ctk_widget_show (item);
    ctk_menu_shell_append (CTK_MENU_SHELL (sidebar->popup_menu), item);

    item = eel_image_menu_item_new_from_icon ("media-eject", _("_Unmount"));
    sidebar->popup_menu_unmount_item = item;
    g_signal_connect (item, "activate",
                      G_CALLBACK (unmount_shortcut_cb), sidebar);
    ctk_widget_show (item);
    ctk_menu_shell_append (CTK_MENU_SHELL (sidebar->popup_menu), item);

    item = eel_image_menu_item_new_from_icon ("media-eject", _("_Eject"));
    sidebar->popup_menu_eject_item = item;
    g_signal_connect (item, "activate",
                      G_CALLBACK (eject_shortcut_cb), sidebar);
    ctk_widget_show (item);
    ctk_menu_shell_append (CTK_MENU_SHELL (sidebar->popup_menu), item);

    item = eel_image_menu_item_new_from_icon (NULL, _("_Detect Media"));
    sidebar->popup_menu_rescan_item = item;
    g_signal_connect (item, "activate",
                      G_CALLBACK (rescan_shortcut_cb), sidebar);
    ctk_widget_show (item);
    ctk_menu_shell_append (CTK_MENU_SHELL (sidebar->popup_menu), item);

    item = eel_image_menu_item_new_from_icon (NULL, _("_Format"));
    sidebar->popup_menu_format_item = item;
    g_signal_connect (item, "activate",
                      G_CALLBACK (format_shortcut_cb), sidebar);
    ctk_widget_show (item);
    ctk_menu_shell_append (CTK_MENU_SHELL (sidebar->popup_menu), item);

    item = eel_image_menu_item_new_from_icon (NULL, _("_Start"));
    sidebar->popup_menu_start_item = item;
    g_signal_connect (item, "activate",
                      G_CALLBACK (start_shortcut_cb), sidebar);
    ctk_widget_show (item);
    ctk_menu_shell_append (CTK_MENU_SHELL (sidebar->popup_menu), item);

    item = eel_image_menu_item_new_from_icon (NULL, _("_Stop"));
    sidebar->popup_menu_stop_item = item;
    g_signal_connect (item, "activate",
                      G_CALLBACK (stop_shortcut_cb), sidebar);
    ctk_widget_show (item);
    ctk_menu_shell_append (CTK_MENU_SHELL (sidebar->popup_menu), item);

    /* Empty Trash menu item */

    item = eel_image_menu_item_new_from_icon (BAUL_ICON_TRASH, _("Empty _Trash"));
    sidebar->popup_menu_empty_trash_item = item;
    g_signal_connect (item, "activate",
                      G_CALLBACK (empty_trash_cb), sidebar);
    ctk_widget_show (item);
    ctk_menu_shell_append (CTK_MENU_SHELL (sidebar->popup_menu), item);

    bookmarks_check_popup_sensitivity (sidebar);
}

static void
bookmarks_update_popup_menu (BaulPlacesSidebar *sidebar)
{
    bookmarks_build_popup_menu (sidebar);
}

static void
bookmarks_popup_menu (BaulPlacesSidebar *sidebar,
                      CdkEventButton        *event)
{
    bookmarks_update_popup_menu (sidebar);
    eel_pop_up_context_menu (CTK_MENU(sidebar->popup_menu),
                             event);
}

/* Callback used for the CtkWidget::popup-menu signal of the shortcuts list */
static gboolean
bookmarks_popup_menu_cb (CtkWidget         *widget G_GNUC_UNUSED,
			 BaulPlacesSidebar *sidebar)
{
    bookmarks_popup_menu (sidebar, NULL);
    return TRUE;
}

static gboolean
bookmarks_button_release_event_cb (CtkWidget *widget,
                                   CdkEventButton *event,
                                   BaulPlacesSidebar *sidebar)
{
    CtkTreePath *path;
    CtkTreeModel *model;
    CtkTreeView *tree_view;

    path = NULL;

    if (event->type != CDK_BUTTON_RELEASE)
    {
        return TRUE;
    }

    if (clicked_eject_button (sidebar, &path))
    {
        eject_or_unmount_bookmark (sidebar, path);
        ctk_tree_path_free (path);
        return FALSE;
    }

    tree_view = CTK_TREE_VIEW (widget);
    model = ctk_tree_view_get_model (tree_view);

    if (event->button == 1)
    {

        if (event->window != ctk_tree_view_get_bin_window (tree_view))
        {
            return FALSE;
        }

        ctk_tree_view_get_path_at_pos (tree_view, (int) event->x, (int) event->y,
                                       &path, NULL, NULL, NULL);

        open_selected_bookmark (sidebar, model, path, 0);

        ctk_tree_path_free (path);
    }

    return FALSE;
}

static void
update_eject_buttons (BaulPlacesSidebar *sidebar,
                      CtkTreePath         *path)
{
    CtkTreeIter iter;
    gboolean icon_visible, path_same;

    icon_visible = TRUE;

    if (path == NULL && sidebar->eject_highlight_path == NULL) {
        /* Both are null - highlight up to date */
        return;
    }

    path_same = (path != NULL) &&
        (sidebar->eject_highlight_path != NULL) &&
        (ctk_tree_path_compare (sidebar->eject_highlight_path, path) == 0);

    if (path_same) {
        /* Same path - highlight up to date */
        return;
    }

    if (path) {
        ctk_tree_model_get_iter (CTK_TREE_MODEL (sidebar->filter_model),
                     &iter,
                     path);

        ctk_tree_model_get (CTK_TREE_MODEL (sidebar->filter_model),
                    &iter,
                    PLACES_SIDEBAR_COLUMN_EJECT, &icon_visible,
                    -1);
    }

    if (!icon_visible || path == NULL || !path_same) {
        /* remove highlighting and reset the saved path, as we are leaving
         * an eject button area.
         */
        if (sidebar->eject_highlight_path) {
            ctk_tree_model_get_iter (CTK_TREE_MODEL (sidebar->store),
                         &iter,
                         sidebar->eject_highlight_path);

            ctk_list_store_set (sidebar->store,
                        &iter,
                        PLACES_SIDEBAR_COLUMN_EJECT_ICON, get_eject_icon (FALSE),
                        -1);
            ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (sidebar->filter_model));

            ctk_tree_path_free (sidebar->eject_highlight_path);
            sidebar->eject_highlight_path = NULL;
        }

        if (!icon_visible) {
            return;
        }
    }

    if (path != NULL) {
        /* add highlighting to the selected path, as the icon is visible and
         * we're hovering it.
         */
        ctk_tree_model_get_iter (CTK_TREE_MODEL (sidebar->store),
                     &iter,
                     path);
        ctk_list_store_set (sidebar->store,
                    &iter,
                    PLACES_SIDEBAR_COLUMN_EJECT_ICON, get_eject_icon (TRUE),
                    -1);
        ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (sidebar->filter_model));

        sidebar->eject_highlight_path = ctk_tree_path_copy (path);
    }
}

static gboolean
bookmarks_motion_event_cb (CtkWidget         *widget G_GNUC_UNUSED,
			   CdkEventMotion    *event,
			   BaulPlacesSidebar *sidebar)
{
    CtkTreePath *path;

    path = NULL;

    if (over_eject_button (sidebar, event->x, event->y, &path)) {
        update_eject_buttons (sidebar, path);
        ctk_tree_path_free (path);

        return TRUE;
    }

    update_eject_buttons (sidebar, NULL);

    return FALSE;
}

/* Callback used when a button is pressed on the shortcuts list.
 * We trap button 3 to bring up a popup menu, and button 2 to
 * open in a new tab.
 */
static gboolean
bookmarks_button_press_event_cb (CtkWidget             *widget,
                                 CdkEventButton        *event,
                                 BaulPlacesSidebar *sidebar)
{
    if (event->type != CDK_BUTTON_PRESS)
    {
        /* ignore multiple clicks */
        return TRUE;
    }

    if (event->button == 3)
    {
        bookmarks_popup_menu (sidebar, event);
    }
    else if (event->button == 2)
    {
        CtkTreeModel *model;
        CtkTreePath *path;
        CtkTreeView *tree_view;

        tree_view = CTK_TREE_VIEW (widget);
        g_assert (tree_view == sidebar->tree_view);

        model = ctk_tree_view_get_model (tree_view);

        ctk_tree_view_get_path_at_pos (tree_view, (int) event->x, (int) event->y,
                                       &path, NULL, NULL, NULL);

        open_selected_bookmark (sidebar, model, path,
                                event->state & CDK_CONTROL_MASK ?
                                BAUL_WINDOW_OPEN_FLAG_NEW_WINDOW :
                                BAUL_WINDOW_OPEN_FLAG_NEW_TAB);

        if (path != NULL)
        {
            ctk_tree_path_free (path);
            return TRUE;
        }
    }

    return FALSE;
}


static void
bookmarks_edited (CtkCellRenderer       *cell,
                  gchar                 *path_string,
                  gchar                 *new_text,
                  BaulPlacesSidebar *sidebar)
{
    CtkTreePath *path;
    CtkTreeIter iter;
    BaulBookmark *bookmark;
    int index;

    g_object_set (cell, "editable", FALSE, NULL);

    path = ctk_tree_path_new_from_string (path_string);
    ctk_tree_model_get_iter (CTK_TREE_MODEL (sidebar->filter_model), &iter, path);
    ctk_tree_model_get (CTK_TREE_MODEL (sidebar->filter_model), &iter,
                        PLACES_SIDEBAR_COLUMN_INDEX, &index,
                        -1);
    ctk_tree_path_free (path);
    bookmark = baul_bookmark_list_item_at (sidebar->bookmarks, index);

    if (bookmark != NULL)
    {
        baul_bookmark_set_name (bookmark, new_text);
    }
}

static void
bookmarks_editing_canceled (CtkCellRenderer   *cell,
			    BaulPlacesSidebar *sidebar G_GNUC_UNUSED)
{
    g_object_set (cell, "editable", FALSE, NULL);
}

static void
trash_state_changed_cb (BaulTrashMonitor *trash_monitor G_GNUC_UNUSED,
			gboolean          state G_GNUC_UNUSED,
			gpointer          data)
{
    BaulPlacesSidebar *sidebar;

    sidebar = BAUL_PLACES_SIDEBAR (data);

    /* The trash icon changed, update the sidebar */
    update_places (sidebar);

    bookmarks_check_popup_sensitivity (sidebar);
}

static gboolean
tree_selection_func (CtkTreeSelection *selection G_GNUC_UNUSED,
		     CtkTreeModel     *model,
		     CtkTreePath      *path,
		     gboolean          path_currently_selected G_GNUC_UNUSED,
		     gpointer          user_data G_GNUC_UNUSED)
{
    CtkTreeIter iter;
    PlaceType row_type;

    ctk_tree_model_get_iter (model, &iter, path);
    ctk_tree_model_get (model, &iter,
                PLACES_SIDEBAR_COLUMN_ROW_TYPE, &row_type,
                -1);

    if (row_type == PLACES_HEADING) {
        return FALSE;
    }

    return TRUE;
}

static void
icon_cell_renderer_func (CtkTreeViewColumn *column G_GNUC_UNUSED,
			 CtkCellRenderer   *cell,
			 CtkTreeModel      *model,
			 CtkTreeIter       *iter,
			 gpointer           user_data G_GNUC_UNUSED)
{
    PlaceType type;

    ctk_tree_model_get (model, iter,
                PLACES_SIDEBAR_COLUMN_ROW_TYPE, &type,
                -1);

    if (type == PLACES_HEADING) {
        g_object_set (cell,
                  "visible", FALSE,
                  NULL);
    } else {
        g_object_set (cell,
                  "visible", TRUE,
                  NULL);
    }
}

static void
padding_cell_renderer_func (CtkTreeViewColumn *column G_GNUC_UNUSED,
			    CtkCellRenderer   *cell,
			    CtkTreeModel      *model,
			    CtkTreeIter       *iter,
			    gpointer           user_data G_GNUC_UNUSED)
{
    PlaceType type;

    ctk_tree_model_get (model, iter,
                        PLACES_SIDEBAR_COLUMN_ROW_TYPE, &type,
                        -1);

    if (type == PLACES_HEADING) {
        g_object_set (cell,
                      "visible", FALSE,
                      "xpad", 0,
                      "ypad", 0,
                      NULL);
    } else {
        g_object_set (cell,
                      "visible", TRUE,
                      "xpad", 3,
                      "ypad", 0,
                      NULL);
    }
}

static void
heading_cell_renderer_func (CtkTreeViewColumn *column G_GNUC_UNUSED,
			    CtkCellRenderer   *cell,
			    CtkTreeModel      *model,
			    CtkTreeIter       *iter,
			    gpointer           user_data G_GNUC_UNUSED)
{
    PlaceType type;

    ctk_tree_model_get (model, iter,
                        PLACES_SIDEBAR_COLUMN_ROW_TYPE, &type,
                        -1);

    if (type == PLACES_HEADING) {
        g_object_set (cell,
                      "visible", TRUE,
                      NULL);
    } else {
        g_object_set (cell,
                      "visible", FALSE,
                      NULL);
    }
}

static void
baul_places_sidebar_init (BaulPlacesSidebar *sidebar)
{
    CtkTreeView       *tree_view;
    CtkTreeViewColumn *col;
    CtkCellRenderer   *cell;
    CtkTreeSelection  *selection;

    sidebar->volume_monitor = g_volume_monitor_get ();

    ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sidebar),
                                    CTK_POLICY_NEVER,
                                    CTK_POLICY_AUTOMATIC);
    ctk_scrolled_window_set_hadjustment (CTK_SCROLLED_WINDOW (sidebar), NULL);
    ctk_scrolled_window_set_vadjustment (CTK_SCROLLED_WINDOW (sidebar), NULL);
    ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (sidebar), CTK_SHADOW_IN);
    ctk_scrolled_window_set_overlay_scrolling(CTK_SCROLLED_WINDOW (sidebar), FALSE);

    /* tree view */
    tree_view = CTK_TREE_VIEW (ctk_tree_view_new ());
    ctk_tree_view_set_headers_visible (tree_view, FALSE);

    col = CTK_TREE_VIEW_COLUMN (ctk_tree_view_column_new ());

    /* initial padding */
    cell = ctk_cell_renderer_text_new ();
    ctk_tree_view_column_pack_start (col, cell, FALSE);
    g_object_set (cell,
                  "xpad", 6,
                  NULL);

    /* headings */
    cell = ctk_cell_renderer_text_new ();
    ctk_tree_view_column_pack_start (col, cell, FALSE);
    ctk_tree_view_column_set_attributes (col, cell,
                                         "text", PLACES_SIDEBAR_COLUMN_HEADING_TEXT,
                                         NULL);
    g_object_set (cell,
                  "weight", PANGO_WEIGHT_BOLD,
                  "weight-set", TRUE,
                  "ypad", 1,
                  "xpad", 0,
                  NULL);
    ctk_tree_view_column_set_cell_data_func (col, cell,
                         heading_cell_renderer_func,
                         sidebar, NULL);

    /* icon padding */
    cell = ctk_cell_renderer_text_new ();
    ctk_tree_view_column_pack_start (col, cell, FALSE);
    ctk_tree_view_column_set_cell_data_func (col, cell,
                                             padding_cell_renderer_func,
                                             sidebar, NULL);

    /* icon renderer */
    cell = ctk_cell_renderer_pixbuf_new ();
    ctk_tree_view_column_pack_start (col, cell, FALSE);
    ctk_tree_view_column_set_attributes (col, cell,
                                         "surface", PLACES_SIDEBAR_COLUMN_ICON,
                                         NULL);
    ctk_tree_view_column_set_cell_data_func (col, cell,
                                             icon_cell_renderer_func,
                                             sidebar, NULL);

    /* eject text renderer */
    cell = ctk_cell_renderer_text_new ();
    ctk_tree_view_column_pack_start (col, cell, TRUE);
    ctk_tree_view_column_set_attributes (col, cell,
                                         "text", PLACES_SIDEBAR_COLUMN_NAME,
                                         "visible", PLACES_SIDEBAR_COLUMN_EJECT,
                                         NULL);
    g_object_set (cell,
                  "ellipsize", PANGO_ELLIPSIZE_END,
                  "ellipsize-set", TRUE,
                  NULL);

    /* eject icon renderer */
    cell = ctk_cell_renderer_pixbuf_new ();
    sidebar->eject_icon_cell_renderer = cell;
    g_object_set (cell,
                  "mode", CTK_CELL_RENDERER_MODE_ACTIVATABLE,
                  "stock-size", CTK_ICON_SIZE_MENU,
                  "xpad", EJECT_BUTTON_XPAD,
                  /* align right, because for some reason ctk+ expands
                  this even though we tell it not to. */
                  "xalign", 1.0,
                  NULL);
    ctk_tree_view_column_pack_start (col, cell, FALSE);
    ctk_tree_view_column_set_attributes (col, cell,
                                         "visible", PLACES_SIDEBAR_COLUMN_EJECT,
                                         "surface", PLACES_SIDEBAR_COLUMN_EJECT_ICON,
                                         NULL);

    /* normal text renderer */
    cell = ctk_cell_renderer_text_new ();
    ctk_tree_view_column_pack_start (col, cell, TRUE);
    g_object_set (G_OBJECT (cell), "editable", FALSE, NULL);
    ctk_tree_view_column_set_attributes (col, cell,
                                         "text", PLACES_SIDEBAR_COLUMN_NAME,
                                         "visible", PLACES_SIDEBAR_COLUMN_NO_EJECT,
                                         "editable-set", PLACES_SIDEBAR_COLUMN_BOOKMARK,
                                         NULL);
    g_object_set (cell,
                  "ellipsize", PANGO_ELLIPSIZE_END,
                  "ellipsize-set", TRUE,
                  NULL);

    g_signal_connect (cell, "edited",
                      G_CALLBACK (bookmarks_edited), sidebar);
    g_signal_connect (cell, "editing-canceled",
                      G_CALLBACK (bookmarks_editing_canceled), sidebar);

    /* this is required to align the eject buttons to the right */
    ctk_tree_view_column_set_max_width (CTK_TREE_VIEW_COLUMN (col), BAUL_ICON_SIZE_SMALLER);
    ctk_tree_view_append_column (tree_view, col);

    sidebar->store = ctk_list_store_new (PLACES_SIDEBAR_COLUMN_COUNT,
                                         G_TYPE_INT,
                                         G_TYPE_STRING,
                                         G_TYPE_DRIVE,
                                         G_TYPE_VOLUME,
                                         G_TYPE_MOUNT,
                                         G_TYPE_STRING,
                                         CAIRO_GOBJECT_TYPE_SURFACE,
                                         G_TYPE_INT,
                                         G_TYPE_BOOLEAN,
                                         G_TYPE_BOOLEAN,
                                         G_TYPE_BOOLEAN,
                                         G_TYPE_STRING,
                                         CAIRO_GOBJECT_TYPE_SURFACE,
                                         G_TYPE_INT,
                                         G_TYPE_STRING);

    ctk_tree_view_set_tooltip_column (tree_view, PLACES_SIDEBAR_COLUMN_TOOLTIP);

    sidebar->filter_model = baul_shortcuts_model_filter_new (sidebar,
                            CTK_TREE_MODEL (sidebar->store),
                            NULL);

    ctk_tree_view_set_model (tree_view, sidebar->filter_model);
    ctk_container_add (CTK_CONTAINER (sidebar), CTK_WIDGET (tree_view));
    ctk_widget_show (CTK_WIDGET (tree_view));

    ctk_widget_show (CTK_WIDGET (sidebar));
    sidebar->tree_view = tree_view;

    ctk_tree_view_set_search_column (tree_view, PLACES_SIDEBAR_COLUMN_NAME);
    selection = ctk_tree_view_get_selection (tree_view);
    ctk_tree_selection_set_mode (selection, CTK_SELECTION_BROWSE);

    ctk_tree_selection_set_select_function (selection,
                                            tree_selection_func,
                                            sidebar,
                                            NULL);

    ctk_tree_view_enable_model_drag_source (CTK_TREE_VIEW (tree_view),
                                            CDK_BUTTON1_MASK,
                                            baul_shortcuts_source_targets,
                                            G_N_ELEMENTS (baul_shortcuts_source_targets),
                                            CDK_ACTION_MOVE);
    ctk_drag_dest_set (CTK_WIDGET (tree_view),
                       0,
                       baul_shortcuts_drop_targets, G_N_ELEMENTS (baul_shortcuts_drop_targets),
                       CDK_ACTION_MOVE | CDK_ACTION_COPY | CDK_ACTION_LINK);

    g_signal_connect (tree_view, "key-press-event",
                      G_CALLBACK (bookmarks_key_press_event_cb), sidebar);

    g_signal_connect (tree_view, "drag-motion",
                      G_CALLBACK (drag_motion_callback), sidebar);
    g_signal_connect (tree_view, "drag-leave",
                      G_CALLBACK (drag_leave_callback), sidebar);
    g_signal_connect (tree_view, "drag-data-received",
                      G_CALLBACK (drag_data_received_callback), sidebar);
    g_signal_connect (tree_view, "drag-drop",
                      G_CALLBACK (drag_drop_callback), sidebar);

    g_signal_connect (selection, "changed",
                      G_CALLBACK (bookmarks_selection_changed_cb), sidebar);
    g_signal_connect (tree_view, "popup-menu",
                      G_CALLBACK (bookmarks_popup_menu_cb), sidebar);
    g_signal_connect (tree_view, "button-press-event",
                      G_CALLBACK (bookmarks_button_press_event_cb), sidebar);
    g_signal_connect (tree_view, "motion-notify-event",
                      G_CALLBACK (bookmarks_motion_event_cb), sidebar);
    g_signal_connect (tree_view, "button-release-event",
                      G_CALLBACK (bookmarks_button_release_event_cb), sidebar);

    eel_ctk_tree_view_set_activate_on_single_click (sidebar->tree_view,
            TRUE);

    g_signal_connect_swapped (baul_preferences, "changed::" BAUL_PREFERENCES_DESKTOP_IS_HOME_DIR,
                              G_CALLBACK(desktop_location_changed_callback),
                              sidebar);

    g_signal_connect_object (baul_trash_monitor_get (),
                             "trash_state_changed",
                             G_CALLBACK (trash_state_changed_cb),
                             sidebar, 0);
}

static void
baul_places_sidebar_dispose (GObject *object)
{
    BaulPlacesSidebar *sidebar;

    sidebar = BAUL_PLACES_SIDEBAR (object);

    sidebar->window = NULL;
    sidebar->tree_view = NULL;

    g_free (sidebar->uri);
    sidebar->uri = NULL;

    free_drag_data (sidebar);

    if (sidebar->eject_highlight_path != NULL) {
        ctk_tree_path_free (sidebar->eject_highlight_path);
        sidebar->eject_highlight_path = NULL;
    }

    g_clear_object (&sidebar->store);
    g_clear_object (&sidebar->volume_monitor);
    g_clear_object (&sidebar->bookmarks);
    g_clear_object (&sidebar->filter_model);

    eel_remove_weak_pointer (&(sidebar->go_to_after_mount_slot));

    g_signal_handlers_disconnect_by_func (baul_preferences,
                                          desktop_location_changed_callback,
                                          sidebar);

    G_OBJECT_CLASS (baul_places_sidebar_parent_class)->dispose (object);
}

static void
baul_places_sidebar_class_init (BaulPlacesSidebarClass *class)
{
    G_OBJECT_CLASS (class)->dispose = baul_places_sidebar_dispose;

    CTK_WIDGET_CLASS (class)->style_updated = baul_places_sidebar_style_updated;
}

static const char *
baul_places_sidebar_get_sidebar_id (BaulSidebar *sidebar G_GNUC_UNUSED)
{
    return BAUL_PLACES_SIDEBAR_ID;
}

static char *
baul_places_sidebar_get_tab_label (BaulSidebar *sidebar G_GNUC_UNUSED)
{
    return g_strdup (_("Places"));
}

static char *
baul_places_sidebar_get_tab_tooltip (BaulSidebar *sidebar G_GNUC_UNUSED)
{
    return g_strdup (_("Show Places"));
}

static GdkPixbuf *
baul_places_sidebar_get_tab_icon (BaulSidebar *sidebar G_GNUC_UNUSED)
{
    return NULL;
}

static void
baul_places_sidebar_is_visible_changed (BaulSidebar *sidebar G_GNUC_UNUSED,
					gboolean     is_visible G_GNUC_UNUSED)
{
    /* Do nothing */
}

static void
baul_places_sidebar_iface_init (BaulSidebarIface *iface)
{
    iface->get_sidebar_id = baul_places_sidebar_get_sidebar_id;
    iface->get_tab_label = baul_places_sidebar_get_tab_label;
    iface->get_tab_tooltip = baul_places_sidebar_get_tab_tooltip;
    iface->get_tab_icon = baul_places_sidebar_get_tab_icon;
    iface->is_visible_changed = baul_places_sidebar_is_visible_changed;
}

static void
baul_places_sidebar_set_parent_window (BaulPlacesSidebar *sidebar,
                                       BaulWindowInfo *window)
{
    BaulWindowSlotInfo *slot;

    sidebar->window = window;

    slot = baul_window_info_get_active_slot (window);

    sidebar->bookmarks = baul_bookmark_list_new ();
    sidebar->uri = baul_window_slot_info_get_current_location (slot);

    g_signal_connect_object (sidebar->bookmarks, "contents_changed",
                             G_CALLBACK (update_places),
                             sidebar, G_CONNECT_SWAPPED);

    g_signal_connect_object (window, "loading_uri",
                             G_CALLBACK (loading_uri_callback),
                             sidebar, 0);

    g_signal_connect_object (sidebar->volume_monitor, "volume_added",
                             G_CALLBACK (volume_added_callback), sidebar, 0);
    g_signal_connect_object (sidebar->volume_monitor, "volume_removed",
                             G_CALLBACK (volume_removed_callback), sidebar, 0);
    g_signal_connect_object (sidebar->volume_monitor, "volume_changed",
                             G_CALLBACK (volume_changed_callback), sidebar, 0);
    g_signal_connect_object (sidebar->volume_monitor, "mount_added",
                             G_CALLBACK (mount_added_callback), sidebar, 0);
    g_signal_connect_object (sidebar->volume_monitor, "mount_removed",
                             G_CALLBACK (mount_removed_callback), sidebar, 0);
    g_signal_connect_object (sidebar->volume_monitor, "mount_changed",
                             G_CALLBACK (mount_changed_callback), sidebar, 0);
    g_signal_connect_object (sidebar->volume_monitor, "drive_disconnected",
                             G_CALLBACK (drive_disconnected_callback), sidebar, 0);
    g_signal_connect_object (sidebar->volume_monitor, "drive_connected",
                             G_CALLBACK (drive_connected_callback), sidebar, 0);
    g_signal_connect_object (sidebar->volume_monitor, "drive_changed",
                             G_CALLBACK (drive_changed_callback), sidebar, 0);

    update_places (sidebar);
}

static void
baul_places_sidebar_style_updated (CtkWidget *widget)
{
    BaulPlacesSidebar *sidebar;

    sidebar = BAUL_PLACES_SIDEBAR (widget);

    update_places (sidebar);
}

static BaulSidebar *
baul_places_sidebar_create (BaulSidebarProvider *provider G_GNUC_UNUSED,
			    BaulWindowInfo      *window)
{
    BaulPlacesSidebar *sidebar;

    sidebar = g_object_new (baul_places_sidebar_get_type (), NULL);
    baul_places_sidebar_set_parent_window (sidebar, window);
    g_object_ref_sink (sidebar);

    return BAUL_SIDEBAR (sidebar);
}

static void
sidebar_provider_iface_init (BaulSidebarProviderIface *iface)
{
    iface->create = baul_places_sidebar_create;
}

static void
baul_places_sidebar_provider_init (BaulPlacesSidebarProvider *sidebar G_GNUC_UNUSED)
{
}

static void
baul_places_sidebar_provider_class_init (BaulPlacesSidebarProviderClass *class G_GNUC_UNUSED)
{
}

void
baul_places_sidebar_register (void)
{
    baul_module_add_type (baul_places_sidebar_provider_get_type ());
}

/* Drag and drop interfaces */

static void
_baul_shortcuts_model_filter_class_init (BaulShortcutsModelFilterClass *class G_GNUC_UNUSED)
{
}

static void
_baul_shortcuts_model_filter_init (BaulShortcutsModelFilter *model)
{
    model->sidebar = NULL;
}

/* CtkTreeDragSource::row_draggable implementation for the shortcuts filter model */
static gboolean
baul_shortcuts_model_filter_row_draggable (CtkTreeDragSource *drag_source,
                                           CtkTreePath       *path)
{
    CtkTreeModel *model;
    CtkTreeIter iter;
    PlaceType place_type;
    SectionType section_type;

    model = CTK_TREE_MODEL (drag_source);

    ctk_tree_model_get_iter (model, &iter, path);
    ctk_tree_model_get (model, &iter,
                        PLACES_SIDEBAR_COLUMN_ROW_TYPE, &place_type,
                        PLACES_SIDEBAR_COLUMN_SECTION_TYPE, &section_type,
                        -1);

    if (place_type != PLACES_HEADING && section_type == SECTION_BOOKMARKS)
        return TRUE;

    return FALSE;
}

/* Fill the CtkTreeDragSourceIface vtable */
static void
baul_shortcuts_model_filter_drag_source_iface_init (CtkTreeDragSourceIface *iface)
{
    iface->row_draggable = baul_shortcuts_model_filter_row_draggable;
}

static CtkTreeModel *
baul_shortcuts_model_filter_new (BaulPlacesSidebar *sidebar,
                                 CtkTreeModel          *child_model,
                                 CtkTreePath           *root)
{
    BaulShortcutsModelFilter *model;

    model = g_object_new (BAUL_SHORTCUTS_MODEL_FILTER_TYPE,
                          "child-model", child_model,
                          "virtual-root", root,
                          NULL);

    model->sidebar = sidebar;

    return CTK_TREE_MODEL (model);
}
