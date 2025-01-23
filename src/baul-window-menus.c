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
#include <string.h>

#include <ctk/ctk.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#include <eel/eel-ctk-extensions.h>

#include <libbaul-extension/baul-menu-provider.h>
#include <libbaul-private/baul-extensions.h>
#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-icon-names.h>
#include <libbaul-private/baul-ui-utilities.h>
#include <libbaul-private/baul-module.h>
#include <libbaul-private/baul-search-directory.h>
#include <libbaul-private/baul-search-engine.h>
#include <libbaul-private/baul-signaller.h>
#include <libbaul-private/baul-trash-monitor.h>

#include "baul-actions.h"
#include "baul-application.h"
#include "baul-connect-server-dialog.h"
#include "baul-file-management-properties.h"
#include "baul-property-browser.h"
#include "baul-window-manage-views.h"
#include "baul-window-bookmarks.h"
#include "baul-window-private.h"
#include "baul-desktop-window.h"
#include "baul-search-bar.h"

#define MENU_PATH_EXTENSION_ACTIONS                     "/MenuBar/File/Extension Actions"
#define POPUP_PATH_EXTENSION_ACTIONS                     "/background/Before Zoom Items/Extension Actions"

#define NETWORK_URI          "network:"
#define COMPUTER_URI         "computer:"

/* Struct that stores all the info necessary to activate a bookmark. */
typedef struct
{
    BaulBookmark *bookmark;
    BaulWindow *window;
    guint changed_handler_id;
    BaulBookmarkFailedCallback failed_callback;
} BookmarkHolder;

static BookmarkHolder *
bookmark_holder_new (BaulBookmark *bookmark,
                     BaulWindow *window,
                     GCallback refresh_callback,
                     BaulBookmarkFailedCallback failed_callback)
{
    BookmarkHolder *new_bookmark_holder;

    new_bookmark_holder = g_new (BookmarkHolder, 1);
    new_bookmark_holder->window = window;
    new_bookmark_holder->bookmark = bookmark;
    new_bookmark_holder->failed_callback = failed_callback;
    /* Ref the bookmark because it might be unreffed away while
     * we're holding onto it (not an issue for window).
     */
    g_object_ref (bookmark);
    new_bookmark_holder->changed_handler_id =
        g_signal_connect_object (bookmark, "appearance_changed",
                                 refresh_callback,
                                 window, G_CONNECT_SWAPPED);

    return new_bookmark_holder;
}

static void
bookmark_holder_free (BookmarkHolder *bookmark_holder)
{
    if (g_signal_handler_is_connected(bookmark_holder->bookmark,
                                      bookmark_holder->changed_handler_id)){
    g_signal_handler_disconnect (bookmark_holder->bookmark,
                                      bookmark_holder->changed_handler_id);
    }
    g_object_unref (bookmark_holder->bookmark);
    g_free (bookmark_holder);
}

static void
bookmark_holder_free_cover (gpointer  callback_data,
			    GClosure *closure G_GNUC_UNUSED)
{
    bookmark_holder_free (callback_data);
}

static gboolean
should_open_in_new_tab (void)
{
    /* FIXME this is duplicated */
    CdkEvent *event;

    event = ctk_get_current_event ();

    if (event == NULL)
    {
        return FALSE;
    }

    if (event->type == CDK_BUTTON_PRESS || event->type == CDK_BUTTON_RELEASE)
    {
        return event->button.button == 2;
    }

    cdk_event_free (event);

    return FALSE;
}

static void
activate_bookmark_in_menu_item (CtkAction *action G_GNUC_UNUSED,
				gpointer   user_data)
{
    BookmarkHolder *holder;

    holder = (BookmarkHolder *)user_data;

    if (baul_bookmark_uri_known_not_to_exist (holder->bookmark))
    {
        holder->failed_callback (holder->window, holder->bookmark);
    }
    else
    {
        BaulWindowSlot *slot;
        GFile *location;

        location = baul_bookmark_get_location (holder->bookmark);
        slot = baul_window_get_active_slot (holder->window);
        baul_window_slot_go_to (slot,
                                location,
                                should_open_in_new_tab ());
        g_object_unref (location);
    }
}

