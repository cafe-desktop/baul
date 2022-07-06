/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2000 Eazel, Inc.
 * Copyright (C) 2004 Red Hat, Inc.
 *
 * Baul is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Baul is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Andy Hertzfeld <andy@eazel.com>
 *         Alexander Larsson <alexl@redhat.com>
 *
 * This is the zoom control for the location bar
 *
 */

#include <config.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <atk/atkaction.h>
#include <glib/gi18n.h>
#include <ctk/ctk.h>
#include <ctk/ctk-a11y.h>
#include <cdk/cdkkeysyms.h>

#include <eel/eel-accessibility.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-graphic-effects.h>
#include <eel/eel-ctk-extensions.h>

#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-global-preferences.h>

#include "baul-zoom-control.h"

enum
{
    ZOOM_IN,
    ZOOM_OUT,
    ZOOM_TO_LEVEL,
    ZOOM_TO_DEFAULT,
    CHANGE_VALUE,
    LAST_SIGNAL
};

struct _BaulZoomControlPrivate
{
    CtkWidget *zoom_in;
    CtkWidget *zoom_out;
    CtkWidget *zoom_label;
    CtkWidget *zoom_button;

    BaulZoomLevel zoom_level;
    BaulZoomLevel min_zoom_level;
    BaulZoomLevel max_zoom_level;
    gboolean has_min_zoom_level;
    gboolean has_max_zoom_level;
    GList *preferred_zoom_levels;

    gboolean marking_menu_items;
};


static guint signals[LAST_SIGNAL] = { 0 };

static gpointer accessible_parent_class;

static const char * const baul_zoom_control_accessible_action_names[] =
{
    N_("Zoom In"),
    N_("Zoom Out"),
    N_("Zoom to Default"),
};

static const int baul_zoom_control_accessible_action_signals[] =
{
    ZOOM_IN,
    ZOOM_OUT,
    ZOOM_TO_DEFAULT,
};

static const char * const baul_zoom_control_accessible_action_descriptions[] =
{
    N_("Increase the view size"),
    N_("Decrease the view size"),
    N_("Use the normal view size")
};

static CtkMenu *create_zoom_menu (BaulZoomControl *zoom_control);

static GType baul_zoom_control_accessible_get_type (void);

/* button assignments */
#define CONTEXTUAL_MENU_BUTTON 3

#define NUM_ACTIONS ((int)G_N_ELEMENTS (baul_zoom_control_accessible_action_names))

G_DEFINE_TYPE_WITH_PRIVATE (BaulZoomControl, baul_zoom_control, CTK_TYPE_BOX);

static void
baul_zoom_control_finalize (GObject *object)
{
    g_list_free (BAUL_ZOOM_CONTROL (object)->details->preferred_zoom_levels);

    G_OBJECT_CLASS (baul_zoom_control_parent_class)->finalize (object);
}

static void
zoom_button_clicked (CtkButton *button, BaulZoomControl *zoom_control)
{
    g_signal_emit (zoom_control, signals[ZOOM_TO_DEFAULT], 0);
}

static void
zoom_popup_menu_show (CtkWidget *widget, CdkEventButton *event, BaulZoomControl *zoom_control)
{
    CtkMenu *menu;

    menu = create_zoom_menu (zoom_control);
    ctk_menu_popup_at_widget (menu,
                              widget,
                              CDK_GRAVITY_SOUTH_WEST,
                              CDK_GRAVITY_NORTH_WEST,
                              (const CdkEvent*) event);
}

static void
zoom_popup_menu (CtkWidget *widget, BaulZoomControl *zoom_control)
{
    CtkMenu *menu;

    menu = create_zoom_menu (zoom_control);
    ctk_menu_popup_at_widget (menu,
                              widget,
                              CDK_GRAVITY_SOUTH_WEST,
                              CDK_GRAVITY_NORTH_WEST,
                              NULL);

    ctk_menu_shell_select_first (CTK_MENU_SHELL (menu), FALSE);
}

