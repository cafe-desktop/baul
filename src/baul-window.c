/*
 *  Baul
 *
 *  Copyright (C) 1999, 2000, 2004 Red Hat, Inc.
 *  Copyright (C) 1999, 2000, 2001 Eazel, Inc.
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
 *           Alexander Larsson <alexl@redhat.com>
 */

/* baul-window.c: Implementation of the main window object */

#include <config.h>

#include <cdk-pixbuf/cdk-pixbuf.h>
#include <cdk/cdkx.h>
#include <cdk/cdkkeysyms.h>
#include <ctk/ctk.h>
#include <glib/gi18n.h>
#ifdef HAVE_X11_XF86KEYSYM_H
#include <X11/XF86keysym.h>
#endif

#include <eel/eel-debug.h>
#include <eel/eel-ctk-macros.h>
#include <eel/eel-string.h>

#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-file-attributes.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-metadata.h>
#include <libbaul-private/baul-mime-actions.h>
#include <libbaul-private/baul-program-choosing.h>
#include <libbaul-private/baul-view-factory.h>
#include <libbaul-private/baul-clipboard.h>
#include <libbaul-private/baul-search-directory.h>
#include <libbaul-private/baul-signaller.h>

#include "baul-window-private.h"
#include "baul-actions.h"
#include "baul-application.h"
#include "baul-bookmarks-window.h"
#include "baul-information-panel.h"
#include "baul-window-manage-views.h"
#include "baul-window-bookmarks.h"
#include "baul-window-slot.h"
#include "baul-navigation-window-slot.h"
#include "baul-search-bar.h"
#include "baul-navigation-window-pane.h"
#include "baul-src-marshal.h"

#define MAX_HISTORY_ITEMS 50

#define EXTRA_VIEW_WIDGETS_BACKGROUND "#a7c6e1"

/* dock items */

#define BAUL_MENU_PATH_EXTRA_VIEWER_PLACEHOLDER	"/MenuBar/View/View Choices/Extra Viewer"
#define BAUL_MENU_PATH_SHORT_LIST_PLACEHOLDER  	"/MenuBar/View/View Choices/Short List"

enum {
	ARG_0,
	ARG_APP
};

enum {
	GO_UP,
	RELOAD,
	PROMPT_FOR_LOCATION,
	ZOOM_CHANGED,
	VIEW_AS_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct
{
    BaulWindow *window;
    char *id;
} ActivateViewData;

static void cancel_view_as_callback         (BaulWindowSlot      *slot);
static void baul_window_info_iface_init (BaulWindowInfoIface *iface);
static void action_view_as_callback         (CtkAction               *action,
        ActivateViewData        *data);

static GList *history_list;

G_DEFINE_TYPE_WITH_CODE (BaulWindow, baul_window, CTK_TYPE_WINDOW,
                         G_ADD_PRIVATE (BaulWindow)
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_WINDOW_INFO,
                                 baul_window_info_iface_init));

static const struct
{
    unsigned int keyval;
    const char *action;
} extra_window_keybindings [] =
{
#ifdef HAVE_X11_XF86KEYSYM_H
    { XF86XK_AddFavorite,	BAUL_ACTION_ADD_BOOKMARK },
    { XF86XK_Favorites,	BAUL_ACTION_EDIT_BOOKMARKS },
    { XF86XK_Go,		BAUL_ACTION_GO_TO_LOCATION },
    /* TODO?{ XF86XK_History,	BAUL_ACTION_HISTORY }, */
    { XF86XK_HomePage,      BAUL_ACTION_GO_HOME },
    { XF86XK_OpenURL,	BAUL_ACTION_GO_TO_LOCATION },
    { XF86XK_Refresh,	BAUL_ACTION_RELOAD },
    { XF86XK_Reload,	BAUL_ACTION_RELOAD },
    { XF86XK_Search,	BAUL_ACTION_SEARCH },
    { XF86XK_Start,		BAUL_ACTION_GO_HOME },
    { XF86XK_Stop,		BAUL_ACTION_STOP },
    { XF86XK_ZoomIn,	BAUL_ACTION_ZOOM_IN },
    { XF86XK_ZoomOut,	BAUL_ACTION_ZOOM_OUT }
#endif
};

