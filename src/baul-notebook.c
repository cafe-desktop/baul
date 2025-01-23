/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Copyright © 2002 Christophe Fergeau
 *  Copyright © 2003, 2004 Marco Pesenti Gritti
 *  Copyright © 2003, 2004, 2005 Christian Persch
 *    (ephy-notebook.c)
 *
 *  Copyright © 2008 Free Software Foundation, Inc.
 *    (baul-notebook.c)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <config.h>

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <ctk/ctk.h>

#include <libbaul-private/baul-dnd.h>

#include "baul-notebook.h"
#include "baul-navigation-window.h"
#include "baul-window-manage-views.h"
#include "baul-window-private.h"
#include "baul-window-slot.h"
#include "baul-navigation-window-pane.h"

#define AFTER_ALL_TABS -1

static int  baul_notebook_insert_page	 (CtkNotebook *notebook,
        CtkWidget *child,
        CtkWidget *tab_label,
        CtkWidget *menu_label,
        int position);
static void baul_notebook_remove	 (CtkContainer *container,
                                      CtkWidget *tab_widget);

enum
{
    TAB_CLOSE_REQUEST,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BaulNotebook, baul_notebook, CTK_TYPE_NOTEBOOK);

static void
baul_notebook_class_init (BaulNotebookClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);
    CtkNotebookClass *notebook_class = CTK_NOTEBOOK_CLASS (klass);

    container_class->remove = baul_notebook_remove;

    notebook_class->insert_page = baul_notebook_insert_page;

    signals[TAB_CLOSE_REQUEST] =
        g_signal_new ("tab-close-request",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulNotebookClass, tab_close_request),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE,
                      1,
                      BAUL_TYPE_WINDOW_SLOT);
}

static gint
find_tab_num_at_pos (BaulNotebook *notebook, gint abs_x, gint abs_y)
{
    CtkPositionType tab_pos;
    int page_num = 0;
    CtkNotebook *nb = CTK_NOTEBOOK (notebook);
    CtkWidget *page;
    CtkAllocation allocation;

    tab_pos = ctk_notebook_get_tab_pos (CTK_NOTEBOOK (notebook));

    while ((page = ctk_notebook_get_nth_page (nb, page_num)))
    {
        CtkWidget *tab;
        gint max_x, max_y;
        gint x_root, y_root;

        tab = ctk_notebook_get_tab_label (nb, page);
        g_return_val_if_fail (tab != NULL, -1);

        if (!ctk_widget_get_mapped (CTK_WIDGET (tab)))
        {
            page_num++;
            continue;
        }

        cdk_window_get_origin (ctk_widget_get_window (tab),
                               &x_root, &y_root);
        ctk_widget_get_allocation (tab, &allocation);

        max_x = x_root + allocation.x + allocation.width;
        max_y = y_root + allocation.y + allocation.height;

        if (((tab_pos == CTK_POS_TOP)
                || (tab_pos == CTK_POS_BOTTOM))
                &&(abs_x<=max_x))
        {
            return page_num;
        }
        else if (((tab_pos == CTK_POS_LEFT)
                  || (tab_pos == CTK_POS_RIGHT))
                 && (abs_y<=max_y))
        {
            return page_num;
        }

        page_num++;
    }
    return AFTER_ALL_TABS;
}

static gboolean
button_press_cb (BaulNotebook   *notebook,
		 CdkEventButton *event,
		 gpointer        data G_GNUC_UNUSED)
{
    int tab_clicked;

    tab_clicked = find_tab_num_at_pos (notebook, event->x_root, event->y_root);

    if (event->type == CDK_BUTTON_PRESS &&
            (event->button == 3 || event->button == 2) &&
            (event->state & ctk_accelerator_get_default_mod_mask ()) == 0)
    {
        if (tab_clicked == -1)
        {
            /* consume event, so that we don't pop up the context menu when
             * the mouse if not over a tab label
             */
            return TRUE;
        }

        /* switch to the page the mouse is over, but don't consume the event */
        ctk_notebook_set_current_page (CTK_NOTEBOOK (notebook), tab_clicked);
    }

    return FALSE;
}

