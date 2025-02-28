/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Baul
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 1999, 2000, 2001 Eazel, Inc.
 *  Copyright (C) 2003 Ximian, Inc.
 *
 *  Baul is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  Baul is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Authors: Elliot Lee <sopwith@redhat.com>
 *  	     John Sullivan <sullivan@eazel.com>
 *
 */

/* baul-window.c: Implementation of the main window object */

#include <config.h>
#include <math.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cdk/cdkx.h>
#include <ctk/ctk.h>
#include <glib/gi18n.h>
#ifdef HAVE_X11_XF86KEYSYM_H
#include <X11/XF86keysym.h>
#endif
#include <sys/time.h>

#include <eel/eel-ctk-extensions.h>
#include <eel/eel-ctk-macros.h>
#include <eel/eel-string.h>

#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-file-attributes.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-icon-info.h>
#include <libbaul-private/baul-metadata.h>
#include <libbaul-private/baul-mime-actions.h>
#include <libbaul-private/baul-program-choosing.h>
#include <libbaul-private/baul-sidebar.h>
#include <libbaul-private/baul-view-factory.h>
#include <libbaul-private/baul-clipboard.h>
#include <libbaul-private/baul-module.h>
#include <libbaul-private/baul-sidebar-provider.h>
#include <libbaul-private/baul-search-directory.h>
#include <libbaul-private/baul-signaller.h>

#include "baul-navigation-window.h"
#include "baul-window-private.h"
#include "baul-actions.h"
#include "baul-application.h"
#include "baul-bookmarks-window.h"
#include "baul-location-bar.h"
#include "baul-query-editor.h"
#include "baul-search-bar.h"
#include "baul-navigation-window-slot.h"
#include "baul-notebook.h"
#include "baul-window-manage-views.h"
#include "baul-navigation-window-pane.h"

#define MAX_TITLE_LENGTH 180

#define MENU_PATH_BOOKMARKS_PLACEHOLDER			"/MenuBar/Other Menus/Bookmarks/Bookmarks Placeholder"

/* Forward and back buttons on the mouse */
static gboolean mouse_extra_buttons = TRUE;
static int mouse_forward_button = 9;
static int mouse_back_button = 8;

static void add_sidebar_panels                       (BaulNavigationWindow *window);
static void side_panel_image_changed_callback        (BaulSidebar          *side_panel,
        gpointer                  callback_data);
static void always_use_location_entry_changed        (gpointer                  callback_data);
static void always_use_browser_changed               (gpointer                  callback_data);
static void mouse_back_button_changed		     (gpointer                  callback_data);
static void mouse_forward_button_changed	     (gpointer                  callback_data);
static void use_extra_mouse_buttons_changed          (gpointer                  callback_data);
static BaulWindowSlot *create_extra_pane         (BaulNavigationWindow *window);


G_DEFINE_TYPE_WITH_PRIVATE (BaulNavigationWindow, baul_navigation_window, BAUL_TYPE_WINDOW)
#define parent_class baul_navigation_window_parent_class

static const struct
{
    unsigned int keyval;
    const char *action;
} extra_navigation_window_keybindings [] =
{
#ifdef HAVE_X11_XF86KEYSYM_H
    { XF86XK_Back,		BAUL_ACTION_BACK },
    { XF86XK_Forward,	BAUL_ACTION_FORWARD }
#endif
};

static void
baul_navigation_window_init (BaulNavigationWindow *window)
{
    CtkUIManager *ui_manager;
    CtkWidget *toolbar;
    BaulWindow *win;
    BaulNavigationWindowPane *pane;
    CtkWidget *hpaned;
    CtkWidget *vbox;

    win = BAUL_WINDOW (window);

    window->details = baul_navigation_window_get_instance_private (window);

    CtkStyleContext *context;

    context = ctk_widget_get_style_context (CTK_WIDGET (window));
    ctk_style_context_add_class (context, "baul-navigation-window");

    pane = baul_navigation_window_pane_new (win);
    win->details->panes = g_list_prepend (win->details->panes, pane);

    window->details->header_size_group = ctk_size_group_new (CTK_SIZE_GROUP_VERTICAL);

    window->details->content_paned = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
    ctk_widget_set_hexpand (window->details->content_paned, TRUE);
    ctk_widget_set_vexpand (window->details->content_paned, TRUE);
    ctk_grid_attach (CTK_GRID (BAUL_WINDOW (window)->details->grid),
                     window->details->content_paned,
                     0, 3, 1, 1);
    ctk_widget_show (window->details->content_paned);

    vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
    ctk_paned_pack2 (CTK_PANED (window->details->content_paned), vbox,
    		     TRUE, FALSE);
    ctk_widget_show (vbox);

    hpaned = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
    ctk_box_pack_start (CTK_BOX (vbox), hpaned, TRUE, TRUE, 0);
    ctk_widget_show (hpaned);
    window->details->split_view_hpane = hpaned;

    ctk_box_pack_start (CTK_BOX (vbox), win->details->statusbar, FALSE, FALSE, 0);
    ctk_widget_show (win->details->statusbar);

    baul_navigation_window_pane_setup (pane);

    ctk_paned_pack1 (CTK_PANED(hpaned), pane->widget, TRUE, FALSE);
    ctk_widget_show (pane->widget);

    /* this has to be done after the location bar has been set up,
     * but before menu stuff is being called */
    baul_window_set_active_pane (win, BAUL_WINDOW_PANE (pane));

    baul_navigation_window_initialize_actions (window);

    baul_navigation_window_initialize_menus (window);

    ui_manager = baul_window_get_ui_manager (BAUL_WINDOW (window));
    toolbar = ctk_ui_manager_get_widget (ui_manager, "/Toolbar");
    ctk_style_context_add_class (ctk_widget_get_style_context (toolbar), CTK_STYLE_CLASS_PRIMARY_TOOLBAR);
    window->details->toolbar = toolbar;
    ctk_widget_set_hexpand (toolbar, TRUE);
    ctk_grid_attach (CTK_GRID (BAUL_WINDOW (window)->details->grid),
                     toolbar,
                     0, 1, 1, 1);
    ctk_widget_show (toolbar);

    baul_navigation_window_initialize_toolbars (window);

    /* Set initial sensitivity of some buttons & menu items
     * now that they're all created.
     */
    baul_navigation_window_allow_back (window, FALSE);
    baul_navigation_window_allow_forward (window, FALSE);

    g_signal_connect_swapped (baul_preferences,
                              "changed::" BAUL_PREFERENCES_ALWAYS_USE_LOCATION_ENTRY,
                              G_CALLBACK(always_use_location_entry_changed),
                              window);

    g_signal_connect_swapped (baul_preferences,
                              "changed::" BAUL_PREFERENCES_ALWAYS_USE_BROWSER,
                              G_CALLBACK(always_use_browser_changed),
                              window);
}