void
baul_menus_append_bookmark_to_menu (BaulWindow *window,
                                    BaulBookmark *bookmark,
                                    const char *parent_path,
                                    const char *parent_id,
                                    guint index_in_parent,
                                    CtkActionGroup *action_group,
                                    guint merge_id,
                                    GCallback refresh_callback,
                                    BaulBookmarkFailedCallback failed_callback)
{
    BookmarkHolder *bookmark_holder;
    char action_name[128];
    char *name;
    char *path;
    cairo_surface_t *surface;
    CtkAction *action;
    CtkWidget *menuitem;

    g_assert (BAUL_IS_WINDOW (window));
    g_assert (BAUL_IS_BOOKMARK (bookmark));

    bookmark_holder = bookmark_holder_new (bookmark, window, refresh_callback, failed_callback);
    name = baul_bookmark_get_name (bookmark);

    /* Create menu item with surface */
    surface = baul_bookmark_get_surface (bookmark, CTK_ICON_SIZE_MENU);

    g_snprintf (action_name, sizeof (action_name), "%s%d", parent_id, index_in_parent);

    action = ctk_action_new (action_name,
                             name,
                             _("Go to the location specified by this bookmark"),
                             NULL);

    g_object_set_data_full (G_OBJECT (action), "menu-icon",
                            cairo_surface_reference (surface),
                            (GDestroyNotify)cairo_surface_destroy);

    g_signal_connect_data (action, "activate",
                           G_CALLBACK (activate_bookmark_in_menu_item),
                           bookmark_holder,
                           bookmark_holder_free_cover, 0);

    ctk_action_group_add_action (action_group,
                                 CTK_ACTION (action));

    g_object_unref (action);

    ctk_ui_manager_add_ui (window->details->ui_manager,
                           merge_id,
                           parent_path,
                           action_name,
                           action_name,
                           CTK_UI_MANAGER_MENUITEM,
                           FALSE);

    path = g_strdup_printf ("%s/%s", parent_path, action_name);
    menuitem = ctk_ui_manager_get_widget (window->details->ui_manager,
                                          path);
    ctk_image_menu_item_set_always_show_image (CTK_IMAGE_MENU_ITEM (menuitem),
            TRUE);

    cairo_surface_destroy (surface);
    g_free (path);
    g_free (name);
}

static void
action_close_window_slot_callback (CtkAction *action G_GNUC_UNUSED,
				   gpointer   user_data)
{
    BaulWindow *window;
    BaulWindowSlot *slot;

    window = BAUL_WINDOW (user_data);
    slot = baul_window_get_active_slot (window);

    baul_window_slot_close (slot);
}

static void
action_connect_to_server_callback (CtkAction *action G_GNUC_UNUSED,
				   gpointer   user_data)
{
    BaulWindow *window = BAUL_WINDOW (user_data);
    CtkWidget *dialog;

    dialog = baul_connect_server_dialog_new (window);

    ctk_widget_show (dialog);
}

static void
action_stop_callback (CtkAction *action G_GNUC_UNUSED,
		      gpointer   user_data)
{
    BaulWindow *window;
    BaulWindowSlot *slot;

    window = BAUL_WINDOW (user_data);
    slot = baul_window_get_active_slot (window);

    baul_window_slot_stop_loading (slot);
}

static void
action_home_callback (CtkAction *action G_GNUC_UNUSED,
		      gpointer   user_data)
{
    BaulWindow *window;
    BaulWindowSlot *slot;

    window = BAUL_WINDOW (user_data);
    slot = baul_window_get_active_slot (window);

    baul_window_slot_go_home (slot,
                              should_open_in_new_tab ());
}

static void
action_go_to_computer_callback (CtkAction *action G_GNUC_UNUSED,
				gpointer   user_data)
{
    BaulWindow *window;
    BaulWindowSlot *slot;
    GFile *computer;

    window = BAUL_WINDOW (user_data);
    slot = baul_window_get_active_slot (window);

    computer = g_file_new_for_uri (COMPUTER_URI);
    baul_window_slot_go_to (slot,
                            computer,
                            should_open_in_new_tab ());
    g_object_unref (computer);
}

static void
action_go_to_network_callback (CtkAction *action G_GNUC_UNUSED,
			       gpointer   user_data)
{
    BaulWindow *window;
    BaulWindowSlot *slot;
    GFile *network;

    window = BAUL_WINDOW (user_data);
    slot = baul_window_get_active_slot (window);

    network = g_file_new_for_uri (NETWORK_URI);
    baul_window_slot_go_to (slot,
                            network,
                            should_open_in_new_tab ());
    g_object_unref (network);
}

static void
action_go_to_templates_callback (CtkAction *action G_GNUC_UNUSED,
				 gpointer   user_data)
{
    BaulWindow *window;
    BaulWindowSlot *slot;
    char *path;
    GFile *location;

    window = BAUL_WINDOW (user_data);
    slot = baul_window_get_active_slot (window);

    path = baul_get_templates_directory ();
    location = g_file_new_for_path (path);
    g_free (path);
    baul_window_slot_go_to (slot,
                            location,
                            should_open_in_new_tab ());
    g_object_unref (location);
}

static void
action_go_to_trash_callback (CtkAction *action G_GNUC_UNUSED,
			     gpointer   user_data)
{
    BaulWindow *window;
    BaulWindowSlot *slot;
    GFile *trash;

    window = BAUL_WINDOW (user_data);
    slot = baul_window_get_active_slot (window);

    trash = g_file_new_for_uri ("trash:///");
    baul_window_slot_go_to (slot,
                            trash,
                            should_open_in_new_tab ());
    g_object_unref (trash);
}

