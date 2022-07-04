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

#include <gtk/gtk.h>

#include <eel/eel-gtk-extensions.h>

#include "baul-zoom-action.h"
#include "baul-zoom-control.h"
#include "baul-navigation-window.h"
#include "baul-window-private.h"
#include "baul-navigation-window-slot.h"

static GObjectClass *parent_class = NULL;

struct _BaulZoomActionPrivate
{
    BaulNavigationWindow *window;
};

enum
{
    PROP_0,
    PROP_WINDOW
};

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
G_DEFINE_TYPE_WITH_PRIVATE (BaulZoomAction, baul_zoom_action, GTK_TYPE_ACTION)
G_GNUC_END_IGNORE_DEPRECATIONS;

static void
zoom_changed_callback (BaulWindow *window,
                       BaulZoomLevel zoom_level,
                       gboolean supports_zooming,
                       gboolean can_zoom,
                       gboolean can_zoom_in,
                       gboolean can_zoom_out,
                       GtkWidget *zoom_control)
{
    if (supports_zooming)
    {
        gtk_widget_set_sensitive (zoom_control, can_zoom);
        gtk_widget_show (zoom_control);
        if (can_zoom)
        {
            baul_zoom_control_set_zoom_level (BAUL_ZOOM_CONTROL (zoom_control),
                                              zoom_level);
        }
    }
    else
    {
        gtk_widget_hide (zoom_control);
    }
}

static void
connect_proxy (GtkAction *action,
               GtkWidget *proxy)
{
    if (GTK_IS_TOOL_ITEM (proxy))
    {
        GtkToolItem *item = GTK_TOOL_ITEM (proxy);
        BaulZoomAction *zaction = BAUL_ZOOM_ACTION (action);
        BaulNavigationWindow *window = zaction->priv->window;
        GtkWidget *zoom_control;

        zoom_control = baul_zoom_control_new ();
        gtk_container_add (GTK_CONTAINER (item),  zoom_control);
        gtk_widget_show (zoom_control);

        g_signal_connect_object (zoom_control, "zoom_in",
                                 G_CALLBACK (baul_window_zoom_in),
                                 window, G_CONNECT_SWAPPED);
        g_signal_connect_object (zoom_control, "zoom_out",
                                 G_CALLBACK (baul_window_zoom_out),
                                 window, G_CONNECT_SWAPPED);
        g_signal_connect_object (zoom_control, "zoom_to_level",
                                 G_CALLBACK (baul_window_zoom_to_level),
                                 window, G_CONNECT_SWAPPED);
        g_signal_connect_object (zoom_control, "zoom_to_default",
                                 G_CALLBACK (baul_window_zoom_to_default),
                                 window, G_CONNECT_SWAPPED);

        g_signal_connect (window, "zoom-changed",
                          G_CALLBACK (zoom_changed_callback),
                          zoom_control);
    }

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    (* GTK_ACTION_CLASS (parent_class)->connect_proxy) (action, proxy);
    G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
disconnect_proxy (GtkAction *action,
                  GtkWidget *proxy)
{
    if (GTK_IS_TOOL_ITEM (proxy))
    {
        GtkToolItem *item = GTK_TOOL_ITEM (proxy);
        BaulZoomAction *zaction = BAUL_ZOOM_ACTION (action);
        BaulNavigationWindow *window = zaction->priv->window;
        GtkWidget *child;

        child = gtk_bin_get_child (GTK_BIN (item));

        g_signal_handlers_disconnect_by_func (window, G_CALLBACK (zoom_changed_callback), child);

    }

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    (* GTK_ACTION_CLASS (parent_class)->disconnect_proxy) (action, proxy);
    G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
baul_zoom_action_finalize (GObject *object)
{
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
baul_zoom_action_set_property (GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
    BaulZoomAction *zoom;

    zoom = BAUL_ZOOM_ACTION (object);

    switch (prop_id)
    {
    case PROP_WINDOW:
        zoom->priv->window = BAUL_NAVIGATION_WINDOW (g_value_get_object (value));
        break;
    }
}

static void
baul_zoom_action_get_property (GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
    BaulZoomAction *zoom;

    zoom = BAUL_ZOOM_ACTION (object);

    switch (prop_id)
    {
    case PROP_WINDOW:
        g_value_set_object (value, zoom->priv->window);
        break;
    }
}

static void
baul_zoom_action_class_init (BaulZoomActionClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    GtkActionClass *action_class = GTK_ACTION_CLASS (class);
    G_GNUC_END_IGNORE_DEPRECATIONS;

    object_class->finalize = baul_zoom_action_finalize;
    object_class->set_property = baul_zoom_action_set_property;
    object_class->get_property = baul_zoom_action_get_property;

    parent_class = g_type_class_peek_parent (class);

    action_class->toolbar_item_type = GTK_TYPE_TOOL_ITEM;
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
baul_zoom_action_init (BaulZoomAction *action)
{
    action->priv = baul_zoom_action_get_instance_private (action);
}