static void
always_use_location_entry_changed (gpointer callback_data)
{
    BaulNavigationWindow *window;
    GList *walk;
    gboolean use_entry;

    window = BAUL_NAVIGATION_WINDOW (callback_data);

    use_entry = g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_ALWAYS_USE_LOCATION_ENTRY);

    for (walk = BAUL_WINDOW(window)->details->panes; walk; walk = walk->next)
    {
        baul_navigation_window_pane_always_use_location_entry (walk->data, use_entry);
    }
}

static void
always_use_browser_changed (gpointer callback_data)
{
    BaulNavigationWindow *window;

    window = BAUL_NAVIGATION_WINDOW (callback_data);

    baul_navigation_window_update_spatial_menu_item (window);
}

/* Sanity check: highest mouse button value I could find was 14. 5 is our
 * lower threshold (well-documented to be the one of the button events for the
 * scrollwheel), so it's hardcoded in the functions below. However, if you have
 * a button that registers higher and want to map it, file a bug and
 * we'll move the bar. Makes you wonder why the X guys don't have
 * defined values for these like the XKB stuff, huh?
 */
#define UPPER_MOUSE_LIMIT 14

static void
mouse_back_button_changed (gpointer callback_data G_GNUC_UNUSED)
{
    int new_back_button;

    new_back_button = g_settings_get_int (baul_preferences, BAUL_PREFERENCES_MOUSE_BACK_BUTTON);

    /* Bounds checking */
    if (new_back_button < 6 || new_back_button > UPPER_MOUSE_LIMIT)
        return;

    mouse_back_button = new_back_button;
}

static void
mouse_forward_button_changed (gpointer callback_data G_GNUC_UNUSED)
{
    int new_forward_button;

    new_forward_button = g_settings_get_int (baul_preferences, BAUL_PREFERENCES_MOUSE_FORWARD_BUTTON);

    /* Bounds checking */
    if (new_forward_button < 6 || new_forward_button > UPPER_MOUSE_LIMIT)
        return;

    mouse_forward_button = new_forward_button;
}

static void
use_extra_mouse_buttons_changed (gpointer callback_data G_GNUC_UNUSED)
{
    mouse_extra_buttons = g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_MOUSE_USE_EXTRA_BUTTONS);
}

void
baul_navigation_window_unset_focus_widget (BaulNavigationWindow *window)
{
    if (window->details->last_focus_widget != NULL)
    {
        g_object_remove_weak_pointer (G_OBJECT (window->details->last_focus_widget),
                                      (gpointer *) &window->details->last_focus_widget);
        window->details->last_focus_widget = NULL;
    }
}

gboolean
baul_navigation_window_is_in_temporary_navigation_bar (CtkWidget *widget,
        BaulNavigationWindow *window)
{
    GList *walk;
    gboolean is_in_any = FALSE;

    for (walk = BAUL_WINDOW(window)->details->panes; walk; walk = walk->next)
    {
        BaulNavigationWindowPane *pane = walk->data;
        if(ctk_widget_get_ancestor (widget, BAUL_TYPE_LOCATION_BAR) != NULL &&
                pane->temporary_navigation_bar)
            is_in_any = TRUE;
    }
    return is_in_any;
}

gboolean
baul_navigation_window_is_in_temporary_search_bar (CtkWidget *widget,
        BaulNavigationWindow *window)
{
    GList *walk;
    gboolean is_in_any = FALSE;

    for (walk = BAUL_WINDOW(window)->details->panes; walk; walk = walk->next)
    {
        BaulNavigationWindowPane *pane = walk->data;
        if(ctk_widget_get_ancestor (widget, BAUL_TYPE_SEARCH_BAR) != NULL &&
                pane->temporary_search_bar)
            is_in_any = TRUE;
    }
    return is_in_any;
}

static void
remember_focus_widget (BaulNavigationWindow *window)
{
    BaulNavigationWindow *navigation_window;
    CtkWidget *focus_widget;

    navigation_window = BAUL_NAVIGATION_WINDOW (window);

    focus_widget = ctk_window_get_focus (CTK_WINDOW (window));
    if (focus_widget != NULL &&
            !baul_navigation_window_is_in_temporary_navigation_bar (focus_widget, navigation_window) &&
            !baul_navigation_window_is_in_temporary_search_bar (focus_widget, navigation_window))
    {
        baul_navigation_window_unset_focus_widget (navigation_window);

        navigation_window->details->last_focus_widget = focus_widget;
        g_object_add_weak_pointer (G_OBJECT (focus_widget),
                                   (gpointer *) &(BAUL_NAVIGATION_WINDOW (window)->details->last_focus_widget));
    }
}

