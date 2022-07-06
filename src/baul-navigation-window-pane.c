/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-navigation-window-pane.c: Baul navigation window pane

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

   Author: Holger Berndt <berndth@gmx.de>
*/

#include <eel/eel-ctk-extensions.h>

#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-window-slot-info.h>
#include <libbaul-private/baul-view-factory.h>
#include <libbaul-private/baul-entry.h>

#include "baul-navigation-window-pane.h"
#include "baul-window-private.h"
#include "baul-window-manage-views.h"
#include "baul-pathbar.h"
#include "baul-location-bar.h"
#include "baul-notebook.h"
#include "baul-window-slot.h"

static void baul_navigation_window_pane_dispose    (GObject *object);

G_DEFINE_TYPE (BaulNavigationWindowPane,
               baul_navigation_window_pane,
               BAUL_TYPE_WINDOW_PANE)
#define parent_class baul_navigation_window_pane_parent_class

static void
real_set_active (BaulWindowPane *pane, gboolean is_active)
{
    BaulNavigationWindowPane *nav_pane;
    GList *l;

    nav_pane = BAUL_NAVIGATION_WINDOW_PANE (pane);

    /* path bar */
    for (l = BAUL_PATH_BAR (nav_pane->path_bar)->button_list; l; l = l->next)
    {
        ctk_widget_set_sensitive (ctk_bin_get_child (CTK_BIN (baul_path_bar_get_button_from_button_list_entry (l->data))), is_active);
    }

    /* navigation bar (manual entry) */
    baul_location_bar_set_active (BAUL_LOCATION_BAR (nav_pane->navigation_bar), is_active);

    /* location button */
    ctk_widget_set_sensitive (ctk_bin_get_child (CTK_BIN (nav_pane->location_button)), is_active);
}

static gboolean
navigation_bar_focus_in_callback (CtkWidget *widget, CdkEventFocus *event, gpointer user_data)
{
    BaulWindowPane *pane;
    pane = BAUL_WINDOW_PANE (user_data);
    baul_window_set_active_pane (pane->window, pane);
    return FALSE;
}

static int
bookmark_list_get_uri_index (GList *list, GFile *location)
{
    GList *l;
    int i;
    BaulBookmark *bookmark = NULL;
    GFile *tmp = NULL;

    g_return_val_if_fail (location != NULL, -1);

    for (i = 0, l = list; l != NULL; i++, l = l->next)
    {
        bookmark = BAUL_BOOKMARK (l->data);

        tmp = baul_bookmark_get_location (bookmark);
        if (g_file_equal (location, tmp))
        {
            g_object_unref (tmp);
            return i;
        }
        g_object_unref (tmp);
    }

    return -1;
}

static void
search_bar_focus_in_callback (BaulSearchBar *bar,
                              BaulWindowPane *pane)
{
    baul_window_set_active_pane (pane->window, pane);
}


static void
search_bar_activate_callback (BaulSearchBar *bar,
                              BaulNavigationWindowPane *pane)
{
    char *uri;
    BaulDirectory *directory;
    BaulSearchDirectory *search_directory;
    BaulQuery *query;
    GFile *location;

    uri = baul_search_directory_generate_new_uri ();
    location = g_file_new_for_uri (uri);
    g_free (uri);

    directory = baul_directory_get (location);

    g_assert (BAUL_IS_SEARCH_DIRECTORY (directory));

    search_directory = BAUL_SEARCH_DIRECTORY (directory);

    query = baul_search_bar_get_query (BAUL_SEARCH_BAR (pane->search_bar));
    if (query != NULL)
    {
        BaulWindowSlot *slot = BAUL_WINDOW_PANE (pane)->active_slot;
        if (!baul_search_directory_is_indexed (search_directory))
        {
            char *current_uri;

            current_uri = baul_window_slot_get_location_uri (slot);
            baul_query_set_location (query, current_uri);
            g_free (current_uri);
        }
        baul_search_directory_set_query (search_directory, query);
        g_object_unref (query);
    }

    baul_window_slot_go_to (BAUL_WINDOW_PANE (pane)->active_slot, location, FALSE);

    baul_directory_unref (directory);
    g_object_unref (location);
}

