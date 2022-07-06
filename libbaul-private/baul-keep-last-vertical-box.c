/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-keep-last-vertical-box.c: Subclass of CtkBox that clips off
 				      items that don't fit, except the last one.

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

   Author: John Sullivan <sullivan@eazel.com>,
 */

#include <config.h>
#include "baul-keep-last-vertical-box.h"

static void	baul_keep_last_vertical_box_size_allocate 	  (CtkWidget 			    *widget,
        CtkAllocation 		    *allocation);

G_DEFINE_TYPE (BaulKeepLastVerticalBox, baul_keep_last_vertical_box, CTK_TYPE_BOX)

#define parent_class baul_keep_last_vertical_box_parent_class

/* Standard class initialization function */
static void
baul_keep_last_vertical_box_class_init (BaulKeepLastVerticalBoxClass *klass)
{
    CtkWidgetClass *widget_class;

    widget_class = (CtkWidgetClass *) klass;

    widget_class->size_allocate = baul_keep_last_vertical_box_size_allocate;
}

/* Standard object initialization function */
static void
baul_keep_last_vertical_box_init (BaulKeepLastVerticalBox *box)
{
    ctk_orientable_set_orientation (CTK_ORIENTABLE (box), CTK_ORIENTATION_VERTICAL);
}


/* baul_keep_last_vertical_box_new:
 *
 * Create a new vertical box that clips off items from the end that don't
 * fit, except the last item, which is always kept. When packing this widget
 * into another vbox, use TRUE for expand and TRUE for fill or this class's
 * special clipping magic won't work because this widget's allocation might
 * be larger than the available space.
 *
 * @spacing: Vertical space between items.
 *
 * Return value: A new BaulKeepLastVerticalBox
 */
CtkWidget *
baul_keep_last_vertical_box_new (gint spacing)
{
    BaulKeepLastVerticalBox *box;

    box = BAUL_KEEP_LAST_VERTICAL_BOX (ctk_widget_new (baul_keep_last_vertical_box_get_type (), NULL));

    ctk_box_set_spacing (CTK_BOX (box), spacing);

    /* If homogeneous is TRUE and there are too many items to fit
     * naturally, they will be squashed together to fit in the space.
     * We want the ones that don't fit to be not shown at all, so
     * we set homogeneous to FALSE.
     */
    ctk_box_set_homogeneous (CTK_BOX (box), FALSE);

    return CTK_WIDGET (box);
}

static void
baul_keep_last_vertical_box_size_allocate (CtkWidget *widget,
        CtkAllocation *allocation)
{
    GList *children, *l;
    CtkAllocation last_child_allocation, child_allocation, tiny_allocation;

    g_return_if_fail (BAUL_IS_KEEP_LAST_VERTICAL_BOX (widget));
    g_return_if_fail (allocation != NULL);

    CTK_WIDGET_CLASS (baul_keep_last_vertical_box_parent_class)->size_allocate (widget, allocation);

    children = ctk_container_get_children (CTK_CONTAINER (widget));
    l = g_list_last (children);

    if (l != NULL)
    {
        CtkWidget *last_child;

        last_child = l->data;
        l = l->prev;

        ctk_widget_get_allocation (last_child, &last_child_allocation);

        /* If last child doesn't fit vertically, prune items from the end of the
         * list one at a time until it does.
         */
        if (last_child_allocation.y + last_child_allocation.height >
                allocation->y + allocation->height)
        {
            CtkWidget *child = NULL;

            while (l != NULL)
            {
                child = l->data;
                l = l->prev;

                ctk_widget_get_allocation (child, &child_allocation);

                /* Reallocate this child's position so that it does not appear.
                 * Setting the width & height to 0 is not enough, as
                 * one pixel is still drawn. Must also move it outside
                 * visible range. For the cases I've seen, -1, -1 works fine.
                 * This might not work in all future cases. Alternatively, the
                 * items that don't fit could be hidden, but that would interfere
                 * with having other hidden children.
                 *
                 * Note that these children are having their size allocated twice,
                 * once by ctk_vbox_size_allocate and then again here. I don't
                 * know of any problems with this, but holler if you do.
                 */
                tiny_allocation.x = tiny_allocation.y = -1;
                tiny_allocation.height = tiny_allocation.width = 0;
                ctk_widget_size_allocate (child, &tiny_allocation);

                /* We're done if the special last item fits now. */
                if (child_allocation.y + last_child_allocation.height <=
                        allocation->y + allocation->height)
                {
                    last_child_allocation.y = child_allocation.y;
                    ctk_widget_size_allocate (last_child, &last_child_allocation);
                    break;
                }

                /* If the special last item still doesn't fit, but we've
                 * run out of earlier items, then the special last item is
                 * just too darn tall. Let's squash it down to fit in the box's
                 * allocation.
                 */
                if (l == NULL)
                {
                    last_child_allocation.y = allocation->y;
                    last_child_allocation.height = allocation->height;
                    ctk_widget_size_allocate (last_child, &last_child_allocation);
                }
            }
        }
    }
    g_list_free (children);
}