static void
action_reload_callback (CtkAction *action G_GNUC_UNUSED,
			gpointer   user_data)
{
    baul_window_reload (BAUL_WINDOW (user_data));
}

static void
action_zoom_in_callback (CtkAction *action G_GNUC_UNUSED,
			 gpointer   user_data)
{
    baul_window_zoom_in (BAUL_WINDOW (user_data));
}

static void
action_zoom_out_callback (CtkAction *action G_GNUC_UNUSED,
			  gpointer   user_data)
{
    baul_window_zoom_out (BAUL_WINDOW (user_data));
}

static void
action_zoom_normal_callback (CtkAction *action G_GNUC_UNUSED,
			     gpointer   user_data)
{
    baul_window_zoom_to_default (BAUL_WINDOW (user_data));
}

static void
action_show_hidden_files_callback (CtkAction *action,
                                   gpointer callback_data)
{
    BaulWindow *window;
    BaulWindowShowHiddenFilesMode mode;

    window = BAUL_WINDOW (callback_data);

    if (ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)))
    {
        mode = BAUL_WINDOW_SHOW_HIDDEN_FILES_ENABLE;
    }
    else
    {
        mode = BAUL_WINDOW_SHOW_HIDDEN_FILES_DISABLE;
    }

    baul_window_info_set_hidden_files_mode (window, mode);
}

static void
action_show_backup_files_callback (CtkAction *action,
                                   gpointer callback_data)
{
    BaulWindow *window;
    BaulWindowShowBackupFilesMode mode;

    window = BAUL_WINDOW (callback_data);

    if (ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)))
    {
        mode = BAUL_WINDOW_SHOW_BACKUP_FILES_ENABLE;
    }
    else
    {
        mode = BAUL_WINDOW_SHOW_BACKUP_FILES_DISABLE;
    }

    baul_window_info_set_backup_files_mode (window, mode);
}

static void
show_hidden_files_preference_callback (gpointer callback_data)
{
    BaulWindow *window;

    window = BAUL_WINDOW (callback_data);

    if (window->details->show_hidden_files_mode == BAUL_WINDOW_SHOW_HIDDEN_FILES_DEFAULT)
    {
        CtkAction *action;

        action = ctk_action_group_get_action (window->details->main_action_group, BAUL_ACTION_SHOW_HIDDEN_FILES);
        g_assert (CTK_IS_ACTION (action));

        /* update button */
        g_signal_handlers_block_by_func (action, action_show_hidden_files_callback, window);
        ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
                                      g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_SHOW_HIDDEN_FILES));
        g_signal_handlers_unblock_by_func (action, action_show_hidden_files_callback, window);

        /* inform views */
        baul_window_info_set_hidden_files_mode (window, BAUL_WINDOW_SHOW_HIDDEN_FILES_DEFAULT);

    }
}

static void
show_backup_files_preference_callback (gpointer callback_data)
{
    BaulWindow *window;

    window = BAUL_WINDOW (callback_data);

    if (window->details->show_backup_files_mode == BAUL_WINDOW_SHOW_BACKUP_FILES_DEFAULT)
    {
        CtkAction *action;

        action = ctk_action_group_get_action (window->details->main_action_group, BAUL_ACTION_SHOW_BACKUP_FILES);
        g_assert (CTK_IS_ACTION (action));

        /* update button */
        g_signal_handlers_block_by_func (action, action_show_backup_files_callback, window);
        ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
                                      g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_SHOW_BACKUP_FILES));
        g_signal_handlers_unblock_by_func (action, action_show_backup_files_callback, window);

        /* inform views */
        baul_window_info_set_backup_files_mode (window, BAUL_WINDOW_SHOW_BACKUP_FILES_DEFAULT);
    }
}

static void
preferences_respond_callback (CtkDialog *dialog,
                              gint response_id)
{
    if (response_id == CTK_RESPONSE_CLOSE)
    {
        ctk_widget_destroy (CTK_WIDGET (dialog));
    }
}

static void
action_preferences_callback (CtkAction *action G_GNUC_UNUSED,
			     gpointer   user_data)
{
    CtkWindow *window;

    window = CTK_WINDOW (user_data);

    baul_file_management_properties_dialog_show (G_CALLBACK (preferences_respond_callback), window);
}

static void
action_backgrounds_and_emblems_callback (CtkAction *action G_GNUC_UNUSED,
					 gpointer   user_data)
{
    CtkWindow *window;

    window = CTK_WINDOW (user_data);

    baul_property_browser_show (ctk_window_get_screen (window));
}