static void
search_bar_cancel_callback (CtkWidget *widget,
                            BaulNavigationWindowPane *pane)
{
    if (baul_navigation_window_pane_hide_temporary_bars (pane))
    {
        baul_navigation_window_restore_focus_widget (BAUL_NAVIGATION_WINDOW (BAUL_WINDOW_PANE (pane)->window));
    }
}

static void
navigation_bar_cancel_callback (CtkWidget *widget,
                                BaulNavigationWindowPane *pane)
{
    if (baul_navigation_window_pane_hide_temporary_bars (pane))
    {
        baul_navigation_window_restore_focus_widget (BAUL_NAVIGATION_WINDOW (BAUL_WINDOW_PANE (pane)->window));
    }
}

static void
navigation_bar_location_changed_callback (CtkWidget *widget,
        const char *uri,
        BaulNavigationWindowPane *pane)
{
    GFile *location;

    if (baul_navigation_window_pane_hide_temporary_bars (pane))
    {
        baul_navigation_window_restore_focus_widget (BAUL_NAVIGATION_WINDOW (BAUL_WINDOW_PANE (pane)->window));
    }

    location = g_file_new_for_uri (uri);
    baul_window_slot_go_to (BAUL_WINDOW_PANE (pane)->active_slot, location, FALSE);
    g_object_unref (location);
}

static void
path_bar_location_changed_callback (CtkWidget *widget,
                                    GFile *location,
                                    BaulNavigationWindowPane *pane)
{
    BaulNavigationWindowSlot *slot;
    BaulWindowPane *win_pane;
    int i;

    g_assert (BAUL_IS_NAVIGATION_WINDOW_PANE (pane));

    win_pane = BAUL_WINDOW_PANE(pane);

    slot = BAUL_NAVIGATION_WINDOW_SLOT (win_pane->active_slot);

    /* Make sure we are changing the location on the correct pane */
    baul_window_set_active_pane (BAUL_WINDOW_PANE (pane)->window, BAUL_WINDOW_PANE (pane));

    /* check whether we already visited the target location */
    i = bookmark_list_get_uri_index (slot->back_list, location);
    if (i >= 0)
    {
        baul_navigation_window_back_or_forward (BAUL_NAVIGATION_WINDOW (win_pane->window), TRUE, i, FALSE);
    }
    else
    {
        baul_window_slot_go_to (win_pane->active_slot, location, FALSE);
    }
}

static gboolean
location_button_should_be_active (BaulNavigationWindowPane *pane)
{
    return g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_ALWAYS_USE_LOCATION_ENTRY);
}

static void
location_button_toggled_cb (CtkToggleButton *toggle,
                            BaulNavigationWindowPane *pane)
{
    gboolean is_active;

    is_active = ctk_toggle_button_get_active (toggle);
    g_settings_set_boolean (baul_preferences, BAUL_PREFERENCES_ALWAYS_USE_LOCATION_ENTRY, is_active);

    if (is_active) {
        baul_location_bar_activate (BAUL_LOCATION_BAR (pane->navigation_bar));
    }

    baul_window_set_active_pane (BAUL_WINDOW_PANE (pane)->window, BAUL_WINDOW_PANE (pane));
}

static CtkWidget *
location_button_create (BaulNavigationWindowPane *pane)
{
    CtkWidget *image;
    CtkWidget *button;

    image = ctk_image_new_from_icon_name ("ctk-edit", CTK_ICON_SIZE_MENU);
    ctk_widget_show (image);

    button = g_object_new (CTK_TYPE_TOGGLE_BUTTON,
                   "image", image,
                   "focus-on-click", FALSE,
                   "active", location_button_should_be_active (pane),
                   NULL);

    ctk_widget_set_tooltip_text (button,
                     _("Toggle between button and text-based location bar"));

    g_signal_connect (button, "toggled",
              G_CALLBACK (location_button_toggled_cb), pane);
    return button;
}

static gboolean
path_bar_path_event_callback (BaulPathBar *path_bar,
                   GFile *location,
                   CdkEventButton *event,
                   BaulWindowPane *pane)

