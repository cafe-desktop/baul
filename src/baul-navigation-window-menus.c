/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2000, 2001 Eazel, Inc.
 *
 * Baul is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Baul is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Author: John Sullivan <sullivan@eazel.com>
 */

/* baul-window-menus.h - implementation of baul window menu operations,
 *                           split into separate file just for convenience.
 */
#include <config.h>
#include <locale.h>

#include <libxml/parser.h>
#include <ctk/ctk.h>
#include <glib/gi18n.h>

#include <eel/eel-glib-extensions.h>
#include <eel/eel-cafe-extensions.h>
#include <eel/eel-stock-dialogs.h>
#include <eel/eel-string.h>

#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-ui-utilities.h>
#include <libbaul-private/baul-search-engine.h>
#include <libbaul-private/baul-signaller.h>

#include "baul-actions.h"
#include "baul-notebook.h"
#include "baul-navigation-action.h"
#include "baul-zoom-action.h"
#include "baul-view-as-action.h"
#include "baul-application.h"
#include "baul-bookmark-list.h"
#include "baul-bookmarks-window.h"
#include "baul-file-management-properties.h"
#include "baul-property-browser.h"
#include "baul-window-manage-views.h"
#include "baul-window-private.h"
#include "baul-window-bookmarks.h"
#include "baul-navigation-window-pane.h"

#define MENU_PATH_HISTORY_PLACEHOLDER			"/MenuBar/Other Menus/Go/History Placeholder"

#define RESPONSE_FORGET		1000
#define MENU_ITEM_MAX_WIDTH_CHARS 32

static void                  schedule_refresh_go_menu                      (BaulNavigationWindow   *window);


static void
action_close_all_windows_callback (CtkAction *action,
                                   gpointer user_data)
{
    BaulApplication *app;

    app = BAUL_APPLICATION (g_application_get_default ());
    baul_application_close_all_navigation_windows (app);
}

static gboolean
should_open_in_new_tab (void)
{
    /* FIXME this is duplicated */
    GdkEvent *event;

    event = ctk_get_current_event ();

    if (event == NULL)
    {
        return FALSE;
    }

    if (event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE)
    {
        return event->button.button == 2;
    }

    gdk_event_free (event);

    return FALSE;
}

static void
action_back_callback (CtkAction *action,
                      gpointer user_data)
{
    baul_navigation_window_back_or_forward (BAUL_NAVIGATION_WINDOW (user_data),
                                            TRUE, 0, should_open_in_new_tab ());
}

static void
action_forward_callback (CtkAction *action,
                         gpointer user_data)
{
    baul_navigation_window_back_or_forward (BAUL_NAVIGATION_WINDOW (user_data),
                                            FALSE, 0, should_open_in_new_tab ());
}

static void
forget_history_if_yes (CtkDialog *dialog, int response, gpointer callback_data)
{
    if (response == RESPONSE_FORGET)
    {
        baul_forget_history ();
    }
    ctk_widget_destroy (CTK_WIDGET (dialog));
}

static void
forget_history_if_confirmed (BaulWindow *window)
{
    CtkDialog *dialog;

    dialog = eel_create_question_dialog (_("Are you sure you want to clear the list "
                                           "of locations you have visited?"),
                                         NULL,
                                         "process-stop", CTK_RESPONSE_CANCEL,
                                         "edit-clear", RESPONSE_FORGET,
                                         CTK_WINDOW (window));

    ctk_widget_show (CTK_WIDGET (dialog));

    g_signal_connect (dialog, "response",
                      G_CALLBACK (forget_history_if_yes), NULL);

    ctk_dialog_set_default_response (dialog, CTK_RESPONSE_CANCEL);
}

static void
action_clear_history_callback (CtkAction *action,
                               gpointer user_data)
{
    forget_history_if_confirmed (BAUL_WINDOW (user_data));
}

static void
action_split_view_switch_next_pane_callback(CtkAction *action,
        gpointer user_data)
{
    baul_window_pane_switch_to (baul_window_get_next_pane (BAUL_WINDOW (user_data)));
}