void
baul_navigation_window_restore_focus_widget (BaulNavigationWindow *window)
{
    if (window->details->last_focus_widget != NULL)
    {
        if (BAUL_IS_VIEW (window->details->last_focus_widget))
        {
            baul_view_grab_focus (BAUL_VIEW (window->details->last_focus_widget));
        }
        else
        {
            ctk_widget_grab_focus (window->details->last_focus_widget);
        }

        baul_navigation_window_unset_focus_widget (window);
    }
}

static void
side_pane_close_requested_callback (CtkWidget *widget G_GNUC_UNUSED,
				    gpointer   user_data)
{
    BaulNavigationWindow *window;

    window = BAUL_NAVIGATION_WINDOW (user_data);

    baul_navigation_window_hide_sidebar (window);
}

static void
side_pane_size_allocate_callback (CtkWidget *widget,
                                  CtkAllocation *allocation,
                                  gpointer user_data)
{
    BaulNavigationWindow *window;
    gint scale;

    window = BAUL_NAVIGATION_WINDOW (user_data);
    scale = ctk_widget_get_scale_factor (widget);

    allocation->width = allocation->width / scale;
    if (allocation->width != window->details->side_pane_width)
    {
        window->details->side_pane_width = allocation->width;
        g_settings_set_int (baul_window_state,
                            BAUL_WINDOW_STATE_SIDEBAR_WIDTH,
                            allocation->width <= 1 ? 0 : allocation->width);
    }
}

static void
setup_side_pane_width (BaulNavigationWindow *window)
{
    gint scale;

    g_return_if_fail (window->sidebar != NULL);

    scale = ctk_widget_get_scale_factor (CTK_WIDGET (window->sidebar));
    window->details->side_pane_width =
        g_settings_get_int (baul_window_state,
                            BAUL_WINDOW_STATE_SIDEBAR_WIDTH) * scale;

    ctk_paned_set_position (CTK_PANED (window->details->content_paned),
                            window->details->side_pane_width);
}

static void
set_current_side_panel (BaulNavigationWindow *window,
                        BaulSidebar *panel)
{
    if (window->details->current_side_panel)
    {
        baul_sidebar_is_visible_changed (window->details->current_side_panel,
                                         FALSE);
        eel_remove_weak_pointer (&window->details->current_side_panel);
    }

    if (panel != NULL)
    {
        baul_sidebar_is_visible_changed (panel, TRUE);
    }
    window->details->current_side_panel = panel;
    eel_add_weak_pointer (&window->details->current_side_panel);
}

static void
side_pane_switch_page_callback (BaulSidePane         *side_pane G_GNUC_UNUSED,
				CtkWidget            *widget,
				BaulNavigationWindow *window)
{
    const char *id;
    BaulSidebar *sidebar;

    sidebar = BAUL_SIDEBAR (widget);

    if (sidebar == NULL)
    {
        return;
    }

    set_current_side_panel (window, sidebar);

    id = baul_sidebar_get_sidebar_id (sidebar);
    g_settings_set_string (baul_window_state, BAUL_WINDOW_STATE_SIDE_PANE_VIEW, id);
}

static void
baul_navigation_window_set_up_sidebar (BaulNavigationWindow *window)
{
    CtkWidget *title;

    window->sidebar = baul_side_pane_new ();

    title = baul_side_pane_get_title (window->sidebar);
    ctk_size_group_add_widget (window->details->header_size_group,
                               title);

    ctk_paned_pack1 (CTK_PANED (window->details->content_paned),
                     CTK_WIDGET (window->sidebar),
                     FALSE, FALSE);

    setup_side_pane_width (window);
    g_signal_connect (window->sidebar,
                      "size_allocate",
                      G_CALLBACK (side_pane_size_allocate_callback),
                      window);

    add_sidebar_panels (window);

    g_signal_connect (window->sidebar,
                      "close_requested",
                      G_CALLBACK (side_pane_close_requested_callback),
                      window);

    g_signal_connect (window->sidebar,
                      "switch_page",
                      G_CALLBACK (side_pane_switch_page_callback),
                      window);

    ctk_widget_show (CTK_WIDGET (window->sidebar));
}

static void
baul_navigation_window_tear_down_sidebar (BaulNavigationWindow *window)
{
    GList *node, *next;
    BaulSidebar *sidebar_panel = NULL;

    g_signal_handlers_disconnect_by_func (window->sidebar,
                                          side_pane_switch_page_callback,
                                          window);

    for (node = window->sidebar_panels; node != NULL; node = next)
    {
        next = node->next;

        sidebar_panel = BAUL_SIDEBAR (node->data);

        baul_navigation_window_remove_sidebar_panel (window,
                sidebar_panel);
    }

    ctk_widget_destroy (CTK_WIDGET (window->sidebar));
    window->sidebar = NULL;
}

static gboolean
baul_navigation_window_state_event (CtkWidget *widget,
                                    CdkEventWindowState *event)
{
    if (event->changed_mask & CDK_WINDOW_STATE_MAXIMIZED)
    {
        g_settings_set_boolean (baul_window_state, BAUL_WINDOW_STATE_MAXIMIZED,
                                event->new_window_state & CDK_WINDOW_STATE_MAXIMIZED);
    }

    if (CTK_WIDGET_CLASS (parent_class)->window_state_event != NULL)
    {
        return CTK_WIDGET_CLASS (parent_class)->window_state_event (widget, event);
    }

    return FALSE;
}