{
    BaulWindowSlot *slot;
    BaulWindowOpenFlags flags;

    if (event->type == CDK_BUTTON_RELEASE) {
        int mask;

        mask = event->state & ctk_accelerator_get_default_mod_mask ();
        flags = 0;

        if (event->button == 2 && mask == 0)
        {
            flags = BAUL_WINDOW_OPEN_FLAG_NEW_TAB;
        }
        else if (event->button == 1 && mask == CDK_CONTROL_MASK)
        {
            flags = BAUL_WINDOW_OPEN_FLAG_NEW_WINDOW;
        }

        if (flags != 0)
        {
            slot = baul_window_get_active_slot (BAUL_WINDOW_PANE (pane)->window);
            baul_window_slot_info_open_location (slot, location,
                                                 BAUL_WINDOW_OPEN_ACCORDING_TO_MODE,
                                                 flags, NULL);
        }

         return FALSE;
    }

    if (event->button == 3) {
        BaulView *view;

        slot = baul_window_get_active_slot (pane->window);
        view = slot->content_view;

        if (view != NULL) {
            char *uri;

            uri = g_file_get_uri (location);
            baul_view_pop_up_location_context_menu (view, event, uri);
            g_free (uri);
        }
        return TRUE;
    }
    return FALSE;
}

static void
notebook_popup_menu_new_tab_cb (CtkMenuItem *menuitem,
    			gpointer user_data)
{
    BaulWindowPane *pane;

    pane = BAUL_WINDOW_PANE (user_data);
    baul_window_new_tab (pane->window);
}

static void
notebook_popup_menu_move_left_cb (CtkMenuItem *menuitem,
                                  gpointer user_data)
{
    BaulNavigationWindowPane *pane;

    pane = BAUL_NAVIGATION_WINDOW_PANE (user_data);
    baul_notebook_reorder_current_child_relative (BAUL_NOTEBOOK (pane->notebook), -1);
}

static void
notebook_popup_menu_move_right_cb (CtkMenuItem *menuitem,
                                   gpointer user_data)
{
    BaulNavigationWindowPane *pane;

    pane = BAUL_NAVIGATION_WINDOW_PANE (user_data);
    baul_notebook_reorder_current_child_relative (BAUL_NOTEBOOK (pane->notebook), 1);
}

static void
notebook_popup_menu_close_cb (CtkMenuItem *menuitem,
                              gpointer user_data)
{
    BaulWindowPane *pane;
    BaulWindowSlot *slot;

    pane = BAUL_WINDOW_PANE (user_data);
    slot = pane->active_slot;
    baul_window_slot_close (slot);
}

static void
notebook_popup_menu_show (BaulNavigationWindowPane *pane,
                          CdkEventButton *event)
{
    CtkWidget *popup;
    CtkWidget *item;
    gboolean can_move_left, can_move_right;
    BaulNotebook *notebook;

    notebook = BAUL_NOTEBOOK (pane->notebook);

    can_move_left = baul_notebook_can_reorder_current_child_relative (notebook, -1);
    can_move_right = baul_notebook_can_reorder_current_child_relative (notebook, 1);

    popup = ctk_menu_new();

    ctk_menu_set_reserve_toggle_size (CTK_MENU (popup), FALSE);

    item = eel_image_menu_item_new_from_icon (NULL, _("_New Tab"));
    g_signal_connect (item, "activate",
    		  G_CALLBACK (notebook_popup_menu_new_tab_cb),
    		  pane);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup),
    		       item);

    ctk_menu_shell_append (CTK_MENU_SHELL (popup),
    		       ctk_separator_menu_item_new ());

    item = eel_image_menu_item_new_from_icon (NULL, _("Move Tab _Left"));
    g_signal_connect (item, "activate",
                      G_CALLBACK (notebook_popup_menu_move_left_cb),
                      pane);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup),
                           item);
    ctk_widget_set_sensitive (item, can_move_left);

    item = eel_image_menu_item_new_from_icon (NULL, _("Move Tab _Right"));
    g_signal_connect (item, "activate",
                      G_CALLBACK (notebook_popup_menu_move_right_cb),
                      pane);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup),
                           item);
    ctk_widget_set_sensitive (item, can_move_right);

    ctk_menu_shell_append (CTK_MENU_SHELL (popup),
                           ctk_separator_menu_item_new ());

    item = eel_image_menu_item_new_from_icon ("window-close", _("_Close Tab"));

    g_signal_connect (item, "activate",
                      G_CALLBACK (notebook_popup_menu_close_cb), pane);
    ctk_menu_shell_append (CTK_MENU_SHELL (popup),
                           item);

    ctk_widget_show_all (popup);

    /* TODO is this correct? */
    ctk_menu_attach_to_widget (CTK_MENU (popup),
                               pane->notebook,
                               NULL);

    ctk_menu_popup_at_pointer (CTK_MENU (popup),
                               (const CdkEvent*) event);
}