static void
action_split_view_same_location_callback (CtkAction *action,
        gpointer user_data)
{
    BaulWindow *window;
    BaulWindowPane *next_pane;
    GFile *location;

    window = BAUL_WINDOW (user_data);
    next_pane = baul_window_get_next_pane (window);

    if (!next_pane)
    {
        return;
    }
    location = baul_window_slot_get_location (next_pane->active_slot);
    if (location)
    {
        baul_window_slot_go_to (window->details->active_pane->active_slot, location, FALSE);
        g_object_unref (location);
    }
}

static void
action_show_hide_toolbar_callback (CtkAction *action,
                                   gpointer user_data)
{
    BaulNavigationWindow *window;

    window = BAUL_NAVIGATION_WINDOW (user_data);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    if (ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)))
    {
        baul_navigation_window_show_toolbar (window);
    }
    else
    {
        baul_navigation_window_hide_toolbar (window);
    }
    G_GNUC_END_IGNORE_DEPRECATIONS;
}



static void
action_show_hide_sidebar_callback (CtkAction *action,
                                   gpointer user_data)
{
    BaulNavigationWindow *window;

    window = BAUL_NAVIGATION_WINDOW (user_data);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    if (ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)))
    {
        baul_navigation_window_show_sidebar (window);
    }
    else
    {
        baul_navigation_window_hide_sidebar (window);
    }
    G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
pane_show_hide_location_bar (BaulNavigationWindowPane *pane, gboolean is_active)
{
    if (baul_navigation_window_pane_location_bar_showing (pane) != is_active)
    {
        if (is_active)
        {
            baul_navigation_window_pane_show_location_bar (pane, TRUE);
        }
        else
        {
            baul_navigation_window_pane_hide_location_bar (pane, TRUE);
        }
    }
}

static void
action_show_hide_location_bar_callback (CtkAction *action,
                                        gpointer user_data)
{
    BaulWindow *window;
    GList *walk;
    gboolean is_active;

    window = BAUL_WINDOW (user_data);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    is_active = ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action));
    G_GNUC_END_IGNORE_DEPRECATIONS;

    /* Do the active pane first, because this will trigger an update of the menu items,
     * which in turn relies on the active pane. */
    pane_show_hide_location_bar (BAUL_NAVIGATION_WINDOW_PANE (window->details->active_pane), is_active);

    for (walk = window->details->panes; walk; walk = walk->next)
    {
        pane_show_hide_location_bar (BAUL_NAVIGATION_WINDOW_PANE (walk->data), is_active);
    }
}

static void
action_show_hide_statusbar_callback (CtkAction *action,
                                     gpointer user_data)
{
    BaulNavigationWindow *window;

    window = BAUL_NAVIGATION_WINDOW (user_data);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    if (ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)))
    {
        baul_navigation_window_show_status_bar (window);
    }
    else
    {
        baul_navigation_window_hide_status_bar (window);
    }
    G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
action_split_view_callback (CtkAction *action,
                            gpointer user_data)
{
    BaulNavigationWindow *window;
    gboolean is_active;

    window = BAUL_NAVIGATION_WINDOW (user_data);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    is_active = ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action));
    G_GNUC_END_IGNORE_DEPRECATIONS;
    if (is_active != baul_navigation_window_split_view_showing (window))
    {
        BaulWindow *baul_window;

        if (is_active)
        {
            baul_navigation_window_split_view_on (window);
        }
        else
        {
            baul_navigation_window_split_view_off (window);
        }
        baul_window = BAUL_WINDOW (window);
        if (baul_window->details->active_pane && baul_window->details->active_pane->active_slot)
        {
            baul_view_update_menus (baul_window->details->active_pane->active_slot->content_view);
        }
    }
}