static gboolean
baul_navigation_window_key_press_event (CtkWidget *widget,
                                        CdkEventKey *event)
{
    BaulNavigationWindow *window;
    int i;

    window = BAUL_NAVIGATION_WINDOW (widget);

    if (event->state & CDK_CONTROL_MASK)
    {
        GSettings *settings = g_settings_new ("org.cafe.baul.preferences");
        gboolean handled = FALSE;

        if (g_settings_get_boolean (settings, "ctrl-tab-switch-tabs"))
        {
            BaulWindow *baulwin;
            BaulWindowSlot *slot;
            BaulNavigationWindowPane *pane;
            BaulNotebook *baulnotebook;
            CtkNotebook *notebook;
            int pages;
            int page_num;

            baulwin = BAUL_WINDOW (window);
            slot = baul_window_get_active_slot (baulwin);
            pane = BAUL_NAVIGATION_WINDOW_PANE (slot->pane);
            baulnotebook = BAUL_NOTEBOOK (pane->notebook);
            notebook = CTK_NOTEBOOK (baulnotebook);
            pages = ctk_notebook_get_n_pages (notebook);
            page_num = ctk_notebook_get_current_page (notebook);

            if (event->keyval == CDK_KEY_ISO_Left_Tab)
            {
                if (page_num != 0)
                    ctk_notebook_prev_page (notebook);
                else
                    ctk_notebook_set_current_page (notebook, (pages - 1));
                handled = TRUE;
            }
            if (event->keyval == CDK_KEY_Tab)
            {
                if (page_num != (pages -1))
                    ctk_notebook_next_page (notebook);
                else
                    ctk_notebook_set_current_page (notebook, 0);
                handled = TRUE;
            }
        }
        g_object_unref (settings);

        if (handled)
            return TRUE;
    }

    for (i = 0; i < G_N_ELEMENTS (extra_navigation_window_keybindings); i++)
    {
        if (extra_navigation_window_keybindings[i].keyval == event->keyval)
        {
            CtkAction *action;

            action = ctk_action_group_get_action (window->details->navigation_action_group,
                                                  extra_navigation_window_keybindings[i].action);

            g_assert (action != NULL);
            if (ctk_action_is_sensitive (action))
            {
                ctk_action_activate (action);
                return TRUE;
            }

            break;
        }
    }

    return CTK_WIDGET_CLASS (baul_navigation_window_parent_class)->key_press_event (widget, event);
}

static gboolean
baul_navigation_window_button_press_event (CtkWidget *widget,
        CdkEventButton *event)
{
    BaulNavigationWindow *window;
    gboolean handled;

    handled = FALSE;
    window = BAUL_NAVIGATION_WINDOW (widget);

    if (mouse_extra_buttons && (event->button == mouse_back_button))
    {
        baul_navigation_window_go_back (window);
        handled = TRUE;
    }
    else if (mouse_extra_buttons && (event->button == mouse_forward_button))
    {
        baul_navigation_window_go_forward (window);
        handled = TRUE;
    }
    else if (CTK_WIDGET_CLASS (baul_navigation_window_parent_class)->button_press_event)
    {
        handled = CTK_WIDGET_CLASS (baul_navigation_window_parent_class)->button_press_event (widget, event);
    }
    else
    {
        handled = FALSE;
    }
    return handled;
}

static void
baul_navigation_window_destroy (CtkWidget *object)
{
    BaulNavigationWindow *window;

    window = BAUL_NAVIGATION_WINDOW (object);

    baul_navigation_window_unset_focus_widget (window);

    window->sidebar = NULL;
    g_list_foreach (window->sidebar_panels, (GFunc)g_object_unref, NULL);
    g_list_free (window->sidebar_panels);
    window->sidebar_panels = NULL;

    window->details->content_paned = NULL;
    window->details->split_view_hpane = NULL;

    CTK_WIDGET_CLASS (parent_class)->destroy (object);
}

