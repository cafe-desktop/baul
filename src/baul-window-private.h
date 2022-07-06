/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/*
 *  Baul
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 1999, 2000, 2001 Eazel, Inc.
 *
 *  Baul is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Baul is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Authors: Elliot Lee <sopwith@redhat.com>
 *           Darin Adler <darin@bentspoon.com>
 *
 */

#ifndef BAUL_WINDOW_PRIVATE_H
#define BAUL_WINDOW_PRIVATE_H

#include <libbaul-private/baul-directory.h>

#include "baul-window.h"
#include "baul-window-slot.h"
#include "baul-window-pane.h"
#include "baul-spatial-window.h"
#include "baul-navigation-window.h"
#include "baul-bookmark-list.h"

struct _BaulNavigationWindowPane;

/* FIXME bugzilla.gnome.org 42575: Migrate more fields into here. */
struct _BaulWindowPrivate
{
    CtkWidget *grid;

    CtkWidget *statusbar;
    CtkWidget *menubar;

    CtkUIManager *ui_manager;
    CtkActionGroup *main_action_group; /* owned by ui_manager */
    guint help_message_cid;

    /* Menus. */
    guint extensions_menu_merge_id;
    CtkActionGroup *extensions_menu_action_group;

    CtkActionGroup *bookmarks_action_group;
    guint bookmarks_merge_id;
    BaulBookmarkList *bookmark_list;

    BaulWindowShowHiddenFilesMode show_hidden_files_mode;
    BaulWindowShowBackupFilesMode show_backup_files_mode;

    /* View As menu */
    GList *short_list_viewers;
    char *extra_viewer;

    /* View As choices */
    CtkActionGroup *view_as_action_group; /* owned by ui_manager */
    CtkRadioAction *view_as_radio_action;
    CtkRadioAction *extra_viewer_radio_action;
    guint short_list_merge_id;
    guint extra_viewer_merge_id;

    /* Ensures that we do not react on signals of a
     * view that is re-used as new view when its loading
     * is cancelled
     */
    gboolean temporarily_ignore_view_signals;

    /* available panes, and active pane.
     * Both of them may never be NULL.
     */
    GList *panes;
    BaulWindowPane *active_pane;

    /* So we can tell which window initiated
     * an unmount operation.
     */
    gboolean initiated_unmount;
};

struct _BaulNavigationWindowPrivate
{
    CtkWidget *content_paned;
    CtkWidget *content_box;
    CtkActionGroup *navigation_action_group; /* owned by ui_manager */

    CtkSizeGroup *header_size_group;

    /* Side Pane */
    int side_pane_width;
    BaulSidebar *current_side_panel;

    /* Menus */
    CtkActionGroup *go_menu_action_group;
    guint refresh_go_menu_idle_id;
    guint go_menu_merge_id;

    /* Toolbar */
    CtkWidget *toolbar;

    guint extensions_toolbar_merge_id;
    CtkActionGroup *extensions_toolbar_action_group;

    /* spinner */
    gboolean    spinner_active;
    CtkWidget  *spinner;

    /* focus widget before the location bar has been shown temporarily */
    CtkWidget *last_focus_widget;

    /* split view */
    CtkWidget *split_view_hpane;
};

#define BAUL_MENU_PATH_BACK_ITEM			"/menu/Go/Back"
#define BAUL_MENU_PATH_FORWARD_ITEM			"/menu/Go/Forward"
#define BAUL_MENU_PATH_UP_ITEM			"/menu/Go/Up"

#define BAUL_MENU_PATH_RELOAD_ITEM			"/menu/View/Reload"
#define BAUL_MENU_PATH_ZOOM_IN_ITEM			"/menu/View/Zoom Items Placeholder/Zoom In"
#define BAUL_MENU_PATH_ZOOM_OUT_ITEM		"/menu/View/Zoom Items Placeholder/Zoom Out"
#define BAUL_MENU_PATH_ZOOM_NORMAL_ITEM		"/menu/View/Zoom Items Placeholder/Zoom Normal"

#define BAUL_COMMAND_BACK				"/commands/Back"
#define BAUL_COMMAND_FORWARD			"/commands/Forward"
#define BAUL_COMMAND_UP				"/commands/Up"

#define BAUL_COMMAND_RELOAD				"/commands/Reload"
#define BAUL_COMMAND_BURN_CD			"/commands/Burn CD"
#define BAUL_COMMAND_STOP				"/commands/Stop"
#define BAUL_COMMAND_ZOOM_IN			"/commands/Zoom In"
#define BAUL_COMMAND_ZOOM_OUT			"/commands/Zoom Out"
#define BAUL_COMMAND_ZOOM_NORMAL			"/commands/Zoom Normal"

/* window geometry */
/* Min values are very small, and a Baul window at this tiny size is *almost*
 * completely unusable. However, if all the extra bits (sidebar, location bar, etc)
 * are turned off, you can see an icon or two at this size. See bug 5946.
 */

#define BAUL_SPATIAL_WINDOW_MIN_WIDTH			100
#define BAUL_SPATIAL_WINDOW_MIN_HEIGHT			100
#define BAUL_SPATIAL_WINDOW_DEFAULT_WIDTH			500
#define BAUL_SPATIAL_WINDOW_DEFAULT_HEIGHT			300