void
baul_navigation_window_update_show_hide_menu_items (BaulNavigationWindow *window)
{
    CtkAction *action;

    g_assert (BAUL_IS_NAVIGATION_WINDOW (window));

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    action = ctk_action_group_get_action (window->details->navigation_action_group,
                                          BAUL_ACTION_SHOW_HIDE_TOOLBAR);
    ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
                                  baul_navigation_window_toolbar_showing (window));

    action = ctk_action_group_get_action (window->details->navigation_action_group,
                                          BAUL_ACTION_SHOW_HIDE_SIDEBAR);
    ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
                                  baul_navigation_window_sidebar_showing (window));

    action = ctk_action_group_get_action (window->details->navigation_action_group,
                                          BAUL_ACTION_SHOW_HIDE_LOCATION_BAR);
    ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
                                  baul_navigation_window_pane_location_bar_showing (BAUL_NAVIGATION_WINDOW_PANE (BAUL_WINDOW (window)->details->active_pane)));

    action = ctk_action_group_get_action (window->details->navigation_action_group,
                                          BAUL_ACTION_SHOW_HIDE_STATUSBAR);
    ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
                                  baul_navigation_window_status_bar_showing (window));

    action = ctk_action_group_get_action (window->details->navigation_action_group,
                                          BAUL_ACTION_SHOW_HIDE_EXTRA_PANE);
    ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
                                  baul_navigation_window_split_view_showing (window));
    G_GNUC_END_IGNORE_DEPRECATIONS;
}

void
baul_navigation_window_update_spatial_menu_item (BaulNavigationWindow *window)
{
    CtkAction *action;

    g_assert (BAUL_IS_NAVIGATION_WINDOW (window));

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    action = ctk_action_group_get_action (window->details->navigation_action_group,
                                          BAUL_ACTION_FOLDER_WINDOW);
    ctk_action_set_visible (action,
                            !g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_ALWAYS_USE_BROWSER));
    G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
action_add_bookmark_callback (CtkAction *action,
                              gpointer user_data)
{
    baul_window_add_bookmark_for_current_location (BAUL_WINDOW (user_data));
}

static void
action_edit_bookmarks_callback (CtkAction *action,
                                gpointer user_data)
{
    baul_window_edit_bookmarks (BAUL_WINDOW (user_data));
}

void
baul_navigation_window_remove_go_menu_callback (BaulNavigationWindow *window)
{
    if (window->details->refresh_go_menu_idle_id != 0)
    {
        g_source_remove (window->details->refresh_go_menu_idle_id);
        window->details->refresh_go_menu_idle_id = 0;
    }
}

void
baul_navigation_window_remove_go_menu_items (BaulNavigationWindow *window)
{
    CtkUIManager *ui_manager;

    ui_manager = baul_window_get_ui_manager (BAUL_WINDOW (window));
    if (window->details->go_menu_merge_id != 0)
    {
        ctk_ui_manager_remove_ui (ui_manager,
                                  window->details->go_menu_merge_id);
        window->details->go_menu_merge_id = 0;
    }
    if (window->details->go_menu_action_group != NULL)
    {
        ctk_ui_manager_remove_action_group (ui_manager,
                                            window->details->go_menu_action_group);
        window->details->go_menu_action_group = NULL;
    }
}

static void
show_bogus_history_window (BaulWindow *window,
                           BaulBookmark *bookmark)
{
    GFile *file;
    char *uri_for_display;
    char *detail;

    file = baul_bookmark_get_location (bookmark);
    uri_for_display = g_file_get_parse_name (file);

    detail = g_strdup_printf (_("The location \"%s\" does not exist."), uri_for_display);

    eel_show_warning_dialog (_("The history location doesn't exist."),
                             detail,
                             CTK_WINDOW (window));

    g_object_unref (file);
    g_free (uri_for_display);
    g_free (detail);
}

static void
connect_proxy_cb (CtkActionGroup *action_group,
                  CtkAction *action,
                  CtkWidget *proxy,
                  gpointer dummy)
{
    CtkLabel *label;

    if (!CTK_IS_MENU_ITEM (proxy))
        return;

    label = CTK_LABEL (ctk_bin_get_child (CTK_BIN (proxy)));

    ctk_label_set_use_underline (label, FALSE);
    ctk_label_set_ellipsize (label, PANGO_ELLIPSIZE_END);
    ctk_label_set_max_width_chars (label, MENU_ITEM_MAX_WIDTH_CHARS);
}

/**
 * refresh_go_menu:
 *
 * Refresh list of bookmarks at end of Go menu to match centralized history list.
 * @window: The BaulWindow whose Go menu will be refreshed.
 **/
