/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/*
 *  Baul
 *
 *  Copyright (C) 2009 Red Hat, Inc.
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
 *  Authors: Alexander Larsson <alexl@redhat.com>
 *
 */

#include <config.h>

#include <ctk/ctk.h>

#include <eel/eel-ctk-extensions.h>

#include <libbaul-private/baul-view-factory.h>

#include "baul-view-as-action.h"
#include "baul-navigation-window.h"
#include "baul-window-private.h"
#include "baul-navigation-window-slot.h"

static GObjectClass *parent_class = NULL;

struct _BaulViewAsActionPrivate
{
    BaulNavigationWindow *window;
};

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
G_DEFINE_TYPE_WITH_PRIVATE (BaulViewAsAction, baul_view_as_action, CTK_TYPE_ACTION)
G_GNUC_END_IGNORE_DEPRECATIONS;

enum
{
    PROP_0,
    PROP_WINDOW
};


static void
activate_nth_short_list_item (BaulWindow *window, guint index)
{
    BaulWindowSlot *slot;

    g_assert (BAUL_IS_WINDOW (window));

    slot = baul_window_get_active_slot (window);
    g_assert (index < g_list_length (window->details->short_list_viewers));

    baul_window_slot_set_content_view (slot,
                                       g_list_nth_data (window->details->short_list_viewers, index));
}

static void
activate_extra_viewer (BaulWindow *window)
{
    BaulWindowSlot *slot;

    g_assert (BAUL_IS_WINDOW (window));

    slot = baul_window_get_active_slot (window);
    g_assert (window->details->extra_viewer != NULL);

    baul_window_slot_set_content_view (slot, window->details->extra_viewer);
}

static void
view_as_menu_switch_views_callback (CtkComboBox *combo_box, BaulNavigationWindow *window)
{
    int active;

    g_assert (CTK_IS_COMBO_BOX (combo_box));
    g_assert (BAUL_IS_NAVIGATION_WINDOW (window));

    active = ctk_combo_box_get_active (combo_box);

    if (active < 0)
    {
        return;
    }
    else if (active < GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo_box), "num viewers")))
    {
        activate_nth_short_list_item (BAUL_WINDOW (window), active);
    }
    else
    {
        activate_extra_viewer (BAUL_WINDOW (window));
    }
}

static void
view_as_changed_callback (BaulWindow *window,
                          CtkComboBox *combo_box)
{
    BaulWindowSlot *slot;
    GList *node;
    int index;
    int selected_index = -1;
    CtkTreeModel *model;
    CtkListStore *store;
    const BaulViewInfo *info;

    /* Clear the contents of ComboBox in a wacky way because there
     * is no function to clear all items and also no function to obtain
     * the number of items in a combobox.
     */
    model = ctk_combo_box_get_model (combo_box);
    g_return_if_fail (CTK_IS_LIST_STORE (model));
    store = CTK_LIST_STORE (model);
    ctk_list_store_clear (store);

    slot = baul_window_get_active_slot (window);

    /* Add a menu item for each view in the preferred list for this location. */
    for (node = window->details->short_list_viewers, index = 0;
            node != NULL;
            node = node->next, ++index)
    {
        info = baul_view_factory_lookup (node->data);
        ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), _(info->view_combo_label));

        if (baul_window_slot_content_view_matches_iid (slot, (char *)node->data))
        {
            selected_index = index;
        }
    }
    g_object_set_data (G_OBJECT (combo_box), "num viewers", GINT_TO_POINTER (index));

    if (g_list_length (window->details->short_list_viewers) == 1)
    {
        selected_index = 0;
    }

    if (selected_index == -1)
    {
        const char *id;
        /* We're using an extra viewer, add a menu item for it */

        id = baul_window_slot_get_content_view_id (slot);
        info = baul_view_factory_lookup (id);
        ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box),
                                        _(info->view_combo_label));
        selected_index = index;
    }
    ctk_combo_box_set_active (combo_box, selected_index);
    if (g_list_length (window->details->short_list_viewers) == 1) {
        ctk_widget_hide(CTK_WIDGET(combo_box));
    } else {
        ctk_widget_show(CTK_WIDGET(combo_box));
    }
}