/* handle button presses */
static gboolean
baul_zoom_control_button_press_event (CtkWidget *widget,
                                      CdkEventButton *event,
                                      BaulZoomControl *zoom_control)
{
    if (event->type != CDK_BUTTON_PRESS)
    {
        return FALSE;
    }

    /* check for the context menu button and show the menu */
    if (event->button == CONTEXTUAL_MENU_BUTTON)
    {
        zoom_popup_menu_show (widget, event, zoom_control);
        return TRUE;
    }
    /* We don't change our state (to reflect the new zoom) here.
       The zoomable will call back with the new level.
       Actually, the callback goes to the viewframe containing the
       zoomable which, in turn, emits zoom_level_changed,
       which someone (e.g. baul_window) picks up and handles by
       calling into is - baul_zoom_control_set_zoom_level.
    */

    return FALSE;
}

static void
zoom_out_clicked (CtkButton *button,
                  BaulZoomControl *zoom_control)
{
    if (baul_zoom_control_can_zoom_out (zoom_control))
    {
        g_signal_emit (G_OBJECT (zoom_control), signals[ZOOM_OUT], 0);
    }
}

static void
zoom_in_clicked (CtkButton *button,
                 BaulZoomControl *zoom_control)
{
    if (baul_zoom_control_can_zoom_in (zoom_control))
    {
        g_signal_emit (G_OBJECT (zoom_control), signals[ZOOM_IN], 0);
    }
}

static void
set_label_size (BaulZoomControl *zoom_control)
{
    const char *text;
    PangoLayout *layout;
    int width;
    int height;

    text = ctk_label_get_text (CTK_LABEL (zoom_control->details->zoom_label));
    layout = ctk_label_get_layout (CTK_LABEL (zoom_control->details->zoom_label));
    pango_layout_set_text (layout, "100%", -1);
    pango_layout_get_pixel_size (layout, &width, &height);
    ctk_widget_set_size_request (zoom_control->details->zoom_label, width, height);
    ctk_label_set_text (CTK_LABEL (zoom_control->details->zoom_label),
                        text);
}

static void
label_style_set_callback (CtkWidget *label,
                          CtkStyleContext *style,
                          gpointer user_data)
{
    set_label_size (BAUL_ZOOM_CONTROL (user_data));
}