static void
baul_notebook_init (BaulNotebook *notebook)
{
    CtkStyleContext *context;

    context = ctk_widget_get_style_context (CTK_WIDGET (notebook));
    ctk_style_context_add_class (context, "baul-notebook");

    ctk_notebook_set_scrollable (CTK_NOTEBOOK (notebook), TRUE);
    ctk_notebook_set_show_border (CTK_NOTEBOOK (notebook), FALSE);
    ctk_notebook_set_show_tabs (CTK_NOTEBOOK (notebook), FALSE);

    g_signal_connect (notebook, "button-press-event",
                      (GCallback)button_press_cb, NULL);
}

void
baul_notebook_sync_loading (BaulNotebook *notebook,
                            BaulWindowSlot *slot)
{
    CtkWidget *tab_label, *spinner, *icon;
    gboolean active;

    g_return_if_fail (BAUL_IS_NOTEBOOK (notebook));
    g_return_if_fail (BAUL_IS_WINDOW_SLOT (slot));

    tab_label = ctk_notebook_get_tab_label (CTK_NOTEBOOK (notebook), slot->content_box);
    g_return_if_fail (CTK_IS_WIDGET (tab_label));

    spinner = CTK_WIDGET (g_object_get_data (G_OBJECT (tab_label), "spinner"));
    icon = CTK_WIDGET (g_object_get_data (G_OBJECT (tab_label), "icon"));
    g_return_if_fail (spinner != NULL && icon != NULL);

    active = FALSE;
    g_object_get (spinner, "active", &active, NULL);
    if (active == slot->allow_stop)
    {
        return;
    }

    if (slot->allow_stop)
    {
        ctk_widget_hide (icon);
        ctk_widget_show (spinner);
        ctk_spinner_start (CTK_SPINNER (spinner));
    }
    else
    {
        ctk_spinner_stop (CTK_SPINNER (spinner));
        ctk_widget_hide (spinner);
        ctk_widget_show (icon);
    }
}

void
baul_notebook_sync_tab_label (BaulNotebook *notebook,
                              BaulWindowSlot *slot)
{
    CtkWidget *hbox, *label;

    g_return_if_fail (BAUL_IS_NOTEBOOK (notebook));
    g_return_if_fail (BAUL_IS_WINDOW_SLOT (slot));
    g_return_if_fail (CTK_IS_WIDGET (slot->content_box));

    hbox = ctk_notebook_get_tab_label (CTK_NOTEBOOK (notebook), slot->content_box);
    g_return_if_fail (CTK_IS_WIDGET (hbox));

    label = CTK_WIDGET (g_object_get_data (G_OBJECT (hbox), "label"));
    g_return_if_fail (CTK_IS_WIDGET (label));

    ctk_label_set_text (CTK_LABEL (label), slot->title);

    if (slot->location != NULL)
    {
        char *location_name;

        /* Set the tooltip on the label's parent (the tab label hbox),
         * so it covers all of the tab label.
         */
        location_name = g_file_get_parse_name (slot->location);
        ctk_widget_set_tooltip_text (ctk_widget_get_parent (label), location_name);
        g_free (location_name);
    }
    else
    {
        ctk_widget_set_tooltip_text (ctk_widget_get_parent (label), NULL);
    }
}

static void
close_button_clicked_cb (CtkWidget      *widget G_GNUC_UNUSED,
			 BaulWindowSlot *slot)
{
    CtkWidget *notebook;

    notebook = ctk_widget_get_ancestor (slot->content_box, BAUL_TYPE_NOTEBOOK);
    if (notebook != NULL)
    {
        g_signal_emit (notebook, signals[TAB_CLOSE_REQUEST], 0, slot);
    }
}