static void
connect_proxy (CtkAction *action,
               CtkWidget *proxy)
{
    if (CTK_IS_TOOL_ITEM (proxy))
    {
        CtkToolItem *item = CTK_TOOL_ITEM (proxy);
        BaulViewAsAction *vaction = BAUL_VIEW_AS_ACTION (action);
        BaulNavigationWindow *window = vaction->priv->window;
        CtkWidget *view_as_menu_vbox;
        CtkWidget *view_as_combo_box;

        /* Option menu for content view types; it's empty here, filled in when a uri is set.
         * Pack it into vbox so it doesn't grow vertically when location bar does.
         */
        view_as_menu_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 4);
        ctk_widget_show (view_as_menu_vbox);

        ctk_container_add (CTK_CONTAINER (item), view_as_menu_vbox);

        view_as_combo_box = ctk_combo_box_text_new ();

        ctk_widget_set_focus_on_click (view_as_combo_box, FALSE);
        ctk_box_pack_end (CTK_BOX (view_as_menu_vbox), view_as_combo_box, TRUE, FALSE, 0);
        ctk_widget_show (view_as_combo_box);
        g_signal_connect_object (view_as_combo_box, "changed",
                                 G_CALLBACK (view_as_menu_switch_views_callback), window, 0);

        g_signal_connect (window, "view-as-changed",
                          G_CALLBACK (view_as_changed_callback),
                          view_as_combo_box);
    }

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    (* CTK_ACTION_CLASS (parent_class)->connect_proxy) (action, proxy);
    G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
disconnect_proxy (CtkAction *action,
                  CtkWidget *proxy)
{
    if (CTK_IS_TOOL_ITEM (proxy))
    {
        BaulViewAsAction *vaction = BAUL_VIEW_AS_ACTION (action);
        BaulNavigationWindow *window = vaction->priv->window;

        g_signal_handlers_disconnect_matched (window,
                                              G_SIGNAL_MATCH_FUNC,
                                              0, 0, NULL, G_CALLBACK (view_as_changed_callback), NULL);
    }

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    (* CTK_ACTION_CLASS (parent_class)->disconnect_proxy) (action, proxy);
    G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
baul_view_as_action_finalize (GObject *object)
{
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
baul_view_as_action_set_property (GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
    BaulViewAsAction *zoom;

    zoom = BAUL_VIEW_AS_ACTION (object);

    switch (prop_id)
    {
    case PROP_WINDOW:
        zoom->priv->window = BAUL_NAVIGATION_WINDOW (g_value_get_object (value));
        break;
    }
}

static void
baul_view_as_action_get_property (GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
    BaulViewAsAction *zoom;

    zoom = BAUL_VIEW_AS_ACTION (object);

    switch (prop_id)
    {
    case PROP_WINDOW:
        g_value_set_object (value, zoom->priv->window);
        break;
    }
}

static void
baul_view_as_action_class_init (BaulViewAsActionClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    CtkActionClass *action_class = CTK_ACTION_CLASS (class);
    G_GNUC_END_IGNORE_DEPRECATIONS;

    object_class->finalize = baul_view_as_action_finalize;
    object_class->set_property = baul_view_as_action_set_property;
    object_class->get_property = baul_view_as_action_get_property;

    parent_class = g_type_class_peek_parent (class);

    action_class->toolbar_item_type = CTK_TYPE_TOOL_ITEM;
    action_class->connect_proxy = connect_proxy;
    action_class->disconnect_proxy = disconnect_proxy;

    g_object_class_install_property (object_class,
                                     PROP_WINDOW,
                                     g_param_spec_object ("window",
                                             "Window",
                                             "The navigation window",
                                             G_TYPE_OBJECT,
                                             G_PARAM_READWRITE));
}

static void
baul_view_as_action_init (BaulViewAsAction *action)
{
    action->priv = baul_view_as_action_get_instance_private (action);
}