static void
baul_zoom_control_init (BaulZoomControl *zoom_control)
{
    CtkWidget *image;
    int i;

    zoom_control->details = baul_zoom_control_get_instance_private (zoom_control);

    zoom_control->details->zoom_level = BAUL_ZOOM_LEVEL_STANDARD;
    zoom_control->details->min_zoom_level = BAUL_ZOOM_LEVEL_SMALLEST;
    zoom_control->details->max_zoom_level = BAUL_ZOOM_LEVEL_LARGEST;
    zoom_control->details->has_min_zoom_level = TRUE;
    zoom_control->details->has_max_zoom_level = TRUE;

    for (i = BAUL_ZOOM_LEVEL_LARGEST; i >= BAUL_ZOOM_LEVEL_SMALLEST; i--)
    {
        zoom_control->details->preferred_zoom_levels = g_list_prepend (
                    zoom_control->details->preferred_zoom_levels,
                    GINT_TO_POINTER (i));
    }

    image = ctk_image_new_from_icon_name ("zoom-out", CTK_ICON_SIZE_MENU);
    zoom_control->details->zoom_out = ctk_button_new ();
    ctk_widget_set_focus_on_click (zoom_control->details->zoom_out, FALSE);
    ctk_button_set_relief (CTK_BUTTON (zoom_control->details->zoom_out),
                           CTK_RELIEF_NONE);
    ctk_widget_set_tooltip_text (zoom_control->details->zoom_out,
                                 _("Decrease the view size"));
    g_signal_connect (G_OBJECT (zoom_control->details->zoom_out),
                      "clicked", G_CALLBACK (zoom_out_clicked),
                      zoom_control);

    ctk_orientable_set_orientation (CTK_ORIENTABLE (zoom_control), CTK_ORIENTATION_HORIZONTAL);

    ctk_container_add (CTK_CONTAINER (zoom_control->details->zoom_out), image);
    ctk_box_pack_start (CTK_BOX (zoom_control),
                        zoom_control->details->zoom_out, FALSE, FALSE, 0);

    zoom_control->details->zoom_button = ctk_button_new ();
    ctk_widget_set_focus_on_click (zoom_control->details->zoom_button, FALSE);
    ctk_button_set_relief (CTK_BUTTON (zoom_control->details->zoom_button),
                           CTK_RELIEF_NONE);
    ctk_widget_set_tooltip_text (zoom_control->details->zoom_button,
                                 _("Use the normal view size"));

    ctk_widget_add_events (CTK_WIDGET (zoom_control->details->zoom_button),
                           CDK_BUTTON_PRESS_MASK
                           | CDK_BUTTON_RELEASE_MASK
                           | CDK_POINTER_MOTION_MASK
                           | CDK_SCROLL_MASK);

    g_signal_connect (G_OBJECT (zoom_control->details->zoom_button),
                      "button-press-event",
                      G_CALLBACK (baul_zoom_control_button_press_event),
                      zoom_control);

    g_signal_connect (G_OBJECT (zoom_control->details->zoom_button),
                      "clicked", G_CALLBACK (zoom_button_clicked),
                      zoom_control);

    g_signal_connect (G_OBJECT (zoom_control->details->zoom_button),
                      "popup-menu", G_CALLBACK (zoom_popup_menu),
                      zoom_control);

    zoom_control->details->zoom_label = ctk_label_new ("100%");
    g_signal_connect (zoom_control->details->zoom_label,
                      "style_set",
                      G_CALLBACK (label_style_set_callback),
                      zoom_control);
    set_label_size (zoom_control);

    ctk_container_add (CTK_CONTAINER (zoom_control->details->zoom_button), zoom_control->details->zoom_label);

    ctk_box_pack_start (CTK_BOX (zoom_control),
                        zoom_control->details->zoom_button, TRUE, TRUE, 0);

    image = ctk_image_new_from_icon_name ("zoom-in", CTK_ICON_SIZE_MENU);
    zoom_control->details->zoom_in = ctk_button_new ();
    ctk_widget_set_focus_on_click (zoom_control->details->zoom_in, FALSE);
    ctk_button_set_relief (CTK_BUTTON (zoom_control->details->zoom_in),
                           CTK_RELIEF_NONE);
    ctk_widget_set_tooltip_text (zoom_control->details->zoom_in,
                                 _("Increase the view size"));
    g_signal_connect (G_OBJECT (zoom_control->details->zoom_in),
                      "clicked", G_CALLBACK (zoom_in_clicked),
                      zoom_control);

    ctk_container_add (CTK_CONTAINER (zoom_control->details->zoom_in), image);
    ctk_box_pack_start (CTK_BOX (zoom_control),
                        zoom_control->details->zoom_in, FALSE, FALSE, 0);

    ctk_widget_show_all (zoom_control->details->zoom_out);
    ctk_widget_show_all (zoom_control->details->zoom_button);
    ctk_widget_show_all (zoom_control->details->zoom_in);
}

/* Allocate a new zoom control */
CtkWidget *
baul_zoom_control_new (void)
{
    return ctk_widget_new (baul_zoom_control_get_type (), NULL);
}

static void
baul_zoom_control_redraw (BaulZoomControl *zoom_control)
{
    int percent;
    char *num_str;

    ctk_widget_set_sensitive (zoom_control->details->zoom_in,
                              baul_zoom_control_can_zoom_in (zoom_control));
    ctk_widget_set_sensitive (zoom_control->details->zoom_out,
                              baul_zoom_control_can_zoom_out (zoom_control));

    percent = floor ((100.0 * baul_get_relative_icon_size_for_zoom_level (zoom_control->details->zoom_level)) + .2);
    num_str = g_strdup_printf ("%d%%", percent);
    ctk_label_set_text (CTK_LABEL (zoom_control->details->zoom_label), num_str);
    g_free (num_str);
}

/* routines to create and handle the zoom menu */