/* emitted when the user clicks the "close" button of tabs */
static void
notebook_tab_close_requested (BaulNotebook *notebook,
                              BaulWindowSlot *slot,
                              BaulWindowPane *pane)
{
    baul_window_pane_slot_close (pane, slot);
}

static gboolean
notebook_button_press_cb (CtkWidget *widget,
                          CdkEventButton *event,
                          gpointer user_data)
{
    BaulNavigationWindowPane *pane;

    pane = BAUL_NAVIGATION_WINDOW_PANE (user_data);
    if (CDK_BUTTON_PRESS == event->type && 3 == event->button)
    {
        notebook_popup_menu_show (pane, event);
        return TRUE;
    }
    else if (CDK_BUTTON_PRESS == event->type && 2 == event->button)
    {
        BaulWindowPane *wpane;
        BaulWindowSlot *slot;

        wpane = BAUL_WINDOW_PANE (pane);
        slot = wpane->active_slot;
        baul_window_slot_close (slot);

        return FALSE;
    }

    return FALSE;
}

static gboolean
notebook_popup_menu_cb (CtkWidget *widget,
                        gpointer user_data)
{
    BaulNavigationWindowPane *pane;

    pane = BAUL_NAVIGATION_WINDOW_PANE (user_data);
    notebook_popup_menu_show (pane, NULL);
    return TRUE;
}

static gboolean
notebook_switch_page_cb (CtkNotebook *notebook,
                         CtkWidget *page,
                         unsigned int page_num,
                         BaulNavigationWindowPane *pane)
{
    BaulWindowSlot *slot;
    CtkWidget *widget;

    widget = ctk_notebook_get_nth_page (CTK_NOTEBOOK (pane->notebook), page_num);
    g_assert (widget != NULL);

    /* find slot corresponding to the target page */
    slot = baul_window_pane_get_slot_for_content_box (BAUL_WINDOW_PANE (pane), widget);
    g_assert (slot != NULL);

    baul_window_set_active_slot (slot->pane->window, slot);

    baul_window_slot_update_icon (slot);

    return FALSE;
}

void
baul_navigation_window_pane_remove_page (BaulNavigationWindowPane *pane, int page_num)
{
    CtkNotebook *notebook;
    notebook = CTK_NOTEBOOK (pane->notebook);

    g_signal_handlers_block_by_func (notebook,
                                     G_CALLBACK (notebook_switch_page_cb),
                                     pane);
    ctk_notebook_remove_page (notebook, page_num);
    g_signal_handlers_unblock_by_func (notebook,
                                       G_CALLBACK (notebook_switch_page_cb),
                                       pane);
}

void
baul_navigation_window_pane_add_slot_in_tab (BaulNavigationWindowPane *pane, BaulWindowSlot *slot, BaulWindowOpenSlotFlags flags)
{
    BaulNotebook *notebook;

    notebook = BAUL_NOTEBOOK (pane->notebook);
    g_signal_handlers_block_by_func (notebook,
                                     G_CALLBACK (notebook_switch_page_cb),
                                     pane);
    baul_notebook_add_tab (notebook,
                           slot,
                           (flags & BAUL_WINDOW_OPEN_SLOT_APPEND) != 0 ?
                           -1 :
                           ctk_notebook_get_current_page (CTK_NOTEBOOK (notebook)) + 1,
                           FALSE);
    g_signal_handlers_unblock_by_func (notebook,
                                       G_CALLBACK (notebook_switch_page_cb),
                                       pane);
}