#define ABOUT_GROUP "About"
#define EMAILIFY(string) (g_strdelimit ((string), "%", '@'))

static void
action_about_baul_callback (CtkAction *action G_GNUC_UNUSED,
			    gpointer   user_data)
{
    const gchar *license[] =
    {
        N_("Baul is free software; you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation; either version 2 of the License, or "
        "(at your option) any later version."),
        N_("Baul is distributed in the hope that it will be useful, "
        "but WITHOUT ANY WARRANTY; without even the implied warranty of "
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
        "GNU General Public License for more details."),
        N_("You should have received a copy of the GNU General Public License "
        "along with Baul; if not, write to the Free Software Foundation, Inc., "
        "51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA")
    };
    gchar *license_trans;
    GKeyFile *key_file;
    GError *error = NULL;
    char **authors, **documenters;
    gsize n_authors = 0, n_documenters = 0 , i;

    key_file = g_key_file_new ();
    if (!g_key_file_load_from_file (key_file, BAUL_DATADIR G_DIR_SEPARATOR_S "baul.about", 0, &error))
    {
        g_warning ("Couldn't load about data: %s\n", error->message);
        g_error_free (error);
        g_key_file_free (key_file);
        return;
    }

    authors = g_key_file_get_string_list (key_file, ABOUT_GROUP, "Authors", &n_authors, NULL);
    documenters = g_key_file_get_string_list (key_file, ABOUT_GROUP, "Documenters", &n_documenters, NULL);
    g_key_file_free (key_file);

    for (i = 0; i < n_authors; ++i)
        authors[i] = EMAILIFY (authors[i]);
    for (i = 0; i < n_documenters; ++i)
        documenters[i] = EMAILIFY (documenters[i]);

    license_trans = g_strjoin ("\n\n", _(license[0]), _(license[1]), _(license[2]), NULL);

    ctk_show_about_dialog (CTK_WINDOW (user_data),
                           "program-name", _("Baul"),
                           "title", _("About Baul"),
                           "version", VERSION,
                           "comments", _("Baul lets you organize "
                                         "files and folders, both on "
                                         "your computer and online."),
                           "copyright", _("Copyright \xC2\xA9 1999-2009 The Nautilus authors\n"
                                          "Copyright \xC2\xA9 2011-2020 The Caja authors\n"
                                          "Copyright \xC2\xA9 2022-2024 Pablo Barciela"),
                           "license", license_trans,
                           "wrap-license", TRUE,
                           "authors", authors,
                           "documenters", documenters,
                           "translator-credits", _("translator-credits"),
                           "logo-icon-name", "system-file-manager",
                           "website", "https://cafe-desktop.org",
                           "website-label", _("CAFE Web Site"),
                           NULL);

    g_strfreev (authors);
    g_strfreev (documenters);
    g_free (license_trans);

}

static void
action_up_callback (CtkAction *action G_GNUC_UNUSED,
		    gpointer   user_data)
{
    baul_window_go_up (BAUL_WINDOW (user_data), FALSE, should_open_in_new_tab ());
}

static void
action_baul_manual_callback (CtkAction *action G_GNUC_UNUSED,
			     gpointer   user_data)
{
    BaulWindow *window;
    GError *error;

    error = NULL;
    window = BAUL_WINDOW (user_data);

    ctk_show_uri_on_window (CTK_WINDOW (window),
                            BAUL_IS_DESKTOP_WINDOW (window)
                               ? "help:cafe-user-guide"
                               : "help:cafe-user-guide/gosbaul-1",
                            ctk_get_current_event_time (), &error);

    if (error)
    {
        CtkWidget *dialog;

        dialog = ctk_message_dialog_new (CTK_WINDOW (window),
                                         CTK_DIALOG_MODAL,
                                         CTK_MESSAGE_ERROR,
                                         CTK_BUTTONS_OK,
                                         _("There was an error displaying help: \n%s"),
                                         error->message);
        g_signal_connect (G_OBJECT (dialog), "response",
                          G_CALLBACK (ctk_widget_destroy),
                          NULL);

        ctk_window_set_resizable (CTK_WINDOW (dialog), FALSE);
        ctk_widget_show (dialog);
        g_error_free (error);
    }
}

static void
menu_item_select_cb (CtkMenuItem *proxy,
                     BaulWindow *window)
{
    CtkAction *action;
    char *message;

    action = ctk_activatable_get_related_action (CTK_ACTIVATABLE (proxy));
    g_return_if_fail (action != NULL);

    g_object_get (G_OBJECT (action), "tooltip", &message, NULL);
    if (message)
    {
        ctk_statusbar_push (CTK_STATUSBAR (window->details->statusbar),
                            window->details->help_message_cid, message);
        g_free (message);
    }
}

