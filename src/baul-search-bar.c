/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2005 Novell, Inc.
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
 * You should have received a copy of the GNU General Public
 * License along with this program; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Anders Carlsson <andersca@imendio.com>
 *
 */

#include <config.h>

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <ctk/ctk.h>

#include <eel/eel-ctk-macros.h>

#include <libbaul-private/baul-clipboard.h>

#include "baul-search-bar.h"

struct BaulSearchBarDetails
{
    CtkWidget *entry;
    gboolean entry_borrowed;
};

enum
{
    ACTIVATE,
    CANCEL,
    FOCUS_IN,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void  baul_search_bar_class_init       (BaulSearchBarClass *class);
static void  baul_search_bar_init             (BaulSearchBar      *bar);

EEL_CLASS_BOILERPLATE (BaulSearchBar,
                       baul_search_bar,
                       GTK_TYPE_EVENT_BOX)


static void
finalize (GObject *object)
{
    BaulSearchBar *bar;

    bar = BAUL_SEARCH_BAR (object);

    g_free (bar->details);

    EEL_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
baul_search_bar_class_init (BaulSearchBarClass *class)
{
    GObjectClass *gobject_class;
    CtkBindingSet *binding_set;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = finalize;

    signals[ACTIVATE] =
        g_signal_new ("activate",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulSearchBarClass, activate),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[FOCUS_IN] =
        g_signal_new ("focus-in",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulSearchBarClass, focus_in),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[CANCEL] =
        g_signal_new ("cancel",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (BaulSearchBarClass, cancel),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    binding_set = ctk_binding_set_by_class (class);
	ctk_binding_entry_add_signal (binding_set, GDK_KEY_Escape, 0, "cancel", 0);
}

static gboolean
entry_has_text (BaulSearchBar *bar)
{
    const char *text;

    text = ctk_entry_get_text (GTK_ENTRY (bar->details->entry));

    return text != NULL && text[0] != '\0';
}

static void
entry_icon_release_cb (CtkEntry *entry,
                       CtkEntryIconPosition position,
                       GdkEvent *event,
                       BaulSearchBar *bar)
{
    g_signal_emit_by_name (entry, "activate", 0);
}

static void
entry_activate_cb (CtkWidget *entry, BaulSearchBar *bar)
{
    if (entry_has_text (bar) && !bar->details->entry_borrowed)
    {
        g_signal_emit (bar, signals[ACTIVATE], 0);
    }
}

static gboolean
focus_in_event_callback (CtkWidget *widget,
                         GdkEventFocus *event,
                         gpointer user_data)
{
    BaulSearchBar *bar;

    bar = BAUL_SEARCH_BAR (user_data);

    g_signal_emit (bar, signals[FOCUS_IN], 0);

    return FALSE;
}

static void
baul_search_bar_init (BaulSearchBar *bar)
{
    CtkWidget *hbox;
    CtkWidget *label;
    CtkStyleContext *context;

    context = ctk_widget_get_style_context (GTK_WIDGET (bar));
    ctk_style_context_add_class (context, "baul-search-bar");

    bar->details = g_new0 (BaulSearchBarDetails, 1);

    ctk_event_box_set_visible_window (GTK_EVENT_BOX (bar), FALSE);

    hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    ctk_widget_set_margin_start (hbox, 6);
    ctk_widget_set_margin_end (hbox, 6);
    ctk_widget_show (hbox);
    ctk_container_add (GTK_CONTAINER (bar), hbox);

    label = ctk_label_new (_("Search:"));
    ctk_widget_show (label);

    ctk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    bar->details->entry = ctk_entry_new ();
    ctk_entry_set_icon_from_icon_name (GTK_ENTRY (bar->details->entry),
                                   GTK_ENTRY_ICON_SECONDARY,
                                   "find");
    ctk_box_pack_start (GTK_BOX (hbox), bar->details->entry, TRUE, TRUE, 0);

    g_signal_connect (bar->details->entry, "activate",
                      G_CALLBACK (entry_activate_cb), bar);
    g_signal_connect (bar->details->entry, "icon-release",
                      G_CALLBACK (entry_icon_release_cb), bar);
    g_signal_connect (bar->details->entry, "focus-in-event",
                      G_CALLBACK (focus_in_event_callback), bar);

    ctk_widget_show (bar->details->entry);
}

CtkWidget *
baul_search_bar_borrow_entry (BaulSearchBar *bar)
{
    CtkBindingSet *binding_set;

    bar->details->entry_borrowed = TRUE;

    binding_set = ctk_binding_set_by_class (G_OBJECT_GET_CLASS (bar));
	ctk_binding_entry_remove (binding_set, GDK_KEY_Escape, 0);
    return bar->details->entry;
}

void
baul_search_bar_return_entry (BaulSearchBar *bar)
{
    CtkBindingSet *binding_set;

    bar->details->entry_borrowed = FALSE;

    binding_set = ctk_binding_set_by_class (G_OBJECT_GET_CLASS (bar));
	ctk_binding_entry_add_signal (binding_set, GDK_KEY_Escape, 0, "cancel", 0);
}

CtkWidget *
baul_search_bar_new (BaulWindow *window)
{
    CtkWidget *bar;
    BaulSearchBar *search_bar;

    bar = g_object_new (BAUL_TYPE_SEARCH_BAR, NULL);
    search_bar = BAUL_SEARCH_BAR(bar);

    /* Clipboard */
    baul_clipboard_set_up_editable
    (GTK_EDITABLE (search_bar->details->entry),
     baul_window_get_ui_manager (window),
     FALSE);

    return bar;
}

BaulQuery *
baul_search_bar_get_query (BaulSearchBar *bar)
{
    const char *query_text;
    BaulQuery *query;

    query_text = ctk_entry_get_text (GTK_ENTRY (bar->details->entry));

    /* Empty string is a NULL query */
    if (query_text && query_text[0] == '\0')
    {
        return NULL;
    }

    query = baul_query_new ();
    baul_query_set_text (query, query_text);

    return query;
}

void
baul_search_bar_grab_focus (BaulSearchBar *bar)
{
    ctk_widget_grab_focus (bar->details->entry);
}

void
baul_search_bar_clear (BaulSearchBar *bar)
{
    ctk_entry_set_text (GTK_ENTRY (bar->details->entry), "");
}