static void
refresh_go_menu (BaulNavigationWindow *window)
{
    CtkUIManager *ui_manager;
    GList *node;
    int index;

    g_assert (BAUL_IS_NAVIGATION_WINDOW (window));

    /* Unregister any pending call to this function. */
    baul_navigation_window_remove_go_menu_callback (window);

    /* Remove old set of history items. */
    baul_navigation_window_remove_go_menu_items (window);

    ui_manager = baul_window_get_ui_manager (BAUL_WINDOW (window));

    window->details->go_menu_merge_id = ctk_ui_manager_new_merge_id (ui_manager);
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    window->details->go_menu_action_group = ctk_action_group_new ("GoMenuGroup");
    G_GNUC_END_IGNORE_DEPRECATIONS;
    g_signal_connect (window->details->go_menu_action_group, "connect-proxy",
                      G_CALLBACK (connect_proxy_cb), NULL);

    ctk_ui_manager_insert_action_group (ui_manager,
                                        window->details->go_menu_action_group,
                                        -1);
    g_object_unref (window->details->go_menu_action_group);

    /* Add in a new set of history items. */
    for (node = baul_get_history_list (), index = 0;
            node != NULL && index < 10;
            node = node->next, index++)
    {
        baul_menus_append_bookmark_to_menu
        (BAUL_WINDOW (window),
         BAUL_BOOKMARK (node->data),
         MENU_PATH_HISTORY_PLACEHOLDER,
         "history",
         index,
         window->details->go_menu_action_group,
         window->details->go_menu_merge_id,
         G_CALLBACK (schedule_refresh_go_menu),
         show_bogus_history_window);
    }
}

static gboolean
refresh_go_menu_idle_callback (gpointer data)
{
    g_assert (BAUL_IS_NAVIGATION_WINDOW (data));

    refresh_go_menu (BAUL_NAVIGATION_WINDOW (data));

    /* Don't call this again (unless rescheduled) */
    return FALSE;
}

static void
schedule_refresh_go_menu (BaulNavigationWindow *window)
{
    g_assert (BAUL_IS_NAVIGATION_WINDOW (window));

    if (window->details->refresh_go_menu_idle_id == 0)
    {
        window->details->refresh_go_menu_idle_id
            = g_idle_add (refresh_go_menu_idle_callback,
                          window);
    }
}

/**
 * baul_navigation_window_initialize_go_menu
 *
 * Wire up signals so we'll be notified when history list changes.
 */
static void
baul_navigation_window_initialize_go_menu (BaulNavigationWindow *window)
{
    /* Recreate bookmarks part of menu if history list changes
     */
    g_signal_connect_object (baul_signaller_get_current (), "history_list_changed",
                             G_CALLBACK (schedule_refresh_go_menu), window, G_CONNECT_SWAPPED);
}

void
baul_navigation_window_update_split_view_actions_sensitivity (BaulNavigationWindow *window)
{
    BaulWindow *win;
    CtkActionGroup *action_group;
    CtkAction *action;
    gboolean have_multiple_panes;
    gboolean next_pane_is_in_same_location;
    GFile *active_pane_location;
    GFile *next_pane_location;
    BaulWindowPane *next_pane;

    g_assert (BAUL_IS_NAVIGATION_WINDOW (window));

    action_group = window->details->navigation_action_group;
    win = BAUL_WINDOW (window);

    /* collect information */
    have_multiple_panes = (win->details->panes && win->details->panes->next);
    if (win->details->active_pane->active_slot)
    {
        active_pane_location = baul_window_slot_get_location (win->details->active_pane->active_slot);
    }
    else
    {
        active_pane_location = NULL;
    }
    next_pane = baul_window_get_next_pane (win);
    if (next_pane && next_pane->active_slot)
    {
        next_pane_location = baul_window_slot_get_location (next_pane->active_slot);
        next_pane_is_in_same_location = (active_pane_location && next_pane_location &&
                                         g_file_equal (active_pane_location, next_pane_location));
    }
    else
    {
        next_pane_location = NULL;
        next_pane_is_in_same_location = FALSE;
    }

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    /* switch to next pane */
    action = ctk_action_group_get_action (action_group, "SplitViewNextPane");
    ctk_action_set_sensitive (action, have_multiple_panes);

    /* same location */
    action = ctk_action_group_get_action (action_group, "SplitViewSameLocation");
    ctk_action_set_sensitive (action, have_multiple_panes && !next_pane_is_in_same_location);
    G_GNUC_END_IGNORE_DEPRECATIONS;

    /* clean up */
    if (active_pane_location)
    {
        g_object_unref (active_pane_location);
    }
    if (next_pane_location)
    {
        g_object_unref (next_pane_location);
    }
}

