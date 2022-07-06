/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006 Paolo Borelli <pborelli@katamail.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Authors: Paolo Borelli <pborelli@katamail.com>
 *
 */

#include <config.h>

#include <glib/gi18n-lib.h>
#include <ctk/ctk.h>

#include <libbaul-private/baul-file-operations.h>
#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-file.h>
#include <libbaul-private/baul-trash-monitor.h>

#include "baul-trash-bar.h"
#include "baul-window.h"

#define BAUL_TRASH_BAR_GET_PRIVATE(o)\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), BAUL_TYPE_TRASH_BAR, BaulTrashBarPrivate))

enum
{
    PROP_WINDOW = 1,
    NUM_PROPERTIES
};

struct _BaulTrashBarPrivate
{
    CtkWidget *empty_button;
    CtkWidget *restore_button;

    BaulWindow *window;
    gulong selection_handler_id;
};

G_DEFINE_TYPE_WITH_PRIVATE (BaulTrashBar, baul_trash_bar, GTK_TYPE_BOX);

static void
restore_button_clicked_cb (CtkWidget *button,
                           BaulTrashBar *bar)
{
    GList *locations, *files, *l;

    locations = baul_window_info_get_selection (BAUL_WINDOW_INFO  (bar->priv->window));
    files = NULL;

    for (l = locations; l != NULL; l = l->next)
    {
        files = g_list_prepend (files, baul_file_get (l->data));
    }

    baul_restore_files_from_trash (files, GTK_WINDOW (ctk_widget_get_toplevel (button)));

    baul_file_list_free (files);
    g_list_free_full (locations, g_object_unref);
}

static void
selection_changed_cb (BaulWindow *window,
                      BaulTrashBar *bar)
{
    int count;

    count = baul_window_info_get_selection_count (BAUL_WINDOW_INFO (window));

    ctk_widget_set_sensitive (bar->priv->restore_button, (count > 0));
}

static void
connect_window_and_update_button (BaulTrashBar *bar)
{
    bar->priv->selection_handler_id =
        g_signal_connect (bar->priv->window, "selection_changed",
                          G_CALLBACK (selection_changed_cb), bar);

    selection_changed_cb (bar->priv->window, bar);
}

static void
baul_trash_bar_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    BaulTrashBar *bar;

    bar = BAUL_TRASH_BAR (object);

    switch (prop_id)
    {
    case PROP_WINDOW:
        bar->priv->window = g_value_get_object (value);
        connect_window_and_update_button (bar);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
baul_trash_bar_finalize (GObject *obj)
{
    BaulTrashBar *bar;

    bar = BAUL_TRASH_BAR (obj);

    if (bar->priv->selection_handler_id)
    {
        g_signal_handler_disconnect (bar->priv->window, bar->priv->selection_handler_id);
    }

    G_OBJECT_CLASS (baul_trash_bar_parent_class)->finalize (obj);
}

static void
baul_trash_bar_trash_state_changed (BaulTrashMonitor *trash_monitor,
                                    gboolean              state,
                                    gpointer              data)
{
    BaulTrashBar *bar;

    bar = BAUL_TRASH_BAR (data);

    ctk_widget_set_sensitive (bar->priv->empty_button,
                              !baul_trash_monitor_is_empty ());
}

static void
baul_trash_bar_class_init (BaulTrashBarClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->set_property = baul_trash_bar_set_property;
    object_class->finalize = baul_trash_bar_finalize;

    g_object_class_install_property (object_class,
                                     PROP_WINDOW,
                                     g_param_spec_object ("window",
                                             "window",
                                             "the BaulWindow",
                                             BAUL_TYPE_WINDOW,
                                             G_PARAM_WRITABLE |
                                             G_PARAM_CONSTRUCT_ONLY |
                                             G_PARAM_STATIC_STRINGS));
}

static void
empty_trash_callback (CtkWidget *button, gpointer data)
{
    CtkWidget *window;

    window = ctk_widget_get_toplevel (button);

    baul_file_operations_empty_trash (window);
}

static void
baul_trash_bar_init (BaulTrashBar *bar)
{
    CtkWidget *label;
    CtkWidget *hbox;

    bar->priv = baul_trash_bar_get_instance_private (bar);

    hbox = GTK_WIDGET (bar);

    label = ctk_label_new (_("Trash"));
    ctk_widget_show (label);

    ctk_orientable_set_orientation (GTK_ORIENTABLE (bar), GTK_ORIENTATION_HORIZONTAL);

    ctk_box_pack_start (GTK_BOX (bar), label, FALSE, FALSE, 0);

    bar->priv->empty_button = ctk_button_new_with_mnemonic (_("Empty _Trash"));
    ctk_widget_show (bar->priv->empty_button);
    ctk_box_pack_end (GTK_BOX (hbox), bar->priv->empty_button, FALSE, FALSE, 0);

    ctk_widget_set_sensitive (bar->priv->empty_button,
                              !baul_trash_monitor_is_empty ());
    ctk_widget_set_tooltip_text (bar->priv->empty_button,
                                 _("Delete all items in the Trash"));

    g_signal_connect (bar->priv->empty_button,
                      "clicked",
                      G_CALLBACK (empty_trash_callback),
                      bar);

    bar->priv->restore_button = ctk_button_new_with_mnemonic (_("Restore Selected Items"));
    ctk_widget_show (bar->priv->restore_button);
    ctk_box_pack_end (GTK_BOX (hbox), bar->priv->restore_button, FALSE, FALSE, 6);

    ctk_widget_set_sensitive (bar->priv->restore_button, FALSE);
    ctk_widget_set_tooltip_text (bar->priv->restore_button,
                                 _("Restore selected items to their original position"));

    g_signal_connect (bar->priv->restore_button,
                      "clicked",
                      G_CALLBACK (restore_button_clicked_cb),
                      bar);

    g_signal_connect_object (baul_trash_monitor_get (),
                             "trash_state_changed",
                             G_CALLBACK (baul_trash_bar_trash_state_changed),
                             bar,
                             0);
}

CtkWidget *
baul_trash_bar_new (BaulWindow *window)
{
    GObject *bar;

    bar = g_object_new (BAUL_TYPE_TRASH_BAR, "window", window, NULL);

    return GTK_WIDGET (bar);
}
