/* eggtreemultidnd.c
 * Copyright (C) 2001  Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <ctk/ctk.h>
#include "eggtreemultidnd.h"

#define EGG_TREE_MULTI_DND_STRING "EggTreeMultiDndString"

typedef struct
{
    guint pressed_button;
    gint x;
    gint y;
    guint motion_notify_handler;
    guint button_release_handler;
    guint drag_data_get_handler;
    GSList *event_list;
} EggTreeMultiDndData;

/* CUT-N-PASTE from ctktreeview.c */
typedef struct _TreeViewDragInfo TreeViewDragInfo;
struct _TreeViewDragInfo
{
    CdkModifierType start_button_mask;
    CtkTargetList *source_target_list;
    CdkDragAction source_actions;

    CtkTargetList *dest_target_list;

    guint source_set : 1;
    guint dest_set : 1;
};


GType
egg_tree_multi_drag_source_get_type (void)
{
    static GType our_type = 0;

    if (!our_type)
    {
        const GTypeInfo our_info =
        {
            sizeof (EggTreeMultiDragSourceIface), /* class_size */
            NULL,		/* base_init */
            NULL,		/* base_finalize */
            NULL,
            NULL,		/* class_finalize */
            NULL,		/* class_data */
            0,
            0,              /* n_preallocs */
            NULL
        };

        our_type = g_type_register_static (G_TYPE_INTERFACE, "EggTreeMultiDragSource", &our_info, 0);
    }

    return our_type;
}


/**
 * egg_tree_multi_drag_source_row_draggable:
 * @drag_source: a #EggTreeMultiDragSource
 * @path: row on which user is initiating a drag
 *
 * Asks the #EggTreeMultiDragSource whether a particular row can be used as
 * the source of a DND operation. If the source doesn't implement
 * this interface, the row is assumed draggable.
 *
 * Return value: %TRUE if the row can be dragged
 **/
gboolean
egg_tree_multi_drag_source_row_draggable (EggTreeMultiDragSource *drag_source,
        GList                  *path_list)
{
    EggTreeMultiDragSourceIface *iface = EGG_TREE_MULTI_DRAG_SOURCE_GET_IFACE (drag_source);

    g_return_val_if_fail (EGG_IS_TREE_MULTI_DRAG_SOURCE (drag_source), FALSE);
    g_return_val_if_fail (iface->row_draggable != NULL, FALSE);
    g_return_val_if_fail (path_list != NULL, FALSE);

    if (iface->row_draggable)
        return (* iface->row_draggable) (drag_source, path_list);
    else
        return TRUE;
}


/**
 * egg_tree_multi_drag_source_drag_data_delete:
 * @drag_source: a #EggTreeMultiDragSource
 * @path: row that was being dragged
 *
 * Asks the #EggTreeMultiDragSource to delete the row at @path, because
 * it was moved somewhere else via drag-and-drop. Returns %FALSE
 * if the deletion fails because @path no longer exists, or for
 * some model-specific reason. Should robustly handle a @path no
 * longer found in the model!
 *
 * Return value: %TRUE if the row was successfully deleted
 **/
gboolean
egg_tree_multi_drag_source_drag_data_delete (EggTreeMultiDragSource *drag_source,
        GList                  *path_list)
{
    EggTreeMultiDragSourceIface *iface = EGG_TREE_MULTI_DRAG_SOURCE_GET_IFACE (drag_source);

    g_return_val_if_fail (EGG_IS_TREE_MULTI_DRAG_SOURCE (drag_source), FALSE);
    g_return_val_if_fail (iface->drag_data_delete != NULL, FALSE);
    g_return_val_if_fail (path_list != NULL, FALSE);

    return (* iface->drag_data_delete) (drag_source, path_list);
}

/**
 * egg_tree_multi_drag_source_drag_data_get:
 * @drag_source: a #EggTreeMultiDragSource
 * @path: row that was dragged
 * @selection_data: a #EggSelectionData to fill with data from the dragged row
 *
 * Asks the #EggTreeMultiDragSource to fill in @selection_data with a
 * representation of the row at @path. @selection_data->target gives
 * the required type of the data.  Should robustly handle a @path no
 * longer found in the model!
 *
 * Return value: %TRUE if data of the required type was provided
 **/