#define BAUL_NAVIGATION_WINDOW_MIN_WIDTH			200
#define BAUL_NAVIGATION_WINDOW_MIN_HEIGHT			200
#define BAUL_NAVIGATION_WINDOW_DEFAULT_WIDTH		800
#define BAUL_NAVIGATION_WINDOW_DEFAULT_HEIGHT		550

typedef void (*BaulBookmarkFailedCallback) (BaulWindow *window,
        BaulBookmark *bookmark);

void               baul_window_set_status                            (BaulWindow    *window,
        BaulWindowSlot *slot,
        const char        *status);
void               baul_window_load_view_as_menus                    (BaulWindow    *window);
void               baul_window_load_extension_menus                  (BaulWindow    *window);
void               baul_window_initialize_menus                      (BaulWindow    *window);
void               baul_window_finalize_menus                        (BaulWindow    *window);
BaulWindowPane *baul_window_get_next_pane                        (BaulWindow *window);
void               baul_menus_append_bookmark_to_menu                (BaulWindow    *window,
        BaulBookmark  *bookmark,
        const char        *parent_path,
        const char        *parent_id,
        guint              index_in_parent,
        CtkActionGroup    *action_group,
        guint              merge_id,
        GCallback          refresh_callback,
        BaulBookmarkFailedCallback failed_callback);
void               baul_window_update_find_menu_item                 (BaulWindow    *window);
void               baul_window_zoom_in                               (BaulWindow    *window);
void               baul_window_zoom_out                              (BaulWindow    *window);
void               baul_window_zoom_to_level                         (BaulWindow    *window,
        BaulZoomLevel  level);
void               baul_window_zoom_to_default                       (BaulWindow    *window);

BaulWindowSlot *baul_window_open_slot                            (BaulWindowPane *pane,
        BaulWindowOpenSlotFlags flags);
void                baul_window_close_slot                           (BaulWindowSlot *slot);

BaulWindowSlot *baul_window_get_slot_for_view                    (BaulWindow *window,
        BaulView   *view);

GList *              baul_window_get_slots                           (BaulWindow    *window);
BaulWindowSlot * baul_window_get_active_slot                     (BaulWindow    *window);
BaulWindowSlot * baul_window_get_extra_slot                      (BaulWindow    *window);
void                 baul_window_set_active_slot                     (BaulWindow    *window,
        BaulWindowSlot *slot);
void                 baul_window_set_active_pane                     (BaulWindow *window,
        BaulWindowPane *new_pane);
BaulWindowPane * baul_window_get_active_pane                     (BaulWindow *window);

void               baul_send_history_list_changed                    (void);
void               baul_remove_from_history_list_no_notify           (GFile             *location);
gboolean           baul_add_bookmark_to_history_list                 (BaulBookmark  *bookmark);
gboolean           baul_add_to_history_list_no_notify                (GFile             *location,
        const char        *name,
        gboolean           has_custom_name,
        GIcon            *icon);
GList *            baul_get_history_list                             (void);
void               baul_window_bookmarks_preference_changed_callback (gpointer           user_data);


/* sync window GUI with current slot. Used when changing slots,
 * and when updating the slot state.
 */
void baul_window_sync_status           (BaulWindow *window);
void baul_window_sync_allow_stop       (BaulWindow *window,
                                        BaulWindowSlot *slot);
void baul_window_sync_title            (BaulWindow *window,
                                        BaulWindowSlot *slot);
void baul_window_sync_zoom_widgets     (BaulWindow *window);

/* Navigation window menus */
void               baul_navigation_window_initialize_actions                    (BaulNavigationWindow    *window);
void               baul_navigation_window_initialize_menus                      (BaulNavigationWindow    *window);
void               baul_navigation_window_remove_bookmarks_menu_callback        (BaulNavigationWindow    *window);

void               baul_navigation_window_remove_bookmarks_menu_items           (BaulNavigationWindow    *window);
void               baul_navigation_window_update_show_hide_menu_items           (BaulNavigationWindow     *window);
void               baul_navigation_window_update_spatial_menu_item              (BaulNavigationWindow     *window);
void               baul_navigation_window_remove_go_menu_callback    (BaulNavigationWindow    *window);
void               baul_navigation_window_remove_go_menu_items       (BaulNavigationWindow    *window);

/* Navigation window toolbar */
void               baul_navigation_window_activate_spinner                     (BaulNavigationWindow    *window);
void               baul_navigation_window_initialize_toolbars                   (BaulNavigationWindow    *window);
void               baul_navigation_window_load_extension_toolbar_items          (BaulNavigationWindow    *window);
void               baul_navigation_window_set_spinner_active                   (BaulNavigationWindow    *window,
        gboolean                     active);
void               baul_navigation_window_go_back                               (BaulNavigationWindow    *window);
void               baul_navigation_window_go_forward                            (BaulNavigationWindow    *window);
void               baul_window_close_pane                                       (BaulWindowPane *pane);
void               baul_navigation_window_update_split_view_actions_sensitivity (BaulNavigationWindow    *window);

#endif /* BAUL_WINDOW_PRIVATE_H */
