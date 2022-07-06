/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*  baul-side-pane.c
 *
 *  Copyright (C) 2002 Ximian Inc.
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
 *  Author: Dave Camp <dave@ximian.com>
 */

#include <config.h>

#include <cdk/cdkkeysyms.h>
#include <ctk/ctk.h>
#include <glib/gi18n.h>

#include <eel/eel-glib-extensions.h>
#include <eel/eel-ctk-extensions.h>
#include <eel/eel-ctk-macros.h>

#include "baul-side-pane.h"

typedef struct
{
    char *title;
    char *tooltip;
    CtkWidget *widget;
    CtkWidget *menu_item;
    CtkWidget *shortcut;
} SidePanel;

struct _BaulSidePanePrivate
{
    CtkWidget *notebook;
    CtkWidget *menu;

    CtkWidget *title_hbox;
    CtkWidget *title_label;
    CtkWidget *shortcut_box;
    GList *panels;
};

static void baul_side_pane_dispose    (GObject *object);
static void baul_side_pane_finalize   (GObject *object);

enum
{
    CLOSE_REQUESTED,
    SWITCH_PAGE,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (BaulSidePane, baul_side_pane, CTK_TYPE_BOX)

static SidePanel *
panel_for_widget (BaulSidePane *side_pane, CtkWidget *widget)
{
    GList *l;
    SidePanel *panel = NULL;

    for (l = side_pane->details->panels; l != NULL; l = l->next)
    {
        panel = l->data;
        if (panel->widget == widget)
        {
            return panel;
        }
    }

    return NULL;
}

static void
side_panel_free (SidePanel *panel)
{
    g_free (panel->title);
    g_free (panel->tooltip);
    g_slice_free (SidePanel, panel);
}

static void
switch_page_callback (CtkWidget *notebook,
                      CtkWidget *page,
                      guint page_num,
                      gpointer user_data)
{
    BaulSidePane *side_pane;
    SidePanel *panel;

    side_pane = BAUL_SIDE_PANE (user_data);

    panel = panel_for_widget (side_pane,
                              ctk_notebook_get_nth_page (CTK_NOTEBOOK (side_pane->details->notebook),
                                      page_num));

    if (panel && side_pane->details->title_label)
    {
        ctk_label_set_text (CTK_LABEL (side_pane->details->title_label),
                            panel->title);
    }

    g_signal_emit (side_pane, signals[SWITCH_PAGE], 0,
                   panel ? panel->widget : NULL);
}

static void
select_panel (BaulSidePane *side_pane, SidePanel *panel)
{
    int page_num;

    page_num = ctk_notebook_page_num
               (CTK_NOTEBOOK (side_pane->details->notebook), panel->widget);
    ctk_notebook_set_current_page
    (CTK_NOTEBOOK (side_pane->details->notebook), page_num);
}

/* initializing the class object by installing the operations we override */
static void
baul_side_pane_class_init (BaulSidePaneClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = baul_side_pane_finalize;
    gobject_class->dispose = baul_side_pane_dispose;

    signals[CLOSE_REQUESTED] = g_signal_new
                               ("close_requested",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_LAST,
                                G_STRUCT_OFFSET (BaulSidePaneClass,
                                        close_requested),
                                NULL, NULL,
                                g_cclosure_marshal_VOID__VOID,
                                G_TYPE_NONE, 0);
    signals[SWITCH_PAGE] = g_signal_new
                           ("switch_page",
                            G_TYPE_FROM_CLASS (klass),
                            G_SIGNAL_RUN_LAST,
                            G_STRUCT_OFFSET (BaulSidePaneClass,
                                    switch_page),
                            NULL, NULL,
                            g_cclosure_marshal_VOID__OBJECT,
                            G_TYPE_NONE, 1, CTK_TYPE_WIDGET);
}

static void
panel_item_activate_callback (CtkMenuItem *item,
                              gpointer user_data)
{
    BaulSidePane *side_pane;
    SidePanel *panel;

    side_pane = BAUL_SIDE_PANE (user_data);

    panel = g_object_get_data (G_OBJECT (item), "panel-item");

    select_panel (side_pane, panel);
}

static gboolean
select_button_press_callback (CtkWidget *widget,
                              CdkEventButton *event,
                              gpointer user_data)
{
    BaulSidePane *side_pane;

    side_pane = BAUL_SIDE_PANE (user_data);

    if ((event->type == CDK_BUTTON_PRESS) && event->button == 1)
    {
        CtkRequisition requisition;
        CtkAllocation allocation;
        gint width;

        ctk_widget_get_allocation (widget, &allocation);
        width = allocation.width;
        ctk_widget_set_size_request (side_pane->details->menu, -1, -1);
        ctk_widget_get_preferred_size (side_pane->details->menu, &requisition, NULL);
        ctk_widget_set_size_request (side_pane->details->menu,
                                     MAX (width, requisition.width), -1);

        ctk_widget_grab_focus (widget);

        ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), TRUE);
        ctk_menu_popup_at_widget (CTK_MENU (side_pane->details->menu),
                                  widget,
                                  CDK_GRAVITY_SOUTH_WEST,
                                  CDK_GRAVITY_NORTH_WEST,
                                  (const CdkEvent*) event);

