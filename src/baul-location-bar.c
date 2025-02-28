/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2000 Eazel, Inc.
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
 * Author: Maciej Stachowiak <mjs@eazel.com>
 *         Ettore Perazzoli <ettore@gnu.org>
 *         Michael Meeks <michael@nuclecu.unam.mx>
 *	   Andy Hertzfeld <andy@eazel.com>
 *
 */

/* baul-location-bar.c - Location bar for Baul
 */

#include <config.h>
#include <stdio.h>
#include <string.h>

#include <cdk/cdkkeysyms.h>
#include <ctk/ctk.h>
#include <glib/gi18n.h>

#include <eel/eel-accessibility.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-ctk-macros.h>
#include <eel/eel-stock-dialogs.h>
#include <eel/eel-string.h>
#include <eel/eel-vfs-extensions.h>

#include <libbaul-private/baul-icon-dnd.h>
#include <libbaul-private/baul-clipboard.h>

#include "baul-location-bar.h"
#include "baul-location-entry.h"
#include "baul-window-private.h"
#include "baul-window.h"
#include "baul-navigation-window-pane.h"

#define BAUL_DND_URI_LIST_TYPE 	  "text/uri-list"
#define BAUL_DND_TEXT_PLAIN_TYPE 	  "text/plain"

static const char untranslated_location_label[] = N_("Location:");
static const char untranslated_go_to_label[] = N_("Go To:");
#define LOCATION_LABEL _(untranslated_location_label)
#define GO_TO_LABEL _(untranslated_go_to_label)

struct _BaulLocationBarPrivate
{
    CtkLabel *label;
    BaulEntry *entry;

    char *last_location;

    guint idle_id;
};

enum
{
    BAUL_DND_MC_DESKTOP_ICON,
    BAUL_DND_URI_LIST,
    BAUL_DND_TEXT_PLAIN,
    BAUL_DND_NTARGETS
};