static void
baul_navigation_window_finalize (GObject *object)
{
    BaulNavigationWindow *window;

    window = BAUL_NAVIGATION_WINDOW (object);

    baul_navigation_window_remove_go_menu_callback (window);

    g_signal_handlers_disconnect_by_func (baul_preferences,
                                          always_use_browser_changed,
                                          window);
    g_signal_handlers_disconnect_by_func (baul_preferences,
                                          always_use_location_entry_changed,
                                          window);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

/*
 * Main API
 */

void
baul_navigation_window_add_sidebar_panel (BaulNavigationWindow *window,
        BaulSidebar *sidebar_panel)
{
    const char *sidebar_id;
    char *label;
    char *tooltip;
    char *default_id;
    GdkPixbuf *icon;

    g_return_if_fail (BAUL_IS_NAVIGATION_WINDOW (window));
    g_return_if_fail (BAUL_IS_SIDEBAR (sidebar_panel));
    g_return_if_fail (BAUL_IS_SIDE_PANE (window->sidebar));
    g_return_if_fail (g_list_find (window->sidebar_panels, sidebar_panel) == NULL);

    label = baul_sidebar_get_tab_label (sidebar_panel);
    tooltip = baul_sidebar_get_tab_tooltip (sidebar_panel);
    baul_side_pane_add_panel (window->sidebar,
                              CTK_WIDGET (sidebar_panel),
                              label,
                              tooltip);
    g_free (tooltip);
    g_free (label);

    icon = baul_sidebar_get_tab_icon (sidebar_panel);
    baul_side_pane_set_panel_image (BAUL_NAVIGATION_WINDOW (window)->sidebar,
                                    CTK_WIDGET (sidebar_panel),
                                    icon);
    if (icon)
    {
        g_object_unref (icon);
    }

    g_signal_connect (sidebar_panel, "tab_icon_changed",
                      (GCallback)side_panel_image_changed_callback, window);

    window->sidebar_panels = g_list_prepend (window->sidebar_panels,
                             g_object_ref (sidebar_panel));

    /* Show if default */
    sidebar_id = baul_sidebar_get_sidebar_id (sidebar_panel);
    default_id = g_settings_get_string (baul_window_state, BAUL_WINDOW_STATE_SIDE_PANE_VIEW);
    if (sidebar_id && default_id && !strcmp (sidebar_id, default_id))
    {
        baul_side_pane_show_panel (window->sidebar,
                                   CTK_WIDGET (sidebar_panel));
    }
    g_free (default_id);
}

void
baul_navigation_window_remove_sidebar_panel (BaulNavigationWindow *window,
        BaulSidebar *sidebar_panel)
{
    g_return_if_fail (BAUL_IS_NAVIGATION_WINDOW (window));
    g_return_if_fail (BAUL_IS_SIDEBAR (sidebar_panel));

    if (g_list_find (window->sidebar_panels, sidebar_panel) == NULL)
    {
        return;
    }

    g_signal_handlers_disconnect_by_func (sidebar_panel, side_panel_image_changed_callback, window);

    baul_side_pane_remove_panel (window->sidebar,
                                 CTK_WIDGET (sidebar_panel));
    window->sidebar_panels = g_list_remove (window->sidebar_panels, sidebar_panel);
    g_object_unref (sidebar_panel);
}

void
baul_navigation_window_go_back (BaulNavigationWindow *window)
{
    baul_navigation_window_back_or_forward (window, TRUE, 0, FALSE);
}

void
baul_navigation_window_go_forward (BaulNavigationWindow *window)
{
    baul_navigation_window_back_or_forward (window, FALSE, 0, FALSE);
}

void
baul_navigation_window_allow_back (BaulNavigationWindow *window, gboolean allow)
{
    CtkAction *action;

    action = ctk_action_group_get_action (window->details->navigation_action_group,
                                          BAUL_ACTION_BACK);

    ctk_action_set_sensitive (action, allow);
}

void
baul_navigation_window_allow_forward (BaulNavigationWindow *window, gboolean allow)
{
    CtkAction *action;

    action = ctk_action_group_get_action (window->details->navigation_action_group,
                                          BAUL_ACTION_FORWARD);

    ctk_action_set_sensitive (action, allow);
}

static void
real_sync_title (BaulWindow *window,
                 BaulWindowSlot *slot)
{
    BaulNavigationWindowPane *pane;
    BaulNotebook *notebook;

    EEL_CALL_PARENT (BAUL_WINDOW_CLASS,
                     sync_title, (window, slot));

    if (slot == window->details->active_pane->active_slot)
    {
        char *window_title;

        /* if spatial mode is default, we keep "File Browser" in the window title
         * to recognize browser windows. Otherwise, we default to the directory name.
         */
        if (!g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_ALWAYS_USE_BROWSER))
        {
            char *full_title;

            full_title = g_strdup_printf (_("%s - File Browser"), slot->title);
            window_title = eel_str_middle_truncate (full_title, MAX_TITLE_LENGTH);
            g_free (full_title);
        }
        else
        {
            window_title = eel_str_middle_truncate (slot->title, MAX_TITLE_LENGTH);
        }

        ctk_window_set_title (CTK_WINDOW (window), window_title);
        g_free (window_title);
    }

    pane = BAUL_NAVIGATION_WINDOW_PANE (slot->pane);
    notebook = BAUL_NOTEBOOK (pane->notebook);
    baul_notebook_sync_tab_label (notebook, slot);
}

static BaulIconInfo *
real_get_icon (BaulWindow *window,
               BaulWindowSlot *slot)
{
    return baul_file_get_icon (slot->viewed_file,
                               48, ctk_widget_get_scale_factor (CTK_WIDGET (window)),
                               BAUL_FILE_ICON_FLAGS_IGNORE_VISITING |
                               BAUL_FILE_ICON_FLAGS_USE_MOUNT_ICON);
}

static void
real_sync_allow_stop (BaulWindow *window,
                      BaulWindowSlot *slot)
{
    BaulNavigationWindow *navigation_window;
    BaulNotebook *notebook;

    navigation_window = BAUL_NAVIGATION_WINDOW (window);
    baul_navigation_window_set_spinner_active (navigation_window, slot->allow_stop);

    notebook = BAUL_NOTEBOOK (BAUL_NAVIGATION_WINDOW_PANE (slot->pane)->notebook);
    baul_notebook_sync_loading (notebook, slot);
}

static void
real_prompt_for_location (BaulWindow *window, const char *initial)
{
    BaulNavigationWindowPane *pane;

    remember_focus_widget (BAUL_NAVIGATION_WINDOW (window));

    pane = BAUL_NAVIGATION_WINDOW_PANE (window->details->active_pane);

    baul_navigation_window_pane_show_location_bar_temporarily (pane);
    baul_navigation_window_pane_show_navigation_bar_temporarily (pane);

    if (initial)
    {
        baul_location_bar_set_location (BAUL_LOCATION_BAR (pane->navigation_bar),
                                        initial);
    }
}

void
baul_navigation_window_show_search (BaulNavigationWindow *window)
{
    BaulNavigationWindowPane *pane;

    pane = BAUL_NAVIGATION_WINDOW_PANE (BAUL_WINDOW (window)->details->active_pane);
    if (!baul_navigation_window_pane_search_bar_showing (pane))
    {
        remember_focus_widget (window);

        baul_navigation_window_pane_show_location_bar_temporarily (pane);
        baul_navigation_window_pane_set_bar_mode (pane, BAUL_BAR_SEARCH);
        pane->temporary_search_bar = TRUE;
        baul_search_bar_clear (BAUL_SEARCH_BAR (pane->search_bar));
    }

    baul_search_bar_grab_focus (BAUL_SEARCH_BAR (pane->search_bar));
}

void
baul_navigation_window_hide_search (BaulNavigationWindow *window)
{
    BaulNavigationWindowPane *pane = BAUL_NAVIGATION_WINDOW_PANE (BAUL_WINDOW (window)->details->active_pane);
    if (baul_navigation_window_pane_search_bar_showing (pane))
    {
        if (baul_navigation_window_pane_hide_temporary_bars (pane))
        {
            baul_navigation_window_restore_focus_widget (window);
        }
    }
}

/* This updates the UI state of the search button, but does not
   in itself trigger a search action */