static void
baul_window_init (BaulWindow *window)
{
    CtkWidget *grid;
    CtkWidget *menu;
    CtkWidget *statusbar;

    static const gchar css_custom[] =
      "#baul-extra-view-widget {"
      "  background-color: " EXTRA_VIEW_WIDGETS_BACKGROUND ";"
      "}";

    GError *error = NULL;
    CtkCssProvider *provider = ctk_css_provider_new ();
    ctk_css_provider_load_from_data (provider, css_custom, -1, &error);

    if (error != NULL) {
            g_warning ("Can't parse BaulWindow's CSS custom description: %s\n", error->message);
            g_error_free (error);
    } else {
            ctk_style_context_add_provider (ctk_widget_get_style_context (CTK_WIDGET (window)),
                                            CTK_STYLE_PROVIDER (provider),
                                            CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    g_object_unref (provider);
    window->details = baul_window_get_instance_private (window);

    window->details->panes = NULL;
    window->details->active_pane = NULL;

    window->details->show_hidden_files_mode = BAUL_WINDOW_SHOW_HIDDEN_FILES_DEFAULT;

    /* Set initial window title */
    ctk_window_set_title (CTK_WINDOW (window), _("Baul"));

    grid = ctk_grid_new ();
    ctk_orientable_set_orientation (CTK_ORIENTABLE (grid), CTK_ORIENTATION_VERTICAL);
    window->details->grid = grid;
    ctk_widget_show (grid);
    ctk_container_add (CTK_CONTAINER (window), grid);

    statusbar = ctk_statusbar_new ();
    ctk_widget_set_name (statusbar, "statusbar-noborder");

/* set margin to zero to reduce size of statusbar */
	ctk_widget_set_margin_top (CTK_WIDGET (statusbar), 0);
	ctk_widget_set_margin_bottom (CTK_WIDGET (statusbar), 0);

    window->details->statusbar = statusbar;
    window->details->help_message_cid = ctk_statusbar_get_context_id
                                        (CTK_STATUSBAR (statusbar), "help_message");
    /* Statusbar is packed in the subclasses */

    baul_window_initialize_menus (window);

    menu = ctk_ui_manager_get_widget (window->details->ui_manager, "/MenuBar");
    window->details->menubar = menu;
    ctk_widget_set_hexpand (menu, TRUE);
    ctk_widget_show (menu);
    ctk_grid_attach (CTK_GRID (grid), menu, 0, 0, 1, 1);

    /* Register to menu provider extension signal managing menu updates */
    g_signal_connect_object (baul_signaller_get_current (), "popup_menu_changed",
                             G_CALLBACK (baul_window_load_extension_menus), window, G_CONNECT_SWAPPED);
}

/* Unconditionally synchronize the CtkUIManager of WINDOW. */
static void
baul_window_ui_update (BaulWindow *window)
{
    g_assert (BAUL_IS_WINDOW (window));

    ctk_ui_manager_ensure_update (window->details->ui_manager);
}

static void
baul_window_push_status (BaulWindow *window,
                         const char *text)
{
    g_return_if_fail (BAUL_IS_WINDOW (window));

    /* clear any previous message, underflow is allowed */
    ctk_statusbar_pop (CTK_STATUSBAR (window->details->statusbar), 0);

    if (text != NULL && text[0] != '\0')
    {
        ctk_statusbar_push (CTK_STATUSBAR (window->details->statusbar), 0, text);
    }
}

void
baul_window_sync_status (BaulWindow *window)
{
    BaulWindowSlot *slot;

    slot = window->details->active_pane->active_slot;
    baul_window_push_status (window, slot->status_text);
}

void
baul_window_go_to (BaulWindow *window, GFile *location)
{
    g_return_if_fail (BAUL_IS_WINDOW (window));

    baul_window_slot_go_to (window->details->active_pane->active_slot, location, FALSE);
}

void
baul_window_go_to_tab (BaulWindow *window, GFile *location)
{
    g_return_if_fail (BAUL_IS_WINDOW (window));

    baul_window_slot_go_to (window->details->active_pane->active_slot, location, TRUE);
}

void
baul_window_go_to_full (BaulWindow *window,
                        GFile                 *location,
                        BaulWindowGoToCallback callback,
                        gpointer               user_data)
{
    g_return_if_fail (BAUL_IS_WINDOW (window));

    baul_window_slot_go_to_full (window->details->active_pane->active_slot, location, FALSE, callback, user_data);
}

void
baul_window_go_to_with_selection (BaulWindow *window, GFile *location, GList *new_selection)
{
    g_return_if_fail (BAUL_IS_WINDOW (window));

    baul_window_slot_go_to_with_selection (window->details->active_pane->active_slot, location, new_selection);
}

static gboolean
baul_window_go_up_signal (BaulWindow *window, gboolean close_behind)
{
    baul_window_go_up (window, close_behind, FALSE);
    return TRUE;
}

void
baul_window_new_tab (BaulWindow *window)
{
    BaulWindowSlot *current_slot;
    BaulWindowOpenFlags flags;
    GFile *location = NULL;

    current_slot = window->details->active_pane->active_slot;
    location = baul_window_slot_get_location (current_slot);

    if (location != NULL) {
        BaulWindowSlot *new_slot;
        int new_slot_position;
        char *scheme;

    	flags = 0;

    	new_slot_position = g_settings_get_enum (baul_preferences, BAUL_PREFERENCES_NEW_TAB_POSITION);
    	if (new_slot_position == BAUL_NEW_TAB_POSITION_END) {
    		flags = BAUL_WINDOW_OPEN_SLOT_APPEND;
    	}

    	scheme = g_file_get_uri_scheme (location);
    	if (!strcmp (scheme, "x-baul-search")) {
    		g_object_unref (location);
    		location = g_file_new_for_path (g_get_home_dir ());
    	}
    	g_free (scheme);

    	new_slot = baul_window_open_slot (current_slot->pane, flags);
    	baul_window_set_active_slot (window, new_slot);
    	baul_window_slot_go_to (new_slot, location, FALSE);
    	g_object_unref (location);
    }
}

/*Opens a new window when called from an existing window and goes to the same location that's in the existing window.*/
void
baul_window_new_window (BaulWindow *window)
{
    BaulWindowSlot *current_slot;
    GFile *location = NULL;
    g_return_if_fail (BAUL_IS_WINDOW (window));

    /*Get and set the directory location of current window (slot).*/
    current_slot = window->details->active_pane->active_slot;
    location = baul_window_slot_get_location (current_slot);

    if (location != NULL) 
    {
        BaulWindow *new_window;
        BaulWindowSlot *new_slot;
        BaulWindowOpenFlags flags;
        flags = FALSE;

        /*Create a new window*/
        new_window = baul_application_create_navigation_window (
                     window->application,
        ctk_window_get_screen (CTK_WINDOW (window)));

        /*Create a slot in the new window.*/
        new_slot = new_window->details->active_pane->active_slot;
        g_return_if_fail (BAUL_IS_WINDOW_SLOT (new_slot));

        /*Open a directory at the set location in the new window (slot).*/
        baul_window_slot_open_location_full (new_slot, location,
                                             BAUL_WINDOW_OPEN_ACCORDING_TO_MODE,
                                             flags, NULL, NULL, NULL);
        g_object_unref (location);
    }
}

void
baul_window_go_up (BaulWindow *window, gboolean close_behind, gboolean new_tab)
{
    BaulWindowSlot *slot;
    GFile *parent;
    GList *selection;
    BaulWindowOpenFlags flags;

    g_assert (BAUL_IS_WINDOW (window));

    slot = window->details->active_pane->active_slot;

    if (slot->location == NULL)
    {
        return;
    }

    parent = g_file_get_parent (slot->location);

    if (parent == NULL)
    {
        return;
    }

    selection = g_list_prepend (NULL, g_object_ref (slot->location));

    flags = 0;
    if (close_behind)
    {
        flags |= BAUL_WINDOW_OPEN_FLAG_CLOSE_BEHIND;
    }
    if (new_tab)
    {
        flags |= BAUL_WINDOW_OPEN_FLAG_NEW_TAB;
    }

    baul_window_slot_open_location_full (slot, parent,
                                         BAUL_WINDOW_OPEN_ACCORDING_TO_MODE,
                                         flags,
                                         selection,
                                         NULL, NULL);

    g_object_unref (parent);

    g_list_free_full (selection, g_object_unref);
}

static void
real_set_allow_up (BaulWindow *window,
                   gboolean        allow)
{
    CtkAction *action;

    g_assert (BAUL_IS_WINDOW (window));

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    action = ctk_action_group_get_action (window->details->main_action_group,
                                          BAUL_ACTION_UP);
    ctk_action_set_sensitive (action, allow);
    action = ctk_action_group_get_action (window->details->main_action_group,
                                          BAUL_ACTION_UP_ACCEL);
    ctk_action_set_sensitive (action, allow);
    G_GNUC_END_IGNORE_DEPRECATIONS;
}

void
baul_window_allow_up (BaulWindow *window, gboolean allow)
{
    g_return_if_fail (BAUL_IS_WINDOW (window));

    EEL_CALL_METHOD (BAUL_WINDOW_CLASS, window,
                     set_allow_up, (window, allow));
}

static void
update_cursor (BaulWindow *window)
{
    BaulWindowSlot *slot;

    slot = window->details->active_pane->active_slot;

    if (slot->allow_stop)
    {
        CdkDisplay *display;
        CdkCursor * cursor;

        display = ctk_widget_get_display (CTK_WIDGET (window));
        cursor = cdk_cursor_new_for_display (display, GDK_WATCH);
        cdk_window_set_cursor (ctk_widget_get_window (CTK_WIDGET (window)), cursor);
        g_object_unref (cursor);
    }
    else
    {
        cdk_window_set_cursor (ctk_widget_get_window (CTK_WIDGET (window)), NULL);
    }
}

void
baul_window_sync_allow_stop (BaulWindow *window,
                             BaulWindowSlot *slot)
{
    CtkAction *action;
    gboolean allow_stop;

    g_assert (BAUL_IS_WINDOW (window));

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    action = ctk_action_group_get_action (window->details->main_action_group,
                                          BAUL_ACTION_STOP);
    allow_stop = ctk_action_get_sensitive (action);
    G_GNUC_END_IGNORE_DEPRECATIONS;

    if (slot != window->details->active_pane->active_slot ||
            allow_stop != slot->allow_stop)
    {
        if (slot == window->details->active_pane->active_slot)
        {
            G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
            ctk_action_set_sensitive (action, slot->allow_stop);
            G_GNUC_END_IGNORE_DEPRECATIONS;
        }

        if (ctk_widget_get_realized (CTK_WIDGET (window)))
        {
            update_cursor (window);
        }

        EEL_CALL_METHOD (BAUL_WINDOW_CLASS, window,
                         sync_allow_stop, (window, slot));
    }
}

void
baul_window_allow_reload (BaulWindow *window, gboolean allow)
{
    CtkAction *action;

    g_return_if_fail (BAUL_IS_WINDOW (window));

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    action = ctk_action_group_get_action (window->details->main_action_group,
                                          BAUL_ACTION_RELOAD);
    ctk_action_set_sensitive (action, allow);
    G_GNUC_END_IGNORE_DEPRECATIONS;
}

void
baul_window_go_home (BaulWindow *window)
{
    g_return_if_fail (BAUL_IS_WINDOW (window));

    baul_window_slot_go_home (window->details->active_pane->active_slot, FALSE);
}

void
baul_window_prompt_for_location (BaulWindow *window,
                                 const char     *initial)
{
    g_return_if_fail (BAUL_IS_WINDOW (window));

    EEL_CALL_METHOD (BAUL_WINDOW_CLASS, window,
                     prompt_for_location, (window, initial));
}

static char *
baul_window_get_location_uri (BaulWindow *window)
{
    BaulWindowSlot *slot;

    g_assert (BAUL_IS_WINDOW (window));

    slot = window->details->active_pane->active_slot;

    if (slot->location)
    {
        return g_file_get_uri (slot->location);
    }
    return NULL;
}

void
baul_window_zoom_in (BaulWindow *window)
{
    g_assert (window != NULL);

    baul_window_pane_zoom_in (window->details->active_pane);
}

void
baul_window_zoom_to_level (BaulWindow *window,
                           BaulZoomLevel level)
{
    g_assert (window != NULL);

    baul_window_pane_zoom_to_level (window->details->active_pane, level);
}

void
baul_window_zoom_out (BaulWindow *window)
{
    g_assert (window != NULL);

    baul_window_pane_zoom_out (window->details->active_pane);
}

void
baul_window_zoom_to_default (BaulWindow *window)
{
    g_assert (window != NULL);

    baul_window_pane_zoom_to_default (window->details->active_pane);
}

/* Code should never force the window taller than this size.
 * (The user can still stretch the window taller if desired).
 */
static guint
get_max_forced_height (CdkScreen *screen)
{
    gint scale = cdk_window_get_scale_factor (cdk_screen_get_root_window (screen));
    return (HeightOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale * 90) / 100;
}

/* Code should never force the window wider than this size.
 * (The user can still stretch the window wider if desired).
 */
static guint
get_max_forced_width (CdkScreen *screen)
{
    gint scale = cdk_window_get_scale_factor (cdk_screen_get_root_window (screen));
    return (WidthOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale * 90) / 100;
}

/* This must be called when construction of BaulWindow is finished,
 * since it depends on the type of the argument, which isn't decided at
 * construction time.
 */
static void
baul_window_set_initial_window_geometry (BaulWindow *window)
{
    CdkScreen *screen;
    guint max_width_for_screen, max_height_for_screen;

    guint default_width = 0;
    guint default_height = 0;

    screen = ctk_window_get_screen (CTK_WINDOW (window));

    max_width_for_screen = get_max_forced_width (screen);
    max_height_for_screen = get_max_forced_height (screen);

    EEL_CALL_METHOD (BAUL_WINDOW_CLASS, window,
                     get_default_size, (window, &default_width, &default_height));

    ctk_window_set_default_size (CTK_WINDOW (window),
                                 MIN (default_width,
                                      max_width_for_screen),
                                 MIN (default_height,
                                      max_height_for_screen));
}

static void
baul_window_constructed (GObject *self)
{
    BaulWindow *window;

    window = BAUL_WINDOW (self);

    baul_window_initialize_bookmarks_menu (window);
    baul_window_set_initial_window_geometry (window);
}

static void
baul_window_set_property (GObject *object,
                          guint arg_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
    BaulWindow *window;

    window = BAUL_WINDOW (object);

    switch (arg_id)
    {
    case ARG_APP:
        window->application = BAUL_APPLICATION (g_value_get_object (value));
        break;
    }
}

static void
baul_window_get_property (GObject *object,
                          guint arg_id,
                          GValue *value,
                          GParamSpec *pspec)
{
    switch (arg_id)
    {
    case ARG_APP:
        g_value_set_object (value, BAUL_WINDOW (object)->application);
        break;
    }
}

static void
free_stored_viewers (BaulWindow *window)
{
    g_list_free_full (window->details->short_list_viewers, g_free);
    window->details->short_list_viewers = NULL;
    g_free (window->details->extra_viewer);
    window->details->extra_viewer = NULL;
}

static void
baul_window_destroy (CtkWidget *object)
{
    BaulWindow *window;
    GList *panes_copy;

    window = BAUL_WINDOW (object);

    /* close all panes safely */
    panes_copy = g_list_copy (window->details->panes);
    g_list_free_full (panes_copy, (GDestroyNotify) baul_window_close_pane);

    /* the panes list should now be empty */
    g_assert (window->details->panes == NULL);
    g_assert (window->details->active_pane == NULL);

    CTK_WIDGET_CLASS (baul_window_parent_class)->destroy (object);
}

static void
baul_window_finalize (GObject *object)
{
    BaulWindow *window;

    window = BAUL_WINDOW (object);

    baul_window_finalize_menus (window);
    free_stored_viewers (window);

    if (window->details->bookmark_list != NULL)
    {
        g_object_unref (window->details->bookmark_list);
    }

    /* baul_window_close() should have run */
    g_assert (window->details->panes == NULL);

    g_object_unref (window->details->ui_manager);

    G_OBJECT_CLASS (baul_window_parent_class)->finalize (object);
}

static GObject *
baul_window_constructor (GType type,
                         guint n_construct_properties,
                         GObjectConstructParam *construct_params)
{
    GObject *object;
    BaulWindow *window;
    BaulWindowSlot *slot;

    object = (* G_OBJECT_CLASS (baul_window_parent_class)->constructor) (type,
             n_construct_properties,
             construct_params);

    window = BAUL_WINDOW (object);

    slot = baul_window_open_slot (window->details->active_pane, 0);
    baul_window_set_active_slot (window, slot);

    return object;
}

void
baul_window_show_window (BaulWindow    *window)
{
    BaulWindowSlot *slot;
    BaulWindowPane *pane;
    GList *l, *walk;

    for (walk = window->details->panes; walk; walk = walk->next)
    {
        pane = walk->data;
        for (l = pane->slots; l != NULL; l = l->next)
        {
            slot = l->data;

            baul_window_slot_update_title (slot);
            baul_window_slot_update_icon (slot);
        }
    }

    ctk_widget_show (CTK_WIDGET (window));

    slot = window->details->active_pane->active_slot;

    if (slot->viewed_file)
    {
        if (BAUL_IS_SPATIAL_WINDOW (window))
        {
            baul_file_set_has_open_window (slot->viewed_file, TRUE);
        }
    }
}

static void
baul_window_view_visible (BaulWindow *window,
                          BaulView *view)
{
    BaulWindowSlot *slot;
    BaulWindowPane *pane;
    GList *l, *walk;

    g_return_if_fail (BAUL_IS_WINDOW (window));

    slot = baul_window_get_slot_for_view (window, view);

    /* Ensure we got the right active state for newly added panes */
    baul_window_slot_is_in_active_pane (slot, slot->pane->is_active);

    if (slot->visible)
    {
        return;
    }

    slot->visible = TRUE;

    pane = slot->pane;

    if (pane->visible)
    {
        return;
    }

    /* Look for other non-visible slots */
    for (l = pane->slots; l != NULL; l = l->next)
    {
        slot = l->data;

        if (!slot->visible)
        {
            return;
        }
    }

    /* None, this pane is visible */
    baul_window_pane_show (pane);

    /* Look for other non-visible panes */
    for (walk = window->details->panes; walk; walk = walk->next)
    {
        pane = walk->data;

        if (!pane->visible)
        {
            return;
        }
    }

    baul_window_pane_grab_focus (window->details->active_pane);

    /* All slots and panes visible, show window */
    baul_window_show_window (window);
}

void
baul_window_close (BaulWindow *window)
{
    g_return_if_fail (BAUL_IS_WINDOW (window));

    EEL_CALL_METHOD (BAUL_WINDOW_CLASS, window,
                     close, (window));

    ctk_widget_destroy (CTK_WIDGET (window));
}

BaulWindowSlot *
baul_window_open_slot (BaulWindowPane *pane,
                       BaulWindowOpenSlotFlags flags)
{
    BaulWindowSlot *slot;

    g_assert (BAUL_IS_WINDOW_PANE (pane));
    g_assert (BAUL_IS_WINDOW (pane->window));

    slot = EEL_CALL_METHOD_WITH_RETURN_VALUE (BAUL_WINDOW_CLASS, pane->window,
            open_slot, (pane, flags));

    g_assert (BAUL_IS_WINDOW_SLOT (slot));
    g_assert (pane->window == slot->pane->window);

    pane->slots = g_list_append (pane->slots, slot);

    return slot;
}

void
baul_window_close_pane (BaulWindowPane *pane)
{
    BaulWindow *window;

    g_assert (BAUL_IS_WINDOW_PANE (pane));
    g_assert (BAUL_IS_WINDOW (pane->window));
    g_assert (g_list_find (pane->window->details->panes, pane) != NULL);

    while (pane->slots != NULL)
    {
        BaulWindowSlot *slot = pane->slots->data;

        baul_window_close_slot (slot);
    }

    window = pane->window;

    /* If the pane was active, set it to NULL. The caller is responsible
     * for setting a new active pane with baul_window_pane_switch_to()
     * if it wants to continue using the window. */
    if (window->details->active_pane == pane)
    {
        window->details->active_pane = NULL;
    }

    window->details->panes = g_list_remove (window->details->panes, pane);

    g_object_unref (pane);
}

static void
real_close_slot (BaulWindowPane *pane,
                 BaulWindowSlot *slot)
{
    baul_window_manage_views_close_slot (pane, slot);
    cancel_view_as_callback (slot);
}

void
baul_window_close_slot (BaulWindowSlot *slot)
{
    BaulWindowPane *pane;

    g_assert (BAUL_IS_WINDOW_SLOT (slot));
    g_assert (BAUL_IS_WINDOW_PANE(slot->pane));
    g_assert (g_list_find (slot->pane->slots, slot) != NULL);

    /* save pane because slot is not valid anymore after this call */
    pane = slot->pane;

    EEL_CALL_METHOD (BAUL_WINDOW_CLASS, slot->pane->window,
                     close_slot, (slot->pane, slot));

    g_object_run_dispose (G_OBJECT (slot));
    slot->pane = NULL;
    g_object_unref (slot);
    pane->slots = g_list_remove (pane->slots, slot);
    pane->active_slots = g_list_remove (pane->active_slots, slot);

}

BaulWindowPane*
baul_window_get_active_pane (BaulWindow *window)
{
    g_assert (BAUL_IS_WINDOW (window));
    return window->details->active_pane;
}

static void
real_set_active_pane (BaulWindow *window, BaulWindowPane *new_pane)
{
    /* make old pane inactive, and new one active.
     * Currently active pane may be NULL (after init). */
    if (window->details->active_pane &&
            window->details->active_pane != new_pane)
    {
        baul_window_pane_set_active (new_pane->window->details->active_pane, FALSE);
    }
    baul_window_pane_set_active (new_pane, TRUE);

    window->details->active_pane = new_pane;
}

/* Make the given pane the active pane of its associated window. This
 * always implies making the containing active slot the active slot of
 * the window. */
void
baul_window_set_active_pane (BaulWindow *window,
                             BaulWindowPane *new_pane)
{
    g_assert (BAUL_IS_WINDOW_PANE (new_pane));
    if (new_pane->active_slot)
    {
        baul_window_set_active_slot (window, new_pane->active_slot);
    }
    else if (new_pane != window->details->active_pane)
    {
        real_set_active_pane (window, new_pane);
    }
}

/* Make both, the given slot the active slot and its corresponding
 * pane the active pane of the associated window.
 * new_slot may be NULL. */
void
baul_window_set_active_slot (BaulWindow *window, BaulWindowSlot *new_slot)
{
    BaulWindowSlot *old_slot;

    g_assert (BAUL_IS_WINDOW (window));

    if (new_slot)
    {
        g_assert (BAUL_IS_WINDOW_SLOT (new_slot));
        g_assert (BAUL_IS_WINDOW_PANE (new_slot->pane));
        g_assert (window == new_slot->pane->window);
        g_assert (g_list_find (new_slot->pane->slots, new_slot) != NULL);
    }

    if (window->details->active_pane != NULL)
    {
        old_slot = window->details->active_pane->active_slot;
    }
    else
    {
        old_slot = NULL;
    }

    if (old_slot == new_slot)
    {
        return;
    }

    /* make old slot inactive if it exists (may be NULL after init, for example) */
    if (old_slot != NULL)
    {
        /* inform window */
        if (old_slot->content_view != NULL)
        {
            baul_window_slot_disconnect_content_view (old_slot, old_slot->content_view);
        }

        /* inform slot & view */
        g_signal_emit_by_name (old_slot, "inactive");
    }

    /* deal with panes */
    if (new_slot &&
            new_slot->pane != window->details->active_pane)
    {
        real_set_active_pane (window, new_slot->pane);
    }

    window->details->active_pane->active_slot = new_slot;

    /* make new slot active, if it exists */
    if (new_slot)
    {
        window->details->active_pane->active_slots =
            g_list_remove (window->details->active_pane->active_slots, new_slot);
        window->details->active_pane->active_slots =
            g_list_prepend (window->details->active_pane->active_slots, new_slot);

        /* inform sidebar panels */
        baul_window_report_location_change (window);
        /* TODO decide whether "selection-changed" should be emitted */

        if (new_slot->content_view != NULL)
        {
            /* inform window */
            baul_window_slot_connect_content_view (new_slot, new_slot->content_view);
        }

        /* inform slot & view */
        g_signal_emit_by_name (new_slot, "active");
    }
}

void
baul_window_slot_close (BaulWindowSlot *slot)
{
    baul_window_pane_slot_close (slot->pane, slot);
}

static void
baul_window_realize (CtkWidget *widget)
{
    CTK_WIDGET_CLASS (baul_window_parent_class)->realize (widget);
    update_cursor (BAUL_WINDOW (widget));
}

static gboolean
baul_window_key_press_event (CtkWidget *widget,
                             CdkEventKey *event)
{
    /* Fix for https://github.com/cafe-desktop/baul/issues/1024 */
    if ((event->state & GDK_CONTROL_MASK) &&
        ((event->keyval == '.') || (event->keyval == ';')))
        return TRUE;

    BaulWindow *window;
    int i;

    window = BAUL_WINDOW (widget);

    for (i = 0; i < G_N_ELEMENTS (extra_window_keybindings); i++)
    {
        if (extra_window_keybindings[i].keyval == event->keyval)
        {
            const GList *action_groups;
            CtkAction *action;

            action = NULL;

            action_groups = ctk_ui_manager_get_action_groups (window->details->ui_manager);
            G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
            while (action_groups != NULL && action == NULL)
            {
                action = ctk_action_group_get_action (action_groups->data, extra_window_keybindings[i].action);
                action_groups = action_groups->next;
            }

            g_assert (action != NULL);
            if (ctk_action_is_sensitive (action))
            {
                ctk_action_activate (action);
                return TRUE;
            }
            G_GNUC_END_IGNORE_DEPRECATIONS;

            break;
        }
    }

    return CTK_WIDGET_CLASS (baul_window_parent_class)->key_press_event (widget, event);
}

/*
 * Main API
 */

static void
free_activate_view_data (gpointer data)
{
    ActivateViewData *activate_data;

    activate_data = data;

    g_free (activate_data->id);

    g_slice_free (ActivateViewData, activate_data);
}

static void
action_view_as_callback (CtkAction *action,
                         ActivateViewData *data)
{
    BaulWindow *window;

    window = data->window;

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    if (ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)))
    {
        BaulWindowSlot *slot;

        slot = window->details->active_pane->active_slot;
        baul_window_slot_set_content_view (slot,
                                           data->id);
    }
    G_GNUC_END_IGNORE_DEPRECATIONS;
}