static void
action_new_window_callback (CtkAction *action,
                            gpointer user_data)
{
    BaulWindow *current_window;

    current_window = BAUL_WINDOW (user_data);   
    baul_window_new_window (current_window);
}


static void
action_new_tab_callback (CtkAction *action,
                         gpointer user_data)
{
    BaulWindow *window;

    window = BAUL_WINDOW (user_data);
    baul_window_new_tab (window);
}

static void
action_folder_window_callback (CtkAction *action,
                               gpointer user_data)
{
    BaulWindow *current_window, *window;
    BaulWindowSlot *slot;
    GFile *current_location;

    current_window = BAUL_WINDOW (user_data);
    slot = current_window->details->active_pane->active_slot;
    current_location = baul_window_slot_get_location (slot);
    window = baul_application_get_spatial_window
            (current_window->application,
             current_window,
             NULL,
             current_location,
             ctk_window_get_screen (CTK_WINDOW (current_window)),
             NULL);

    baul_window_go_to (window, current_location);

    if (current_location != NULL)
    {
        g_object_unref (current_location);
    }
}

static void
action_go_to_location_callback (CtkAction *action,
                                gpointer user_data)
{
    BaulWindow *window;

    window = BAUL_WINDOW (user_data);

    baul_window_prompt_for_location (window, NULL);
}

/* The ctrl-f Keyboard shortcut always enables, rather than toggles
   the search mode */
static void
action_show_search_callback (CtkAction *action,
                             gpointer user_data)
{
    CtkAction *search_action;
    BaulNavigationWindow *window;

    window = BAUL_NAVIGATION_WINDOW (user_data);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    search_action =
        ctk_action_group_get_action (window->details->navigation_action_group,
                                     BAUL_ACTION_SEARCH);

    if (ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (search_action)))
    {
        /* Already visible, just show it */
        baul_navigation_window_show_search (window);
    }
    else
    {
        /* Otherwise, enable */
        ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (search_action),
                                      TRUE);
    }
    G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
action_show_hide_search_callback (CtkAction *action,
                                  gpointer user_data)
{
    gboolean var_action;
    BaulNavigationWindow *window;

    /* This is used when toggling the action for updating the UI
       state only, not actually activating the action */
    if (g_object_get_data (G_OBJECT (action), "blocked") != NULL)
    {
        return;
    }

    window = BAUL_NAVIGATION_WINDOW (user_data);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    var_action = ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action));
    G_GNUC_END_IGNORE_DEPRECATIONS;

    if (var_action)
    {
        baul_navigation_window_show_search (window);
    }
    else
    {
        BaulWindowSlot *slot;
        GFile *location = NULL;

        slot = BAUL_WINDOW (window)->details->active_pane->active_slot;

        /* Use the location bar as the return location */
        if (slot->query_editor == NULL)
        {
            location = baul_window_slot_get_location (slot);
            /* Use the search location as the return location */
        }
        else
        {
            BaulQuery *query;

            query = baul_query_editor_get_query (slot->query_editor);
            if (query != NULL)
            {
                char *uri;

                uri = baul_query_get_location (query);
                if (uri != NULL)
                {
                    location = g_file_new_for_uri (uri);
                    g_free (uri);
                }
                g_object_unref (query);
            }
        }

        /* Last try: use the home directory as the return location */
        if (location == NULL)
        {
            location = g_file_new_for_path (g_get_home_dir ());
        }

        baul_window_go_to (BAUL_WINDOW (window), location);
        g_object_unref (location);

        baul_navigation_window_hide_search (window);
    }
}

static void
action_tabs_previous_callback (CtkAction *action,
                               gpointer user_data)
{
    BaulNavigationWindowPane *pane;

    pane = BAUL_NAVIGATION_WINDOW_PANE (BAUL_WINDOW (user_data)->details->active_pane);
    baul_notebook_set_current_page_relative (BAUL_NOTEBOOK (pane->notebook), -1);
}