        return TRUE;
    }
    return FALSE;
}

static gboolean
select_button_key_press_callback (CtkWidget *widget,
                                  CdkEventKey *event,
                                  gpointer user_data)
{
    BaulSidePane *side_pane;

    side_pane = BAUL_SIDE_PANE (user_data);

    if (event->keyval == CDK_KEY_space ||
        event->keyval == CDK_KEY_KP_Space ||
        event->keyval == CDK_KEY_Return ||
        event->keyval == CDK_KEY_KP_Enter)
    {
        ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (widget), TRUE);
        ctk_menu_popup_at_widget (CTK_MENU (side_pane->details->menu),
                                  widget,
                                  CDK_GRAVITY_SOUTH_WEST,
                                  CDK_GRAVITY_NORTH_WEST,
                                  (const CdkEvent*) event);
        return TRUE;
    }

    return FALSE;
}

static void
close_clicked_callback (CtkWidget *widget,
                        gpointer user_data)
{
    BaulSidePane *side_pane;

    side_pane = BAUL_SIDE_PANE (user_data);

    g_signal_emit (side_pane, signals[CLOSE_REQUESTED], 0);
}

static void
menu_deactivate_callback (CtkWidget *widget,
                          gpointer user_data)
{
    CtkWidget *menu_button;

    menu_button = CTK_WIDGET (user_data);

    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (menu_button), FALSE);
}

static void
menu_detach_callback (CtkWidget *widget,
                      CtkMenu *menu)
{
    BaulSidePane *side_pane;

    side_pane = BAUL_SIDE_PANE (widget);

    side_pane->details->menu = NULL;
}

static void
baul_side_pane_init (BaulSidePane *side_pane)
{
    CtkWidget *hbox;
    CtkWidget *close_button;
    CtkWidget *select_button;
    CtkWidget *select_hbox;
    CtkWidget *arrow;
    CtkWidget *image;

    side_pane->details = baul_side_pane_get_instance_private (side_pane);

    CtkStyleContext *context;

    context = ctk_widget_get_style_context (CTK_WIDGET (side_pane));
    ctk_style_context_add_class (context, "baul-side-pane");

    hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
    ctk_container_set_border_width (CTK_CONTAINER (hbox), 4);
    side_pane->details->title_hbox = hbox;
    ctk_widget_show (hbox);
    ctk_orientable_set_orientation (CTK_ORIENTABLE (side_pane), CTK_ORIENTATION_VERTICAL);
    ctk_box_pack_start (CTK_BOX (side_pane), hbox, FALSE, FALSE, 0);

    select_button = ctk_toggle_button_new ();
    ctk_button_set_relief (CTK_BUTTON (select_button), CTK_RELIEF_NONE);
    ctk_widget_show (select_button);

    g_signal_connect (select_button,
                      "button_press_event",
                      G_CALLBACK (select_button_press_callback),
                      side_pane);
    g_signal_connect (select_button,
                      "key_press_event",
                      G_CALLBACK (select_button_key_press_callback),
                      side_pane);

    select_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
    ctk_widget_show (select_hbox);

    side_pane->details->title_label = ctk_label_new ("");
    eel_add_weak_pointer (&side_pane->details->title_label);

    ctk_widget_show (side_pane->details->title_label);
    ctk_box_pack_start (CTK_BOX (select_hbox),
                        side_pane->details->title_label,
                        FALSE, FALSE, 0);

    arrow = ctk_image_new_from_icon_name ("pan-down-symbolic", CTK_ICON_SIZE_BUTTON);
    ctk_widget_show (arrow);
    ctk_box_pack_end (CTK_BOX (select_hbox), arrow, FALSE, FALSE, 0);

    ctk_container_add (CTK_CONTAINER (select_button), select_hbox);
    ctk_box_pack_start (CTK_BOX (hbox), select_button, TRUE, TRUE, 0);

    close_button = ctk_button_new ();
    ctk_button_set_relief (CTK_BUTTON (close_button), CTK_RELIEF_NONE);
    g_signal_connect (close_button,
                      "clicked",
                      G_CALLBACK (close_clicked_callback),
                      side_pane);

    ctk_widget_show (close_button);

    image = ctk_image_new_from_icon_name ("window-close",
                                      CTK_ICON_SIZE_MENU);
    ctk_widget_show (image);

    ctk_container_add (CTK_CONTAINER (close_button), image);

    ctk_box_pack_end (CTK_BOX (hbox), close_button, FALSE, FALSE, 0);

    side_pane->details->shortcut_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
    ctk_widget_show (side_pane->details->shortcut_box);
    ctk_box_pack_end (CTK_BOX (hbox),
                      side_pane->details->shortcut_box,
                      FALSE, FALSE, 0);

    side_pane->details->notebook = ctk_notebook_new ();
    ctk_notebook_set_show_tabs (CTK_NOTEBOOK (side_pane->details->notebook),
                                FALSE);
    ctk_notebook_set_show_border (CTK_NOTEBOOK (side_pane->details->notebook),
                                  FALSE);
    g_signal_connect_object (side_pane->details->notebook,
                             "switch_page",
                             G_CALLBACK (switch_page_callback),
                             side_pane,
                             0);

    ctk_widget_show (side_pane->details->notebook);

    ctk_box_pack_start (CTK_BOX (side_pane), side_pane->details->notebook,
                        TRUE, TRUE, 0);

    side_pane->details->menu = ctk_menu_new ();

    ctk_menu_set_reserve_toggle_size (CTK_MENU (side_pane->details->menu), FALSE);

    g_signal_connect (side_pane->details->menu,
                      "deactivate",
                      G_CALLBACK (menu_deactivate_callback),
                      select_button);
    ctk_menu_attach_to_widget (CTK_MENU (side_pane->details->menu),
                               CTK_WIDGET (side_pane),
                               menu_detach_callback);

    ctk_widget_show (side_pane->details->menu);

    ctk_widget_set_tooltip_text (close_button,
                                 _("Close the side pane"));
}