static void
real_sync_location_widgets (BaulWindowPane *pane)
{
    BaulNavigationWindowPane *navigation_pane;
    BaulWindowSlot *slot;

    slot = pane->active_slot;
    navigation_pane = BAUL_NAVIGATION_WINDOW_PANE (pane);

    /* Change the location bar and path bar to match the current location. */
    if (slot->location != NULL)
    {
        char *uri;

        /* this may be NULL if we just created the slot */
        uri = baul_window_slot_get_location_uri (slot);
        baul_location_bar_set_location (BAUL_LOCATION_BAR (navigation_pane->navigation_bar), uri);
        g_free (uri);
        baul_path_bar_set_path (BAUL_PATH_BAR (navigation_pane->path_bar), slot->location);
    }

    /* Update window global UI if this is the active pane */
    if (pane == pane->window->details->active_pane)
    {
        BaulNavigationWindowSlot *navigation_slot;

        baul_window_update_up_button (pane->window);

        /* Check if the back and forward buttons need enabling or disabling. */
        navigation_slot = BAUL_NAVIGATION_WINDOW_SLOT (pane->window->details->active_pane->active_slot);
        baul_navigation_window_allow_back (BAUL_NAVIGATION_WINDOW (pane->window),
                                           navigation_slot->back_list != NULL);
        baul_navigation_window_allow_forward (BAUL_NAVIGATION_WINDOW (pane->window),
                                              navigation_slot->forward_list != NULL);
    }
}

gboolean
baul_navigation_window_pane_hide_temporary_bars (BaulNavigationWindowPane *pane)
{
    BaulWindowSlot *slot;
    gboolean success;

    g_assert (BAUL_IS_NAVIGATION_WINDOW_PANE (pane));

    slot = BAUL_WINDOW_PANE(pane)->active_slot;
    success = FALSE;

    if (pane->temporary_location_bar)
    {
        if (baul_navigation_window_pane_location_bar_showing (pane))
        {
            baul_navigation_window_pane_hide_location_bar (pane, FALSE);
        }
        pane->temporary_location_bar = FALSE;
        success = TRUE;
    }
    if (pane->temporary_navigation_bar)
    {
        BaulDirectory *directory;

        directory = baul_directory_get (slot->location);

        if (BAUL_IS_SEARCH_DIRECTORY (directory))
        {
            baul_navigation_window_pane_set_bar_mode (pane, BAUL_BAR_SEARCH);
        }
        else
        {
            if (!g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_ALWAYS_USE_LOCATION_ENTRY))
            {
                baul_navigation_window_pane_set_bar_mode (pane, BAUL_BAR_PATH);
            }
        }
        pane->temporary_navigation_bar = FALSE;
        success = TRUE;

        baul_directory_unref (directory);
    }
    if (pane->temporary_search_bar)
    {
        BaulNavigationWindow *window;

        if (!g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_ALWAYS_USE_LOCATION_ENTRY))
        {
            baul_navigation_window_pane_set_bar_mode (pane, BAUL_BAR_PATH);
        }
        else
        {
            baul_navigation_window_pane_set_bar_mode (pane, BAUL_BAR_NAVIGATION);
        }
        window = BAUL_NAVIGATION_WINDOW (BAUL_WINDOW_PANE (pane)->window);
        baul_navigation_window_set_search_button (window, FALSE);
        pane->temporary_search_bar = FALSE;
        success = TRUE;
    }

    return success;
}

void
baul_navigation_window_pane_always_use_location_entry (BaulNavigationWindowPane *pane, gboolean use_entry)
{
    if (use_entry)
    {
        baul_navigation_window_pane_set_bar_mode (pane, BAUL_BAR_NAVIGATION);
    }
    else
    {
        baul_navigation_window_pane_set_bar_mode (pane, BAUL_BAR_PATH);
    }

    g_signal_handlers_block_by_func (pane->location_button,
                                     G_CALLBACK (location_button_toggled_cb),
                                     pane);
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (pane->location_button), use_entry);
    g_signal_handlers_unblock_by_func (pane->location_button,
                                       G_CALLBACK (location_button_toggled_cb),
                                       pane);
}