static CtkRadioAction *
add_view_as_menu_item (BaulWindow *window,
                       const char *placeholder_path,
                       const char *identifier,
                       int index, /* extra_viewer is always index 0 */
                       guint merge_id)
{
    const BaulViewInfo *info;
    CtkRadioAction *action;
    char action_name[32];
    ActivateViewData *data;

    info = baul_view_factory_lookup (identifier);

    g_snprintf (action_name, sizeof (action_name), "view_as_%d", index);
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    action = ctk_radio_action_new (action_name,
                                   _(info->view_menu_label_with_mnemonic),
                                   _(info->display_location_label),
                                   NULL,
                                   0);
    G_GNUC_END_IGNORE_DEPRECATIONS;

    if (index >= 1 && index <= 9)
    {
        char accel[32];
        char accel_path[48];
        unsigned int accel_keyval;

        g_snprintf (accel, sizeof (accel), "%d", index);
        g_snprintf (accel_path, sizeof (accel_path), "<Baul-Window>/%s", action_name);

        accel_keyval = cdk_keyval_from_name (accel);
		g_assert (accel_keyval != GDK_KEY_VoidSymbol);

        ctk_accel_map_add_entry (accel_path, accel_keyval, GDK_CONTROL_MASK);
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
        ctk_action_set_accel_path (CTK_ACTION (action), accel_path);
        G_GNUC_END_IGNORE_DEPRECATIONS;
    }

    if (window->details->view_as_radio_action != NULL)
    {
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
        ctk_radio_action_set_group (action,
                                    ctk_radio_action_get_group (window->details->view_as_radio_action));
        G_GNUC_END_IGNORE_DEPRECATIONS;
    }
    else if (index != 0)
    {
        /* Index 0 is the extra view, and we don't want to use that here,
           as it can get deleted/changed later */
        window->details->view_as_radio_action = action;
    }

    data = g_slice_new (ActivateViewData);
    data->window = window;
    data->id = g_strdup (identifier);
    g_signal_connect_data (action, "activate",
                           G_CALLBACK (action_view_as_callback),
                           data, (GClosureNotify) free_activate_view_data, 0);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    ctk_action_group_add_action (window->details->view_as_action_group,
                                 CTK_ACTION (action));
    g_object_unref (action);
    G_GNUC_END_IGNORE_DEPRECATIONS;

    ctk_ui_manager_add_ui (window->details->ui_manager,
                           merge_id,
                           placeholder_path,
                           action_name,
                           action_name,
                           CTK_UI_MANAGER_MENUITEM,
                           FALSE);

    return action; /* return value owned by group */
}