static void
baul_side_pane_dispose (GObject *object)
{
    BaulSidePane *side_pane;

    side_pane = BAUL_SIDE_PANE (object);

    if (side_pane->details->menu)
    {
        ctk_menu_detach (CTK_MENU (side_pane->details->menu));
        side_pane->details->menu = NULL;
    }

    G_OBJECT_CLASS (baul_side_pane_parent_class)->dispose (object);
}

static void
baul_side_pane_finalize (GObject *object)
{
    BaulSidePane *side_pane;
    GList *l;

    side_pane = BAUL_SIDE_PANE (object);

    for (l = side_pane->details->panels; l != NULL; l = l->next)
    {
        side_panel_free (l->data);
    }

    g_list_free (side_pane->details->panels);

    G_OBJECT_CLASS (baul_side_pane_parent_class)->finalize (object);
}

BaulSidePane *
baul_side_pane_new (void)
{
    return BAUL_SIDE_PANE (ctk_widget_new (baul_side_pane_get_type (), NULL));
}

void
baul_side_pane_add_panel (BaulSidePane *side_pane,
                          CtkWidget *widget,
                          const char *title,
                          const char *tooltip)
{
    SidePanel *panel;

    g_return_if_fail (side_pane != NULL);
    g_return_if_fail (BAUL_IS_SIDE_PANE (side_pane));
    g_return_if_fail (widget != NULL);
    g_return_if_fail (CTK_IS_WIDGET (widget));
    g_return_if_fail (title != NULL);
    g_return_if_fail (tooltip != NULL);

    panel = g_slice_new0 (SidePanel);
    panel->title = g_strdup (title);
    panel->tooltip = g_strdup (tooltip);
    panel->widget = widget;

    ctk_widget_show (widget);

    panel->menu_item = eel_image_menu_item_new_from_icon (NULL, title);

    ctk_widget_show (panel->menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (side_pane->details->menu),
                           panel->menu_item);
    g_object_set_data (G_OBJECT (panel->menu_item), "panel-item", panel);

    g_signal_connect (panel->menu_item,
                      "activate",
                      G_CALLBACK (panel_item_activate_callback),
                      side_pane);

    side_pane->details->panels = g_list_append (side_pane->details->panels,
                                 panel);

    ctk_notebook_append_page (CTK_NOTEBOOK (side_pane->details->notebook),
                              widget,
                              NULL);
}