static void
menu_item_deselect_cb (CtkMenuItem *proxy G_GNUC_UNUSED,
		       BaulWindow  *window)
{
    ctk_statusbar_pop (CTK_STATUSBAR (window->details->statusbar),
                       window->details->help_message_cid);
}

static CtkWidget *
get_event_widget (CtkWidget *proxy)
{
    CtkWidget *widget;

    /**
     * Finding the interesting widget requires internal knowledge of
     * the widgets in question. This can't be helped, but by keeping
     * the sneaky code in one place, it can easily be updated.
     */
    if (CTK_IS_MENU_ITEM (proxy))
    {
        /* Menu items already forward middle clicks */
        widget = NULL;
    }
    else if (CTK_IS_MENU_TOOL_BUTTON (proxy))
    {
        widget = eel_ctk_menu_tool_button_get_button (CTK_MENU_TOOL_BUTTON (proxy));
    }
    else if (CTK_IS_TOOL_BUTTON (proxy))
    {
        /* The tool button's button is the direct child */
        widget = ctk_bin_get_child (CTK_BIN (proxy));
    }
    else if (CTK_IS_BUTTON (proxy))
    {
        widget = proxy;
    }
    else
    {
        /* Don't touch anything we don't know about */
        widget = NULL;
    }

    return widget;
}

static gboolean
proxy_button_press_event_cb (CtkButton      *button,
			     CdkEventButton *event,
			     gpointer        user_data G_GNUC_UNUSED)
{
    if (event->button == 2)
    {
        g_signal_emit_by_name (button, "pressed", 0);
    }

    return FALSE;
}

static gboolean
proxy_button_release_event_cb (CtkButton      *button,
			       CdkEventButton *event,
			       gpointer        user_data G_GNUC_UNUSED)
{
    if (event->button == 2)
    {
        g_signal_emit_by_name (button, "released", 0);
    }

    return FALSE;
}

static void
disconnect_proxy_cb (CtkUIManager *manager G_GNUC_UNUSED,
		     CtkAction    *action,
		     CtkWidget    *proxy,
		     BaulWindow   *window)
{
    CtkWidget *widget;

    if (CTK_IS_MENU_ITEM (proxy))
    {
        g_signal_handlers_disconnect_by_func
        (proxy, G_CALLBACK (menu_item_select_cb), window);
        g_signal_handlers_disconnect_by_func
        (proxy, G_CALLBACK (menu_item_deselect_cb), window);
    }

    widget = get_event_widget (proxy);
    if (widget)
    {
        g_signal_handlers_disconnect_by_func (widget,
                                              G_CALLBACK (proxy_button_press_event_cb),
                                              action);
        g_signal_handlers_disconnect_by_func (widget,
                                              G_CALLBACK (proxy_button_release_event_cb),
                                              action);
    }

}

static void
connect_proxy_cb (CtkUIManager *manager G_GNUC_UNUSED,
		  CtkAction    *action,
		  CtkWidget    *proxy,
		  BaulWindow   *window)
{
    cairo_surface_t *icon;
    CtkWidget *widget;

    if (CTK_IS_MENU_ITEM (proxy))
    {
        g_signal_connect (proxy, "select",
                          G_CALLBACK (menu_item_select_cb), window);
        g_signal_connect (proxy, "deselect",
                          G_CALLBACK (menu_item_deselect_cb), window);


        /* This is a way to easily get surfaces into the menu items */
        icon = g_object_get_data (G_OBJECT (action), "menu-icon");
        if (icon != NULL)
        {
            ctk_image_menu_item_set_image (CTK_IMAGE_MENU_ITEM (proxy),
                                           ctk_image_new_from_surface (icon));
        }
    }
    if (CTK_IS_TOOL_BUTTON (proxy))
    {
        icon = g_object_get_data (G_OBJECT (action), "toolbar-icon");
        if (icon != NULL)
        {
            widget = ctk_image_new_from_surface (icon);
            ctk_widget_show (widget);
            ctk_tool_button_set_icon_widget (CTK_TOOL_BUTTON (proxy),
                                             widget);
        }
    }

    widget = get_event_widget (proxy);
    if (widget)
    {
        g_signal_connect (widget, "button-press-event",
                          G_CALLBACK (proxy_button_press_event_cb),
                          action);
        g_signal_connect (widget, "button-release-event",
                          G_CALLBACK (proxy_button_release_event_cb),
                          action);
    }
}

static void
trash_state_changed_cb (BaulTrashMonitor *monitor G_GNUC_UNUSED,
			gboolean          state G_GNUC_UNUSED,
			BaulWindow       *window)
{
    CtkActionGroup *action_group;
    CtkAction *action;
    GIcon *gicon;

    action_group = window->details->main_action_group;
    action = ctk_action_group_get_action (action_group, "Go to Trash");

    gicon = baul_trash_monitor_get_icon ();

    if (gicon)
    {
        g_object_set (action, "gicon", gicon, NULL);
        g_object_unref (gicon);
    }
}

