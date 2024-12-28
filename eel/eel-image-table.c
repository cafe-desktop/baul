/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-image-table.c - An image table.

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
#include "eel-image-table.h"

#include "eel-art-extensions.h"
#include "eel-art-ctk-extensions.h"
#include "eel-ctk-extensions.h"
#include "eel-labeled-image.h"
#include "eel-marshal.h"
#include <ctk/ctk.h>

/* Arguments */
enum
{
    ARG_0,
    ARG_CHILD_UNDER_POINTER
};

/* Detail member struct */
struct EelImageTablePrivate
{
    CtkWidget *child_under_pointer;
    CtkWidget *child_being_pressed;
};

/* Signals */
typedef enum
{
    CHILD_ENTER,
    CHILD_LEAVE,
    CHILD_PRESSED,
    CHILD_RELEASED,
    CHILD_CLICKED,
    LAST_SIGNAL
} ImageTableSignals;

/* Signals */
static guint image_table_signals[LAST_SIGNAL] = { 0 };

/* Ancestor methods */
CtkWidget *    find_windowed_ancestor               (CtkWidget          *widget);

static void    signal_connect_while_realized        (CtkWidget          *object,
    						     const char         *name,
    						     GCallback           callback,
    						     gpointer            callback_data,
    						     CtkWidget          *realized_widget);
/* Ancestor callbacks */
static int     ancestor_enter_notify_event          (CtkWidget          *widget,
        CdkEventCrossing   *event,
        gpointer            event_data);
static int     ancestor_leave_notify_event          (CtkWidget          *widget,
        CdkEventCrossing   *event,
        gpointer            event_data);
static int     ancestor_motion_notify_event         (CtkWidget          *widget,
        CdkEventMotion     *event,
        gpointer            event_data);
static int     ancestor_button_press_event          (CtkWidget          *widget,
        CdkEventButton     *event,
        gpointer            event_data);
static int     ancestor_button_release_event        (CtkWidget          *widget,
        CdkEventButton     *event,
        gpointer            event_data);

G_DEFINE_TYPE_WITH_PRIVATE (EelImageTable, eel_image_table, EEL_TYPE_WRAP_TABLE)

static void
eel_image_table_init (EelImageTable *image_table)
{
    ctk_widget_set_has_window (CTK_WIDGET (image_table), FALSE);

    image_table->details = eel_image_table_get_instance_private (image_table);
}

/* GObjectClass methods */
static void
eel_image_table_finalize (GObject *object)
{
    G_OBJECT_CLASS (eel_image_table_parent_class)->finalize (object);
}

static void
eel_image_table_realize (CtkWidget *widget)
{
    CtkWidget *windowed_ancestor;

    g_assert (EEL_IS_IMAGE_TABLE (widget));

    /* Chain realize */
    CTK_WIDGET_CLASS (eel_image_table_parent_class)->realize (widget);

    windowed_ancestor = find_windowed_ancestor (widget);
    g_assert (CTK_IS_WIDGET (windowed_ancestor));

    ctk_widget_add_events (windowed_ancestor,
                           CDK_BUTTON_PRESS_MASK
                           | CDK_BUTTON_RELEASE_MASK
                           | CDK_BUTTON_MOTION_MASK
                           | CDK_ENTER_NOTIFY_MASK
                           | CDK_LEAVE_NOTIFY_MASK
                           | CDK_POINTER_MOTION_MASK);


    signal_connect_while_realized (windowed_ancestor,
    				   "enter_notify_event",
    				   G_CALLBACK (ancestor_enter_notify_event),
    				   widget,
    				   widget);

    signal_connect_while_realized (windowed_ancestor,
    				   "leave_notify_event",
    				   G_CALLBACK (ancestor_leave_notify_event),
    				   widget,
    				   widget);

    signal_connect_while_realized (windowed_ancestor,
    				   "motion_notify_event",
    				   G_CALLBACK (ancestor_motion_notify_event),
    				   widget,
    				   widget);

    signal_connect_while_realized (windowed_ancestor,
    				   "button_press_event",
    				   G_CALLBACK (ancestor_button_press_event),
    				   widget,
    				   widget);

    signal_connect_while_realized (windowed_ancestor,
    				   "button_release_event",
    				   G_CALLBACK (ancestor_button_release_event),
    				   widget,
    				   widget);
}

/* CtkContainerClass methods */
static void
eel_image_table_remove (CtkContainer *container,
                        CtkWidget *child)
{
    EelImageTable *image_table;

    g_assert (EEL_IS_IMAGE_TABLE (container));
    g_assert (EEL_IS_LABELED_IMAGE (child));

    image_table = EEL_IMAGE_TABLE (container);

    if (child == image_table->details->child_under_pointer)
    {
        image_table->details->child_under_pointer = NULL;
    }

    if (child == image_table->details->child_being_pressed)
    {
        image_table->details->child_being_pressed = NULL;
    }

    CTK_CONTAINER_CLASS (eel_image_table_parent_class)->remove (container, child);
}