void
baul_navigation_window_set_search_button (BaulNavigationWindow *window,
        gboolean state)
{
    CtkAction *action;

    action = ctk_action_group_get_action (window->details->navigation_action_group,
                                          "Search");

    /* Block callback so we don't activate the action and thus focus the
       search entry */
    g_object_set_data (G_OBJECT (action), "blocked", GINT_TO_POINTER (1));
    ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action), state);
    g_object_set_data (G_OBJECT (action), "blocked", NULL);
}

static void
side_panel_image_changed_callback (BaulSidebar *side_panel,
                                   gpointer callback_data)
{
    BaulWindow *window;
    GdkPixbuf *icon;

    window = BAUL_WINDOW (callback_data);

    icon = baul_sidebar_get_tab_icon (side_panel);
    baul_side_pane_set_panel_image (BAUL_NAVIGATION_WINDOW (window)->sidebar,
                                    CTK_WIDGET (side_panel),
                                    icon);
    if (icon != NULL)
    {
        g_object_unref (icon);
    }
}

/**
 * add_sidebar_panels:
 * @window:	A BaulNavigationWindow
 *
 * Adds all sidebars available
 *
 */
static void
add_sidebar_panels (BaulNavigationWindow *window)
{
    CtkWidget *current;
    GList *providers;
    GList *p;
    BaulSidebar *sidebar_panel = NULL;

    g_assert (BAUL_IS_NAVIGATION_WINDOW (window));

    if (window->sidebar == NULL)
    {
        return;
    }

    providers = baul_module_get_extensions_for_type (BAUL_TYPE_SIDEBAR_PROVIDER);

    for (p = providers; p != NULL; p = p->next)
    {
        BaulSidebarProvider *provider;

        provider = BAUL_SIDEBAR_PROVIDER (p->data);

        sidebar_panel = baul_sidebar_provider_create (provider,
                        BAUL_WINDOW_INFO (window));
        baul_navigation_window_add_sidebar_panel (window,
                sidebar_panel);

        g_object_unref (sidebar_panel);
    }

    baul_module_extension_list_free (providers);

    current = baul_side_pane_get_current_panel (window->sidebar);
    set_current_side_panel
    (window,
     BAUL_SIDEBAR (current));
}

gboolean
baul_navigation_window_toolbar_showing (BaulNavigationWindow *window)
{
    if (window->details->toolbar != NULL)
    {
        return ctk_widget_get_visible (window->details->toolbar);
    }
    /* If we're not visible yet we haven't changed visibility, so its TRUE */
    return TRUE;
}

void
baul_navigation_window_hide_status_bar (BaulNavigationWindow *window)
{
    ctk_widget_hide (BAUL_WINDOW (window)->details->statusbar);

    baul_navigation_window_update_show_hide_menu_items (window);

    g_settings_set_boolean (baul_window_state, BAUL_WINDOW_STATE_START_WITH_STATUS_BAR, FALSE);
}

void
baul_navigation_window_show_status_bar (BaulNavigationWindow *window)
{
    ctk_widget_show (BAUL_WINDOW (window)->details->statusbar);

    baul_navigation_window_update_show_hide_menu_items (window);

    g_settings_set_boolean (baul_window_state, BAUL_WINDOW_STATE_START_WITH_STATUS_BAR, TRUE);
}

gboolean
baul_navigation_window_status_bar_showing (BaulNavigationWindow *window)
{
    if (BAUL_WINDOW (window)->details->statusbar != NULL)
    {
        return ctk_widget_get_visible (BAUL_WINDOW (window)->details->statusbar);
    }
    /* If we're not visible yet we haven't changed visibility, so its TRUE */
    return TRUE;
}


void
baul_navigation_window_hide_toolbar (BaulNavigationWindow *window)
{
    ctk_widget_hide (window->details->toolbar);
    baul_navigation_window_update_show_hide_menu_items (window);
    g_settings_set_boolean (baul_window_state, BAUL_WINDOW_STATE_START_WITH_TOOLBAR, FALSE);
}

void
baul_navigation_window_show_toolbar (BaulNavigationWindow *window)
{
    ctk_widget_show (window->details->toolbar);
    baul_navigation_window_update_show_hide_menu_items (window);
    g_settings_set_boolean (baul_window_state, BAUL_WINDOW_STATE_START_WITH_TOOLBAR, TRUE);
}

void
baul_navigation_window_hide_sidebar (BaulNavigationWindow *window)
{
    if (window->sidebar == NULL)
    {
        return;
    }

    baul_navigation_window_tear_down_sidebar (window);
    baul_navigation_window_update_show_hide_menu_items (window);

    g_settings_set_boolean (baul_window_state, BAUL_WINDOW_STATE_START_WITH_SIDEBAR, FALSE);
}

void
baul_navigation_window_show_sidebar (BaulNavigationWindow *window)
{
    if (window->sidebar != NULL)
    {
        return;
    }

    baul_navigation_window_set_up_sidebar (window);
    baul_navigation_window_update_show_hide_menu_items (window);
    g_settings_set_boolean (baul_window_state, BAUL_WINDOW_STATE_START_WITH_SIDEBAR, TRUE);
}

gboolean
baul_navigation_window_sidebar_showing (BaulNavigationWindow *window)
{
    g_return_val_if_fail (BAUL_IS_NAVIGATION_WINDOW (window), FALSE);

    return (window->sidebar != NULL)
           && ctk_widget_get_visible (ctk_paned_get_child1 (CTK_PANED (window->details->content_paned)));
}

/**
 * baul_navigation_window_get_base_page_index:
 * @window:	Window to get index from
 *
 * Returns the index of the base page in the history list.
 * Base page is not the currently displayed page, but the page
 * that acts as the base from which the back and forward commands
 * navigate from.
 */