static void
action_tabs_next_callback (CtkAction *action,
                           gpointer user_data)
{
    BaulNavigationWindowPane *pane;

    pane = BAUL_NAVIGATION_WINDOW_PANE (BAUL_WINDOW (user_data)->details->active_pane);
    baul_notebook_set_current_page_relative (BAUL_NOTEBOOK (pane->notebook), 1);
}

static void
action_tabs_move_left_callback (CtkAction *action,
                                gpointer user_data)
{
    BaulNavigationWindowPane *pane;

    pane = BAUL_NAVIGATION_WINDOW_PANE (BAUL_WINDOW (user_data)->details->active_pane);
    baul_notebook_reorder_current_child_relative (BAUL_NOTEBOOK (pane->notebook), -1);
}

static void
action_tabs_move_right_callback (CtkAction *action,
                                 gpointer user_data)
{
    BaulNavigationWindowPane *pane;

    pane = BAUL_NAVIGATION_WINDOW_PANE (BAUL_WINDOW (user_data)->details->active_pane);
    baul_notebook_reorder_current_child_relative (BAUL_NOTEBOOK (pane->notebook), 1);
}

static void
action_tab_change_action_activate_callback (CtkAction *action, gpointer user_data)
{
    BaulWindow *window;

    window = BAUL_WINDOW (user_data);
    if (window && window->details->active_pane)
    {
        CtkNotebook *notebook;
        notebook = CTK_NOTEBOOK (BAUL_NAVIGATION_WINDOW_PANE (window->details->active_pane)->notebook);
        if (notebook)
        {
            int num;
            num = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (action), "num"));
            if (num < ctk_notebook_get_n_pages (notebook))
            {
                ctk_notebook_set_current_page (notebook, num);
            }
        }
    }
}

static const CtkActionEntry navigation_entries[] =
{
    /* name, icon name, label */ { "Go", NULL, N_("_Go") },
    /* name, icon name, label */ { "Bookmarks", NULL, N_("_Bookmarks") },
    /* name, icon name, label */ { "Tabs", NULL, N_("_Tabs") },
    /* name, icon name, label */ { "New Window", "window-new", N_("New _Window"),
        "<control>N", N_("Open another Baul window for the displayed location"),
        G_CALLBACK (action_new_window_callback)
    },
    /* name, icon name, label */ { "New Tab", "tab-new", N_("New _Tab"),
        "<control>T", N_("Open another tab for the displayed location"),
        G_CALLBACK (action_new_tab_callback)
    },
    /* name, icon name, label */ { "Folder Window", "folder", N_("Open Folder W_indow"),
        NULL, N_("Open a folder window for the displayed location"),
        G_CALLBACK (action_folder_window_callback)
    },
    /* name, icon name, label */ { "Close All Windows", NULL, N_("Close _All Windows"),
        "<control>Q", N_("Close all Navigation windows"),
        G_CALLBACK (action_close_all_windows_callback)
    },
    /* name, icon name, label */ { "Go to Location", NULL, N_("_Location..."),
        "<control>L", N_("Specify a location to open"),
        G_CALLBACK (action_go_to_location_callback)
    },
    /* name, icon name, label */ { "Clear History", NULL, N_("Clea_r History"),
        NULL, N_("Clear contents of Go menu and Back/Forward lists"),
        G_CALLBACK (action_clear_history_callback)
    },
    /* name, icon name, label */ { "SplitViewNextPane", NULL, N_("S_witch to Other Pane"),
        "F6", N_("Move focus to the other pane in a split view window"),
        G_CALLBACK (action_split_view_switch_next_pane_callback)
    },
    /* name, icon name, label */ { "SplitViewSameLocation", NULL, N_("Sa_me Location as Other Pane"),
        NULL, N_("Go to the same location as in the extra pane"),
        G_CALLBACK (action_split_view_same_location_callback)
    },
    /* name, icon name, label */ { "Add Bookmark", "list-add", N_("_Add Bookmark"),
        "<control>d", N_("Add a bookmark for the current location to this menu"),
        G_CALLBACK (action_add_bookmark_callback)
    },
    /* name, icon name, label */ { "Edit Bookmarks", NULL, N_("_Edit Bookmarks..."),
        "<control>b", N_("Display a window that allows editing the bookmarks in this menu"),
        G_CALLBACK (action_edit_bookmarks_callback)
    },
    {
        "TabsPrevious", NULL, N_("_Previous Tab"), "<control>Page_Up",
        N_("Activate previous tab"),
        G_CALLBACK (action_tabs_previous_callback)
    },
    {
        "TabsNext", NULL, N_("_Next Tab"), "<control>Page_Down",
        N_("Activate next tab"),
        G_CALLBACK (action_tabs_next_callback)
    },
    {
        "TabsMoveLeft", NULL, N_("Move Tab _Left"), "<shift><control>Page_Up",
        N_("Move current tab to left"),
        G_CALLBACK (action_tabs_move_left_callback)
    },
    {
        "TabsMoveRight", NULL, N_("Move Tab _Right"), "<shift><control>Page_Down",
        N_("Move current tab to right"),
        G_CALLBACK (action_tabs_move_right_callback)
    },
    {
        "ShowSearch", NULL, N_("S_how Search"), "<control>f",
        N_("Show search"),
        G_CALLBACK (action_show_search_callback)
    }
};

