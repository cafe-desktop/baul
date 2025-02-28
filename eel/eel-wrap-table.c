/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-wrap-box.c - A table that can wrap its contents as needed.

   Copyright (C) 2000 Eazel, Inc.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Cafe Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

#include <config.h>
#include "eel-wrap-table.h"

#include "eel-art-extensions.h"
#include "eel-art-ctk-extensions.h"
#include "eel-ctk-extensions.h"
#include <ctk/ctk.h>

/* Arguments */
enum
{
    PROP_0,
    PROP_X_SPACING,
    PROP_Y_SPACING,
    PROP_X_JUSTIFICATION,
    PROP_Y_JUSTIFICATION,
    PROP_HOMOGENEOUS
};

/* Detail member struct */
struct EelWrapTablePrivate
{
    guint x_spacing;
    guint y_spacing;
    EelJustification x_justification;
    EelJustification y_justification;
    gboolean homogeneous;
    GList *children;

    guint is_scrolled : 1;
    guint cols;
    gboolean drawn;
};

/* Private EelWrapTable methods */
static EelDimensions wrap_table_irect_max_dimensions     (const EelDimensions *one,
        const EelDimensions *two);
static EelDimensions wrap_table_get_max_child_dimensions (const EelWrapTable  *wrap_table);
static EelDimensions wrap_table_get_content_dimensions   (const EelWrapTable  *wrap_table);
static EelIRect      wrap_table_get_content_bounds       (const EelWrapTable  *wrap_table);
static gboolean      wrap_table_child_visible_in         (CtkWidget           *child,
							  CtkWidget           *scrolled);
static gboolean      wrap_table_child_focus_in           (CtkWidget           *widget,
        CdkEventFocus       *event,
        gpointer             data);
static void          wrap_table_layout                   (EelWrapTable        *wrap_table);


G_DEFINE_TYPE_WITH_PRIVATE (EelWrapTable, eel_wrap_table, CTK_TYPE_CONTAINER)

static void
eel_wrap_table_init (EelWrapTable *wrap_table)
{
    ctk_widget_set_has_window (CTK_WIDGET (wrap_table), FALSE);

    wrap_table->details = eel_wrap_table_get_instance_private (wrap_table);
    wrap_table->details->x_justification = EEL_JUSTIFICATION_BEGINNING;
    wrap_table->details->y_justification = EEL_JUSTIFICATION_END;
    wrap_table->details->cols = 1;
}

static void
eel_wrap_table_finalize (GObject *object)
{
    EelWrapTable *wrap_table;

    wrap_table = EEL_WRAP_TABLE (object);

    g_list_free (wrap_table->details->children);

    G_OBJECT_CLASS (eel_wrap_table_parent_class)->finalize (object);
}

/* GObjectClass methods */

static void
eel_wrap_table_set_property (GObject      *object,
			     guint         property_id,
			     const GValue *value,
			     GParamSpec   *pspec G_GNUC_UNUSED)
{
    EelWrapTable *wrap_table;

    g_assert (EEL_IS_WRAP_TABLE (object));

    wrap_table = EEL_WRAP_TABLE (object);

    switch (property_id)
    {
    case PROP_X_SPACING:
        eel_wrap_table_set_x_spacing (wrap_table, g_value_get_uint (value));
        break;

    case PROP_Y_SPACING:
        eel_wrap_table_set_y_spacing (wrap_table, g_value_get_uint (value));
        break;

    case PROP_X_JUSTIFICATION:
        eel_wrap_table_set_x_justification (wrap_table, g_value_get_enum (value));
        break;

    case PROP_Y_JUSTIFICATION:
        eel_wrap_table_set_y_justification (wrap_table, g_value_get_enum (value));
        break;

    case PROP_HOMOGENEOUS:
        eel_wrap_table_set_homogeneous (wrap_table, g_value_get_boolean (value));
        break;

    default:
        g_assert_not_reached ();
    }
}