gint
baul_navigation_window_get_base_page_index (BaulNavigationWindow *window)
{
    BaulNavigationWindowSlot *slot;
    gint forward_count;

    slot = BAUL_NAVIGATION_WINDOW_SLOT (BAUL_WINDOW (window)->details->active_pane->active_slot);

    forward_count = g_list_length (slot->forward_list);

    /* If forward is empty, the base it at the top of the list */
    if (forward_count == 0)
    {
        return 0;
    }

    /* The forward count indicate the relative postion of the base page
     * in the history list
     */
    return forward_count;
}

/**
 * baul_navigation_window_show:
 * @widget: a #CtkWidget.
 *
 * Call parent and then show/hide window items
 * base on user prefs.
 */
static void
baul_navigation_window_show (CtkWidget *widget)
{
    BaulNavigationWindow *window;
    gboolean show_location_bar;
    gboolean always_use_location_entry;
    GList *walk;

    window = BAUL_NAVIGATION_WINDOW (widget);

    /* Initially show or hide views based on preferences; once the window is displayed
     * these can be controlled on a per-window basis from View menu items.
     */

    if (g_settings_get_boolean (baul_window_state, BAUL_WINDOW_STATE_START_WITH_TOOLBAR))
    {
        baul_navigation_window_show_toolbar (window);
    }
    else
    {
        baul_navigation_window_hide_toolbar (window);
    }

    show_location_bar = g_settings_get_boolean (baul_window_state, BAUL_WINDOW_STATE_START_WITH_LOCATION_BAR);
    always_use_location_entry = g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_ALWAYS_USE_LOCATION_ENTRY);
    for (walk = BAUL_WINDOW(window)->details->panes; walk; walk = walk->next)
    {
        BaulNavigationWindowPane *pane = walk->data;
        if (show_location_bar)
        {
            baul_navigation_window_pane_show_location_bar (pane, FALSE);
        }
        else
        {
            baul_navigation_window_pane_hide_location_bar (pane, FALSE);
        }

        if (always_use_location_entry)
        {
            baul_navigation_window_pane_set_bar_mode (pane, BAUL_BAR_NAVIGATION);
        }
        else
        {
            baul_navigation_window_pane_set_bar_mode (pane, BAUL_BAR_PATH);
        }
    }

    if (g_settings_get_boolean (baul_window_state, BAUL_WINDOW_STATE_START_WITH_SIDEBAR))
    {
        baul_navigation_window_show_sidebar (window);
    }
    else
    {
        baul_navigation_window_hide_sidebar (window);
    }

    if (g_settings_get_boolean (baul_window_state, BAUL_WINDOW_STATE_START_WITH_STATUS_BAR))
    {
        baul_navigation_window_show_status_bar (window);
    }
    else
    {
        baul_navigation_window_hide_status_bar (window);
    }

    CTK_WIDGET_CLASS (parent_class)->show (widget);
}

static void
baul_navigation_window_save_geometry (BaulNavigationWindow *window)
{
    gboolean is_maximized;

    g_assert (BAUL_IS_WINDOW (window));

    if (ctk_widget_get_window (CTK_WIDGET (window)))
    {
        char *geometry_string;

        geometry_string = eel_ctk_window_get_geometry_string (CTK_WINDOW (window));
        is_maximized = cdk_window_get_state (ctk_widget_get_window (CTK_WIDGET (window)))
                       & CDK_WINDOW_STATE_MAXIMIZED;

        if (!is_maximized)
        {
            g_settings_set_string (baul_window_state,
                                   BAUL_WINDOW_STATE_GEOMETRY,
                                   geometry_string);
        }
        g_free (geometry_string);

        g_settings_set_boolean (baul_window_state,
                                BAUL_WINDOW_STATE_MAXIMIZED,
                                is_maximized);
    }
}

static void
real_window_close (BaulWindow *window)
{
    baul_navigation_window_save_geometry (BAUL_NAVIGATION_WINDOW (window));
}

static void
real_get_min_size (BaulWindow *window G_GNUC_UNUSED,
		   guint      *min_width,
		   guint      *min_height)
{
    if (min_width)
    {
        *min_width = BAUL_NAVIGATION_WINDOW_MIN_WIDTH;
    }
    if (min_height)
    {
        *min_height = BAUL_NAVIGATION_WINDOW_MIN_HEIGHT;
    }
}

static void
real_get_default_size (BaulWindow *window G_GNUC_UNUSED,
		       guint      *default_width,
		       guint      *default_height)
{
    if (default_width)
    {
        *default_width = BAUL_NAVIGATION_WINDOW_DEFAULT_WIDTH;
    }

    if (default_height)
    {
        *default_height = BAUL_NAVIGATION_WINDOW_DEFAULT_HEIGHT;
    }
}

static BaulWindowSlot *
real_open_slot (BaulWindowPane *pane,
                BaulWindowOpenSlotFlags flags)
{
    BaulWindowSlot *slot;

    slot = (BaulWindowSlot *) g_object_new (BAUL_TYPE_NAVIGATION_WINDOW_SLOT, NULL);
    slot->pane = pane;

    baul_navigation_window_pane_add_slot_in_tab (BAUL_NAVIGATION_WINDOW_PANE (pane), slot, flags);
    ctk_widget_show (slot->content_box);

    return slot;
}

static void
real_close_slot (BaulWindowPane *pane,
                 BaulWindowSlot *slot)
{
    int page_num;
    CtkNotebook *notebook;

    notebook = CTK_NOTEBOOK (BAUL_NAVIGATION_WINDOW_PANE (pane)->notebook);

    page_num = ctk_notebook_page_num (notebook, slot->content_box);
    g_assert (page_num >= 0);

    baul_navigation_window_pane_remove_page (BAUL_NAVIGATION_WINDOW_PANE (pane), page_num);

    ctk_notebook_set_show_tabs (notebook,
                                ctk_notebook_get_n_pages (notebook) > 1);

    EEL_CALL_PARENT (BAUL_WINDOW_CLASS,
                     close_slot, (pane, slot));
}