static GType
eel_image_table_child_type (CtkContainer *container G_GNUC_UNUSED)
{
    return EEL_TYPE_LABELED_IMAGE;
}

/* Private EelImageTable methods */

static void
image_table_emit_signal (EelImageTable *image_table,
                         CtkWidget *child,
                         guint signal_index,
                         int x,
                         int y,
                         int button,
                         guint state,
                         CdkEvent *cdk_event)
{
    EelImageTableEvent event;

    g_assert (EEL_IS_IMAGE_TABLE (image_table));
    g_assert (CTK_IS_WIDGET (child));
    g_assert (signal_index < LAST_SIGNAL);

    event.x = x;
    event.y = y;
    event.button = button;
    event.state = state;
    event.event = cdk_event;

    g_signal_emit (image_table,
                   image_table_signals[signal_index],
                   0,
                   child,
                   &event);
}

static void
eel_image_table_class_init (EelImageTableClass *image_table_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (image_table_class);
    CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (image_table_class);
    CtkContainerClass *container_class = CTK_CONTAINER_CLASS (image_table_class);

    /* GObjectClass */
    object_class->finalize = eel_image_table_finalize;

    /* CtkWidgetClass */
    widget_class->realize = eel_image_table_realize;

    /* CtkContainerClass */
    container_class->remove = eel_image_table_remove;
    container_class->child_type = eel_image_table_child_type;

    /* Signals */
    image_table_signals[CHILD_ENTER] = g_signal_new ("child_enter",
                                       G_TYPE_FROM_CLASS (object_class),
                                       G_SIGNAL_RUN_LAST,
                                       G_STRUCT_OFFSET (EelImageTableClass, child_enter),
                                       NULL, NULL,
                                       eel_marshal_VOID__OBJECT_POINTER,
                                       G_TYPE_NONE,
                                       2,
                                       CTK_TYPE_WIDGET,
                                       G_TYPE_POINTER);
    image_table_signals[CHILD_LEAVE] = g_signal_new ("child_leave",
                                       G_TYPE_FROM_CLASS (object_class),
                                       G_SIGNAL_RUN_LAST,
                                       G_STRUCT_OFFSET (EelImageTableClass, child_leave),
                                       NULL, NULL,
                                       eel_marshal_VOID__OBJECT_POINTER,
                                       G_TYPE_NONE,
                                       2,
                                       CTK_TYPE_WIDGET,
                                       G_TYPE_POINTER);
    image_table_signals[CHILD_PRESSED] = g_signal_new ("child_pressed",
                                         G_TYPE_FROM_CLASS (object_class),
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (EelImageTableClass, child_pressed),
                                         NULL, NULL,
                                         eel_marshal_VOID__OBJECT_POINTER,
                                         G_TYPE_NONE,
                                         2,
                                         CTK_TYPE_WIDGET,
                                         G_TYPE_POINTER);
    image_table_signals[CHILD_RELEASED] = g_signal_new ("child_released",
                                          G_TYPE_FROM_CLASS (object_class),
                                          G_SIGNAL_RUN_LAST,
                                          G_STRUCT_OFFSET (EelImageTableClass, child_released),
                                          NULL, NULL,
                                          eel_marshal_VOID__OBJECT_POINTER,
                                          G_TYPE_NONE,
                                          2,
                                          CTK_TYPE_WIDGET,
                                          G_TYPE_POINTER);
    image_table_signals[CHILD_CLICKED] = g_signal_new ("child_clicked",
                                         G_TYPE_FROM_CLASS (object_class),
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (EelImageTableClass, child_clicked),
                                         NULL, NULL,
                                         eel_marshal_VOID__OBJECT_POINTER,
                                         G_TYPE_NONE,
                                         2,
                                         CTK_TYPE_WIDGET,
                                         G_TYPE_POINTER);

}

static void
image_table_handle_motion (EelImageTable *image_table,
                           int x,
                           int y,
                           CdkEvent *event)
{
    CtkWidget *child;
    CtkWidget *leave_emit_child = NULL;
    CtkWidget *enter_emit_child = NULL;

    g_assert (EEL_IS_IMAGE_TABLE (image_table));

    child = eel_wrap_table_find_child_at_event_point (EEL_WRAP_TABLE (image_table), x, y);

    if (child && !ctk_widget_get_sensitive (child))
    {
        return;
    }

    if (child == image_table->details->child_under_pointer)
    {
        return;
    }

    if (child != NULL)
    {
        if (image_table->details->child_under_pointer != NULL)
        {
            leave_emit_child = image_table->details->child_under_pointer;
        }

        image_table->details->child_under_pointer = child;
        enter_emit_child = image_table->details->child_under_pointer;
    }
    else
    {
        if (image_table->details->child_under_pointer != NULL)
        {
            leave_emit_child = image_table->details->child_under_pointer;
        }

        image_table->details->child_under_pointer = NULL;
    }

    if (leave_emit_child != NULL)
    {
        image_table_emit_signal (image_table,
                                 leave_emit_child,
                                 CHILD_LEAVE,
                                 x,
                                 y,
                                 0,
                                 0,
                                 (CdkEvent *)event);
    }

    if (enter_emit_child != NULL)
    {
        image_table_emit_signal (image_table,
                                 enter_emit_child,
                                 CHILD_ENTER,
                                 x,
                                 y,
                                 0,
                                 0,
                                 (CdkEvent *)event);
    }
}