gboolean
egg_tree_multi_drag_source_drag_data_get    (EggTreeMultiDragSource *drag_source,
        GList                  *path_list,
        CtkSelectionData  *selection_data)
{
    EggTreeMultiDragSourceIface *iface = EGG_TREE_MULTI_DRAG_SOURCE_GET_IFACE (drag_source);

    g_return_val_if_fail (EGG_IS_TREE_MULTI_DRAG_SOURCE (drag_source), FALSE);
    g_return_val_if_fail (iface->drag_data_get != NULL, FALSE);
    g_return_val_if_fail (path_list != NULL, FALSE);
    g_return_val_if_fail (selection_data != NULL, FALSE);

    return (* iface->drag_data_get) (drag_source, path_list, selection_data);
}

static void
stop_drag_check (CtkWidget *widget)
{
    EggTreeMultiDndData *priv_data;
    GSList *l;

    priv_data = g_object_get_data (G_OBJECT (widget), EGG_TREE_MULTI_DND_STRING);

    for (l = priv_data->event_list; l != NULL; l = l->next)
        cdk_event_free (l->data);

    g_slist_free (priv_data->event_list);
    priv_data->event_list = NULL;
    g_signal_handler_disconnect (widget, priv_data->motion_notify_handler);
    g_signal_handler_disconnect (widget, priv_data->button_release_handler);
}

static gboolean
egg_tree_multi_drag_button_release_event (CtkWidget      *widget,
					  CdkEventButton *event G_GNUC_UNUSED,
					  gpointer        data G_GNUC_UNUSED)
{
    EggTreeMultiDndData *priv_data;
    GSList *l;

    priv_data = g_object_get_data (G_OBJECT (widget), EGG_TREE_MULTI_DND_STRING);

    for (l = priv_data->event_list; l != NULL; l = l->next)
        ctk_propagate_event (widget, l->data);

    stop_drag_check (widget);

    return FALSE;
}

static void
selection_foreach (CtkTreeModel *model,
		   CtkTreePath  *path,
		   CtkTreeIter  *iter G_GNUC_UNUSED,
		   gpointer      data)
{
    GList **list_ptr;

    list_ptr = (GList **) data;

    *list_ptr = g_list_prepend (*list_ptr, ctk_tree_row_reference_new (model, path));
}

static void
path_list_free (GList *path_list)
{
    g_list_foreach (path_list, (GFunc) ctk_tree_row_reference_free, NULL);
    g_list_free (path_list);
}

static void
set_context_data (CdkDragContext *context,
                  GList          *path_list)
{
    g_object_set_data_full (G_OBJECT (context),
                            "egg-tree-view-multi-source-row",
                            path_list,
                            (GDestroyNotify) path_list_free);
}

static GList *
get_context_data (CdkDragContext *context)
{
    return g_object_get_data (G_OBJECT (context),
                              "egg-tree-view-multi-source-row");
}

/* CUT-N-PASTE from ctktreeview.c */
static TreeViewDragInfo*
get_info (CtkTreeView *tree_view)
{
    return g_object_get_data (G_OBJECT (tree_view), "ctk-tree-view-drag-info");
}


static void
egg_tree_multi_drag_drag_data_get (CtkWidget        *widget,
				   CdkDragContext   *context,
				   CtkSelectionData *selection_data,
				   guint             info G_GNUC_UNUSED,
				   guint             time G_GNUC_UNUSED)
{
    CtkTreeView *tree_view;
    CtkTreeModel *model;
    TreeViewDragInfo *di;
    GList *path_list;

    tree_view = CTK_TREE_VIEW (widget);

    model = ctk_tree_view_get_model (tree_view);

    if (model == NULL)
        return;

    di = get_info (CTK_TREE_VIEW (widget));

    if (di == NULL)
        return;

    path_list = get_context_data (context);

    if (path_list == NULL)
        return;

    /* We can implement the CTK_TREE_MODEL_ROW target generically for
     * any model; for DragSource models there are some other targets
     * we also support.
     */

    if (EGG_IS_TREE_MULTI_DRAG_SOURCE (model))
    {
        egg_tree_multi_drag_source_drag_data_get (EGG_TREE_MULTI_DRAG_SOURCE (model),
                path_list,
                selection_data);
    }
}