static void
zoom_menu_callback (CtkMenuItem *item, gpointer callback_data)
{
    BaulZoomLevel zoom_level;
    BaulZoomControl *zoom_control;
    gboolean can_zoom;

    zoom_control = BAUL_ZOOM_CONTROL (callback_data);

    /* Don't do anything if we're just setting the toggle state of menu items. */
    if (zoom_control->details->marking_menu_items)
    {
        return;
    }

    /* Don't send the signal if the menuitem was toggled off */
    if (!ctk_check_menu_item_get_active (CTK_CHECK_MENU_ITEM (item)))
    {
        return;
    }

    zoom_level = (BaulZoomLevel) GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "zoom_level"));

    /* Assume we can zoom and then check whether we're right. */
    can_zoom = TRUE;
    if (zoom_control->details->has_min_zoom_level &&
            zoom_level < zoom_control->details->min_zoom_level)
        can_zoom = FALSE; /* no, we're below the minimum zoom level. */
    if (zoom_control->details->has_max_zoom_level &&
            zoom_level > zoom_control->details->max_zoom_level)
        can_zoom = FALSE; /* no, we're beyond the upper zoom level. */

    /* if we can zoom */
    if (can_zoom)
    {
        g_signal_emit (zoom_control, signals[ZOOM_TO_LEVEL], 0, zoom_level);
    }
}

static CtkRadioMenuItem *
create_zoom_menu_item (BaulZoomControl *zoom_control, CtkMenu *menu,
                       BaulZoomLevel zoom_level,
                       CtkRadioMenuItem *previous_radio_item)
{
    CtkWidget *menu_item;
    char *item_text;
    GSList *radio_item_group;
    int percent;

    /* Set flag so that callback isn't activated when set_active called
     * to set toggle state of other radio items.
     */
    zoom_control->details->marking_menu_items = TRUE;

    percent = floor ((100.0 * baul_get_relative_icon_size_for_zoom_level (zoom_level)) + .5);
    item_text = g_strdup_printf ("%d%%", percent);

    radio_item_group = previous_radio_item == NULL
                       ? NULL
                       : ctk_radio_menu_item_get_group (previous_radio_item);
    menu_item = ctk_radio_menu_item_new_with_label (radio_item_group, item_text);
    g_free (item_text);

    ctk_check_menu_item_set_active (CTK_CHECK_MENU_ITEM (menu_item),
                                    zoom_level == zoom_control->details->zoom_level);

    g_object_set_data (G_OBJECT (menu_item), "zoom_level", GINT_TO_POINTER (zoom_level));
    g_signal_connect_object (menu_item, "activate",
                             G_CALLBACK (zoom_menu_callback), zoom_control, 0);

    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (menu), menu_item);

    zoom_control->details->marking_menu_items = FALSE;

    return CTK_RADIO_MENU_ITEM (menu_item);
}

static CtkMenu *
create_zoom_menu (BaulZoomControl *zoom_control)
{
    CtkMenu *menu;
    CtkRadioMenuItem *previous_item;
    GList *node;

    menu = CTK_MENU (ctk_menu_new ());

    previous_item = NULL;
    for (node = zoom_control->details->preferred_zoom_levels; node != NULL; node = node->next)
    {
        previous_item = create_zoom_menu_item
                        (zoom_control, menu, GPOINTER_TO_INT (node->data), previous_item);
    }

    return menu;
}

static void
baul_zoom_control_change_value (BaulZoomControl *zoom_control,
                                CtkScrollType scroll)
{
    switch (scroll)
    {
    case CTK_SCROLL_STEP_DOWN :
        if (baul_zoom_control_can_zoom_out (zoom_control))
        {
            g_signal_emit (zoom_control, signals[ZOOM_OUT], 0);
        }
        break;
    case CTK_SCROLL_STEP_UP :
        if (baul_zoom_control_can_zoom_in (zoom_control))
        {
            g_signal_emit (zoom_control, signals[ZOOM_IN], 0);
        }
        break;
    default :
        g_warning ("Invalid scroll type %d for BaulZoomControl:change_value", scroll);
    }
}

void
baul_zoom_control_set_zoom_level (BaulZoomControl *zoom_control,
                                  BaulZoomLevel zoom_level)
{
    zoom_control->details->zoom_level = zoom_level;
    baul_zoom_control_redraw (zoom_control);
}