void
baul_navigation_window_pane_setup (BaulNavigationWindowPane *pane)
{
    CtkWidget *hbox;
    BaulEntry *entry;
    CtkSizeGroup *header_size_group;

    pane->widget = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

    hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
    pane->location_bar = hbox;
    ctk_container_set_border_width (CTK_CONTAINER (hbox), 4);
    ctk_box_pack_start (CTK_BOX (pane->widget), hbox,
                        FALSE, FALSE, 0);
    ctk_widget_show (hbox);

    /* the header size group ensures that the location bar has the same height as the sidebar header */
    header_size_group = BAUL_NAVIGATION_WINDOW (BAUL_WINDOW_PANE (pane)->window)->details->header_size_group;
    ctk_size_group_add_widget (header_size_group, pane->location_bar);

    pane->location_button = location_button_create (pane);
    ctk_box_pack_start (CTK_BOX (hbox), pane->location_button, FALSE, FALSE, 0);
    ctk_widget_show (pane->location_button);

    pane->path_bar = g_object_new (BAUL_TYPE_PATH_BAR, NULL);
    ctk_widget_show (pane->path_bar);

    g_signal_connect_object (pane->path_bar, "path_clicked",
                             G_CALLBACK (path_bar_location_changed_callback), pane, 0);

    g_signal_connect_object (pane->path_bar, "path-event",
                             G_CALLBACK (path_bar_path_event_callback), pane, 0);

    ctk_box_pack_start (CTK_BOX (hbox),
                        pane->path_bar,
                        TRUE, TRUE, 0);

    pane->navigation_bar = baul_location_bar_new (pane);
    g_signal_connect_object (pane->navigation_bar, "location_changed",
                             G_CALLBACK (navigation_bar_location_changed_callback), pane, 0);
    g_signal_connect_object (pane->navigation_bar, "cancel",
                             G_CALLBACK (navigation_bar_cancel_callback), pane, 0);
    entry = baul_location_bar_get_entry (BAUL_LOCATION_BAR (pane->navigation_bar));
    g_signal_connect (entry, "focus-in-event",
                      G_CALLBACK (navigation_bar_focus_in_callback), pane);

    ctk_box_pack_start (CTK_BOX (hbox),
                        pane->navigation_bar,
                        TRUE, TRUE, 0);

    pane->search_bar = baul_search_bar_new (BAUL_WINDOW_PANE (pane)->window);
    g_signal_connect_object (pane->search_bar, "activate",
                             G_CALLBACK (search_bar_activate_callback), pane, 0);
    g_signal_connect_object (pane->search_bar, "cancel",
                             G_CALLBACK (search_bar_cancel_callback), pane, 0);
    g_signal_connect_object (pane->search_bar, "focus-in",
                             G_CALLBACK (search_bar_focus_in_callback), pane, 0);
    ctk_box_pack_start (CTK_BOX (hbox),
                        pane->search_bar,
                        TRUE, TRUE, 0);

    pane->notebook = g_object_new (BAUL_TYPE_NOTEBOOK, NULL);
    ctk_box_pack_start (CTK_BOX (pane->widget), pane->notebook,
                        TRUE, TRUE, 0);
    g_signal_connect (pane->notebook,
                      "tab-close-request",
                      G_CALLBACK (notebook_tab_close_requested),
                      pane);
    g_signal_connect_after (pane->notebook,
                            "button_press_event",
                            G_CALLBACK (notebook_button_press_cb),
                            pane);
    g_signal_connect (pane->notebook, "popup-menu",
                      G_CALLBACK (notebook_popup_menu_cb),
                      pane);
    g_signal_connect (pane->notebook,
                      "switch-page",
                      G_CALLBACK (notebook_switch_page_cb),
                      pane);

    ctk_notebook_set_show_tabs (CTK_NOTEBOOK (pane->notebook), FALSE);
    ctk_notebook_set_show_border (CTK_NOTEBOOK (pane->notebook), FALSE);
    ctk_widget_show (pane->notebook);
    ctk_container_set_border_width (CTK_CONTAINER (pane->notebook), 0);

    /* Ensure that the view has some minimal size and that other parts
     * of the UI (like location bar and tabs) don't request more and
     * thus affect the default position of the split view paned.
     */
    ctk_widget_set_size_request (pane->widget, 60, 60);
}


