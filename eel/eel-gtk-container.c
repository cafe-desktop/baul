/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-ctk-container.c - Functions to simplify the implementations of
  			 CtkContainer widgets.

   Copyright (C) 2001 Ramiro Estrugo.

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
#include "eel-ctk-container.h"
#include "eel-art-extensions.h"

/**
 * eel_ctk_container_child_expose_event:
 *
 * @container: A CtkContainer widget.
 * @child: A child of @container or NULL;
 * @event: The expose event.
 *
 * Forward an expose event to a child if needed.  It is valid to give a NULL @child.
 * In that case this function is a noop.  Proper clipping is done to ensure that the @child
 * does indeed need to be forwarded the exposure event.  Finally, the forwarding
 * only occurs if the child is a NO_WINDOW widget.  Of course, it is valid to feed
 * non NO_WINDOW widgets to this function, in which case this function is a noop.
 */
void
eel_ctk_container_child_expose_event (CtkContainer *container,
                                      CtkWidget *child,
                                      cairo_t *cr)
{
    g_return_if_fail (CTK_IS_CONTAINER (container));

    if (child == NULL)
    {
        return;
    }

    g_return_if_fail (CTK_IS_WIDGET (child));

    ctk_container_propagate_draw (container, child, cr);
}

/**
 * eel_ctk_container_child_map:
 *
 * @container: A CtkContainer widget.
 * @child: A child of @container or NULL;
 *
 * Map a child if needed.  This is usually called from the "CtkWidget::map"
 * method of the @container widget.  If @child is NULL, then this function is a noop.
 */
void
eel_ctk_container_child_map (CtkContainer *container,
                             CtkWidget *child)
{
    g_return_if_fail (CTK_IS_CONTAINER (container));

    if (child == NULL)
    {
        return;
    }

    g_return_if_fail (ctk_widget_get_parent (child) == CTK_WIDGET (container));

    if (ctk_widget_get_visible (child) && !ctk_widget_get_mapped (child))
    {
        ctk_widget_map (child);
    }
}

/**
 * eel_ctk_container_child_unmap:
 *
 * @container: A CtkContainer widget.
 * @child: A child of @container or NULL;
 *
 * Unmap a child if needed.  This is usually called from the "CtkWidget::unmap"
 * method of the @container widget.  If @child is NULL, then this function is a noop.
 */
void
eel_ctk_container_child_unmap (CtkContainer *container,
                               CtkWidget *child)
{
    g_return_if_fail (CTK_IS_CONTAINER (container));

    if (child == NULL)
    {
        return;
    }

    g_return_if_fail (ctk_widget_get_parent (child) == CTK_WIDGET (container));

    if (ctk_widget_get_visible (child) && ctk_widget_get_mapped (child))
    {
        ctk_widget_unmap (child);
    }
}

/**
 * eel_ctk_container_child_add:
 *
 * @container: A CtkContainer widget.
 * @child: A non NULL unparented child.
 *
 * Add a @child to a @container.  The @child is realized, mapped
 * and resized if needed.  This is usually called from the "CtkContainer::add"
 * method of the @container.  The @child cannot be NULL.
 */
void
eel_ctk_container_child_add (CtkContainer *container,
                             CtkWidget *child)
{
    CtkWidget *widget;

    g_return_if_fail (CTK_IS_CONTAINER (container));
    g_return_if_fail (CTK_IS_WIDGET (child));

    widget = CTK_WIDGET (container);

    ctk_widget_set_parent (child, widget);

    if (ctk_widget_get_realized (widget))
    {
        ctk_widget_realize (child);
    }

    if (ctk_widget_get_mapped (widget)
            && ctk_widget_get_visible (child))
    {
        if (ctk_widget_get_mapped (widget))
        {
            ctk_widget_map (child);
        }

        ctk_widget_queue_resize (child);
    }
}

/**
 * eel_ctk_container_child_remove:
 *
 * @container: A CtkContainer widget.
 * @child: A non NULL child of @container.
 *
 * Remove @child from @container.  The @container is resized if needed.
 * This is usually called from the "CtkContainer::remove" method of the
 * @container.  The child cannot be NULL.
 */
void
eel_ctk_container_child_remove (CtkContainer *container,
                                CtkWidget *child)
{
    gboolean child_was_visible;

    g_return_if_fail (CTK_IS_CONTAINER (container));
    g_return_if_fail (CTK_IS_WIDGET (child));
    g_return_if_fail (ctk_widget_get_parent (child) == CTK_WIDGET (container));

    child_was_visible = ctk_widget_get_visible (child);

    ctk_widget_unparent (child);

    if (child_was_visible)
    {
        ctk_widget_queue_resize (CTK_WIDGET (container));
    }
}

/**
 * eel_ctk_container_child_size_allocate:
 *
 * @container: A CtkContainer widget.
 * @child: A child of @container or NULL;
 *
 * Invoke the "CtkWidget::size_allocate" method of @child.
 * This function is usually called from the "CtkWidget::size_allocate"
 * method of @container.  The child can be NULL, in which case this
 * function is a noop.
 */
void
eel_ctk_container_child_size_allocate (CtkContainer *container,
                                       CtkWidget *child,
                                       EelIRect child_geometry)
{
    CtkAllocation child_allocation;

    g_return_if_fail (CTK_IS_CONTAINER (container));

    if (child == NULL)
    {
        return;
    }

    g_return_if_fail (CTK_IS_WIDGET (child));
    g_return_if_fail (ctk_widget_get_parent (child) == CTK_WIDGET (container));

    if (eel_irect_is_empty (&child_geometry))
    {
        return;
    }

    child_allocation.x = child_geometry.x0;
    child_allocation.y = child_geometry.y0;
    child_allocation.width = eel_irect_get_width (child_geometry);
    child_allocation.height = eel_irect_get_height (child_geometry);

    ctk_widget_size_allocate (child, &child_allocation);
}