/* Make a special first item in the "View as" option menu that represents
 * the current content view. This should only be called if the current
 * content view isn't already in the "View as" option menu.
 */
static void
update_extra_viewer_in_view_as_menus (BaulWindow *window,
                                      const char *id)
{
    gboolean had_extra_viewer;

    had_extra_viewer = window->details->extra_viewer != NULL;

    if (id == NULL)
    {
        if (!had_extra_viewer)
        {
            return;
        }
    }
    else
    {
        if (had_extra_viewer
                && strcmp (window->details->extra_viewer, id) == 0)
        {
            return;
        }
    }
    g_free (window->details->extra_viewer);
    window->details->extra_viewer = g_strdup (id);

    if (window->details->extra_viewer_merge_id != 0)
    {
        ctk_ui_manager_remove_ui (window->details->ui_manager,
                                  window->details->extra_viewer_merge_id);
        window->details->extra_viewer_merge_id = 0;
    }

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    if (window->details->extra_viewer_radio_action != NULL)
    {
        ctk_action_group_remove_action (window->details->view_as_action_group,
                                        CTK_ACTION (window->details->extra_viewer_radio_action));
        window->details->extra_viewer_radio_action = NULL;
    }
    G_GNUC_END_IGNORE_DEPRECATIONS;

    if (id != NULL)
    {
        window->details->extra_viewer_merge_id = ctk_ui_manager_new_merge_id (window->details->ui_manager);
        window->details->extra_viewer_radio_action =
            add_view_as_menu_item (window,
                                   BAUL_MENU_PATH_EXTRA_VIEWER_PLACEHOLDER,
                                   window->details->extra_viewer,
                                   0,
                                   window->details->extra_viewer_merge_id);
    }
}