CtkWidget *
find_windowed_ancestor (CtkWidget *widget)
{
    g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

    while (widget && !ctk_widget_get_has_window (widget))
    {
        widget = ctk_widget_get_parent (widget);
    }

    return widget;
}

typedef struct
{
    GObject *object;
    guint object_destroy_handler;

    CtkWidget *realized_widget;
    guint realized_widget_destroy_handler;
    guint realized_widget_unrealized_handler;

    guint signal_handler;
} RealizeDisconnectInfo;

static void
while_realized_disconnecter (GObject *object,
                             RealizeDisconnectInfo *info)
{
    g_assert (G_IS_OBJECT (object));
    g_assert (info != NULL);
    g_assert (G_IS_OBJECT (info->object));
    g_assert (info->object_destroy_handler != 0);
    g_assert (info->object_destroy_handler != 0);
    g_assert (info->realized_widget_destroy_handler != 0);
    g_assert (info->realized_widget_unrealized_handler != 0);

    g_signal_handler_disconnect (info->object, info->object_destroy_handler);
    g_signal_handler_disconnect (info->object, info->signal_handler);
    g_signal_handler_disconnect (info->realized_widget, info->realized_widget_destroy_handler);
    g_signal_handler_disconnect (info->realized_widget, info->realized_widget_unrealized_handler);
    g_free (info);
}

/**
 * signal_connect_while_realized:
 *
 * @object: Object to connect to.
 * @name: Name of signal to connect to.
 * @callback: Caller's callback.
 * @callback_data: Caller's callback_data.
 * @realized_widget: Widget to monitor for realized state.  Signal is connected
 *                   while this wigget is realized.
 *
 * Connect to a signal of an object while another widget is realized.  This is
 * useful for non windowed widgets that need to monitor events in their ancestored
 * windowed widget.  The signal is automatically disconnected when &widget is
 * unrealized.  Also, the signal is automatically disconnected when either &object
 * or &widget are destroyed.
 **/
static void
signal_connect_while_realized (CtkWidget *object,
    				const char *name,
    				GCallback callback,
    				gpointer callback_data,
    				CtkWidget *realized_widget)
{
    RealizeDisconnectInfo *info;

    g_return_if_fail (CTK_IS_WIDGET (object));
    g_return_if_fail (name != NULL);
    g_return_if_fail (name[0] != '\0');
    g_return_if_fail (callback != NULL);
    g_return_if_fail (CTK_IS_WIDGET (realized_widget));
    g_return_if_fail (ctk_widget_get_realized (realized_widget));

    info = g_new0 (RealizeDisconnectInfo, 1);

    info->object = G_OBJECT (object);
    info->object_destroy_handler =
        g_signal_connect (G_OBJECT (info->object),
                          "destroy",
                          G_CALLBACK (while_realized_disconnecter),
                          info);

    info->realized_widget = realized_widget;
    info->realized_widget_destroy_handler =
        g_signal_connect (G_OBJECT (info->realized_widget),
                          "destroy",
                          G_CALLBACK (while_realized_disconnecter),
                          info);
    info->realized_widget_unrealized_handler =
        g_signal_connect_after (G_OBJECT (info->realized_widget),
                                "unrealize",
                                G_CALLBACK (while_realized_disconnecter),
                                info);

    info->signal_handler = g_signal_connect (G_OBJECT (info->object),
                           name, callback, callback_data);
}

static int
ancestor_enter_notify_event (CtkWidget *widget,
                             CdkEventCrossing *event,
                             gpointer event_data)
{
    g_assert (CTK_IS_WIDGET (widget));
    g_assert (EEL_IS_IMAGE_TABLE (event_data));
    g_assert (event != NULL);

    image_table_handle_motion (EEL_IMAGE_TABLE (event_data), event->x, event->y, (CdkEvent *) event);

    return FALSE;
}