enum {
	CANCEL,
	LOCATION_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static const CtkTargetEntry drag_types [] =
{
    { BAUL_DND_URI_LIST_TYPE,   0, BAUL_DND_URI_LIST },
    { BAUL_DND_TEXT_PLAIN_TYPE, 0, BAUL_DND_TEXT_PLAIN },
};

static const CtkTargetEntry drop_types [] =
{
    { BAUL_DND_URI_LIST_TYPE,   0, BAUL_DND_URI_LIST },
    { BAUL_DND_TEXT_PLAIN_TYPE, 0, BAUL_DND_TEXT_PLAIN },
};

G_DEFINE_TYPE_WITH_PRIVATE (BaulLocationBar, baul_location_bar, CTK_TYPE_BOX);

static BaulNavigationWindow *
baul_location_bar_get_window (CtkWidget *bar)
{
    return BAUL_NAVIGATION_WINDOW (ctk_widget_get_ancestor (bar, BAUL_TYPE_WINDOW));
}

/**
 * baul_location_bar_get_location
 *
 * Get the "URI" represented by the text in the location bar.
 *
 * @bar: A BaulLocationBar.
 *
 * returns a newly allocated "string" containing the mangled
 * (by g_file_parse_name) text that the user typed in...maybe a URI
 * but not guaranteed.
 *
 **/
static char *
baul_location_bar_get_location (BaulLocationBar *bar)
{
    char *user_location, *uri;
    GFile *location;

    user_location = ctk_editable_get_chars (CTK_EDITABLE (bar->details->entry), 0, -1);
    location = g_file_parse_name (user_location);
    g_free (user_location);
    uri = g_file_get_uri (location);
    g_object_unref (location);
    return uri;
}

static void
emit_location_changed (BaulLocationBar *bar)
{
    char *location;

    location = baul_location_bar_get_location (bar);
    g_signal_emit (bar,
                   signals[LOCATION_CHANGED], 0,
                   location);
    g_free (location);
}

static void
drag_data_received_callback (CtkWidget        *widget,
			     CdkDragContext   *context,
			     int               x G_GNUC_UNUSED,
			     int               y G_GNUC_UNUSED,
			     CtkSelectionData *data,
			     guint             info G_GNUC_UNUSED,
			     guint32           time,
			     gpointer          callback_data)
{
    char **names;
    int name_count;
    BaulNavigationWindow *window;
    gboolean new_windows_for_extras;
    BaulLocationBar *self = BAUL_LOCATION_BAR (widget);

    g_assert (data != NULL);
    g_assert (callback_data == NULL);

    names = g_uri_list_extract_uris (ctk_selection_data_get_data (data));

    if (names == NULL || *names == NULL)
    {
        g_warning ("No D&D URI's");
        g_strfreev (names);
        ctk_drag_finish (context, FALSE, FALSE, time);
        return;
    }

    window = baul_location_bar_get_window (widget);
    new_windows_for_extras = FALSE;
    /* Ask user if they really want to open multiple windows
     * for multiple dropped URIs. This is likely to have been
     * a mistake.
     */
    name_count = g_strv_length (names);
    if (name_count > 1)
    {
        char *prompt;
        char *detail;

        prompt = g_strdup_printf (ngettext("Do you want to view %d location?",
                                           "Do you want to view %d locations?",
                                           name_count),
                                  name_count);
        detail = g_strdup_printf (ngettext("This will open %d separate window.",
                                           "This will open %d separate windows.",
                                           name_count),
                                  name_count);
        /* eel_run_simple_dialog should really take in pairs
         * like ctk_dialog_new_with_buttons() does. */
        new_windows_for_extras = eel_run_simple_dialog
                                 (CTK_WIDGET (window),
                                  TRUE,
                                  CTK_MESSAGE_QUESTION,
                                  prompt,
                                  detail,
                                  "process-stop", "ctk-ok",
                                  NULL) != 0 /* CAFE_OK */;

        g_free (prompt);
        g_free (detail);

        if (!new_windows_for_extras)
        {
            g_strfreev (names);
            ctk_drag_finish (context, FALSE, FALSE, time);
            return;
        }
    }

    baul_location_bar_set_location (self, names[0]);
    emit_location_changed (self);

    if (new_windows_for_extras)
    {
        BaulApplication *application;
        CdkScreen *screen;
        int i;
        BaulWindow *new_window = NULL;
        GFile *location = NULL;

        application = BAUL_WINDOW (window)->application;
        screen = ctk_window_get_screen (CTK_WINDOW (window));

        for (i = 1; names[i] != NULL; ++i)
        {
            new_window = baul_application_create_navigation_window (application, screen);

            location = g_file_new_for_uri (names[i]);
            baul_window_go_to (new_window, location);
            g_object_unref (location);
        }
    }

    g_strfreev (names);

    ctk_drag_finish (context, TRUE, FALSE, time);
}

static void
drag_data_get_callback (CtkWidget        *widget G_GNUC_UNUSED,
			CdkDragContext   *context G_GNUC_UNUSED,
			CtkSelectionData *selection_data,
			guint             info,
			guint32           time G_GNUC_UNUSED,
			gpointer          callback_data)
{
    BaulLocationBar *self;
    char *entry_text;

    g_assert (selection_data != NULL);
    self = callback_data;

    entry_text = baul_location_bar_get_location (self);

    switch (info)
    {
    case BAUL_DND_URI_LIST:
    case BAUL_DND_TEXT_PLAIN:
        ctk_selection_data_set (selection_data,
                                ctk_selection_data_get_target (selection_data),
                                8, (guchar *) entry_text,
                                eel_strlen (entry_text));
        break;
    default:
        g_assert_not_reached ();
    }
    g_free (entry_text);
}

/* routine that determines the usize for the label widget as larger
   then the size of the largest string and then sets it to that so
   that we don't have localization problems. see
   ctk_label_finalize_lines in ctklabel.c (line 618) for the code that
   we are imitating here. */

static void
style_set_handler (CtkWidget       *widget,
		   CtkStyleContext *previous_style G_GNUC_UNUSED)
{
    PangoLayout *layout;
    int width, width2;
    int xpad;
    gint margin_start, margin_end;

    layout = ctk_label_get_layout (CTK_LABEL(widget));

    layout = pango_layout_copy (layout);

    pango_layout_set_text (layout, LOCATION_LABEL, -1);
    pango_layout_get_pixel_size (layout, &width, NULL);

    pango_layout_set_text (layout, GO_TO_LABEL, -1);
    pango_layout_get_pixel_size (layout, &width2, NULL);
    width = MAX (width, width2);

    margin_start = ctk_widget_get_margin_start (widget);
    margin_end = ctk_widget_get_margin_end (widget);
    xpad = margin_start + margin_end;

    width += 2 * xpad;

    ctk_widget_set_size_request (widget, width, -1);

    g_object_unref (layout);
}

static gboolean
label_button_pressed_callback (CtkWidget             *widget,
                               CdkEventButton        *event)
{
    BaulNavigationWindow *window;
    BaulWindowSlot       *slot;
    BaulView             *view;
    CtkWidget                *label;

    if (event->button != 3)
    {
        return FALSE;
    }

    window = baul_location_bar_get_window (ctk_widget_get_parent (widget));
    slot = BAUL_WINDOW (window)->details->active_pane->active_slot;
    view = slot->content_view;
    label = ctk_bin_get_child (CTK_BIN (widget));
    /* only pop-up if the URI in the entry matches the displayed location */
    if (view == NULL ||
            strcmp (ctk_label_get_text (CTK_LABEL (label)), LOCATION_LABEL))
    {
        return FALSE;
    }

    baul_view_pop_up_location_context_menu (view, event, NULL);

    return FALSE;
}

static void
editable_activate_callback (CtkEntry *entry,
                            gpointer user_data)
{
    BaulLocationBar *self = user_data;
    const char *entry_text;

    entry_text = ctk_entry_get_text (entry);
    if (entry_text != NULL && *entry_text != '\0')
    {
            emit_location_changed (self);
    }
}

/**
 * baul_location_bar_update_label
 *
 * if the text in the entry matches the uri, set the label to "location", otherwise use "goto"
 *
 **/
static void
baul_location_bar_update_label (BaulLocationBar *bar)
{
    const char *current_text;
    GFile *location;
    GFile *last_location;

    if (bar->details->last_location == NULL){
        ctk_label_set_text (CTK_LABEL (bar->details->label), GO_TO_LABEL);
        baul_location_entry_set_secondary_action (BAUL_LOCATION_ENTRY (bar->details->entry),
                                                  BAUL_LOCATION_ENTRY_ACTION_GOTO);
        return;
    }

    current_text = ctk_entry_get_text (CTK_ENTRY (bar->details->entry));
    location = g_file_parse_name (current_text);
    last_location = g_file_parse_name (bar->details->last_location);

    if (g_file_equal (last_location, location)) {
        ctk_label_set_text (CTK_LABEL (bar->details->label), LOCATION_LABEL);
        baul_location_entry_set_secondary_action (BAUL_LOCATION_ENTRY (bar->details->entry),
                                                  BAUL_LOCATION_ENTRY_ACTION_CLEAR);
    } else {
        ctk_label_set_text (CTK_LABEL (bar->details->label), GO_TO_LABEL);
        baul_location_entry_set_secondary_action (BAUL_LOCATION_ENTRY (bar->details->entry),
                                                  BAUL_LOCATION_ENTRY_ACTION_GOTO);
    }

    g_object_unref (location);
    g_object_unref (last_location);
}

static void
editable_changed_callback (CtkEntry *entry G_GNUC_UNUSED,
			   gpointer  user_data)
{
    baul_location_bar_update_label (BAUL_LOCATION_BAR (user_data));
}

void
baul_location_bar_activate (BaulLocationBar *bar)
{
    /* Put the keyboard focus in the text field when switching to this mode,
     * and select all text for easy overtyping
     */
    ctk_widget_grab_focus (CTK_WIDGET (bar->details->entry));
    baul_entry_select_all (bar->details->entry);
}

static void
baul_location_bar_cancel (BaulLocationBar *bar)
{
    char *last_location;

    last_location = bar->details->last_location;
    baul_location_bar_set_location (bar, last_location);
}

static void
finalize (GObject *object)
{
    BaulLocationBar *bar;

    bar = BAUL_LOCATION_BAR (object);

    /* cancel the pending idle call, if any */
    if (bar->details->idle_id != 0)
    {
        g_source_remove (bar->details->idle_id);
        bar->details->idle_id = 0;
    }

    g_free (bar->details->last_location);
    bar->details->last_location = NULL;

    G_OBJECT_CLASS (baul_location_bar_parent_class)->finalize (object);
}

static void
baul_location_bar_class_init (BaulLocationBarClass *klass)
 {
    GObjectClass *gobject_class;
    CtkBindingSet *binding_set;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = finalize;

    klass->cancel = baul_location_bar_cancel;

    signals[CANCEL] = g_signal_new
            ("cancel",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (BaulLocationBarClass,
                             cancel),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    signals[LOCATION_CHANGED] = g_signal_new
            ("location-changed",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST, 0,
            NULL, NULL,
            g_cclosure_marshal_VOID__STRING,
            G_TYPE_NONE, 1, G_TYPE_STRING);

    binding_set = ctk_binding_set_by_class (klass);
    ctk_binding_entry_add_signal (binding_set, CDK_KEY_Escape, 0, "cancel", 0);
}

static void
baul_location_bar_init (BaulLocationBar *bar)
{
    CtkWidget *label;
    CtkWidget *entry;
    CtkWidget *event_box;

    bar->details = baul_location_bar_get_instance_private (bar);

    ctk_orientable_set_orientation (CTK_ORIENTABLE (bar),
                                    CTK_ORIENTATION_HORIZONTAL);

    event_box = ctk_event_box_new ();
    ctk_event_box_set_visible_window (CTK_EVENT_BOX (event_box), FALSE);

    ctk_container_set_border_width (CTK_CONTAINER (event_box), 4);
    label = ctk_label_new (LOCATION_LABEL);
    ctk_container_add   (CTK_CONTAINER (event_box), label);
    ctk_label_set_justify (CTK_LABEL (label), CTK_JUSTIFY_RIGHT);
    ctk_label_set_xalign (CTK_LABEL (label), 1.0);
    ctk_label_set_yalign (CTK_LABEL (label), 0.5);
    g_signal_connect (label, "style_set",
                      G_CALLBACK (style_set_handler), NULL);

    ctk_box_pack_start (CTK_BOX (bar), event_box, FALSE, TRUE, 4);

    entry = baul_location_entry_new ();

    g_signal_connect_object (entry, "activate",
                             G_CALLBACK (editable_activate_callback), bar, G_CONNECT_AFTER);
    g_signal_connect_object (entry, "changed",
                             G_CALLBACK (editable_changed_callback), bar, 0);

    ctk_box_pack_start (CTK_BOX (bar), entry, TRUE, TRUE, 0);

    eel_accessibility_set_up_label_widget_relation (label, entry);

    /* Label context menu */
    g_signal_connect (event_box, "button-press-event",
                      G_CALLBACK (label_button_pressed_callback), NULL);

    /* Drag source */
    ctk_drag_source_set (CTK_WIDGET (event_box),
                         CDK_BUTTON1_MASK | CDK_BUTTON3_MASK,
                         drag_types, G_N_ELEMENTS (drag_types),
                         CDK_ACTION_COPY | CDK_ACTION_LINK);
    g_signal_connect_object (event_box, "drag_data_get",
                             G_CALLBACK (drag_data_get_callback), bar, 0);

    /* Drag dest. */
    ctk_drag_dest_set (CTK_WIDGET (bar),
                       CTK_DEST_DEFAULT_ALL,
                       drop_types, G_N_ELEMENTS (drop_types),
                       CDK_ACTION_COPY | CDK_ACTION_MOVE | CDK_ACTION_LINK);
    g_signal_connect (bar, "drag_data_received",
                      G_CALLBACK (drag_data_received_callback), NULL);

    bar->details->label = CTK_LABEL (label);
    bar->details->entry = BAUL_ENTRY (entry);

    ctk_widget_show_all (CTK_WIDGET (bar));
}

CtkWidget *
baul_location_bar_new (BaulNavigationWindowPane *pane)
{
    CtkWidget *bar;
    BaulLocationBar *location_bar;

    bar = ctk_widget_new (BAUL_TYPE_LOCATION_BAR, NULL);
    location_bar = BAUL_LOCATION_BAR (bar);

    /* Clipboard */
    baul_clipboard_set_up_editable
    (CTK_EDITABLE (location_bar->details->entry),
     baul_window_get_ui_manager (BAUL_WINDOW (BAUL_WINDOW_PANE(pane)->window)),
     TRUE);

    return bar;
}

void
baul_location_bar_set_location (BaulLocationBar *bar,
                                const char *location)
{
    g_assert (location != NULL);

    /* Note: This is called in reaction to external changes, and
     * thus should not emit the LOCATION_CHANGED signal. */

    if (eel_uri_is_search (location))
    {
        baul_location_entry_set_special_text (BAUL_LOCATION_ENTRY (bar->details->entry),
                                              "");
    }
    else
    {
        char *formatted_location;
        GFile *file;

        file = g_file_new_for_uri (location);
        formatted_location = g_file_get_parse_name (file);
        g_object_unref (file);
        baul_location_entry_update_current_location (BAUL_LOCATION_ENTRY (bar->details->entry),
                formatted_location);
        g_free (formatted_location);
    }

    /* remember the original location for later comparison */

    if (bar->details->last_location != location)
    {
        g_free (bar->details->last_location);
        bar->details->last_location = g_strdup (location);
    }

    baul_location_bar_update_label (bar);
}

static void
override_background_color (CtkWidget *widget,
                           CdkRGBA   *rgba)
{
    gchar          *css;
    CtkCssProvider *provider;

    provider = ctk_css_provider_new ();

    css = g_strdup_printf ("entry { background-color: %s;}",
                           cdk_rgba_to_string (rgba));
    ctk_css_provider_load_from_data (provider, css, -1, NULL);
    g_free (css);

    ctk_style_context_add_provider (ctk_widget_get_style_context (widget),
                                    CTK_STYLE_PROVIDER (provider),
                                    CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (provider);
}

/* change background color based on activity state */
void
baul_location_bar_set_active (BaulLocationBar *location_bar, gboolean is_active)
{
    CtkStyleContext *style;
    CdkRGBA color;
    CdkRGBA *c;
    static CdkRGBA bg_active;
    static CdkRGBA bg_inactive;

    style = ctk_widget_get_style_context (CTK_WIDGET (location_bar->details->entry));

    if (is_active)
        ctk_style_context_get (style, CTK_STATE_FLAG_NORMAL,
                               CTK_STYLE_PROPERTY_BACKGROUND_COLOR,
                               &c, NULL);
    else
        ctk_style_context_get (style, CTK_STATE_FLAG_INSENSITIVE,
                               CTK_STYLE_PROPERTY_BACKGROUND_COLOR,
                               &c, NULL);

    color = *c;
    cdk_rgba_free (c);

    if (is_active)
    {
        if (cdk_rgba_equal (&bg_active, &bg_inactive))
            bg_active = color;

        override_background_color (CTK_WIDGET (location_bar->details->entry), &bg_active);
    }
    else
    {
        if (cdk_rgba_equal (&bg_active, &bg_inactive))
            bg_inactive = color;

        override_background_color(CTK_WIDGET (location_bar->details->entry), &bg_inactive);
    }
}

BaulEntry *
baul_location_bar_get_entry (BaulLocationBar *location_bar)
{
    return location_bar->details->entry;
}