static void
remove_extra_viewer_in_view_as_menus (BaulWindow *window)
{
    update_extra_viewer_in_view_as_menus (window, NULL);
}

static void
replace_extra_viewer_in_view_as_menus (BaulWindow *window)
{
    BaulWindowSlot *slot;
    const char *id;

    slot = window->details->active_pane->active_slot;

    id = baul_window_slot_get_content_view_id (slot);
    update_extra_viewer_in_view_as_menus (window, id);
}

/**
 * baul_window_synch_view_as_menus:
 *
 * Set the visible item of the "View as" option menu and
 * the marked "View as" item in the View menu to
 * match the current content view.
 *
 * @window: The BaulWindow whose "View as" option menu should be synched.
 */
static void
baul_window_synch_view_as_menus (BaulWindow *window)
{
    BaulWindowSlot *slot;
    int index;
    char action_name[32];
    GList *node;
    CtkAction *action;

    g_assert (BAUL_IS_WINDOW (window));

    slot = window->details->active_pane->active_slot;

    if (slot->content_view == NULL)
    {
        return;
    }
    for (node = window->details->short_list_viewers, index = 1;
            node != NULL;
            node = node->next, ++index)
    {
        if (baul_window_slot_content_view_matches_iid (slot, (char *)node->data))
        {
            break;
        }
    }
    if (node == NULL)
    {
        replace_extra_viewer_in_view_as_menus (window);
        index = 0;
    }
    else
    {
        remove_extra_viewer_in_view_as_menus (window);
    }

    g_snprintf (action_name, sizeof (action_name), "view_as_%d", index);
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    action = ctk_action_group_get_action (window->details->view_as_action_group,
                                          action_name);
    G_GNUC_END_IGNORE_DEPRECATIONS;

    /* Don't trigger the action callback when we're synchronizing */
    g_signal_handlers_block_matched (action,
                                     G_SIGNAL_MATCH_FUNC,
                                     0, 0,
                                     NULL,
                                     action_view_as_callback,
                                     NULL);
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action), TRUE);
    G_GNUC_END_IGNORE_DEPRECATIONS;
    g_signal_handlers_unblock_matched (action,
                                       G_SIGNAL_MATCH_FUNC,
                                       0, 0,
                                       NULL,
                                       action_view_as_callback,
                                       NULL);
}

static void
refresh_stored_viewers (BaulWindow *window)
{
    BaulWindowSlot *slot;
    GList *viewers;
    char *uri, *mimetype;

    slot = window->details->active_pane->active_slot;

    uri = baul_file_get_uri (slot->viewed_file);
    mimetype = baul_file_get_mime_type (slot->viewed_file);
    viewers = baul_view_factory_get_views_for_uri (uri,
              baul_file_get_file_type (slot->viewed_file),
              mimetype);
    g_free (uri);
    g_free (mimetype);

    free_stored_viewers (window);
    window->details->short_list_viewers = viewers;
}