void
baul_side_pane_remove_panel (BaulSidePane *side_pane,
                             CtkWidget *widget)
{
    SidePanel *panel;

    g_return_if_fail (side_pane != NULL);
    g_return_if_fail (BAUL_IS_SIDE_PANE (side_pane));
    g_return_if_fail (widget != NULL);
    g_return_if_fail (CTK_IS_WIDGET (widget));

    panel = panel_for_widget (side_pane, widget);

    g_return_if_fail (panel != NULL);

    if (panel)
    {
        int page_num;

        page_num = ctk_notebook_page_num (CTK_NOTEBOOK (side_pane->details->notebook),
                                          widget);
        ctk_notebook_remove_page (CTK_NOTEBOOK (side_pane->details->notebook),
                                  page_num);
        ctk_container_remove (CTK_CONTAINER (side_pane->details->menu),
                              panel->menu_item);

        side_pane->details->panels =
            g_list_remove (side_pane->details->panels,
                           panel);

        side_panel_free (panel);
    }
}

void
baul_side_pane_show_panel (BaulSidePane *side_pane,
                           CtkWidget        *widget)
{
    SidePanel *panel;
    int page_num;

    g_return_if_fail (side_pane != NULL);
    g_return_if_fail (BAUL_IS_SIDE_PANE (side_pane));
    g_return_if_fail (widget != NULL);
    g_return_if_fail (CTK_IS_WIDGET (widget));

    panel = panel_for_widget (side_pane, widget);

    g_return_if_fail (panel != NULL);

    page_num = ctk_notebook_page_num (CTK_NOTEBOOK (side_pane->details->notebook),
                                      widget);
    ctk_notebook_set_current_page (CTK_NOTEBOOK (side_pane->details->notebook),
                                   page_num);
}


static void
shortcut_clicked_callback (CtkWidget *button,
                           gpointer user_data)
{
    BaulSidePane *side_pane;
    CtkWidget *page;

    side_pane = BAUL_SIDE_PANE (user_data);

    page = CTK_WIDGET (g_object_get_data (G_OBJECT (button), "side-page"));

    baul_side_pane_show_panel (side_pane, page);
}

static CtkWidget *
create_shortcut (BaulSidePane *side_pane,
                 SidePanel *panel,
                 GdkPixbuf *pixbuf)
{
    CtkWidget *button;
    CtkWidget *image;

    button = ctk_button_new ();
    ctk_button_set_relief (CTK_BUTTON (button), CTK_RELIEF_NONE);

    g_object_set_data (G_OBJECT (button), "side-page", panel->widget);
    g_signal_connect (button, "clicked",
                      G_CALLBACK (shortcut_clicked_callback), side_pane);

    ctk_widget_set_tooltip_text (button, panel->tooltip);

    image = ctk_image_new_from_pixbuf (pixbuf);
    ctk_widget_show (image);
    ctk_container_add (CTK_CONTAINER (button), image);

    return button;
}

void
baul_side_pane_set_panel_image (BaulSidePane *side_pane,
                                CtkWidget *widget,
                                GdkPixbuf *pixbuf)
{
    SidePanel *panel;
    CtkWidget *image;

    g_return_if_fail (side_pane != NULL);
    g_return_if_fail (BAUL_IS_SIDE_PANE (side_pane));
    g_return_if_fail (widget != NULL);
    g_return_if_fail (CTK_IS_WIDGET (widget));
    g_return_if_fail (pixbuf == NULL || CDK_IS_PIXBUF (pixbuf));

    panel = panel_for_widget (side_pane, widget);

    g_return_if_fail (panel != NULL);

    if (pixbuf)
    {
        image = ctk_image_new_from_pixbuf (pixbuf);
        ctk_widget_show (image);
    }
    else
    {
        image = NULL;
    }

    if (panel->shortcut)
    {
        ctk_widget_destroy (panel->shortcut);
        panel->shortcut = NULL;
    }

    if (pixbuf)
    {
        panel->shortcut = create_shortcut (side_pane, panel, pixbuf);
        ctk_widget_show (panel->shortcut);
        ctk_box_pack_start (CTK_BOX (side_pane->details->shortcut_box),
                            panel->shortcut,
                            FALSE, FALSE, 0);
    }
}

CtkWidget *
baul_side_pane_get_current_panel (BaulSidePane *side_pane)
{
    int index;

    index = ctk_notebook_get_current_page (CTK_NOTEBOOK (side_pane->details->notebook));
    return ctk_notebook_get_nth_page (CTK_NOTEBOOK (side_pane->details->notebook), index);
}

CtkWidget *
baul_side_pane_get_title (BaulSidePane *side_pane)
{
    return side_pane->details->title_hbox;
}