static int
ancestor_leave_notify_event (CtkWidget *widget,
                             CdkEventCrossing *event,
                             gpointer event_data)
{
    EelIRect bounds;
    int x = -1;
    int y = -1;

    g_assert (CTK_IS_WIDGET (widget));
    g_assert (EEL_IS_IMAGE_TABLE (event_data));
    g_assert (event != NULL);

    bounds = eel_ctk_widget_get_bounds (CTK_WIDGET (event_data));

    if (eel_irect_contains_point (bounds, event->x, event->y))
    {
        x = event->x;
        y = event->y;
    }

    image_table_handle_motion (EEL_IMAGE_TABLE (event_data), x, y, (CdkEvent *) event);

    return FALSE;
}

static int
ancestor_motion_notify_event (CtkWidget *widget,
                              CdkEventMotion *event,
                              gpointer event_data)
{
    g_assert (CTK_IS_WIDGET (widget));
    g_assert (EEL_IS_IMAGE_TABLE (event_data));
    g_assert (event != NULL);

    image_table_handle_motion (EEL_IMAGE_TABLE (event_data), (int) event->x, (int) event->y, (CdkEvent *) event);

    return FALSE;
}

static int
ancestor_button_press_event (CtkWidget *widget,
                             CdkEventButton *event,
                             gpointer event_data)
{
    EelImageTable *image_table;
    CtkWidget *child;

    g_assert (CTK_IS_WIDGET (widget));
    g_assert (EEL_IS_IMAGE_TABLE (event_data));
    g_assert (event != NULL);

    image_table = EEL_IMAGE_TABLE (event_data);

    child = eel_wrap_table_find_child_at_event_point (EEL_WRAP_TABLE (image_table), event->x, event->y);

    if (child && !ctk_widget_get_sensitive (child))
    {
        return FALSE;
    }

    if (child != NULL)
    {
        if (child == image_table->details->child_under_pointer)
        {
            image_table->details->child_being_pressed = child;
            image_table_emit_signal (image_table,
                                     child,
                                     CHILD_PRESSED,
                                     event->x,
                                     event->y,
                                     event->button,
                                     event->state,
                                     (CdkEvent *)event);
        }
    }

    return FALSE;
}

static int
ancestor_button_release_event (CtkWidget *widget,
                               CdkEventButton *event,
                               gpointer event_data)
{
    EelImageTable *image_table;
    CtkWidget *child;
    CtkWidget *released_emit_child = NULL;
    CtkWidget *clicked_emit_child = NULL;

    g_assert (CTK_IS_WIDGET (widget));
    g_assert (EEL_IS_IMAGE_TABLE (event_data));
    g_assert (event != NULL);

    image_table = EEL_IMAGE_TABLE (event_data);

    child = eel_wrap_table_find_child_at_event_point (EEL_WRAP_TABLE (image_table), event->x, event->y);

    if (child && !ctk_widget_get_sensitive (child))
    {
        return FALSE;
    }

    if (image_table->details->child_being_pressed != NULL)
    {
        released_emit_child = image_table->details->child_being_pressed;
    }

    if (child != NULL)
    {
        if (child == image_table->details->child_being_pressed)
        {
            clicked_emit_child = child;
        }
    }

    image_table->details->child_being_pressed = NULL;

    if (released_emit_child != NULL)
    {
        image_table_emit_signal (image_table,
                                 released_emit_child,
                                 CHILD_RELEASED,
                                 event->x,
                                 event->y,
                                 event->button,
                                 event->state,
                                 (CdkEvent *)event);
    }

    if (clicked_emit_child != NULL)
    {

        image_table_emit_signal (image_table,
                                 clicked_emit_child,
                                 CHILD_CLICKED,
                                 event->x,
                                 event->y,
                                 event->button,
                                 event->state,
                                 (CdkEvent *)event);
    }

    return FALSE;
}

/**
 * eel_image_table_new:
 */
CtkWidget*
eel_image_table_new (gboolean homogeneous)
{
    EelImageTable *image_table;

    image_table = EEL_IMAGE_TABLE (ctk_widget_new (eel_image_table_get_type (), NULL));

    eel_wrap_table_set_homogeneous (EEL_WRAP_TABLE (image_table), homogeneous);

    return CTK_WIDGET (image_table);
}

/**
 * eel_image_table_add_empty_child:
 * @image_table: A EelImageTable.
 *
 * Add a "empty" child to the table.  Useful when you want to have
 * empty space between 2 children.
 *
 * Returns: The empty child - A EelLabeledImage widget with no label
 *          or pixbuf.
 */
CtkWidget *
eel_image_table_add_empty_image (EelImageTable *image_table)
{
    CtkWidget *empty;

    g_return_val_if_fail (EEL_IS_IMAGE_TABLE (image_table), NULL);

    empty = eel_labeled_image_new (NULL, NULL);
    ctk_container_add (CTK_CONTAINER (image_table), empty);
    ctk_widget_set_sensitive (empty, FALSE);

    return empty;
}