static CtkWidget *
build_tab_label (BaulNotebook   *nb G_GNUC_UNUSED,
		 BaulWindowSlot *slot)
{
    BaulDragSlotProxyInfo *drag_info;
    CtkWidget *hbox, *label, *close_button, *image, *spinner, *icon;

    /* set hbox spacing and label padding (see below) so that there's an
     * equal amount of space around the label */
    hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 4);
    ctk_widget_show (hbox);

    /* setup load feedback */
    spinner = ctk_spinner_new ();
    ctk_box_pack_start (CTK_BOX (hbox), spinner, FALSE, FALSE, 0);

    /* setup site icon, empty by default */
    icon = ctk_image_new ();
    ctk_box_pack_start (CTK_BOX (hbox), icon, FALSE, FALSE, 0);
    /* don't show the icon */

    /* setup label */
    label = ctk_label_new (NULL);
    ctk_label_set_ellipsize (CTK_LABEL (label), PANGO_ELLIPSIZE_END);
    ctk_label_set_single_line_mode (CTK_LABEL (label), TRUE);
    ctk_label_set_xalign (CTK_LABEL (label), 0.0);
    ctk_label_set_yalign (CTK_LABEL (label), 0.5);

    ctk_widget_set_margin_start (label, 0);
    ctk_widget_set_margin_end (label, 0);
    ctk_widget_set_margin_top (label, 0);
    ctk_widget_set_margin_bottom (label, 0);

    ctk_box_pack_start (CTK_BOX (hbox), label, TRUE, TRUE, 0);
    ctk_widget_show (label);

    /* setup close button */
    close_button = ctk_button_new ();
    ctk_button_set_relief (CTK_BUTTON (close_button),
                           CTK_RELIEF_NONE);
    /* don't allow focus on the close button */
    ctk_widget_set_focus_on_click (close_button, FALSE);

    ctk_widget_set_name (close_button, "baul-tab-close-button");

    image = ctk_image_new_from_icon_name ("window-close", CTK_ICON_SIZE_MENU);
    ctk_widget_set_tooltip_text (close_button, _("Close tab"));
    g_signal_connect_object (close_button, "clicked",
                             G_CALLBACK (close_button_clicked_cb), slot, 0);

    ctk_container_add (CTK_CONTAINER (close_button), image);
    ctk_widget_show (image);

    ctk_box_pack_start (CTK_BOX (hbox), close_button, FALSE, FALSE, 0);
    ctk_widget_show (close_button);

    drag_info = g_new0 (BaulDragSlotProxyInfo, 1);
    drag_info->target_slot = slot;
    g_object_set_data_full (G_OBJECT (hbox), "proxy-drag-info",
                            drag_info, (GDestroyNotify) g_free);

    baul_drag_slot_proxy_init (hbox, drag_info);

    g_object_set_data (G_OBJECT (hbox), "label", label);
    g_object_set_data (G_OBJECT (hbox), "spinner", spinner);
    g_object_set_data (G_OBJECT (hbox), "icon", icon);
    g_object_set_data (G_OBJECT (hbox), "close-button", close_button);

    return hbox;
}

static int
baul_notebook_insert_page (CtkNotebook *gnotebook,
                           CtkWidget *tab_widget,
                           CtkWidget *tab_label,
                           CtkWidget *menu_label,
                           int position)
{
    g_assert (CTK_IS_WIDGET (tab_widget));

    position = CTK_NOTEBOOK_CLASS (baul_notebook_parent_class)->insert_page (gnotebook,
               tab_widget,
               tab_label,
               menu_label,
               position);

    ctk_notebook_set_show_tabs (gnotebook,
                                ctk_notebook_get_n_pages (gnotebook) > 1);
    ctk_notebook_set_tab_reorderable (gnotebook, tab_widget, TRUE);

    return position;
}