void
baul_zoom_control_set_parameters (BaulZoomControl *zoom_control,
                                  BaulZoomLevel min_zoom_level,
                                  BaulZoomLevel max_zoom_level,
                                  gboolean has_min_zoom_level,
                                  gboolean has_max_zoom_level,
                                  GList *zoom_levels)
{
    g_return_if_fail (BAUL_IS_ZOOM_CONTROL (zoom_control));

    zoom_control->details->min_zoom_level = min_zoom_level;
    zoom_control->details->max_zoom_level = max_zoom_level;
    zoom_control->details->has_min_zoom_level = has_min_zoom_level;
    zoom_control->details->has_max_zoom_level = has_max_zoom_level;

    g_list_free (zoom_control->details->preferred_zoom_levels);
    zoom_control->details->preferred_zoom_levels = zoom_levels;

    baul_zoom_control_redraw (zoom_control);
}

BaulZoomLevel
baul_zoom_control_get_zoom_level (BaulZoomControl *zoom_control)
{
    return zoom_control->details->zoom_level;
}

BaulZoomLevel
baul_zoom_control_get_min_zoom_level (BaulZoomControl *zoom_control)
{
    return zoom_control->details->min_zoom_level;
}

BaulZoomLevel
baul_zoom_control_get_max_zoom_level (BaulZoomControl *zoom_control)
{
    return zoom_control->details->max_zoom_level;
}

gboolean
baul_zoom_control_has_min_zoom_level (BaulZoomControl *zoom_control)
{
    return zoom_control->details->has_min_zoom_level;
}

gboolean
baul_zoom_control_has_max_zoom_level (BaulZoomControl *zoom_control)
{
    return zoom_control->details->has_max_zoom_level;
}

gboolean
baul_zoom_control_can_zoom_in (BaulZoomControl *zoom_control)
{
    return !zoom_control->details->has_max_zoom_level ||
           (zoom_control->details->zoom_level
            < zoom_control->details->max_zoom_level);
}

gboolean
baul_zoom_control_can_zoom_out (BaulZoomControl *zoom_control)
{
    return !zoom_control->details->has_min_zoom_level ||
           (zoom_control->details->zoom_level
            > zoom_control->details->min_zoom_level);
}

static gboolean
baul_zoom_control_scroll_event (CtkWidget *widget, CdkEventScroll *event)
{
    BaulZoomControl *zoom_control;

    zoom_control = BAUL_ZOOM_CONTROL (widget);

    if (event->type != CDK_SCROLL)
    {
        return FALSE;
    }

    if (event->direction == CDK_SCROLL_DOWN &&
            baul_zoom_control_can_zoom_out (zoom_control))
    {
        g_signal_emit (widget, signals[ZOOM_OUT], 0);
    }
    else if (event->direction == CDK_SCROLL_UP &&
             baul_zoom_control_can_zoom_in (zoom_control))
    {
        g_signal_emit (widget, signals[ZOOM_IN], 0);
    }

    /* We don't change our state (to reflect the new zoom) here. The zoomable will
     * call back with the new level. Actually, the callback goes to the view-frame
     * containing the zoomable which, in turn, emits zoom_level_changed, which
     * someone (e.g. baul_window) picks up and handles by calling into us -
     * baul_zoom_control_set_zoom_level.
     */
    return TRUE;
}