static void
load_view_as_menu (BaulWindow *window)
{
    GList *node;
    guint merge_id;

    if (window->details->short_list_merge_id != 0)
    {
        ctk_ui_manager_remove_ui (window->details->ui_manager,
                                  window->details->short_list_merge_id);
        window->details->short_list_merge_id = 0;
    }
    if (window->details->extra_viewer_merge_id != 0)
    {
        ctk_ui_manager_remove_ui (window->details->ui_manager,
                                  window->details->extra_viewer_merge_id);
        window->details->extra_viewer_merge_id = 0;
        window->details->extra_viewer_radio_action = NULL;
    }
    if (window->details->view_as_action_group != NULL)
    {
        ctk_ui_manager_remove_action_group (window->details->ui_manager,
                                            window->details->view_as_action_group);
        window->details->view_as_action_group = NULL;
    }


    refresh_stored_viewers (window);

    merge_id = ctk_ui_manager_new_merge_id (window->details->ui_manager);
    window->details->short_list_merge_id = merge_id;
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    window->details->view_as_action_group = ctk_action_group_new ("ViewAsGroup");
    ctk_action_group_set_translation_domain (window->details->view_as_action_group, GETTEXT_PACKAGE);
    G_GNUC_END_IGNORE_DEPRECATIONS;
    window->details->view_as_radio_action = NULL;

    if (g_list_length (window->details->short_list_viewers) > 1) {
        int index;
        /* Add a menu item for each view in the preferred list for this location. */
        /* Start on 1, because extra_viewer gets index 0 */
        for (node = window->details->short_list_viewers, index = 1;
                node != NULL;
                node = node->next, ++index)
        {
            /* Menu item in View menu. */
            add_view_as_menu_item (window,
                    BAUL_MENU_PATH_SHORT_LIST_PLACEHOLDER,
                    node->data,
                    index,
                    merge_id);
        }
    }
    ctk_ui_manager_insert_action_group (window->details->ui_manager,
                                        window->details->view_as_action_group,
                                        -1);
    g_object_unref (window->details->view_as_action_group); /* owned by ui_manager */

    baul_window_synch_view_as_menus (window);

    g_signal_emit (window, signals[VIEW_AS_CHANGED], 0);

}

static void
load_view_as_menus_callback (BaulFile *file,
                             gpointer callback_data)
{
    BaulWindow *window;
    BaulWindowSlot *slot;

    slot = callback_data;
    window = BAUL_WINDOW (slot->pane->window);

    if (slot == window->details->active_pane->active_slot)
    {
        load_view_as_menu (window);
    }
}

static void
cancel_view_as_callback (BaulWindowSlot *slot)
{
    baul_file_cancel_call_when_ready (slot->viewed_file,
                                      load_view_as_menus_callback,
                                      slot);
}

void
baul_window_load_view_as_menus (BaulWindow *window)
{
    BaulWindowSlot *slot;
    BaulFileAttributes attributes;

    g_return_if_fail (BAUL_IS_WINDOW (window));

    attributes = baul_mime_actions_get_required_file_attributes ();

    slot = window->details->active_pane->active_slot;

    cancel_view_as_callback (slot);
    baul_file_call_when_ready (slot->viewed_file,
                               attributes,
                               load_view_as_menus_callback,
                               slot);
}

void
baul_window_display_error (BaulWindow *window, const char *error_msg)
{
    CtkWidget *dialog;

    g_return_if_fail (BAUL_IS_WINDOW (window));

    dialog = ctk_message_dialog_new (CTK_WINDOW (window), 0, CTK_MESSAGE_ERROR,
                                     CTK_BUTTONS_OK, error_msg, NULL);
    ctk_widget_show (dialog);
}

static char *
real_get_title (BaulWindow *window)
{
    g_assert (BAUL_IS_WINDOW (window));

    return baul_window_slot_get_title (window->details->active_pane->active_slot);
}

static void
real_sync_title (BaulWindow *window,
                 BaulWindowSlot *slot)
{
    if (slot == window->details->active_pane->active_slot)
    {
        char *copy;

        copy = g_strdup (slot->title);
        g_signal_emit_by_name (window, "title_changed",
                               slot->title);
        g_free (copy);
    }
}

void
baul_window_sync_title (BaulWindow *window,
                        BaulWindowSlot *slot)
{
    EEL_CALL_METHOD (BAUL_WINDOW_CLASS, window,
                     sync_title, (window, slot));
}

void
baul_window_sync_zoom_widgets (BaulWindow *window)
{
    BaulWindowSlot *slot;
    BaulView *view;
    CtkAction *action;
    gboolean supports_zooming;
    gboolean can_zoom, can_zoom_in, can_zoom_out;
    BaulZoomLevel zoom_level;

    slot = window->details->active_pane->active_slot;
    view = slot->content_view;

    if (view != NULL)
    {
        supports_zooming = baul_view_supports_zooming (view);
        zoom_level = baul_view_get_zoom_level (view);
        can_zoom = supports_zooming &&
                   zoom_level >= BAUL_ZOOM_LEVEL_SMALLEST &&
                   zoom_level <= BAUL_ZOOM_LEVEL_LARGEST;
        can_zoom_in = can_zoom && baul_view_can_zoom_in (view);
        can_zoom_out = can_zoom && baul_view_can_zoom_out (view);
    }
    else
    {
        zoom_level = BAUL_ZOOM_LEVEL_STANDARD;
        supports_zooming = FALSE;
        can_zoom = FALSE;
        can_zoom_in = FALSE;
        can_zoom_out = FALSE;
    }

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    action = ctk_action_group_get_action (window->details->main_action_group,
                                          BAUL_ACTION_ZOOM_IN);
    ctk_action_set_visible (action, supports_zooming);
    ctk_action_set_sensitive (action, can_zoom_in);

    action = ctk_action_group_get_action (window->details->main_action_group,
                                          BAUL_ACTION_ZOOM_OUT);
    ctk_action_set_visible (action, supports_zooming);
    ctk_action_set_sensitive (action, can_zoom_out);

    action = ctk_action_group_get_action (window->details->main_action_group,
                                          BAUL_ACTION_ZOOM_NORMAL);
    ctk_action_set_visible (action, supports_zooming);
    ctk_action_set_sensitive (action, can_zoom);
    G_GNUC_END_IGNORE_DEPRECATIONS;

    g_signal_emit (window, signals[ZOOM_CHANGED], 0,
                   zoom_level, supports_zooming, can_zoom,
                   can_zoom_in, can_zoom_out);
}

static void
zoom_level_changed_callback (BaulView *view,
                             BaulWindow *window)
{
    g_assert (BAUL_IS_WINDOW (window));

    /* This is called each time the component in
     * the active slot successfully completed
     * a zooming operation.
     */
    baul_window_sync_zoom_widgets (window);
}


/* These are called
 *   A) when switching the view within the active slot
 *   B) when switching the active slot
 *   C) when closing the active slot (disconnect)
*/
void
baul_window_connect_content_view (BaulWindow *window,
                                  BaulView *view)
{
    BaulWindowSlot *slot;

    g_assert (BAUL_IS_WINDOW (window));
    g_assert (BAUL_IS_VIEW (view));

    slot = baul_window_get_slot_for_view (window, view);
    g_assert (slot == baul_window_get_active_slot (window));

    g_signal_connect (view, "zoom-level-changed",
                      G_CALLBACK (zoom_level_changed_callback),
                      window);

    /* Update displayed view in menu. Only do this if we're not switching
     * locations though, because if we are switching locations we'll
     * install a whole new set of views in the menu later (the current
     * views in the menu are for the old location).
     */
    if (slot->pending_location == NULL)
    {
        baul_window_load_view_as_menus (window);
    }

    baul_view_grab_focus (view);
}

void
baul_window_disconnect_content_view (BaulWindow *window,
                                     BaulView *view)
{
    BaulWindowSlot *slot;

    g_assert (BAUL_IS_WINDOW (window));
    g_assert (BAUL_IS_VIEW (view));

    slot = baul_window_get_slot_for_view (window, view);
    g_assert (slot == baul_window_get_active_slot (window));

    g_signal_handlers_disconnect_by_func (view, G_CALLBACK (zoom_level_changed_callback), window);
}

/**
 * baul_window_show:
 * @widget:	CtkWidget
 *
 * Call parent and then show/hide window items
 * base on user prefs.
 */
static void
baul_window_show (CtkWidget *widget)
{
    BaulWindow *window;

    window = BAUL_WINDOW (widget);

    CTK_WIDGET_CLASS (baul_window_parent_class)->show (widget);

    baul_window_ui_update (window);
}