static const CtkToggleActionEntry navigation_toggle_entries[] =
{
    /* name, icon name */    { "Show Hide Toolbar", NULL,
        /* label, accelerator */   N_("_Main Toolbar"), NULL,
        /* tooltip */              N_("Change the visibility of this window's main toolbar"),
        G_CALLBACK (action_show_hide_toolbar_callback),
        /* is_active */            TRUE
    },
    /* name, icon name */    { "Show Hide Sidebar", NULL,
        /* label, accelerator */   N_("_Side Pane"), "F9",
        /* tooltip */              N_("Change the visibility of this window's side pane"),
        G_CALLBACK (action_show_hide_sidebar_callback),
        /* is_active */            TRUE
    },
    /* name, icon name */    { "Show Hide Location Bar", NULL,
        /* label, accelerator */   N_("Location _Bar"), NULL,
        /* tooltip */              N_("Change the visibility of this window's location bar"),
        G_CALLBACK (action_show_hide_location_bar_callback),
        /* is_active */            TRUE
    },
    /* name, icon name */    { "Show Hide Statusbar", NULL,
        /* label, accelerator */   N_("St_atusbar"), NULL,
        /* tooltip */              N_("Change the visibility of this window's statusbar"),
        G_CALLBACK (action_show_hide_statusbar_callback),
        /* is_active */            TRUE
    },
    /* name, icon name */    { "Search", "edit-find",
        /* label, accelerator */   N_("_Search for Files..."),
        /* Accelerator is in ShowSearch */"",
        /* tooltip */              N_("Search documents and folders by name"),
        G_CALLBACK (action_show_hide_search_callback),
        /* is_active */            FALSE
    },
    /* name, icon name */    {
        BAUL_ACTION_SHOW_HIDE_EXTRA_PANE, NULL,
        /* label, accelerator */   N_("E_xtra Pane"), "F3",
        /* tooltip */              N_("Open an extra folder view side-by-side"),
        G_CALLBACK (action_split_view_callback),
        /* is_active */            FALSE
    },
};