static void
baul_window_initialize_trash_icon_monitor (BaulWindow *window)
{
    BaulTrashMonitor *monitor;

    monitor = baul_trash_monitor_get ();

    trash_state_changed_cb (monitor, TRUE, window);

    g_signal_connect (monitor, "trash_state_changed",
                      G_CALLBACK (trash_state_changed_cb), window);
}

static const CtkActionEntry main_entries[] =
{
    /* name, icon name, label */ { "File", NULL, N_("_File") },
    /* name, icon name, label */ { "Edit", NULL, N_("_Edit") },
    /* name, icon name, label */ { "View", NULL, N_("_View") },
    /* name, icon name, label */ { "Help", NULL, N_("_Help") },
    /* name, icon name */        { "Close", "window-close",
        /* label, accelerator */       N_("_Close"), "<control>W",
        /* tooltip */                  N_("Close this folder"),
        G_CALLBACK (action_close_window_slot_callback)
    },
    {
        "Backgrounds and Emblems", NULL,
        N_("_Backgrounds and Emblems..."),
        NULL, N_("Display patterns, colors, and emblems that can be used to customize appearance"),
        G_CALLBACK (action_backgrounds_and_emblems_callback)
    },
    {
        "Preferences", "preferences-desktop",
        N_("Prefere_nces"),
        NULL, N_("Edit Baul preferences"),
        G_CALLBACK (action_preferences_callback)
    },
    /* name, icon name, label */ { "Up", "go-up", N_("Open _Parent"),
        "<alt>Up", N_("Open the parent folder"),
        G_CALLBACK (action_up_callback)
    },
    /* name, icon name, label */ { "UpAccel", NULL, "UpAccel",
        "", NULL,
        G_CALLBACK (action_up_callback)
    },
    /* name, icon name */        { "Stop", "process-stop",
        /* label, accelerator */       N_("_Stop"), NULL,
        /* tooltip */                  N_("Stop loading the current location"),
        G_CALLBACK (action_stop_callback)
    },
    /* name, icon name */        { "Reload", "view-refresh",
        /* label, accelerator */       N_("_Reload"), "<control>R",
        /* tooltip */                  N_("Reload the current location"),
        G_CALLBACK (action_reload_callback)
    },
    /* name, icon name */        { "Baul Manual", "help-browser",
        /* label, accelerator */       N_("_Contents"), "F1",
        /* tooltip */                  N_("Display Baul help"),
        G_CALLBACK (action_baul_manual_callback)
    },
    /* name, icon name */        { "About Baul", "help-about",
        /* label, accelerator */       N_("_About"), NULL,
        /* tooltip */                  N_("Display credits for the creators of Baul"),
        G_CALLBACK (action_about_baul_callback)
    },
    /* name, icon name */        { "Zoom In", "zoom-in",
        /* label, accelerator */       N_("Zoom _In"), "<control>plus",
        /* tooltip */                  N_("Increase the view size"),
        G_CALLBACK (action_zoom_in_callback)
    },
    /* name, icon name */        { "ZoomInAccel", NULL,
        /* label, accelerator */       "ZoomInAccel", "<control>equal",
        /* tooltip */                  NULL,
        G_CALLBACK (action_zoom_in_callback)
    },
    /* name, icon name */        { "ZoomInAccel2", NULL,
        /* label, accelerator */       "ZoomInAccel2", "<control>KP_Add",
        /* tooltip */                  NULL,
        G_CALLBACK (action_zoom_in_callback)
    },
    /* name, icon name */        { "Zoom Out", "zoom-out",
        /* label, accelerator */       N_("Zoom _Out"), "<control>minus",
        /* tooltip */                  N_("Decrease the view size"),
        G_CALLBACK (action_zoom_out_callback)
    },
    /* name, icon name */        { "ZoomOutAccel", NULL,
        /* label, accelerator */       "ZoomOutAccel", "<control>KP_Subtract",
        /* tooltip */                  NULL,
        G_CALLBACK (action_zoom_out_callback)
    },
    /* name, icon name */        { "Zoom Normal", "zoom-original",
        /* label, accelerator */       N_("Normal Si_ze"), "<control>0",
        /* tooltip */                  N_("Use the normal view size"),
        G_CALLBACK (action_zoom_normal_callback)
    },
    /* name, icon name */        { "Connect to Server", NULL,
        /* label, accelerator */       N_("Connect to _Server..."), NULL,
        /* tooltip */                  N_("Connect to a remote computer or shared disk"),
        G_CALLBACK (action_connect_to_server_callback)
    },
    /* name, icon name */        { "Home", BAUL_ICON_HOME,
        /* label, accelerator */       N_("_Home Folder"), "<alt>Home",
        /* tooltip */                  N_("Open your personal folder"),
        G_CALLBACK (action_home_callback)
    },
    /* name, icon name */        { "Go to Computer", BAUL_ICON_COMPUTER,
        /* label, accelerator */       N_("_Computer"), NULL,
        /* tooltip */                  N_("Browse all local and remote disks and folders accessible from this computer"),
        G_CALLBACK (action_go_to_computer_callback)
    },
    /* name, icon name */        { "Go to Network", BAUL_ICON_NETWORK,
        /* label, accelerator */       N_("_Network"), NULL,
        /* tooltip */                  N_("Browse bookmarked and local network locations"),
        G_CALLBACK (action_go_to_network_callback)
    },
    /* name, icon name */        { "Go to Templates", BAUL_ICON_TEMPLATE,
        /* label, accelerator */       N_("T_emplates"), NULL,
        /* tooltip */                  N_("Open your personal templates folder"),
        G_CALLBACK (action_go_to_templates_callback)
    },
    /* name, icon name */        { "Go to Trash", BAUL_ICON_TRASH,
        /* label, accelerator */       N_("_Trash"), NULL,
        /* tooltip */                  N_("Open your personal trash folder"),
        G_CALLBACK (action_go_to_trash_callback)
    },
};