CtkUIManager *
baul_window_get_ui_manager (BaulWindow *window)
{
    g_return_val_if_fail (BAUL_IS_WINDOW (window), NULL);

    return window->details->ui_manager;
}

BaulWindowPane *
baul_window_get_next_pane (BaulWindow *window)
{
    BaulWindowPane *next_pane;
    GList *node;

    /* return NULL if there is only one pane */
    if (!window->details->panes || !window->details->panes->next)
    {
        return NULL;
    }

    /* get next pane in the (wrapped around) list */
    node = g_list_find (window->details->panes, window->details->active_pane);
    g_return_val_if_fail (node, NULL);
    if (node->next)
    {
        next_pane = node->next->data;
    }
    else
    {
        next_pane =  window->details->panes->data;
    }

    return next_pane;
}


void
baul_window_slot_set_viewed_file (BaulWindowSlot *slot,
                                  BaulFile *file)
{
    BaulFileAttributes attributes;

    if (slot->viewed_file == file)
    {
        return;
    }

    baul_file_ref (file);

    cancel_view_as_callback (slot);

    if (slot->viewed_file != NULL)
    {
        BaulWindow *window;

        window = slot->pane->window;

        if (BAUL_IS_SPATIAL_WINDOW (window))
        {
            baul_file_set_has_open_window (slot->viewed_file,
                                           FALSE);
        }
        baul_file_monitor_remove (slot->viewed_file,
                                  slot);
    }

    if (file != NULL)
    {
        attributes =
            BAUL_FILE_ATTRIBUTE_INFO |
            BAUL_FILE_ATTRIBUTE_LINK_INFO;
        baul_file_monitor_add (file, slot, attributes);
    }

    baul_file_unref (slot->viewed_file);
    slot->viewed_file = file;
}

void
baul_send_history_list_changed (void)
{
    g_signal_emit_by_name (baul_signaller_get_current (),
                           "history_list_changed");
}

static void
free_history_list (void)
{
    g_list_free_full (history_list, g_object_unref);
    history_list = NULL;
}

/* Remove the this URI from the history list.
 * Do not sent out a change notice.
 * We pass in a bookmark for convenience.
 */
static void
remove_from_history_list (BaulBookmark *bookmark)
{
    GList *node;

    /* Compare only the uris here. Comparing the names also is not
     * necessary and can cause problems due to the asynchronous
     * nature of when the title of the window is set.
     */
    node = g_list_find_custom (history_list,
                               bookmark,
                               baul_bookmark_compare_uris);

    /* Remove any older entry for this same item. There can be at most 1. */
    if (node != NULL)
    {
        history_list = g_list_remove_link (history_list, node);
        g_object_unref (node->data);
        g_list_free_1 (node);
    }
}

gboolean
baul_add_bookmark_to_history_list (BaulBookmark *bookmark)
{
    /* Note that the history is shared amongst all windows so
     * this is not a BaulNavigationWindow function. Perhaps it belongs
     * in its own file.
     */
    GList *l, *next;
    static gboolean free_history_list_is_set_up;

    g_assert (BAUL_IS_BOOKMARK (bookmark));

    if (!free_history_list_is_set_up)
    {
        eel_debug_call_at_shutdown (free_history_list);
        free_history_list_is_set_up = TRUE;
    }

    /*	g_warning ("Add to history list '%s' '%s'",
    		   baul_bookmark_get_name (bookmark),
    		   baul_bookmark_get_uri (bookmark)); */

    if (!history_list ||
            baul_bookmark_compare_uris (history_list->data, bookmark))
    {
        int i;

        g_object_ref (bookmark);
        remove_from_history_list (bookmark);
        history_list = g_list_prepend (history_list, bookmark);

        for (i = 0, l = history_list; l; l = next)
        {
            next = l->next;

            if (i++ >= MAX_HISTORY_ITEMS)
            {
                g_object_unref (l->data);
                history_list = g_list_delete_link (history_list, l);
            }
        }

        return TRUE;
    }

    return FALSE;
}

void
baul_remove_from_history_list_no_notify (GFile *location)
{
    BaulBookmark *bookmark;

    bookmark = baul_bookmark_new (location, "", FALSE, NULL);
    remove_from_history_list (bookmark);
    g_object_unref (bookmark);
}

gboolean
baul_add_to_history_list_no_notify (GFile *location,
                                    const char *name,
                                    gboolean has_custom_name,
                                    GIcon *icon)
{
    BaulBookmark *bookmark;
    gboolean ret;

    bookmark = baul_bookmark_new (location, name, has_custom_name, icon);
    ret = baul_add_bookmark_to_history_list (bookmark);
    g_object_unref (bookmark);

    return ret;
}

BaulWindowSlot *
baul_window_get_slot_for_view (BaulWindow *window,
                               BaulView *view)
{
    BaulWindowSlot *slot;
    GList *l, *walk;

    for (walk = window->details->panes; walk; walk = walk->next)
    {
        BaulWindowPane *pane = walk->data;

        for (l = pane->slots; l != NULL; l = l->next)
        {
            slot = l->data;
            if (slot->content_view == view ||
                    slot->new_content_view == view)
            {
                return slot;
            }
        }
    }

    return NULL;
}

void
baul_forget_history (void)
{
    BaulWindowSlot *slot;
    BaulNavigationWindowSlot *navigation_slot;
    GList *window_node, *l, *walk;
    BaulApplication *app;

    app = BAUL_APPLICATION (g_application_get_default ());
    /* Clear out each window's back & forward lists. Also, remove
     * each window's current location bookmark from history list
     * so it doesn't get clobbered.
     */
    for (window_node = ctk_application_get_windows (CTK_APPLICATION (app));
            window_node != NULL;
            window_node = window_node->next)
    {

        if (BAUL_IS_NAVIGATION_WINDOW (window_node->data))
        {
            BaulNavigationWindow *window;

            window = BAUL_NAVIGATION_WINDOW (window_node->data);

            for (walk = BAUL_WINDOW (window_node->data)->details->panes; walk; walk = walk->next)
            {
                BaulWindowPane *pane = walk->data;
                for (l = pane->slots; l != NULL; l = l->next)
                {
                    navigation_slot = l->data;

                    baul_navigation_window_slot_clear_back_list (navigation_slot);
                    baul_navigation_window_slot_clear_forward_list (navigation_slot);
                }
            }

            baul_navigation_window_allow_back (window, FALSE);
            baul_navigation_window_allow_forward (window, FALSE);
        }

        for (walk = BAUL_WINDOW (window_node->data)->details->panes; walk; walk = walk->next)
        {
            BaulWindowPane *pane = walk->data;
            for (l = pane->slots; l != NULL; l = l->next)
            {
                slot = l->data;
                history_list = g_list_remove (history_list,
                                              slot->current_location_bookmark);
            }
        }
    }

    /* Clobber history list. */
    free_history_list ();

    /* Re-add each window's current location to history list. */
    for (window_node = ctk_application_get_windows (CTK_APPLICATION (app));
            window_node != NULL;
            window_node = window_node->next)
    {
        BaulWindow *window;
        BaulWindowSlot *slot;
        GList *l;

        window = BAUL_WINDOW (window_node->data);
        for (walk = window->details->panes; walk; walk = walk->next)
        {
            BaulWindowPane *pane = walk->data;
            for (l = pane->slots; l != NULL; l = l->next)
            {
                slot = BAUL_WINDOW_SLOT (l->data);
                baul_window_slot_add_current_location_to_history_list (slot);
            }
        }
    }
}

GList *
baul_get_history_list (void)
{
    return history_list;
}

static GList *
baul_window_get_history (BaulWindow *window)
{
    return g_list_copy_deep (history_list, (GCopyFunc) g_object_ref, NULL);
}