void
baul_navigation_window_initialize_actions (BaulNavigationWindow *window)
{
    CtkActionGroup *action_group;
    CtkUIManager *ui_manager;
    CtkAction *action;
    int i;

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    action_group = ctk_action_group_new ("NavigationActions");
    ctk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    window->details->navigation_action_group = action_group;
    ctk_action_group_add_actions (action_group,
                                  navigation_entries, G_N_ELEMENTS (navigation_entries),
                                  window);
    ctk_action_group_add_toggle_actions (action_group,
                                         navigation_toggle_entries, G_N_ELEMENTS (navigation_toggle_entries),
                                         window);

    action = g_object_new (BAUL_TYPE_NAVIGATION_ACTION,
                           "name", "Back",
                           "label", _("_Back"),
                           "icon-name", "go-previous",
                           "tooltip", _("Go to the previous visited location"),
                           "arrow-tooltip", _("Back history"),
                           "window", window,
                           "direction", BAUL_NAVIGATION_DIRECTION_BACK,
                           "is_important", TRUE,
                           NULL);
    g_signal_connect (action, "activate",
                      G_CALLBACK (action_back_callback), window);
    ctk_action_group_add_action_with_accel (action_group,
                                            action,
                                            "<alt>Left");
    g_object_unref (action);

    action = g_object_new (BAUL_TYPE_NAVIGATION_ACTION,
                           "name", "Forward",
                           "label", _("_Forward"),
                           "icon-name", "go-next",
                           "tooltip", _("Go to the next visited location"),
                           "arrow-tooltip", _("Forward history"),
                           "window", window,
                           "direction", BAUL_NAVIGATION_DIRECTION_FORWARD,
                           "is_important", TRUE,
                           NULL);
    g_signal_connect (action, "activate",
                      G_CALLBACK (action_forward_callback), window);
    ctk_action_group_add_action_with_accel (action_group,
                                            action,
                                            "<alt>Right");

    g_object_unref (action);

    action = g_object_new (BAUL_TYPE_ZOOM_ACTION,
                           "name", "Zoom",
                           "label", _("_Zoom"),
                           "window", window,
                           "is_important", FALSE,
                           NULL);
    ctk_action_group_add_action (action_group,
                                 action);
    g_object_unref (action);

    action = g_object_new (BAUL_TYPE_VIEW_AS_ACTION,
                           "name", "ViewAs",
                           "label", _("_View As"),
                           "window", window,
                           "is_important", FALSE,
                           NULL);
    ctk_action_group_add_action (action_group,
                                 action);
    g_object_unref (action);
    G_GNUC_END_IGNORE_DEPRECATIONS;

    ui_manager = baul_window_get_ui_manager (BAUL_WINDOW (window));

    /* Alt+N for the first 10 tabs */
    for (i = 0; i < 10; ++i)
    {
        gchar action_name[80];
        gchar accelerator[80];

        snprintf(action_name, sizeof (action_name), "Tab%d", i);
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
        action = ctk_action_new (action_name, NULL, NULL, NULL);
        g_object_set_data (G_OBJECT (action), "num", GINT_TO_POINTER (i));
        g_signal_connect (action, "activate",
                          G_CALLBACK (action_tab_change_action_activate_callback), window);
        snprintf(accelerator, sizeof (accelerator), "<alt>%d", (i+1)%10);
        ctk_action_group_add_action_with_accel (action_group, action, accelerator);
        g_object_unref (action);
        G_GNUC_END_IGNORE_DEPRECATIONS;
        ctk_ui_manager_add_ui (ui_manager,
                               ctk_ui_manager_new_merge_id (ui_manager),
                               "/",
                               action_name,
                               action_name,
                               CTK_UI_MANAGER_ACCELERATOR,
                               FALSE);

    }

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    action = ctk_action_group_get_action (action_group, BAUL_ACTION_SEARCH);
    g_object_set (action, "short_label", _("_Search"), NULL);

    action = ctk_action_group_get_action (action_group, "ShowSearch");
    ctk_action_set_sensitive (action, TRUE);
    G_GNUC_END_IGNORE_DEPRECATIONS;

    ctk_ui_manager_insert_action_group (ui_manager, action_group, 0);
    g_object_unref (action_group); /* owned by ui_manager */

    g_signal_connect (window, "loading_uri",
                      G_CALLBACK (baul_navigation_window_update_split_view_actions_sensitivity),
                      NULL);

    baul_navigation_window_update_split_view_actions_sensitivity (window);
}


/**
 * baul_window_initialize_menus
 *
 * Create and install the set of menus for this window.
 * @window: A recently-created BaulWindow.
 */
void
baul_navigation_window_initialize_menus (BaulNavigationWindow *window)
{
    CtkUIManager *ui_manager;
    const char *ui;

    ui_manager = baul_window_get_ui_manager (BAUL_WINDOW (window));

    ui = baul_ui_string_get ("baul-navigation-window-ui.xml");
    ctk_ui_manager_add_ui_from_string (ui_manager, ui, -1, NULL);

    baul_navigation_window_update_show_hide_menu_items (window);
    baul_navigation_window_update_spatial_menu_item (window);

    baul_navigation_window_initialize_go_menu (window);
}