static const CtkToggleActionEntry main_toggle_entries[] =
{
    /* name, icon name */        { "Show Hidden Files", NULL,
        /* label, accelerator */       N_("Show _Hidden Files"), "<control>H",
        /* tooltip */                  N_("Toggle the display of hidden files in the current window"),
        G_CALLBACK (action_show_hidden_files_callback),
        TRUE
    },
    /* name, stock id */         { "Show Backup Files", NULL,
    /* label, accelerator */       N_("Show Bac_kup Files"), "<control>K",
    /* tooltip */                  N_("Toggle the display of backup files in the current window"),
        G_CALLBACK (action_show_backup_files_callback),
        TRUE
    },

};

/**
 * baul_window_initialize_menus
 *
 * Create and install the set of menus for this window.
 * @window: A recently-created BaulWindow.
 */
void
baul_window_initialize_menus (BaulWindow *window)
{
    CtkActionGroup *action_group;
    CtkUIManager *ui_manager;
    CtkAction *action;
    const char *ui;

    action_group = ctk_action_group_new ("ShellActions");
    ctk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    window->details->main_action_group = action_group;
    ctk_action_group_add_actions (action_group,
                                  main_entries, G_N_ELEMENTS (main_entries),
                                  window);
    ctk_action_group_add_toggle_actions (action_group,
                                         main_toggle_entries, G_N_ELEMENTS (main_toggle_entries),
                                         window);

    action = ctk_action_group_get_action (action_group, BAUL_ACTION_UP);
    g_object_set (action, "short_label", _("_Up"), NULL);

    action = ctk_action_group_get_action (action_group, BAUL_ACTION_HOME);
    g_object_set (action, "short_label", _("_Home"), NULL);

    action = ctk_action_group_get_action (action_group, BAUL_ACTION_SHOW_HIDDEN_FILES);
    g_signal_handlers_block_by_func (action, action_show_hidden_files_callback, window);
    ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
                                  g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_SHOW_HIDDEN_FILES));
    g_signal_handlers_unblock_by_func (action, action_show_hidden_files_callback, window);
    g_signal_connect_swapped (baul_preferences, "changed::" BAUL_PREFERENCES_SHOW_HIDDEN_FILES,
                              G_CALLBACK(show_hidden_files_preference_callback),
                              window);

    action = ctk_action_group_get_action (action_group, BAUL_ACTION_SHOW_BACKUP_FILES);
    g_signal_handlers_block_by_func (action, action_show_backup_files_callback, window);
    ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
                                  g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_SHOW_BACKUP_FILES));
    g_signal_handlers_unblock_by_func (action, action_show_backup_files_callback, window);

    g_signal_connect_swapped (baul_preferences, "changed::" BAUL_PREFERENCES_SHOW_BACKUP_FILES,
                              G_CALLBACK(show_backup_files_preference_callback),
                              window);

    window->details->ui_manager = ctk_ui_manager_new ();
    ui_manager = window->details->ui_manager;
    ctk_window_add_accel_group (CTK_WINDOW (window),
                                ctk_ui_manager_get_accel_group (ui_manager));

    g_signal_connect (ui_manager, "connect_proxy",
                      G_CALLBACK (connect_proxy_cb), window);
    g_signal_connect (ui_manager, "disconnect_proxy",
                      G_CALLBACK (disconnect_proxy_cb), window);

    ctk_ui_manager_insert_action_group (ui_manager, action_group, 0);
    g_object_unref (action_group); /* owned by ui manager */

    ui = baul_ui_string_get ("baul-shell-ui.xml");
    ctk_ui_manager_add_ui_from_string (ui_manager, ui, -1, NULL);

    baul_window_initialize_trash_icon_monitor (window);
}