void
baul_navigation_window_pane_show_location_bar_temporarily (BaulNavigationWindowPane *pane)
{
    if (!baul_navigation_window_pane_location_bar_showing (pane))
    {
        baul_navigation_window_pane_show_location_bar (pane, FALSE);
        pane->temporary_location_bar = TRUE;
    }
}

void
baul_navigation_window_pane_show_navigation_bar_temporarily (BaulNavigationWindowPane *pane)
{
    if (baul_navigation_window_pane_path_bar_showing (pane)
            || baul_navigation_window_pane_search_bar_showing (pane))
    {
        baul_navigation_window_pane_set_bar_mode (pane, BAUL_BAR_NAVIGATION);
        pane->temporary_navigation_bar = TRUE;
    }
    baul_location_bar_activate
    (BAUL_LOCATION_BAR (pane->navigation_bar));
}

gboolean
baul_navigation_window_pane_path_bar_showing (BaulNavigationWindowPane *pane)
{
    if (pane->path_bar != NULL)
    {
        return ctk_widget_get_visible (pane->path_bar);
    }
    /* If we're not visible yet we haven't changed visibility, so its TRUE */
    return TRUE;
}

void
baul_navigation_window_pane_set_bar_mode (BaulNavigationWindowPane *pane,
        BaulBarMode mode)
{
    gboolean use_entry;
    CtkWidget *focus_widget;
    BaulNavigationWindow *window;

    switch (mode)
    {

    case BAUL_BAR_PATH:
        ctk_widget_show (pane->path_bar);
        ctk_widget_hide (pane->navigation_bar);
        ctk_widget_hide (pane->search_bar);
        break;

    case BAUL_BAR_NAVIGATION:
        ctk_widget_show (pane->navigation_bar);
        ctk_widget_hide (pane->path_bar);
        ctk_widget_hide (pane->search_bar);
        break;

    case BAUL_BAR_SEARCH:
        ctk_widget_show (pane->search_bar);
        ctk_widget_hide (pane->path_bar);
        ctk_widget_hide (pane->navigation_bar);
        break;
    }

    if (mode == BAUL_BAR_NAVIGATION || mode == BAUL_BAR_PATH) {
        use_entry = (mode == BAUL_BAR_NAVIGATION);

        g_signal_handlers_block_by_func (pane->location_button,
                         G_CALLBACK (location_button_toggled_cb),
                         pane);
        ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (pane->location_button),
                          use_entry);
        g_signal_handlers_unblock_by_func (pane->location_button,
                           G_CALLBACK (location_button_toggled_cb),
                           pane);
    }

    window = BAUL_NAVIGATION_WINDOW (BAUL_WINDOW_PANE (pane)->window);
    focus_widget = ctk_window_get_focus (CTK_WINDOW (window));
    if (focus_widget != NULL && !baul_navigation_window_is_in_temporary_navigation_bar (focus_widget, window) &&
            !baul_navigation_window_is_in_temporary_search_bar (focus_widget, window))
    {
        if (mode == BAUL_BAR_NAVIGATION || mode == BAUL_BAR_PATH)
        {
            baul_navigation_window_set_search_button (window, FALSE);
        }
        else
        {
            baul_navigation_window_set_search_button (window, TRUE);
        }
    }
}

gboolean
baul_navigation_window_pane_search_bar_showing (BaulNavigationWindowPane *pane)
{
    if (pane->search_bar != NULL)
    {
        return ctk_widget_get_visible (pane->search_bar);
    }
    /* If we're not visible yet we haven't changed visibility, so its TRUE */
    return TRUE;
}