static void
eel_wrap_table_get_property (GObject    *object,
			     guint       property_id,
			     GValue     *value,
			     GParamSpec *pspec G_GNUC_UNUSED)
{
    EelWrapTable *wrap_table;

    g_assert (EEL_IS_WRAP_TABLE (object));

    wrap_table = EEL_WRAP_TABLE (object);

    switch (property_id)
    {
    case PROP_X_SPACING:
        g_value_set_uint (value, eel_wrap_table_get_x_spacing (wrap_table));
        break;

    case PROP_Y_SPACING:
        g_value_set_uint (value, eel_wrap_table_get_y_spacing (wrap_table));
        break;

    case PROP_X_JUSTIFICATION:
        g_value_set_enum (value, eel_wrap_table_get_x_justification (wrap_table));
        break;

    case PROP_Y_JUSTIFICATION:
        g_value_set_enum (value, eel_wrap_table_get_y_justification (wrap_table));
        break;

    case PROP_HOMOGENEOUS:
        g_value_set_boolean (value, eel_wrap_table_get_homogeneous (wrap_table));
        break;

    default:
        g_assert_not_reached ();
    }
}

/* CtkWidgetClass methods */
static void
eel_wrap_table_size_request (CtkWidget *widget,
                             CtkRequisition *requisition)
{
    EelWrapTable *wrap_table;
    EelDimensions content_dimensions;

    g_assert (EEL_IS_WRAP_TABLE (widget));
    g_assert (requisition != NULL);

    wrap_table = EEL_WRAP_TABLE (widget);

    content_dimensions = wrap_table_get_content_dimensions (wrap_table);

    /* The -1 tells Satan to use as much space as is available */
    requisition->width = -1;
    requisition->height = content_dimensions.height + ctk_container_get_border_width (CTK_CONTAINER (widget)) * 2;
}

static void
eel_wrap_table_get_preferred_width (CtkWidget *widget,
                                    gint *minimum_width,
                                    gint *natural_width)
{
    CtkRequisition req;
    eel_wrap_table_size_request (widget, &req);
    *minimum_width = *natural_width = req.width;
}

static void
eel_wrap_table_get_preferred_height (CtkWidget *widget,
                                     gint *minimum_height,
                                     gint *natural_height)
{
    CtkRequisition req;
    eel_wrap_table_size_request (widget, &req);
    *minimum_height = *natural_height = req.height;
}

static void
eel_wrap_table_size_allocate (CtkWidget *widget,
                              CtkAllocation *allocation)
{
    EelWrapTable *wrap_table;

    g_assert (EEL_IS_WRAP_TABLE (widget));
    g_assert (allocation != NULL);

    wrap_table = EEL_WRAP_TABLE (widget);

    ctk_widget_set_allocation (widget, allocation);

    wrap_table_layout (wrap_table);
}

static int
eel_wrap_table_draw (CtkWidget *widget,
                     cairo_t *cr)
{
    EelWrapTable *wrap_table;
    GList *iterator;

    g_assert (EEL_IS_WRAP_TABLE (widget));
    g_assert (ctk_widget_get_realized (widget));

    wrap_table = EEL_WRAP_TABLE (widget);

    for (iterator = wrap_table->details->children; iterator; iterator = iterator->next)
    {
        g_assert (CTK_IS_WIDGET (iterator->data));
        ctk_container_propagate_draw (CTK_CONTAINER (widget),
                                      CTK_WIDGET (iterator->data),
                                      cr);
    }

    /*Redraw the table once and only once to ensure it is displayed */
    if (wrap_table->details->drawn == FALSE){
        ctk_widget_queue_allocate (CTK_WIDGET(widget));
        wrap_table->details->drawn = TRUE;
        }
    return FALSE;
}

static void
eel_wrap_table_map (CtkWidget *widget)
{
    EelWrapTable *wrap_table;
    GList *iterator;

    g_assert (EEL_IS_WRAP_TABLE (widget));

    wrap_table = EEL_WRAP_TABLE (widget);

    ctk_widget_set_mapped (widget, TRUE);

    for (iterator = wrap_table->details->children; iterator; iterator = iterator->next)
    {
        CtkWidget *item;

        item = iterator->data;

        if (ctk_widget_get_visible (item) && !ctk_widget_get_mapped (item))
        {
            ctk_widget_map (item);
        }
    }
}

static void
eel_wrap_table_unmap (CtkWidget *widget)
{
    EelWrapTable *wrap_table;
    GList *iterator;

    g_assert (EEL_IS_WRAP_TABLE (widget));

    wrap_table = EEL_WRAP_TABLE (widget);

    ctk_widget_set_mapped (widget, FALSE);

    for (iterator = wrap_table->details->children; iterator; iterator = iterator->next)
    {
        CtkWidget *item;

        item = iterator->data;

        if (ctk_widget_get_visible (item) && ctk_widget_get_mapped (item))
        {
            ctk_widget_unmap (item);
        }
    }
}