static void
baul_zoom_control_class_init (BaulZoomControlClass *class)
{
    CtkWidgetClass *widget_class;
    CtkBindingSet *binding_set;

    G_OBJECT_CLASS (class)->finalize = baul_zoom_control_finalize;

    widget_class = CTK_WIDGET_CLASS (class);


    ctk_widget_class_set_accessible_type (widget_class,
                                          baul_zoom_control_accessible_get_type ());

    widget_class->scroll_event = baul_zoom_control_scroll_event;

    class->change_value = baul_zoom_control_change_value;

    signals[ZOOM_IN] =
        g_signal_new ("zoom_in",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulZoomControlClass,
                                       zoom_in),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[ZOOM_OUT] =
        g_signal_new ("zoom_out",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulZoomControlClass,
                                       zoom_out),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[ZOOM_TO_LEVEL] =
        g_signal_new ("zoom_to_level",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulZoomControlClass,
                                       zoom_to_level),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__INT,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_INT);

    signals[ZOOM_TO_DEFAULT] =
        g_signal_new ("zoom_to_default",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (BaulZoomControlClass,
                                       zoom_to_default),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[CHANGE_VALUE] =
        g_signal_new ("change_value",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (BaulZoomControlClass,
                                       change_value),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__ENUM,
                      G_TYPE_NONE, 1, CTK_TYPE_SCROLL_TYPE);

    binding_set = ctk_binding_set_by_class (class);

    ctk_binding_entry_add_signal (binding_set,
				      CDK_KEY_KP_Subtract, 0,
                                  "change_value",
                                  1, CTK_TYPE_SCROLL_TYPE,
                                  CTK_SCROLL_STEP_DOWN);
    ctk_binding_entry_add_signal (binding_set,
				      CDK_KEY_minus, 0,
                                  "change_value",
                                  1, CTK_TYPE_SCROLL_TYPE,
                                  CTK_SCROLL_STEP_DOWN);

    ctk_binding_entry_add_signal (binding_set,
				      CDK_KEY_KP_Equal, 0,
                                  "zoom_to_default",
                                  0);
    ctk_binding_entry_add_signal (binding_set,
				      CDK_KEY_KP_Equal, 0,
                                  "zoom_to_default",
                                  0);

    ctk_binding_entry_add_signal (binding_set,
				      CDK_KEY_KP_Add, 0,
                                  "change_value",
                                  1, CTK_TYPE_SCROLL_TYPE,
                                  CTK_SCROLL_STEP_UP);
    ctk_binding_entry_add_signal (binding_set,
				      CDK_KEY_plus, 0,
                                  "change_value",
                                  1, CTK_TYPE_SCROLL_TYPE,
                                  CTK_SCROLL_STEP_UP);
}

static gboolean
baul_zoom_control_accessible_do_action (AtkAction *accessible, int i)
{
    CtkWidget *widget;

    g_assert (i >= 0 && i < NUM_ACTIONS);

    widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
    if (!widget)
    {
        return FALSE;
    }

    g_signal_emit (widget,
                   signals[baul_zoom_control_accessible_action_signals [i]],
                   0);

    return TRUE;
}

static int
baul_zoom_control_accessible_get_n_actions (AtkAction *accessible)
{

    return NUM_ACTIONS;
}

static const char* baul_zoom_control_accessible_action_get_description(AtkAction* accessible, int i)
{
    g_assert(i >= 0 && i < NUM_ACTIONS);

    return _(baul_zoom_control_accessible_action_descriptions[i]);
}

static const char* baul_zoom_control_accessible_action_get_name(AtkAction* accessible, int i)
{
    g_assert (i >= 0 && i < NUM_ACTIONS);

    return _(baul_zoom_control_accessible_action_names[i]);
}

static void baul_zoom_control_accessible_action_interface_init(AtkActionIface* iface)
{
    iface->do_action = baul_zoom_control_accessible_do_action;
    iface->get_n_actions = baul_zoom_control_accessible_get_n_actions;
    iface->get_description = baul_zoom_control_accessible_action_get_description;
    iface->get_name = baul_zoom_control_accessible_action_get_name;
}

static void
baul_zoom_control_accessible_get_current_value (AtkValue *accessible,
        GValue *value)
{
    BaulZoomControl *control;

    g_value_init (value, G_TYPE_INT);

    control = BAUL_ZOOM_CONTROL (ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible)));
    if (!control)
    {
        g_value_set_int (value, BAUL_ZOOM_LEVEL_STANDARD);
        return;
    }

    g_value_set_int (value, control->details->zoom_level);
}

static void
baul_zoom_control_accessible_get_maximum_value (AtkValue *accessible,
        GValue *value)
{
    BaulZoomControl *control;

    g_value_init (value, G_TYPE_INT);

    control = BAUL_ZOOM_CONTROL (ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible)));
    if (!control)
    {
        g_value_set_int (value, BAUL_ZOOM_LEVEL_STANDARD);
        return;
    }

    g_value_set_int (value, control->details->max_zoom_level);
}