void
baul_navigation_window_pane_hide_location_bar (BaulNavigationWindowPane *pane, gboolean save_preference)
{
    pane->temporary_location_bar = FALSE;
    ctk_widget_hide(pane->location_bar);
    baul_navigation_window_update_show_hide_menu_items(
        BAUL_NAVIGATION_WINDOW (BAUL_WINDOW_PANE (pane)->window));
    if (save_preference)
    {
        g_settings_set_boolean (baul_window_state, BAUL_WINDOW_STATE_START_WITH_LOCATION_BAR, FALSE);
    }
}

void
baul_navigation_window_pane_show_location_bar (BaulNavigationWindowPane *pane, gboolean save_preference)
{
    ctk_widget_show(pane->location_bar);
    baul_navigation_window_update_show_hide_menu_items(BAUL_NAVIGATION_WINDOW (BAUL_WINDOW_PANE (pane)->window));
    if (save_preference)
    {
        g_settings_set_boolean (baul_window_state, BAUL_WINDOW_STATE_START_WITH_LOCATION_BAR, TRUE);
    }
}

gboolean
baul_navigation_window_pane_location_bar_showing (BaulNavigationWindowPane *pane)
{
    if (!BAUL_IS_NAVIGATION_WINDOW_PANE (pane))
    {
        return FALSE;
    }
    if (pane->location_bar != NULL)
    {
        return ctk_widget_get_visible (pane->location_bar);
    }
    /* If we're not visible yet we haven't changed visibility, so its TRUE */
    return TRUE;
}

static void
baul_navigation_window_pane_init (BaulNavigationWindowPane *pane)
{
}

static void
baul_navigation_window_pane_show (BaulWindowPane *pane)
{
    BaulNavigationWindowPane *npane = BAUL_NAVIGATION_WINDOW_PANE (pane);

    ctk_widget_show (npane->widget);
}

/* either called due to slot change, or due to location change in the current slot. */
static void
real_sync_search_widgets (BaulWindowPane *window_pane)
{
    BaulWindowSlot *slot;
    BaulDirectory *directory;
    BaulSearchDirectory *search_directory;
    BaulNavigationWindowPane *pane;

    pane = BAUL_NAVIGATION_WINDOW_PANE (window_pane);
    slot = window_pane->active_slot;
    search_directory = NULL;

    directory = baul_directory_get (slot->location);
    if (BAUL_IS_SEARCH_DIRECTORY (directory))
    {
        search_directory = BAUL_SEARCH_DIRECTORY (directory);
    }

    if (search_directory != NULL &&
            !baul_search_directory_is_saved_search (search_directory))
    {
        baul_navigation_window_pane_show_location_bar_temporarily (pane);
        baul_navigation_window_pane_set_bar_mode (pane, BAUL_BAR_SEARCH);
        pane->temporary_search_bar = FALSE;
    }
    else
    {
        pane->temporary_search_bar = TRUE;
        baul_navigation_window_pane_hide_temporary_bars (pane);
    }
    baul_directory_unref (directory);
}

static void
baul_navigation_window_pane_class_init (BaulNavigationWindowPaneClass *class)
{
    G_OBJECT_CLASS (class)->dispose = baul_navigation_window_pane_dispose;
    BAUL_WINDOW_PANE_CLASS (class)->show = baul_navigation_window_pane_show;
    BAUL_WINDOW_PANE_CLASS (class)->set_active = real_set_active;
    BAUL_WINDOW_PANE_CLASS (class)->sync_search_widgets = real_sync_search_widgets;
    BAUL_WINDOW_PANE_CLASS (class)->sync_location_widgets = real_sync_location_widgets;
}

static void
baul_navigation_window_pane_dispose (GObject *object)
{
    BaulNavigationWindowPane *pane = BAUL_NAVIGATION_WINDOW_PANE (object);

    ctk_widget_destroy (pane->widget);

    G_OBJECT_CLASS (parent_class)->dispose (object);
}

BaulNavigationWindowPane *
baul_navigation_window_pane_new (BaulWindow *window)
{
    BaulNavigationWindowPane *pane;

    pane = g_object_new (BAUL_TYPE_NAVIGATION_WINDOW_PANE, NULL);
    BAUL_WINDOW_PANE(pane)->window = window;

    return pane;
}