static void
eel_wrap_table_realize (CtkWidget *widget)
{
    g_assert (EEL_IS_WRAP_TABLE (widget));

    CTK_WIDGET_CLASS (eel_wrap_table_parent_class)->realize (widget);

    ctk_widget_queue_resize (widget);
}

/* CtkContainerClass methods */
static void
eel_wrap_table_add (CtkContainer *container,
                    CtkWidget *child)
{
    EelWrapTable *wrap_table;
    CtkWidget *widget;

    g_assert (container != NULL);
    g_assert (EEL_IS_WRAP_TABLE (container));
    g_assert (CTK_IS_WIDGET (child));

    widget = CTK_WIDGET (container);
    wrap_table = EEL_WRAP_TABLE (container);

    ctk_widget_set_parent (child, CTK_WIDGET (container));

    wrap_table->details->children = g_list_append (wrap_table->details->children, child);

    if (ctk_widget_get_realized (widget))
    {
        ctk_widget_realize (child);
    }

    if (ctk_widget_get_visible (widget) && ctk_widget_get_visible (child))
    {
        if (ctk_widget_get_mapped (widget))
        {
            ctk_widget_map (child);
        }

        ctk_widget_queue_resize (child);
    }

    if (wrap_table->details->is_scrolled)
    {
        g_signal_connect (child, "focus_in_event",
                          G_CALLBACK (wrap_table_child_focus_in),
                          wrap_table);
    }
}

static void
eel_wrap_table_remove (CtkContainer *container,
                       CtkWidget *child)
{
    EelWrapTable *wrap_table;
    gboolean child_was_visible;

    g_assert (EEL_IS_WRAP_TABLE (container));
    g_assert (CTK_IS_WIDGET (child));

    wrap_table = EEL_WRAP_TABLE (container);;

    child_was_visible = ctk_widget_get_visible (child);
    ctk_widget_unparent (child);
    wrap_table->details->children = g_list_remove (wrap_table->details->children, child);

    if (child_was_visible)
    {
        ctk_widget_queue_resize (CTK_WIDGET (container));
    }

    if (wrap_table->details->is_scrolled)
    {
        g_signal_handlers_disconnect_by_func (
            child,
            G_CALLBACK (wrap_table_child_focus_in),
            wrap_table);
    }
}

static void
eel_wrap_table_forall (CtkContainer *container,
		       gboolean      include_internals G_GNUC_UNUSED,
		       CtkCallback   callback,
		       gpointer      callback_data)
{
    EelWrapTable *wrap_table;
    GList *node;
    GList *next;

    g_assert (EEL_IS_WRAP_TABLE (container));
    g_assert (callback != NULL);

    wrap_table = EEL_WRAP_TABLE (container);;

    for (node = wrap_table->details->children; node != NULL; node = next)
    {
        g_assert (CTK_IS_WIDGET (node->data));
        next = node->next;
        (* callback) (CTK_WIDGET (node->data), callback_data);
    }
}

static GType
eel_wrap_table_child_type (CtkContainer *container G_GNUC_UNUSED)
{
    return CTK_TYPE_WIDGET;
}