static void
baul_navigation_window_class_init (BaulNavigationWindowClass *class)
{
    BAUL_WINDOW_CLASS (class)->window_type = BAUL_WINDOW_NAVIGATION;
    BAUL_WINDOW_CLASS (class)->bookmarks_placeholder = MENU_PATH_BOOKMARKS_PLACEHOLDER;

    G_OBJECT_CLASS (class)->finalize = baul_navigation_window_finalize;

    CTK_WIDGET_CLASS (class)->destroy = baul_navigation_window_destroy;
    CTK_WIDGET_CLASS (class)->show = baul_navigation_window_show;
    CTK_WIDGET_CLASS (class)->window_state_event = baul_navigation_window_state_event;
    CTK_WIDGET_CLASS (class)->key_press_event = baul_navigation_window_key_press_event;
    CTK_WIDGET_CLASS (class)->button_press_event = baul_navigation_window_button_press_event;
    BAUL_WINDOW_CLASS (class)->sync_allow_stop = real_sync_allow_stop;
    BAUL_WINDOW_CLASS (class)->prompt_for_location = real_prompt_for_location;
    BAUL_WINDOW_CLASS (class)->sync_title = real_sync_title;
    BAUL_WINDOW_CLASS (class)->get_icon = real_get_icon;
    BAUL_WINDOW_CLASS (class)->get_min_size = real_get_min_size;
    BAUL_WINDOW_CLASS (class)->get_default_size = real_get_default_size;
    BAUL_WINDOW_CLASS (class)->close = real_window_close;

    BAUL_WINDOW_CLASS (class)->open_slot = real_open_slot;
    BAUL_WINDOW_CLASS (class)->close_slot = real_close_slot;

    g_signal_connect_swapped (baul_preferences,
                              "changed::" BAUL_PREFERENCES_MOUSE_BACK_BUTTON,
                              G_CALLBACK(mouse_back_button_changed),
                              NULL);

    g_signal_connect_swapped (baul_preferences,
                              "changed::" BAUL_PREFERENCES_MOUSE_FORWARD_BUTTON,
                              G_CALLBACK(mouse_forward_button_changed),
                              NULL);

    g_signal_connect_swapped (baul_preferences,
                              "changed::" BAUL_PREFERENCES_MOUSE_USE_EXTRA_BUTTONS,
                              G_CALLBACK(use_extra_mouse_buttons_changed),
                              NULL);
}

static BaulWindowSlot *
create_extra_pane (BaulNavigationWindow *window)
{
    BaulWindow *win;
    BaulNavigationWindowPane *pane;
    BaulWindowSlot *slot;
    CtkPaned *paned;

    win = BAUL_WINDOW (window);

    /* New pane */
    pane = baul_navigation_window_pane_new (win);
    win->details->panes = g_list_append (win->details->panes, pane);

    baul_navigation_window_pane_setup (pane);

    paned = CTK_PANED (window->details->split_view_hpane);
    if (ctk_paned_get_child1 (paned) == NULL)
    {
        ctk_paned_pack1 (paned, pane->widget, TRUE, FALSE);
    }
    else
    {
        ctk_paned_pack2 (paned, pane->widget, TRUE, FALSE);
    }

    /* slot */
    slot = baul_window_open_slot (BAUL_WINDOW_PANE (pane),
                                  BAUL_WINDOW_OPEN_SLOT_APPEND);
    BAUL_WINDOW_PANE (pane)->active_slot = slot;

    return slot;
}

void
baul_navigation_window_split_view_on (BaulNavigationWindow *window)
{
    BaulWindow *win;
    BaulNavigationWindowPane *pane;
    BaulWindowSlot *slot, *old_active_slot;
    GFile *location;
    CtkAction *action;

    win = BAUL_WINDOW (window);

    old_active_slot = baul_window_get_active_slot (win);
    slot = create_extra_pane (window);
    pane = BAUL_NAVIGATION_WINDOW_PANE (slot->pane);

    location = NULL;
    if (old_active_slot != NULL)
    {
        location = baul_window_slot_get_location (old_active_slot);
        if (location != NULL)
        {
            if (g_file_has_uri_scheme (location, "x-baul-search"))
            {
                g_object_unref (location);
                location = NULL;
            }
        }
    }
    if (location == NULL)
    {
        location = g_file_new_for_path (g_get_home_dir ());
    }

    baul_window_slot_go_to (slot, location, FALSE);
    g_object_unref (location);

    action = ctk_action_group_get_action (BAUL_NAVIGATION_WINDOW (BAUL_WINDOW_PANE (pane)->window)->details->navigation_action_group,
                                          BAUL_ACTION_SHOW_HIDE_LOCATION_BAR);
    if (ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)))
    {
        baul_navigation_window_pane_show_location_bar (pane, TRUE);
    }
    else
    {
        baul_navigation_window_pane_hide_location_bar (pane, TRUE);
    }
}

void
baul_navigation_window_split_view_off (BaulNavigationWindow *window)
{
    BaulWindow *win;
    GList *l, *next;
    BaulWindowPane *active_pane;
    BaulWindowPane *pane = NULL;

    win = BAUL_WINDOW (window);

    g_return_if_fail (win);

    active_pane = win->details->active_pane;

    /* delete all panes except the first (main) pane */
    for (l = win->details->panes; l != NULL; l = next)
    {
        next = l->next;
        pane = l->data;
        if (pane != active_pane)
        {
            baul_window_close_pane (pane);
        }
    }

    baul_navigation_window_update_show_hide_menu_items (window);
    baul_navigation_window_update_split_view_actions_sensitivity (window);
}

gboolean
baul_navigation_window_split_view_showing (BaulNavigationWindow *window)
{
    return g_list_length (BAUL_WINDOW (window)->details->panes) > 1;
}