void
baul_window_finalize_menus (BaulWindow *window)
{
    BaulTrashMonitor *monitor;

    monitor = baul_trash_monitor_get ();

    g_signal_handlers_disconnect_by_func (monitor,
                                          trash_state_changed_cb, window);

    g_signal_handlers_disconnect_by_func (baul_preferences,
                                          show_hidden_files_preference_callback, window);

    g_signal_handlers_disconnect_by_func (baul_preferences,
                                          show_backup_files_preference_callback, window);
}

static GList *
get_extension_menus (BaulWindow *window)
{
    BaulWindowSlot *slot;
    GList *providers;
    GList *items;
    GList *l;

    providers = baul_extensions_get_for_type (BAUL_TYPE_MENU_PROVIDER);
    items = NULL;

    slot = baul_window_get_active_slot (window);

    for (l = providers; l != NULL; l = l->next)
    {
        BaulMenuProvider *provider;
        GList *file_items;

        provider = BAUL_MENU_PROVIDER (l->data);
        file_items = baul_menu_provider_get_background_items (provider,
                     CTK_WIDGET (window),
                     slot->viewed_file);
        items = g_list_concat (items, file_items);
    }

    baul_module_extension_list_free (providers);

    return items;
}

static void
add_extension_menu_items (BaulWindow *window,
                          guint merge_id,
                          CtkActionGroup *action_group,
                          GList *menu_items,
                          const char *subdirectory)
{
    CtkUIManager *ui_manager;
    GList *l;

    ui_manager = window->details->ui_manager;

    for (l = menu_items; l; l = l->next)
    {
        BaulMenuItem *item;
        BaulMenu *menu;
        CtkAction *action;
        char *path;
        const gchar *action_name;

        item = BAUL_MENU_ITEM (l->data);

        g_object_get (item, "menu", &menu, NULL);

        action = baul_action_from_menu_item (item, CTK_WIDGET (window));
        ctk_action_group_add_action_with_accel (action_group, action, NULL);
        action_name = ctk_action_get_name (action);

        path = g_build_path ("/", POPUP_PATH_EXTENSION_ACTIONS, subdirectory, NULL);
        ctk_ui_manager_add_ui (ui_manager,
                               merge_id,
                               path,
                               action_name,
                               action_name,
                               (menu != NULL) ? CTK_UI_MANAGER_MENU : CTK_UI_MANAGER_MENUITEM,
                               FALSE);
        g_free (path);

        path = g_build_path ("/", MENU_PATH_EXTENSION_ACTIONS, subdirectory, NULL);
        ctk_ui_manager_add_ui (ui_manager,
                               merge_id,
                               path,
                               action_name,
                               action_name,
                               (menu != NULL) ? CTK_UI_MANAGER_MENU : CTK_UI_MANAGER_MENUITEM,
                               FALSE);
        g_free (path);

        /* recursively fill the menu */
        if (menu != NULL)
        {
            char *subdir;
            GList *children;

            children = baul_menu_get_items (menu);

            subdir = g_build_path ("/", subdirectory, "/", ctk_action_get_name (action), NULL);
            add_extension_menu_items (window,
                                      merge_id,
                                      action_group,
                                      children,
                                      subdir);

            baul_menu_item_list_free (children);
            g_free (subdir);
        }
    }
}

void
baul_window_load_extension_menus (BaulWindow *window)
{
    CtkActionGroup *action_group;
    GList *items;
    guint merge_id;

    if (window->details->extensions_menu_merge_id != 0)
    {
        ctk_ui_manager_remove_ui (window->details->ui_manager,
                                  window->details->extensions_menu_merge_id);
        window->details->extensions_menu_merge_id = 0;
    }

    if (window->details->extensions_menu_action_group != NULL)
    {
        ctk_ui_manager_remove_action_group (window->details->ui_manager,
                                            window->details->extensions_menu_action_group);
        window->details->extensions_menu_action_group = NULL;
    }

    merge_id = ctk_ui_manager_new_merge_id (window->details->ui_manager);
    window->details->extensions_menu_merge_id = merge_id;
    action_group = ctk_action_group_new ("ExtensionsMenuGroup");
    window->details->extensions_menu_action_group = action_group;
    ctk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    ctk_ui_manager_insert_action_group (window->details->ui_manager, action_group, 0);
    g_object_unref (action_group); /* owned by ui manager */

    items = get_extension_menus (window);

    if (items != NULL)
    {
        add_extension_menu_items (window, merge_id, action_group, items, "");

        g_list_foreach (items, (GFunc) g_object_unref, NULL);
        g_list_free (items);
    }
}