/* Class init methods */
static void
eel_wrap_table_class_init (EelWrapTableClass *wrap_table_class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (wrap_table_class);
    CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (wrap_table_class);
    CtkContainerClass *container_class = CTK_CONTAINER_CLASS (wrap_table_class);

    /* GObjectClass */
    gobject_class->finalize = eel_wrap_table_finalize;
    gobject_class->set_property = eel_wrap_table_set_property;
    gobject_class->get_property = eel_wrap_table_get_property;

    /* CtkWidgetClass */
    widget_class->size_allocate = eel_wrap_table_size_allocate;
    widget_class->get_preferred_width = eel_wrap_table_get_preferred_width;
    widget_class->get_preferred_height = eel_wrap_table_get_preferred_height;
    widget_class->draw = eel_wrap_table_draw;
    widget_class->map = eel_wrap_table_map;
    widget_class->unmap = eel_wrap_table_unmap;
    widget_class->realize = eel_wrap_table_realize;

    /* CtkContainerClass */
    container_class->add = eel_wrap_table_add;
    container_class->remove = eel_wrap_table_remove;
    container_class->forall = eel_wrap_table_forall;
    container_class->child_type = eel_wrap_table_child_type;

    /* Arguments */
    g_object_class_install_property
    (gobject_class,
     PROP_X_SPACING,
     g_param_spec_uint ("x_spacing", NULL, NULL,
                        0, G_MAXINT, 0, G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_Y_SPACING,
     g_param_spec_uint ("y_spacing", NULL, NULL,
                        0, G_MAXINT, 0, G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_X_JUSTIFICATION,
     g_param_spec_enum ("x_justification", NULL, NULL,
                        CTK_TYPE_JUSTIFICATION,
                        CTK_JUSTIFY_LEFT,
                        G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_Y_JUSTIFICATION,
     g_param_spec_enum ("y_justification", NULL, NULL,
                        CTK_TYPE_JUSTIFICATION,
                        CTK_JUSTIFY_LEFT,
                        G_PARAM_READWRITE));

    g_object_class_install_property
    (gobject_class,
     PROP_HOMOGENEOUS,
     g_param_spec_boolean ("homogeneous", NULL, NULL,
                           FALSE, G_PARAM_READWRITE));

}

/* Private EelWrapTable methods */
static int
wrap_table_get_num_fitting (int available,
                            int spacing,
                            int max_child_size)
{
    int num;

    g_assert (max_child_size > 0);
    g_assert (spacing >= 0);

    available = MAX (available, 0);

    num = (available + spacing) / (max_child_size + spacing);
    num = MAX (num, 1);

    return num;
}

static void
wrap_table_layout (EelWrapTable *wrap_table)
{
    GList *iterator;
    EelIPoint pos;
    EelDimensions max_child_dimensions;
    EelIRect content_bounds;
    guint num_cols;
    CtkAllocation allocation;

    g_assert (EEL_IS_WRAP_TABLE (wrap_table));

    max_child_dimensions = wrap_table_get_max_child_dimensions (wrap_table);
    max_child_dimensions.width = MAX (max_child_dimensions.width, 1);
    max_child_dimensions.height = MAX (max_child_dimensions.height, 1);

    content_bounds = wrap_table_get_content_bounds (wrap_table);
    pos.x = content_bounds.x0;
    pos.y = content_bounds.y0;

    ctk_widget_get_allocation (CTK_WIDGET (wrap_table), &allocation);
    num_cols = wrap_table_get_num_fitting (allocation.width -
                                           ctk_container_get_border_width (CTK_CONTAINER (wrap_table)) * 2,
                                           wrap_table->details->x_spacing,
                                           max_child_dimensions.width);
    if (num_cols != wrap_table->details->cols)
    {
        wrap_table->details->cols = num_cols;
        ctk_widget_queue_resize (CTK_WIDGET (wrap_table));
        return;
    }

    for (iterator = wrap_table->details->children; iterator; iterator = iterator->next)
    {
        CtkWidget *item;

        item = iterator->data;

        if (ctk_widget_get_visible (item))
        {
            CtkAllocation item_allocation;

            if (wrap_table->details->homogeneous)
            {
                item_allocation.x = pos.x;
                item_allocation.y = pos.y;
                item_allocation.width = max_child_dimensions.width;
                item_allocation.height = max_child_dimensions.height;

                if ((pos.x + max_child_dimensions.width) > content_bounds.x1)
                {
                    pos.x = content_bounds.x0 + wrap_table->details->x_spacing + max_child_dimensions.width;
                    pos.y += (max_child_dimensions.height + wrap_table->details->y_spacing);
                    item_allocation.x = content_bounds.x0;
                    item_allocation.y = pos.y;
                }
                else
                {
                    pos.x += (wrap_table->details->x_spacing + max_child_dimensions.width);
                }
            }
            else
            {
                CtkRequisition item_requisition;

                ctk_widget_get_preferred_size (item, &item_requisition, NULL);

                item_allocation.x = pos.x;
                item_allocation.y = pos.y;
                item_allocation.width = item_requisition.width;
                item_allocation.height = item_requisition.height;

                g_assert (item_allocation.width <= max_child_dimensions.width);
                g_assert (item_allocation.height <= max_child_dimensions.height);

                if ((pos.x + max_child_dimensions.width) > content_bounds.x1)
                {
                    pos.x = content_bounds.x0 + wrap_table->details->x_spacing + max_child_dimensions.width;
                    pos.y += (max_child_dimensions.height + wrap_table->details->y_spacing);
                    item_allocation.x = content_bounds.x0;
                    item_allocation.y = pos.y;
                }
                else
                {
                    pos.x += (wrap_table->details->x_spacing + max_child_dimensions.width);
                }

                switch (wrap_table->details->x_justification)
                {
                case EEL_JUSTIFICATION_MIDDLE:
                    item_allocation.x += (max_child_dimensions.width - (int) item_allocation.width) / 2;
                    break;
                case EEL_JUSTIFICATION_END:
                    item_allocation.x += (max_child_dimensions.width - (int) item_allocation.width);
                    break;
                default:
                    break;
                }

                switch (wrap_table->details->y_justification)
                {
                case EEL_JUSTIFICATION_MIDDLE:
                    item_allocation.y += (max_child_dimensions.height - (int) item_allocation.height) / 2;
                    break;
                case EEL_JUSTIFICATION_END:
                    item_allocation.y += (max_child_dimensions.height - (int) item_allocation.height);
                    break;
                default:
                    break;
                }
            }

            ctk_widget_size_allocate (item, &item_allocation);
        }
    }
}

static EelDimensions
wrap_table_irect_max_dimensions (const EelDimensions *one,
                                 const EelDimensions *two)
{
    EelDimensions max_dimensions;

    g_assert (one != NULL);
    g_assert (two != NULL);

    max_dimensions.width = MAX (one->width, two->width);
    max_dimensions.height = MAX (one->height, two->height);

    return max_dimensions;
}

static EelDimensions
wrap_table_get_max_child_dimensions (const EelWrapTable *wrap_table)
{
    EelDimensions max_dimensions;
    GList *iterator;

    g_assert (EEL_IS_WRAP_TABLE (wrap_table));

    max_dimensions = eel_dimensions_empty;

    for (iterator = wrap_table->details->children; iterator; iterator = iterator->next)
    {
        CtkWidget *child;

        child = iterator->data;

        if (ctk_widget_get_visible (child))
        {
            CtkRequisition child_requisition;
            EelDimensions child_dimensions;

            ctk_widget_get_preferred_size (child, &child_requisition, NULL);

            child_dimensions.width = (int) child_requisition.width;
            child_dimensions.height = (int) child_requisition.height;

            max_dimensions = wrap_table_irect_max_dimensions (&child_dimensions, &max_dimensions);
        }
    }

    return max_dimensions;
}

static EelDimensions
wrap_table_get_content_dimensions (const EelWrapTable *wrap_table)
{
    EelDimensions content_dimensions;
    guint num_children;

    g_assert (EEL_IS_WRAP_TABLE (wrap_table));

    content_dimensions = eel_dimensions_empty;

    num_children = g_list_length (wrap_table->details->children);

    if (num_children > 0)
    {
        EelDimensions max_child_dimensions;
        EelDimensions dimensions;
        int num_cols;
        int num_rows;

        dimensions = eel_ctk_widget_get_dimensions (CTK_WIDGET (wrap_table));
        max_child_dimensions = wrap_table_get_max_child_dimensions (wrap_table);

        max_child_dimensions.width = MAX (max_child_dimensions.width, 1);
        max_child_dimensions.height = MAX (max_child_dimensions.height, 1);

        num_cols = wrap_table_get_num_fitting (dimensions.width -
                                               ctk_container_get_border_width (CTK_CONTAINER (wrap_table)) * 2,
                                               wrap_table->details->x_spacing,
                                               max_child_dimensions.width);
        num_rows = num_children / num_cols;
        num_rows = MAX (num_rows, 1);

        if ((num_children % num_rows) > 0)
        {
            num_rows++;
        }

        content_dimensions.width = dimensions.width;
        content_dimensions.height = num_rows * max_child_dimensions.height;

        content_dimensions.width += (num_cols - 1) * wrap_table->details->x_spacing;
        content_dimensions.height += (num_rows - 1) * wrap_table->details->y_spacing;
    }

    return content_dimensions;
}

static EelIRect
wrap_table_get_content_bounds (const EelWrapTable *wrap_table)
{
    EelIRect content_bounds;
    guint border;

    g_assert (EEL_IS_WRAP_TABLE (wrap_table));

    content_bounds = eel_ctk_widget_get_bounds (CTK_WIDGET (wrap_table));

    border = ctk_container_get_border_width (CTK_CONTAINER (wrap_table));
    content_bounds.x0 += border;
    content_bounds.y0 += border;
    content_bounds.x1 -= border;
    content_bounds.y1 -= border;

    return content_bounds;
}

/**
 * wrap_table_child_visible_in
 *
 * Get child position relative to parent, then determine whether
 * the whole child rectangle is visible in the scrolled window
 **/
static gboolean
wrap_table_child_visible_in (CtkWidget *child, CtkWidget *scrolled)
{
    gint x, y;
    CtkAllocation child_alloc, scroll_alloc;

    ctk_widget_translate_coordinates (child, scrolled, 0, 0, &x, &y);
    ctk_widget_get_allocation(child, &child_alloc);
    ctk_widget_get_allocation(scrolled, &scroll_alloc);

    return (x >= 0 && y >= 0)
        && x + child_alloc.width <= scroll_alloc.width
        && y + child_alloc.height <= scroll_alloc.height;
}

static gboolean
wrap_table_child_focus_in (CtkWidget     *widget,
			   CdkEventFocus *event G_GNUC_UNUSED,
			   gpointer       data G_GNUC_UNUSED)
{
    gint x, y;
    CtkWidget *container, *viewport = NULL;

    container = ctk_widget_get_parent (widget);
    if (container)
    {
        viewport = ctk_widget_get_parent (container);
    }
    g_assert (container && viewport);
    g_assert (CTK_IS_VIEWPORT (viewport));
    g_return_val_if_fail (ctk_widget_get_realized (viewport), FALSE);

    if (!wrap_table_child_visible_in (widget, viewport))
    {
        CtkAdjustment *hadj, *vadj;

        hadj = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (viewport));
        vadj = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (viewport));

        ctk_widget_translate_coordinates (widget, container, 0, 0, &x, &y);

        ctk_adjustment_set_value (hadj, MIN (x, ctk_adjustment_get_upper(hadj) - ctk_adjustment_get_page_size(hadj)));
        ctk_adjustment_set_value (vadj, MIN (y, ctk_adjustment_get_upper(vadj) - ctk_adjustment_get_page_size(vadj)));
    }

    return FALSE;
}

/**
 * eel_wrap_table_new:
 *
 */
CtkWidget*
eel_wrap_table_new (gboolean homogeneous)
{
    EelWrapTable *wrap_table;

    wrap_table = EEL_WRAP_TABLE (ctk_widget_new (eel_wrap_table_get_type (), NULL));

    eel_wrap_table_set_homogeneous (wrap_table, homogeneous);

    wrap_table->details->drawn = FALSE;

    return CTK_WIDGET (wrap_table);
}

/**
 * eel_wrap_table_set_x_spacing:
 * @wrap_table: A EelWrapTable.
 * @x_spacing: The new horizontal spacing between wraps.
 *
 */
void
eel_wrap_table_set_x_spacing (EelWrapTable *wrap_table,
                              guint x_spacing)
{
    g_return_if_fail (EEL_IS_WRAP_TABLE (wrap_table));

    if (wrap_table->details->x_spacing == x_spacing)
    {
        return;
    }

    wrap_table->details->x_spacing = x_spacing;

    ctk_widget_queue_resize (CTK_WIDGET (wrap_table));
}

/**
 * eel_wrap_table_get_item_spacing:
 * @wrap_table: A EelWrapTable.
 *
 * Returns: The horizontal spacing between wraps.
 */
guint
eel_wrap_table_get_x_spacing (const EelWrapTable *wrap_table)
{
    g_return_val_if_fail (EEL_IS_WRAP_TABLE (wrap_table), 0);

    return wrap_table->details->x_spacing;
}

/**
 * eel_wrap_table_set_y_spacing:
 * @wrap_table: A EelWrapTable.
 * @y_spacing: The new horizontal spacing between wraps.
 *
 */
void
eel_wrap_table_set_y_spacing (EelWrapTable *wrap_table,
                              guint y_spacing)
{
    g_return_if_fail (EEL_IS_WRAP_TABLE (wrap_table));

    if (wrap_table->details->y_spacing == y_spacing)
    {
        return;
    }

    wrap_table->details->y_spacing = y_spacing;

    ctk_widget_queue_resize (CTK_WIDGET (wrap_table));
}

/**
 * eel_wrap_table_get_item_spacing:
 * @wrap_table: A EelWrapTable.
 *
 * Returns: The horizontal spacing between wraps.
 */
guint
eel_wrap_table_get_y_spacing (const EelWrapTable *wrap_table)
{
    g_return_val_if_fail (EEL_IS_WRAP_TABLE (wrap_table), 0);

    return wrap_table->details->y_spacing;
}


/**
 * eel_wrap_table_find_child_at_event_point:
 * @wrap_table: A EelWrapTable.
 * @x: Event x;
 * @y: Event y;
 *
 * Returns: Child found at given coordinates or NULL of no child is found.
 */
CtkWidget *
eel_wrap_table_find_child_at_event_point (const EelWrapTable *wrap_table,
        int x,
        int y)
{
    GList *iterator;

    g_return_val_if_fail (EEL_IS_WRAP_TABLE (wrap_table), NULL);

    for (iterator = wrap_table->details->children; iterator; iterator = iterator->next)
    {
        CtkWidget *child;

        child = iterator->data;

        if (ctk_widget_get_visible (child))
        {
            EelIRect child_bounds;

            child_bounds = eel_ctk_widget_get_bounds (child);

            if (eel_irect_contains_point (child_bounds, x, y))
            {
                return child;
            }
        }
    }

    return NULL;
}

/**
 * eel_wrap_table_set_x_justification:
 * @wrap_table: A EelWrapTable.
 * @x_justification: The new horizontal justification between wraps.
 *
 */
void
eel_wrap_table_set_x_justification (EelWrapTable *wrap_table,
                                    EelJustification x_justification)
{
    g_return_if_fail (EEL_IS_WRAP_TABLE (wrap_table));
    g_return_if_fail (x_justification >= EEL_JUSTIFICATION_BEGINNING);
    g_return_if_fail (x_justification <= EEL_JUSTIFICATION_END);

    if (wrap_table->details->x_justification == x_justification)
    {
        return;
    }

    wrap_table->details->x_justification = x_justification;
    ctk_widget_queue_resize (CTK_WIDGET (wrap_table));
}

/**
 * eel_wrap_table_get_item_justification:
 * @wrap_table: A EelWrapTable.
 *
 * Returns: The horizontal justification between wraps.
 */
EelJustification
eel_wrap_table_get_x_justification (const EelWrapTable *wrap_table)
{
    g_return_val_if_fail (EEL_IS_WRAP_TABLE (wrap_table), 0);

    return wrap_table->details->x_justification;
}

/**
 * eel_wrap_table_set_y_justification:
 * @wrap_table: A EelWrapTable.
 * @y_justification: The new horizontal justification between wraps.
 *
 */
void
eel_wrap_table_set_y_justification (EelWrapTable *wrap_table,
                                    EelJustification y_justification)
{
    g_return_if_fail (EEL_IS_WRAP_TABLE (wrap_table));
    g_return_if_fail (y_justification >= EEL_JUSTIFICATION_BEGINNING);
    g_return_if_fail (y_justification <= EEL_JUSTIFICATION_END);

    if (wrap_table->details->y_justification == y_justification)
    {
        return;
    }

    wrap_table->details->y_justification = y_justification;
    ctk_widget_queue_resize (CTK_WIDGET (wrap_table));
}

/**
 * eel_wrap_table_get_item_justification:
 * @wrap_table: A EelWrapTable.
 *
 * Returns: The horizontal justification between wraps.
 */
EelJustification
eel_wrap_table_get_y_justification (const EelWrapTable *wrap_table)
{
    g_return_val_if_fail (EEL_IS_WRAP_TABLE (wrap_table), 0);

    return wrap_table->details->y_justification;
}

/**
 * eel_wrap_table_set_homogeneous:
 * @wrap_table: A EelWrapTable.
 * @homogeneous: The new horizontal spacing between wraps.
 *
 */
void
eel_wrap_table_set_homogeneous (EelWrapTable *wrap_table,
                                gboolean homogeneous)
{
    if (EEL_IS_WRAP_TABLE (wrap_table) &&
        wrap_table->details->homogeneous != homogeneous)
    {
        wrap_table->details->homogeneous = homogeneous;
        ctk_widget_queue_resize (CTK_WIDGET (wrap_table));
    }
}

/**
 * eel_wrap_table_get_item_spacing:
 * @wrap_table: A EelWrapTable.
 *
 * Returns: The horizontal spacing between wraps.
 */
gboolean
eel_wrap_table_get_homogeneous (const EelWrapTable *wrap_table)
{
    g_return_val_if_fail (EEL_IS_WRAP_TABLE (wrap_table), FALSE);

    return wrap_table->details->homogeneous;
}

/**
 * eel_wrap_table_reorder_child:
 * @wrap_table: A EelWrapTable.
 * @child: Child to reorder.
 * @position: New position to put child at.
 *
 * Reorder the given chilren into the given position.
 *
 * Position is interpreted as follows:
 *
 *  0 - Place child at start of table.
 * -1 - Place child at end of table.
 *  n - Place child at nth position.  Count starts at 0.
 */
void
eel_wrap_table_reorder_child (EelWrapTable *wrap_table,
                              CtkWidget *child,
                              int position)
{
    GList *node;
    gboolean found_child = FALSE;

    g_return_if_fail (EEL_IS_WRAP_TABLE (wrap_table));
    g_return_if_fail (g_list_length (wrap_table->details->children) > 0);

    if (position == -1)
    {
        position = g_list_length (wrap_table->details->children) - 1;
    }

    g_return_if_fail (position >= 0);
    g_return_if_fail ((guint) position < g_list_length (wrap_table->details->children));

    for (node = wrap_table->details->children; node != NULL; node = node->next)
    {
        CtkWidget *next_child;
        next_child = node->data;

        if (next_child == child)
        {
            g_assert (found_child == FALSE);
            found_child = TRUE;
        }
    }

    g_return_if_fail (found_child);

    wrap_table->details->children = g_list_remove (wrap_table->details->children, child);
    wrap_table->details->children = g_list_insert (wrap_table->details->children, child, position);

    ctk_widget_queue_resize (CTK_WIDGET (wrap_table));
}

/**
 * eel_wrap_table_get_num_children:
 * @wrap_table: A EelWrapTable.
 *
 * Returns: The number of children being managed by the wrap table.
 */
guint
eel_wrap_table_get_num_children (const EelWrapTable *wrap_table)
{
    g_return_val_if_fail (EEL_IS_WRAP_TABLE (wrap_table), 0);

    return g_list_length (wrap_table->details->children);
}

CtkWidget *
eel_scrolled_wrap_table_new (gboolean homogeneous,
                             CtkShadowType shadow_type,
                             CtkWidget **wrap_table_out)
{
    CtkWidget *scrolled_window;
    CtkWidget *wrap_table;
    CtkWidget *viewport;

    g_return_val_if_fail (wrap_table_out != NULL, NULL);

    scrolled_window = ctk_scrolled_window_new (NULL, NULL);
    ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
                                    CTK_POLICY_NEVER,
                                    CTK_POLICY_AUTOMATIC);
    ctk_scrolled_window_set_overlay_scrolling (CTK_SCROLLED_WINDOW (scrolled_window), FALSE);

    viewport = ctk_viewport_new (ctk_scrolled_window_get_hadjustment (CTK_SCROLLED_WINDOW (scrolled_window)),
                                 ctk_scrolled_window_get_vadjustment (CTK_SCROLLED_WINDOW (scrolled_window)));
    ctk_viewport_set_shadow_type (CTK_VIEWPORT (viewport),
                                  shadow_type);

    ctk_container_add (CTK_CONTAINER (scrolled_window),
                       viewport);

    wrap_table = eel_wrap_table_new (homogeneous);
    ctk_container_add (CTK_CONTAINER (viewport),
                       wrap_table);

    ctk_widget_show (wrap_table);
    ctk_widget_show (viewport);

    EEL_WRAP_TABLE (wrap_table)->details->is_scrolled = 1;

    *wrap_table_out = wrap_table;
    return scrolled_window;
}