static gboolean
egg_tree_multi_drag_motion_event (CtkWidget      *widget,
				  CdkEventMotion *event,
				  gpointer        data G_GNUC_UNUSED)
{
    EggTreeMultiDndData *priv_data;

    priv_data = g_object_get_data (G_OBJECT (widget), EGG_TREE_MULTI_DND_STRING);

    if (ctk_drag_check_threshold (widget,
                                  priv_data->x,
                                  priv_data->y,
                                  event->x,
                                  event->y))
    {
        GList *path_list = NULL;
        CtkTreeSelection *selection;
        CtkTreeModel *model;
        TreeViewDragInfo *di;

        di = get_info (CTK_TREE_VIEW (widget));

        if (di == NULL)
            return FALSE;

        selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (widget));
        stop_drag_check (widget);
        ctk_tree_selection_selected_foreach (selection, selection_foreach, &path_list);
        path_list = g_list_reverse (path_list);
        model = ctk_tree_view_get_model (CTK_TREE_VIEW (widget));
        if (egg_tree_multi_drag_source_row_draggable (EGG_TREE_MULTI_DRAG_SOURCE (model), path_list))
        {
            CdkDragContext *context;

            context = ctk_drag_begin_with_coordinates (widget,
                                                       ctk_drag_source_get_target_list (widget),
                                                       di->source_actions,
                                                       priv_data->pressed_button,
                                                       (CdkEvent*)event,
                                                       event->x,
                                                       event->y);
            set_context_data (context, path_list);
            ctk_drag_set_icon_default (context);

        }
        else
        {
            path_list_free (path_list);
        }
    }

    return TRUE;
}

static gboolean
egg_tree_multi_drag_button_press_event (CtkWidget      *widget,
					CdkEventButton *event,
					gpointer        data G_GNUC_UNUSED)
{
    CtkTreeView *tree_view;
    CtkTreePath *path = NULL;
    CtkTreeViewColumn *column = NULL;
    gint cell_x, cell_y;
    CtkTreeSelection *selection;
    EggTreeMultiDndData *priv_data;

    tree_view = CTK_TREE_VIEW (widget);
    priv_data = g_object_get_data (G_OBJECT (tree_view), EGG_TREE_MULTI_DND_STRING);
    if (priv_data == NULL)
    {
        priv_data = g_new0 (EggTreeMultiDndData, 1);
        g_object_set_data (G_OBJECT (tree_view), EGG_TREE_MULTI_DND_STRING, priv_data);
    }

    if (g_slist_find (priv_data->event_list, event))
        return FALSE;

    if (priv_data->event_list)
    {
        /* save the event to be propagated in order */
        priv_data->event_list = g_slist_append (priv_data->event_list, cdk_event_copy ((CdkEvent*)event));
        return TRUE;
    }

    if (event->type == CDK_2BUTTON_PRESS)
        return FALSE;

    ctk_tree_view_get_path_at_pos (tree_view,
                                   event->x, event->y,
                                   &path, &column,
                                   &cell_x, &cell_y);

    selection = ctk_tree_view_get_selection (tree_view);

    if (path && ctk_tree_selection_path_is_selected (selection, path))
    {
        priv_data->pressed_button = event->button;
        priv_data->x = event->x;
        priv_data->y = event->y;
        priv_data->event_list = g_slist_append (priv_data->event_list, cdk_event_copy ((CdkEvent*)event));
        priv_data->motion_notify_handler =
            g_signal_connect (G_OBJECT (tree_view), "motion_notify_event", G_CALLBACK (egg_tree_multi_drag_motion_event), NULL);
        priv_data->button_release_handler =
            g_signal_connect (G_OBJECT (tree_view), "button_release_event", G_CALLBACK (egg_tree_multi_drag_button_release_event), NULL);

        if (priv_data->drag_data_get_handler == 0)
        {
            priv_data->drag_data_get_handler =
                g_signal_connect (G_OBJECT (tree_view), "drag_data_get", G_CALLBACK (egg_tree_multi_drag_drag_data_get), NULL);
        }

        ctk_tree_path_free (path);

        return TRUE;
    }

    if (path)
    {
        ctk_tree_path_free (path);
    }

    return FALSE;
}

void
egg_tree_multi_drag_add_drag_support (CtkTreeView *tree_view)
{
    g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
    g_signal_connect (G_OBJECT (tree_view), "button_press_event", G_CALLBACK (egg_tree_multi_drag_button_press_event), NULL);
}