static void
baul_zoom_control_accessible_get_minimum_value (AtkValue *accessible,
        GValue *value)
{
    BaulZoomControl *control;

    g_value_init (value, G_TYPE_INT);

    control = BAUL_ZOOM_CONTROL (ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible)));
    if (!control)
    {
        g_value_set_int (value, BAUL_ZOOM_LEVEL_STANDARD);
        return;
    }

    g_value_set_int (value, control->details->min_zoom_level);
}

static BaulZoomLevel
nearest_preferred (BaulZoomControl *zoom_control, BaulZoomLevel value)
{
    BaulZoomLevel last_value;
    BaulZoomLevel current_value;
    GList *l;

    if (!zoom_control->details->preferred_zoom_levels)
    {
        return value;
    }

    last_value = GPOINTER_TO_INT (zoom_control->details->preferred_zoom_levels->data);
    current_value = last_value;

    for (l = zoom_control->details->preferred_zoom_levels; l != NULL; l = l->next)
    {
        current_value = GPOINTER_TO_INT (l->data);

        if (current_value > value)
        {
            float center = (last_value + current_value) / 2;

            return (value < center) ? last_value : current_value;

        }

        last_value = current_value;
    }

    return current_value;
}

static gboolean
baul_zoom_control_accessible_set_current_value (AtkValue *accessible,
        const GValue *value)
{
    BaulZoomControl *control;
    BaulZoomLevel zoom;

    control = BAUL_ZOOM_CONTROL (ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible)));
    if (!control)
    {
        return FALSE;
    }

    zoom = nearest_preferred (control, g_value_get_int (value));

    g_signal_emit (control, signals[ZOOM_TO_LEVEL], 0, zoom);

    return TRUE;
}

static void
baul_zoom_control_accessible_value_interface_init (AtkValueIface *iface)
{
    iface->get_current_value = baul_zoom_control_accessible_get_current_value;
    iface->get_maximum_value = baul_zoom_control_accessible_get_maximum_value;
    iface->get_minimum_value = baul_zoom_control_accessible_get_minimum_value;
    iface->set_current_value = baul_zoom_control_accessible_set_current_value;
}

static const char* baul_zoom_control_accessible_get_name(AtkObject* accessible)
{
    return _("Zoom");
}

static const char* baul_zoom_control_accessible_get_description(AtkObject* accessible)
{
    return _("Set the zoom level of the current view");
}

static void
baul_zoom_control_accessible_initialize (AtkObject *accessible,
        gpointer  data)
{
    if (ATK_OBJECT_CLASS (accessible_parent_class)->initialize != NULL)
    {
        ATK_OBJECT_CLASS (accessible_parent_class)->initialize (accessible, data);
    }
    atk_object_set_role (accessible, ATK_ROLE_DIAL);
}

typedef struct _BaulZoomControlAccessible BaulZoomControlAccessible;
typedef struct _BaulZoomControlAccessibleClass BaulZoomControlAccessibleClass;

struct _BaulZoomControlAccessible
{
    CtkContainerAccessible parent;
};

struct _BaulZoomControlAccessibleClass
{
    CtkContainerAccessibleClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (BaulZoomControlAccessible,
                         baul_zoom_control_accessible,
                         CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION,
                                                baul_zoom_control_accessible_action_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_VALUE,
                                                baul_zoom_control_accessible_value_interface_init));
static void
baul_zoom_control_accessible_class_init (BaulZoomControlAccessibleClass *klass)
{
    AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);
    accessible_parent_class = g_type_class_peek_parent (klass);

    atk_class->get_name = baul_zoom_control_accessible_get_name;
    atk_class->get_description = baul_zoom_control_accessible_get_description;
    atk_class->initialize = baul_zoom_control_accessible_initialize;
}

static void
baul_zoom_control_accessible_init (BaulZoomControlAccessible *accessible)
{
}

void
baul_zoom_control_set_active_appearance (BaulZoomControl *zoom_control, gboolean is_active)
{
    ctk_widget_set_sensitive (ctk_bin_get_child (CTK_BIN (zoom_control->details->zoom_in)), is_active);
    ctk_widget_set_sensitive (ctk_bin_get_child (CTK_BIN (zoom_control->details->zoom_out)), is_active);
    ctk_widget_set_sensitive (zoom_control->details->zoom_label, is_active);
}