static BaulWindowType
baul_window_get_window_type (BaulWindow *window)
{
    g_assert (BAUL_IS_WINDOW (window));

    return BAUL_WINDOW_GET_CLASS (window)->window_type;
}

static int
baul_window_get_selection_count (BaulWindow *window)
{
    BaulWindowSlot *slot;

    g_assert (BAUL_IS_WINDOW (window));

    slot = window->details->active_pane->active_slot;

    if (slot->content_view != NULL)
    {
        return baul_view_get_selection_count (slot->content_view);
    }

    return 0;
}

static GList *
baul_window_get_selection (BaulWindow *window)
{
    BaulWindowSlot *slot;

    g_assert (BAUL_IS_WINDOW (window));

    slot = window->details->active_pane->active_slot;

    if (slot->content_view != NULL)
    {
        return baul_view_get_selection (slot->content_view);
    }
    return NULL;
}

static BaulWindowShowHiddenFilesMode
baul_window_get_hidden_files_mode (BaulWindowInfo *window)
{
    return window->details->show_hidden_files_mode;
}

static void
baul_window_set_hidden_files_mode (BaulWindowInfo *window,
                                   BaulWindowShowHiddenFilesMode  mode)
{
    window->details->show_hidden_files_mode = mode;

    g_signal_emit_by_name (window, "hidden_files_mode_changed");
}

static BaulWindowShowBackupFilesMode
baul_window_get_backup_files_mode (BaulWindowInfo *window)
{
    return window->details->show_backup_files_mode;
}

static void
baul_window_set_backup_files_mode (BaulWindowInfo *window,
                                   BaulWindowShowBackupFilesMode  mode)
{
    window->details->show_backup_files_mode = mode;

    g_signal_emit_by_name (window, "backup_files_mode_changed");
}

static gboolean
baul_window_get_initiated_unmount (BaulWindowInfo *window)
{
    return window->details->initiated_unmount;
}

static void
baul_window_set_initiated_unmount (BaulWindowInfo *window,
                                   gboolean initiated_unmount)
{
    window->details->initiated_unmount = initiated_unmount;
}

static char *
baul_window_get_cached_title (BaulWindow *window)
{
    BaulWindowSlot *slot;

    g_assert (BAUL_IS_WINDOW (window));

    slot = window->details->active_pane->active_slot;

    return g_strdup (slot->title);
}

BaulWindowSlot *
baul_window_get_active_slot (BaulWindow *window)
{
    g_assert (BAUL_IS_WINDOW (window));

    return window->details->active_pane->active_slot;
}

BaulWindowSlot *
baul_window_get_extra_slot (BaulWindow *window)
{
    BaulWindowPane *extra_pane;
    GList *node;

    g_assert (BAUL_IS_WINDOW (window));


    /* return NULL if there is only one pane */
    if (window->details->panes == NULL ||
            window->details->panes->next == NULL)
    {
        return NULL;
    }

    /* get next pane in the (wrapped around) list */
    node = g_list_find (window->details->panes,
                        window->details->active_pane);
    g_return_val_if_fail (node, FALSE);

    if (node->next)
    {
        extra_pane = node->next->data;
    }
    else
    {
        extra_pane =  window->details->panes->data;
    }

    return extra_pane->active_slot;
}

GList *
baul_window_get_slots (BaulWindow *window)
{
    GList *walk,*list;

    g_assert (BAUL_IS_WINDOW (window));

    list = NULL;
    for (walk = window->details->panes; walk; walk = walk->next)
    {
        BaulWindowPane *pane = walk->data;
        list  = g_list_concat (list, g_list_copy(pane->slots));
    }
    return list;
}

static void
baul_window_info_iface_init (BaulWindowInfoIface *iface)
{
    iface->report_load_underway = baul_window_report_load_underway;
    iface->report_load_complete = baul_window_report_load_complete;
    iface->report_selection_changed = baul_window_report_selection_changed;
    iface->report_view_failed = baul_window_report_view_failed;
    iface->view_visible = baul_window_view_visible;
    iface->close_window = baul_window_close;
    iface->push_status = baul_window_push_status;
    iface->get_window_type = baul_window_get_window_type;
    iface->get_title = baul_window_get_cached_title;
    iface->get_history = baul_window_get_history;
    iface->get_current_location = baul_window_get_location_uri;
    iface->get_ui_manager = baul_window_get_ui_manager;
    iface->get_selection_count = baul_window_get_selection_count;
    iface->get_selection = baul_window_get_selection;
    iface->get_hidden_files_mode = baul_window_get_hidden_files_mode;
    iface->set_hidden_files_mode = baul_window_set_hidden_files_mode;

    iface->get_backup_files_mode = baul_window_get_backup_files_mode;
    iface->set_backup_files_mode = baul_window_set_backup_files_mode;

    iface->get_active_slot = baul_window_get_active_slot;
    iface->get_extra_slot = baul_window_get_extra_slot;
    iface->get_initiated_unmount = baul_window_get_initiated_unmount;
    iface->set_initiated_unmount = baul_window_set_initiated_unmount;
}

static void
baul_window_class_init (BaulWindowClass *class)
{
    CtkBindingSet *binding_set;

    G_OBJECT_CLASS (class)->constructor = baul_window_constructor;
    G_OBJECT_CLASS (class)->constructed = baul_window_constructed;
    G_OBJECT_CLASS (class)->get_property = baul_window_get_property;
    G_OBJECT_CLASS (class)->set_property = baul_window_set_property;
    G_OBJECT_CLASS (class)->finalize = baul_window_finalize;

    CTK_WIDGET_CLASS (class)->destroy = baul_window_destroy;

    CTK_WIDGET_CLASS (class)->show = baul_window_show;

    CTK_WIDGET_CLASS (class)->realize = baul_window_realize;
    CTK_WIDGET_CLASS (class)->key_press_event = baul_window_key_press_event;
    class->get_title = real_get_title;
    class->sync_title = real_sync_title;
    class->set_allow_up = real_set_allow_up;
    class->close_slot = real_close_slot;

    g_object_class_install_property (G_OBJECT_CLASS (class),
                                     ARG_APP,
                                     g_param_spec_object ("app",
                                             "Application",
                                             "The BaulApplication associated with this window.",
                                             BAUL_TYPE_APPLICATION,
                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    signals[GO_UP] =
        g_signal_new ("go_up",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (BaulWindowClass, go_up),
                      g_signal_accumulator_true_handled, NULL,
                      baul_src_marshal_BOOLEAN__BOOLEAN,
                      G_TYPE_BOOLEAN, 1, G_TYPE_BOOLEAN);
    signals[RELOAD] =
        g_signal_new ("reload",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (BaulWindowClass, reload),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
    signals[PROMPT_FOR_LOCATION] =
        g_signal_new ("prompt-for-location",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (BaulWindowClass, prompt_for_location),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__STRING,
                      G_TYPE_NONE, 1, G_TYPE_STRING);
    signals[ZOOM_CHANGED] =
        g_signal_new ("zoom-changed",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      baul_src_marshal_VOID__INT_BOOLEAN_BOOLEAN_BOOLEAN_BOOLEAN,
                      G_TYPE_NONE, 5,
                      G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
                      G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
    signals[VIEW_AS_CHANGED] =
        g_signal_new ("view-as-changed",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    binding_set = ctk_binding_set_by_class (class);
	ctk_binding_entry_add_signal (binding_set, GDK_KEY_BackSpace, 0,
                                  "go_up", 1,
                                  G_TYPE_BOOLEAN, FALSE);
	ctk_binding_entry_add_signal (binding_set, GDK_KEY_F5, 0,
                                  "reload", 0);
	ctk_binding_entry_add_signal (binding_set, GDK_KEY_slash, 0,
                                  "prompt-for-location", 1,
                                  G_TYPE_STRING, "/");

    class->reload = baul_window_reload;
    class->go_up = baul_window_go_up_signal;
}