int
baul_notebook_add_tab (BaulNotebook *notebook,
                       BaulWindowSlot *slot,
                       int position,
                       gboolean jump_to)
{
    CtkNotebook *gnotebook = CTK_NOTEBOOK (notebook);
    CtkWidget *tab_label;

    g_return_val_if_fail (BAUL_IS_NOTEBOOK (notebook), -1);
    g_return_val_if_fail (BAUL_IS_WINDOW_SLOT (slot), -1);

    tab_label = build_tab_label (notebook, slot);

    position = ctk_notebook_insert_page (CTK_NOTEBOOK (notebook),
                                         slot->content_box,
                                         tab_label,
                                         position);

    ctk_container_child_set (CTK_CONTAINER (notebook),
                             slot->content_box,
                             "tab-expand", TRUE,
                             NULL);

    baul_notebook_sync_tab_label (notebook, slot);
    baul_notebook_sync_loading (notebook, slot);


    /* FIXME ctk bug! */
    /* FIXME: this should be fixed in ctk 2.12; check & remove this! */
    /* The signal handler may have reordered the tabs */
    position = ctk_notebook_page_num (gnotebook, slot->content_box);

    if (jump_to)
    {
        ctk_notebook_set_current_page (gnotebook, position);

    }

    return position;
}

static void
baul_notebook_remove (CtkContainer *container,
                      CtkWidget *tab_widget)
{
    CtkNotebook *gnotebook = CTK_NOTEBOOK (container);
    CTK_CONTAINER_CLASS (baul_notebook_parent_class)->remove (container, tab_widget);

    ctk_notebook_set_show_tabs (gnotebook,
                                ctk_notebook_get_n_pages (gnotebook) > 1);

}

void
baul_notebook_reorder_current_child_relative (BaulNotebook *notebook,
        int offset)
{
    CtkNotebook *gnotebook;
    CtkWidget *child;
    int page;

    g_return_if_fail (BAUL_IS_NOTEBOOK (notebook));

    if (!baul_notebook_can_reorder_current_child_relative (notebook, offset))
    {
        return;
    }

    gnotebook = CTK_NOTEBOOK (notebook);

    page = ctk_notebook_get_current_page (gnotebook);
    child = ctk_notebook_get_nth_page (gnotebook, page);
    ctk_notebook_reorder_child (gnotebook, child, page + offset);
}

void
baul_notebook_set_current_page_relative (BaulNotebook *notebook,
        int offset)
{
    CtkNotebook *gnotebook;
    int page;

    g_return_if_fail (BAUL_IS_NOTEBOOK (notebook));

    if (!baul_notebook_can_set_current_page_relative (notebook, offset))
    {
        return;
    }

    gnotebook = CTK_NOTEBOOK (notebook);

    page = ctk_notebook_get_current_page (gnotebook);
    ctk_notebook_set_current_page (gnotebook, page + offset);

}

static gboolean
baul_notebook_is_valid_relative_position (BaulNotebook *notebook,
        int offset)
{
    CtkNotebook *gnotebook;
    int page;
    int n_pages;

    gnotebook = CTK_NOTEBOOK (notebook);

    page = ctk_notebook_get_current_page (gnotebook);
    n_pages = ctk_notebook_get_n_pages (gnotebook) - 1;
    if (page < 0 ||
            (offset < 0 && page < -offset) ||
            (offset > 0 && page > n_pages - offset))
    {
        return FALSE;
    }

    return TRUE;
}

gboolean
baul_notebook_can_reorder_current_child_relative (BaulNotebook *notebook,
        int offset)
{
    g_return_val_if_fail (BAUL_IS_NOTEBOOK (notebook), FALSE);

    return baul_notebook_is_valid_relative_position (notebook, offset);
}

gboolean
baul_notebook_can_set_current_page_relative (BaulNotebook *notebook,
        int offset)
{
    g_return_val_if_fail (BAUL_IS_NOTEBOOK (notebook), FALSE);

    return baul_notebook_is_valid_relative_position (notebook, offset);
}

