/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
 *
 * Baul is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Baul is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Dave Camp <dave@ximian.com>
 * XDS support: Benedikt Meurer <benny@xfce.org> (adapted by Amos Brocco <amos.brocco@unifr.ch>)
 */

/* baul-tree-view-drag-dest.c: Handles drag and drop for treeviews which
 *                                 contain a hierarchy of files
 */

#include <config.h>
#include <stdio.h>
#include <string.h>

#include <ctk/ctk.h>

#include <eel/eel-ctk-macros.h>

#include "baul-tree-view-drag-dest.h"
#include "baul-file-dnd.h"
#include "baul-file-changes-queue.h"
#include "baul-icon-dnd.h"
#include "baul-link.h"
#include "baul-marshal.h"
#include "baul-debug-log.h"

#define AUTO_SCROLL_MARGIN 20

#define HOVER_EXPAND_TIMEOUT 1

struct _BaulTreeViewDragDestDetails
{
    CtkTreeView *tree_view;

    gboolean drop_occurred;

    gboolean have_drag_data;
    guint drag_type;
    CtkSelectionData *drag_data;
    GList *drag_list;

    guint highlight_id;
    guint scroll_id;
    guint expand_id;

    char *direct_save_uri;
};

enum
{
    GET_ROOT_URI,
    GET_FILE_FOR_PATH,
    MOVE_COPY_ITEMS,
    HANDLE_NETSCAPE_URL,
    HANDLE_URI_LIST,
    HANDLE_TEXT,
    HANDLE_RAW,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BaulTreeViewDragDest, baul_tree_view_drag_dest,
               G_TYPE_OBJECT);

#define parent_class baul_tree_view_drag_dest_parent_class

static const CtkTargetEntry drag_types [] =
{
    { BAUL_ICON_DND_CAFE_ICON_LIST_TYPE, 0, BAUL_ICON_DND_CAFE_ICON_LIST },
    /* prefer "_NETSCAPE_URL" over "text/uri-list" to satisfy web browsers. */
    { BAUL_ICON_DND_NETSCAPE_URL_TYPE, 0, BAUL_ICON_DND_NETSCAPE_URL },
    { BAUL_ICON_DND_URI_LIST_TYPE, 0, BAUL_ICON_DND_URI_LIST },
    { BAUL_ICON_DND_KEYWORD_TYPE, 0, BAUL_ICON_DND_KEYWORD },
    { BAUL_ICON_DND_XDNDDIRECTSAVE_TYPE, 0, BAUL_ICON_DND_XDNDDIRECTSAVE }, /* XDS Protocol Type */
    { BAUL_ICON_DND_RAW_TYPE, 0, BAUL_ICON_DND_RAW }
};


static void
ctk_tree_view_vertical_autoscroll (CtkTreeView *tree_view)
{
    CdkRectangle visible_rect;
    CtkAdjustment *vadjustment;
    CdkDisplay *display;
    CdkSeat *seat;
    CdkDevice *pointer;
    CdkWindow *window;
    int y;
    int offset;
    float value;

    window = ctk_tree_view_get_bin_window (tree_view);

    vadjustment = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE(tree_view));

    display = ctk_widget_get_display (CTK_WIDGET (tree_view));
    seat = cdk_display_get_default_seat (display);
    pointer = cdk_seat_get_pointer (seat);
    cdk_window_get_device_position (window, pointer,
                                    NULL, &y, NULL);

    y += ctk_adjustment_get_value (vadjustment);

    ctk_tree_view_get_visible_rect (tree_view, &visible_rect);

    offset = y - (visible_rect.y + 2 * AUTO_SCROLL_MARGIN);
    if (offset > 0)
    {
        offset = y - (visible_rect.y + visible_rect.height - 2 * AUTO_SCROLL_MARGIN);
        if (offset < 0)
        {
            return;
        }
    }

    value = CLAMP (ctk_adjustment_get_value (vadjustment) + offset, 0.0,
                   ctk_adjustment_get_upper (vadjustment) - ctk_adjustment_get_page_size (vadjustment));
    ctk_adjustment_set_value (vadjustment, value);
}

static int
scroll_timeout (gpointer data)
{
    CtkTreeView *tree_view = CTK_TREE_VIEW (data);

    ctk_tree_view_vertical_autoscroll (tree_view);

    return TRUE;
}

static void
remove_scroll_timeout (BaulTreeViewDragDest *dest)
{
    if (dest->details->scroll_id)
    {
        g_source_remove (dest->details->scroll_id);
        dest->details->scroll_id = 0;
    }
}

static int
expand_timeout (gpointer data)
{
    CtkTreeView *tree_view;
    CtkTreePath *drop_path;

    tree_view = CTK_TREE_VIEW (data);

    ctk_tree_view_get_drag_dest_row (tree_view, &drop_path, NULL);

    if (drop_path)
    {
        ctk_tree_view_expand_row (tree_view, drop_path, FALSE);
        ctk_tree_path_free (drop_path);
    }

    return FALSE;
}

static void
remove_expand_timeout (BaulTreeViewDragDest *dest)
{
    if (dest->details->expand_id)
    {
        g_source_remove (dest->details->expand_id);
        dest->details->expand_id = 0;
    }
}

static gboolean
highlight_draw (CtkWidget *widget,
		cairo_t   *cr,
                gpointer data)
{
    CdkWindow *bin_window;
    int width;
    int height;
    CtkStyleContext *style;

    /* FIXMEchpe: is bin window right here??? */
    bin_window = ctk_tree_view_get_bin_window (CTK_TREE_VIEW (widget));

    width = cdk_window_get_width(bin_window);
    height = cdk_window_get_height(bin_window);

    style = ctk_widget_get_style_context (widget);

    ctk_style_context_save (style);
    ctk_style_context_add_class (style, "treeview-drop-indicator");

    ctk_render_focus (style,
                      cr,
                      0, 0, width, height);

    ctk_style_context_restore (style);

    return FALSE;
}

static void
set_widget_highlight (BaulTreeViewDragDest *dest, gboolean highlight)
{
    if (!highlight && dest->details->highlight_id)
    {
        g_signal_handler_disconnect (dest->details->tree_view,
                                     dest->details->highlight_id);
        dest->details->highlight_id = 0;
        ctk_widget_queue_draw (CTK_WIDGET (dest->details->tree_view));
    }

    if (highlight && !dest->details->highlight_id)
    {
        dest->details->highlight_id =
            g_signal_connect_object (dest->details->tree_view,
                                     "draw",
                                     G_CALLBACK (highlight_draw),
                                     dest, 0);
        ctk_widget_queue_draw (CTK_WIDGET (dest->details->tree_view));
    }
}

static void
set_drag_dest_row (BaulTreeViewDragDest *dest,
                   CtkTreePath *path)
{
    if (path)
    {
        set_widget_highlight (dest, FALSE);
        ctk_tree_view_set_drag_dest_row
        (dest->details->tree_view,
         path,
         CTK_TREE_VIEW_DROP_INTO_OR_BEFORE);
    }
    else
    {
        set_widget_highlight (dest, TRUE);
        ctk_tree_view_set_drag_dest_row (dest->details->tree_view,
                                         NULL,
                                         0);
    }
}

static void
clear_drag_dest_row (BaulTreeViewDragDest *dest)
{
    ctk_tree_view_set_drag_dest_row (dest->details->tree_view, NULL, 0);
    set_widget_highlight (dest, FALSE);
}

static gboolean
get_drag_data (BaulTreeViewDragDest *dest,
               CdkDragContext *context,
               guint32 time)
{
    CdkAtom target;

    target = ctk_drag_dest_find_target (CTK_WIDGET (dest->details->tree_view),
                                        context,
                                        NULL);

    if (target == CDK_NONE)
    {
        return FALSE;
    }

    if (target == cdk_atom_intern (BAUL_ICON_DND_XDNDDIRECTSAVE_TYPE, FALSE) &&
            !dest->details->drop_occurred)
    {
        dest->details->drag_type = BAUL_ICON_DND_XDNDDIRECTSAVE;
        dest->details->have_drag_data = TRUE;
        return TRUE;
    }

    ctk_drag_get_data (CTK_WIDGET (dest->details->tree_view),
                       context, target, time);

    return TRUE;
}

static void
free_drag_data (BaulTreeViewDragDest *dest)
{
    dest->details->have_drag_data = FALSE;

    if (dest->details->drag_data)
    {
        ctk_selection_data_free (dest->details->drag_data);
        dest->details->drag_data = NULL;
    }

    if (dest->details->drag_list)
    {
        baul_drag_destroy_selection_list (dest->details->drag_list);
        dest->details->drag_list = NULL;
    }

    g_free (dest->details->direct_save_uri);
    dest->details->direct_save_uri = NULL;
}

static char *
get_root_uri (BaulTreeViewDragDest *dest)
{
    char *uri;

    g_signal_emit (dest, signals[GET_ROOT_URI], 0, &uri);

    return uri;
}

static BaulFile *
file_for_path (BaulTreeViewDragDest *dest, CtkTreePath *path)
{
    BaulFile *file;

    if (path)
    {
        g_signal_emit (dest, signals[GET_FILE_FOR_PATH], 0, path, &file);
    }
    else
    {
        char *uri;

        uri = get_root_uri (dest);

        file = NULL;
        if (uri != NULL)
        {
            file = baul_file_get_by_uri (uri);
        }

        g_free (uri);
    }

    return file;
}

static CtkTreePath *
get_drop_path (BaulTreeViewDragDest *dest,
               CtkTreePath *path)
{
    BaulFile *file;
    CtkTreePath *ret;

    if (!path || !dest->details->have_drag_data)
    {
        return NULL;
    }

    ret = ctk_tree_path_copy (path);
    file = file_for_path (dest, ret);

    /* Go up the tree until we find a file that can accept a drop */
    while (file == NULL /* dummy row */ ||
            !baul_drag_can_accept_info (file,
                                        dest->details->drag_type,
                                        dest->details->drag_list))
    {
        if (ctk_tree_path_get_depth (ret) == 1)
        {
            ctk_tree_path_free (ret);
            ret = NULL;
            break;
        }
        else
        {
            ctk_tree_path_up (ret);

            baul_file_unref (file);
            file = file_for_path (dest, ret);
        }
    }
    baul_file_unref (file);

    return ret;
}

static char *
get_drop_target_uri_for_path (BaulTreeViewDragDest *dest,
                              CtkTreePath *path)
{
    BaulFile *file;
    char *target;

    file = file_for_path (dest, path);
    if (file == NULL)
    {
        return NULL;
    }

    target = baul_file_get_drop_target_uri (file);
    baul_file_unref (file);

    return target;
}

static guint
get_drop_action (BaulTreeViewDragDest *dest,
                 CdkDragContext *context,
                 CtkTreePath *path)
{
    char *drop_target;
    int action;

    if (!dest->details->have_drag_data ||
            (dest->details->drag_type == BAUL_ICON_DND_CAFE_ICON_LIST &&
             dest->details->drag_list == NULL))
    {
        return 0;
    }

    switch (dest->details->drag_type)
    {
    case BAUL_ICON_DND_CAFE_ICON_LIST :
        drop_target = get_drop_target_uri_for_path (dest, path);

        if (!drop_target)
        {
            return 0;
        }

        baul_drag_default_drop_action_for_icons
        (context,
         drop_target,
         dest->details->drag_list,
         &action);

        g_free (drop_target);

        return action;

    case BAUL_ICON_DND_NETSCAPE_URL:
        drop_target = get_drop_target_uri_for_path (dest, path);

        if (drop_target == NULL)
        {
            return 0;
        }

        action = baul_drag_default_drop_action_for_netscape_url (context);

        g_free (drop_target);

        return action;

    case BAUL_ICON_DND_URI_LIST :
        drop_target = get_drop_target_uri_for_path (dest, path);

        if (drop_target == NULL)
        {
            return 0;
        }

        g_free (drop_target);

        return cdk_drag_context_get_suggested_action (context);

    case BAUL_ICON_DND_TEXT:
    case BAUL_ICON_DND_RAW:
    case BAUL_ICON_DND_XDNDDIRECTSAVE:
        return CDK_ACTION_COPY;

    case BAUL_ICON_DND_KEYWORD:

        if (!path)
        {
            return 0;
        }

        return CDK_ACTION_COPY;
    }

    return 0;
}

static gboolean
drag_motion_callback (CtkWidget *widget,
                      CdkDragContext *context,
                      int x,
                      int y,
                      guint32 time,
                      gpointer data)
{
    BaulTreeViewDragDest *dest;
    CtkTreePath *path;
    CtkTreePath *drop_path, *old_drop_path;
    CtkTreeIter drop_iter;
    CtkTreeViewDropPosition pos;
    CdkWindow *bin_window;
    guint action;
    gboolean res = TRUE;

    dest = BAUL_TREE_VIEW_DRAG_DEST (data);

    ctk_tree_view_get_dest_row_at_pos (CTK_TREE_VIEW (widget),
                                       x, y, &path, &pos);


    if (!dest->details->have_drag_data)
    {
        res = get_drag_data (dest, context, time);
    }

    if (!res)
    {
        return FALSE;
    }

    drop_path = get_drop_path (dest, path);

    action = 0;
    bin_window = ctk_tree_view_get_bin_window (CTK_TREE_VIEW (widget));
    if (bin_window != NULL)
    {
        int bin_x, bin_y;
        cdk_window_get_position (bin_window, &bin_x, &bin_y);
        if (bin_y <= y)
        {
            /* ignore drags on the header */
            action = get_drop_action (dest, context, drop_path);
        }
    }

    ctk_tree_view_get_drag_dest_row (CTK_TREE_VIEW (widget), &old_drop_path,
                                     NULL);

    if (action)
    {
        CtkTreeModel *model;

        set_drag_dest_row (dest, drop_path);
        model = ctk_tree_view_get_model (CTK_TREE_VIEW (widget));
        if (drop_path == NULL || (old_drop_path != NULL &&
                                  ctk_tree_path_compare (old_drop_path, drop_path) != 0))
        {
            remove_expand_timeout (dest);
        }
        if (dest->details->expand_id == 0 && drop_path != NULL)
        {
            ctk_tree_model_get_iter (model, &drop_iter, drop_path);
            if (ctk_tree_model_iter_has_child (model, &drop_iter))
            {
                dest->details->expand_id = g_timeout_add_seconds (HOVER_EXPAND_TIMEOUT,
                                           expand_timeout,
                                           dest->details->tree_view);
            }
        }
    }
    else
    {
        clear_drag_dest_row (dest);
        remove_expand_timeout (dest);
    }

    if (path)
    {
        ctk_tree_path_free (path);
    }

    if (drop_path)
    {
        ctk_tree_path_free (drop_path);
    }

    if (old_drop_path)
    {
        ctk_tree_path_free (old_drop_path);
    }

    if (dest->details->scroll_id == 0)
    {
        dest->details->scroll_id =
            g_timeout_add (150,
                           scroll_timeout,
                           dest->details->tree_view);
    }

    cdk_drag_status (context, action, time);

    return TRUE;
}

static void
drag_leave_callback (CtkWidget *widget,
                     CdkDragContext *context,
                     guint32 time,
                     gpointer data)
{
    BaulTreeViewDragDest *dest;

    dest = BAUL_TREE_VIEW_DRAG_DEST (data);

    clear_drag_dest_row (dest);

    free_drag_data (dest);

    remove_scroll_timeout (dest);
    remove_expand_timeout (dest);
}

static char *
get_drop_target_uri_at_pos (BaulTreeViewDragDest *dest, int x, int y)
{
    char *drop_target;
    CtkTreePath *path;
    CtkTreePath *drop_path;
    CtkTreeViewDropPosition pos;

    ctk_tree_view_get_dest_row_at_pos (dest->details->tree_view, x, y,
                                       &path, &pos);

    drop_path = get_drop_path (dest, path);

    drop_target = get_drop_target_uri_for_path (dest, drop_path);

    if (path != NULL)
    {
        ctk_tree_path_free (path);
    }

    if (drop_path != NULL)
    {
        ctk_tree_path_free (drop_path);
    }

    return drop_target;
}

static void
receive_uris (BaulTreeViewDragDest *dest,
              CdkDragContext *context,
              GList *source_uris,
              int x, int y)
{
    char *drop_target;
    CdkDragAction action, real_action;

    drop_target = get_drop_target_uri_at_pos (dest, x, y);
    g_assert (drop_target != NULL);

    real_action = cdk_drag_context_get_selected_action (context);

    if (real_action == CDK_ACTION_ASK)
    {
        if (baul_drag_selection_includes_special_link (dest->details->drag_list))
        {
            /* We only want to move the trash */
            action = CDK_ACTION_MOVE;
        }
        else
        {
            action = CDK_ACTION_MOVE | CDK_ACTION_COPY | CDK_ACTION_LINK;
        }
        real_action = baul_drag_drop_action_ask
                      (CTK_WIDGET (dest->details->tree_view), action);
    }

    /* We only want to copy external uris */
    if (dest->details->drag_type == BAUL_ICON_DND_URI_LIST)
    {
        action = CDK_ACTION_COPY;
    }

    if (real_action > 0)
    {
        if (!baul_drag_uris_local (drop_target, source_uris)
                || real_action != CDK_ACTION_MOVE)
        {
            g_signal_emit (dest, signals[MOVE_COPY_ITEMS], 0,
                           source_uris,
                           drop_target,
                           real_action,
                           x, y);
        }
    }

    g_free (drop_target);
}

static void
receive_dropped_icons (BaulTreeViewDragDest *dest,
                       CdkDragContext *context,
                       int x, int y)
{
    GList *source_uris;
    GList *l;

    /* FIXME: ignore local only moves */

    if (!dest->details->drag_list)
    {
        return;
    }

    source_uris = NULL;
    for (l = dest->details->drag_list; l != NULL; l = l->next)
    {
        source_uris = g_list_prepend (source_uris,
                                      ((BaulDragSelectionItem *)l->data)->uri);
    }

    source_uris = g_list_reverse (source_uris);

    receive_uris (dest, context, source_uris, x, y);

    g_list_free (source_uris);
}

static void
receive_dropped_uri_list (BaulTreeViewDragDest *dest,
                          CdkDragContext *context,
                          int x, int y)
{
    char *drop_target;

    if (!dest->details->drag_data)
    {
        return;
    }

    drop_target = get_drop_target_uri_at_pos (dest, x, y);
    g_assert (drop_target != NULL);

    g_signal_emit (dest, signals[HANDLE_URI_LIST], 0,
                   (char*) ctk_selection_data_get_data (dest->details->drag_data),
                   drop_target,
                   cdk_drag_context_get_selected_action (context),
                   x, y);

    g_free (drop_target);
}

static void
receive_dropped_text (BaulTreeViewDragDest *dest,
                      CdkDragContext *context,
                      int x, int y)
{
    char *drop_target;
    char *text;

    if (!dest->details->drag_data)
    {
        return;
    }

    drop_target = get_drop_target_uri_at_pos (dest, x, y);
    g_assert (drop_target != NULL);

    text = ctk_selection_data_get_text (dest->details->drag_data);
    g_signal_emit (dest, signals[HANDLE_TEXT], 0,
                   (char *) text, drop_target,
                   cdk_drag_context_get_selected_action (context),
                   x, y);

    g_free (text);
    g_free (drop_target);
}

static void
receive_dropped_raw (BaulTreeViewDragDest *dest,
                     const char *raw_data, int length,
                     CdkDragContext *context,
                     int x, int y)
{
    char *drop_target;

    if (!dest->details->drag_data)
    {
        return;
    }

    drop_target = get_drop_target_uri_at_pos (dest, x, y);
    g_assert (drop_target != NULL);

    g_signal_emit (dest, signals[HANDLE_RAW], 0,
                   raw_data, length, drop_target,
                   dest->details->direct_save_uri,
                   cdk_drag_context_get_selected_action (context),
                   x, y);

    g_free (drop_target);
}

static void
receive_dropped_netscape_url (BaulTreeViewDragDest *dest,
                              CdkDragContext *context,
                              int x, int y)
{
    char *drop_target;

    if (!dest->details->drag_data)
    {
        return;
    }

    drop_target = get_drop_target_uri_at_pos (dest, x, y);
    g_assert (drop_target != NULL);

    g_signal_emit (dest, signals[HANDLE_NETSCAPE_URL], 0,
                   (char*) ctk_selection_data_get_data (dest->details->drag_data),
                   drop_target,
                   cdk_drag_context_get_selected_action (context),
                   x, y);

    g_free (drop_target);
}

static void
receive_dropped_keyword (BaulTreeViewDragDest *dest,
                         CdkDragContext *context,
                         int x, int y)
{
    char *drop_target_uri;
    BaulFile *drop_target_file;

    if (!dest->details->drag_data)
    {
        return;
    }

    drop_target_uri = get_drop_target_uri_at_pos (dest, x, y);
    g_assert (drop_target_uri != NULL);

    drop_target_file = baul_file_get_by_uri (drop_target_uri);

    if (drop_target_file != NULL)
    {
        baul_drag_file_receive_dropped_keyword (drop_target_file,
                                                (char *) ctk_selection_data_get_data (dest->details->drag_data));
        baul_file_unref (drop_target_file);
    }

    g_free (drop_target_uri);
}

static gboolean
receive_xds (BaulTreeViewDragDest *dest,
             CtkWidget *widget,
             guint32 time,
             CdkDragContext *context,
             int x, int y)
{
    GFile *location;
    const guchar *selection_data;
    gint selection_format;
    gint selection_length;

    selection_data = ctk_selection_data_get_data (dest->details->drag_data);
    selection_format = ctk_selection_data_get_format (dest->details->drag_data);
    selection_length = ctk_selection_data_get_length (dest->details->drag_data);

    if (selection_format == 8
            && selection_length == 1
            && selection_data[0] == 'F')
    {
        ctk_drag_get_data (widget, context,
                           cdk_atom_intern (BAUL_ICON_DND_RAW_TYPE,
                                            FALSE),
                           time);
        return FALSE;
    }
    else if (selection_format == 8
             && selection_length == 1
             && selection_data[0] == 'S')
    {
        g_assert (dest->details->direct_save_uri != NULL);
        location = g_file_new_for_uri (dest->details->direct_save_uri);

        baul_file_changes_queue_file_added (location);
        baul_file_changes_consume_changes (TRUE);

        g_object_unref (location);
    }
    return TRUE;
}


static gboolean
drag_data_received_callback (CtkWidget *widget,
                             CdkDragContext *context,
                             int x,
                             int y,
                             CtkSelectionData *selection_data,
                             guint info,
                             guint32 time,
                             gpointer data)
{
    BaulTreeViewDragDest *dest;
    gboolean success, finished;

    dest = BAUL_TREE_VIEW_DRAG_DEST (data);

    if (!dest->details->have_drag_data)
    {
        dest->details->have_drag_data = TRUE;
        dest->details->drag_type = info;
        dest->details->drag_data =
            ctk_selection_data_copy (selection_data);
        if (info == BAUL_ICON_DND_CAFE_ICON_LIST)
        {
            dest->details->drag_list =
                baul_drag_build_selection_list (selection_data);
        }
    }

    if (dest->details->drop_occurred)
    {
        success = FALSE;
        finished = TRUE;
        switch (info)
        {
        case BAUL_ICON_DND_CAFE_ICON_LIST :
            receive_dropped_icons (dest, context, x, y);
            success = TRUE;
            break;
        case BAUL_ICON_DND_NETSCAPE_URL :
            receive_dropped_netscape_url (dest, context, x, y);
            success = TRUE;
            break;
        case BAUL_ICON_DND_URI_LIST :
            receive_dropped_uri_list (dest, context, x, y);
            success = TRUE;
            break;
        case BAUL_ICON_DND_TEXT:
            receive_dropped_text (dest, context, x, y);
            success = TRUE;
            break;
        case BAUL_ICON_DND_RAW:
        {
            const char *tmp;
            int length;

            length = ctk_selection_data_get_length (selection_data);
            tmp = ctk_selection_data_get_data (selection_data);
            receive_dropped_raw (dest, tmp, length, context, x, y);
            success = TRUE;
            break;
        }
        case BAUL_ICON_DND_KEYWORD:
            receive_dropped_keyword (dest, context, x, y);
            success = TRUE;
            break;
        case BAUL_ICON_DND_XDNDDIRECTSAVE:
            finished = receive_xds (dest, widget, time, context, x, y);
            success = TRUE;
            break;
        }

        if (finished)
        {
            dest->details->drop_occurred = FALSE;
            free_drag_data (dest);
            ctk_drag_finish (context, success, FALSE, time);
        }
    }

    /* appease CtkTreeView by preventing its drag_data_receive
     * from being called */
    g_signal_stop_emission_by_name (dest->details->tree_view,
                                    "drag_data_received");

    return TRUE;
}

static char *
get_direct_save_filename (CdkDragContext *context)
{
    guchar *prop_text;
    gint prop_len;

    if (!cdk_property_get (cdk_drag_context_get_source_window (context), cdk_atom_intern (BAUL_ICON_DND_XDNDDIRECTSAVE_TYPE, FALSE),
                           cdk_atom_intern ("text/plain", FALSE), 0, 1024, FALSE, NULL, NULL,
                           &prop_len, &prop_text))
    {
        return NULL;
    }

    /* Zero-terminate the string */
    prop_text = g_realloc (prop_text, prop_len + 1);
    prop_text[prop_len] = '\0';

    /* Verify that the file name provided by the source is valid */
    if (*prop_text == '\0' ||
            strchr ((const gchar *) prop_text, G_DIR_SEPARATOR) != NULL)
    {
        baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                        "Invalid filename provided by XDS drag site");
        g_free (prop_text);
        return NULL;
    }

    return prop_text;
}

static gboolean
set_direct_save_uri (BaulTreeViewDragDest *dest,
                     CdkDragContext *context,
                     int x, int y)
{
    char *drop_uri;
    char *uri;

    g_assert (dest->details->direct_save_uri == NULL);

    uri = NULL;

    drop_uri = get_drop_target_uri_at_pos (dest, x, y);
    if (drop_uri != NULL)
    {
        char *filename;

        filename = get_direct_save_filename (context);
        if (filename != NULL)
        {
            GFile *base, *child;

            /* Resolve relative path */
            base = g_file_new_for_uri (drop_uri);
            child = g_file_get_child (base, filename);
            uri = g_file_get_uri (child);

            g_object_unref (base);
            g_object_unref (child);
            g_free (filename);

            /* Change the property */
            cdk_property_change (cdk_drag_context_get_source_window (context),
                                 cdk_atom_intern (BAUL_ICON_DND_XDNDDIRECTSAVE_TYPE, FALSE),
                                 cdk_atom_intern ("text/plain", FALSE), 8,
                                 CDK_PROP_MODE_REPLACE, (const guchar *) uri,
                                 strlen (uri));

            dest->details->direct_save_uri = uri;
        }
        else
        {
            baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                            "Invalid filename provided by XDS drag site");
        }
    }
    else
    {
        baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                        "Could not retrieve XDS drop destination");
    }

    return uri != NULL;
}

static gboolean
drag_drop_callback (CtkWidget *widget,
                    CdkDragContext *context,
                    int x,
                    int y,
                    guint32 time,
                    gpointer data)
{
    BaulTreeViewDragDest *dest;
    guint info;
    CdkAtom target;

    dest = BAUL_TREE_VIEW_DRAG_DEST (data);

    target = ctk_drag_dest_find_target (CTK_WIDGET (dest->details->tree_view),
                                        context,
                                        NULL);
    if (target == CDK_NONE)
    {
        return FALSE;
    }

    info = dest->details->drag_type;

    if (info == BAUL_ICON_DND_XDNDDIRECTSAVE)
    {
        /* We need to set this or get_drop_path will fail, and it
           was unset by drag_leave_callback */
        dest->details->have_drag_data = TRUE;
        if (!set_direct_save_uri (dest, context, x, y))
        {
            return FALSE;
        }
        dest->details->have_drag_data = FALSE;
    }

    dest->details->drop_occurred = TRUE;

    get_drag_data (dest, context, time);
    remove_scroll_timeout (dest);
    remove_expand_timeout (dest);
    clear_drag_dest_row (dest);

    return TRUE;
}

static void
tree_view_weak_notify (gpointer user_data,
                       GObject *object)
{
    BaulTreeViewDragDest *dest;

    dest = BAUL_TREE_VIEW_DRAG_DEST (user_data);

    remove_scroll_timeout (dest);
    remove_expand_timeout (dest);

    dest->details->tree_view = NULL;
}

static void
baul_tree_view_drag_dest_dispose (GObject *object)
{
    BaulTreeViewDragDest *dest;

    dest = BAUL_TREE_VIEW_DRAG_DEST (object);

    if (dest->details->tree_view)
    {
        g_object_weak_unref (G_OBJECT (dest->details->tree_view),
                             tree_view_weak_notify,
                             dest);
    }

    remove_scroll_timeout (dest);
    remove_expand_timeout (dest);

    EEL_CALL_PARENT (G_OBJECT_CLASS, dispose, (object));
}

static void
baul_tree_view_drag_dest_finalize (GObject *object)
{
    BaulTreeViewDragDest *dest;

    dest = BAUL_TREE_VIEW_DRAG_DEST (object);

    free_drag_data (dest);

    g_free (dest->details);

    EEL_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
baul_tree_view_drag_dest_init (BaulTreeViewDragDest *dest)
{
    dest->details = g_new0 (BaulTreeViewDragDestDetails, 1);
}

static void
baul_tree_view_drag_dest_class_init (BaulTreeViewDragDestClass *class)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (class);

    gobject_class->dispose = baul_tree_view_drag_dest_dispose;
    gobject_class->finalize = baul_tree_view_drag_dest_finalize;

    signals[GET_ROOT_URI] =
        g_signal_new ("get_root_uri",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulTreeViewDragDestClass,
                                       get_root_uri),
                      NULL, NULL,
                      baul_marshal_STRING__VOID,
                      G_TYPE_STRING, 0);
    signals[GET_FILE_FOR_PATH] =
        g_signal_new ("get_file_for_path",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulTreeViewDragDestClass,
                                       get_file_for_path),
                      NULL, NULL,
                      baul_marshal_OBJECT__BOXED,
                      BAUL_TYPE_FILE, 1,
                      CTK_TYPE_TREE_PATH);
    signals[MOVE_COPY_ITEMS] =
        g_signal_new ("move_copy_items",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulTreeViewDragDestClass,
                                       move_copy_items),
                      NULL, NULL,

                      baul_marshal_VOID__POINTER_STRING_ENUM_INT_INT,
                      G_TYPE_NONE, 5,
                      G_TYPE_POINTER,
                      G_TYPE_STRING,
                      CDK_TYPE_DRAG_ACTION,
                      G_TYPE_INT,
                      G_TYPE_INT);
    signals[HANDLE_NETSCAPE_URL] =
        g_signal_new ("handle_netscape_url",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulTreeViewDragDestClass,
                                       handle_netscape_url),
                      NULL, NULL,
                      baul_marshal_VOID__STRING_STRING_ENUM_INT_INT,
                      G_TYPE_NONE, 5,
                      G_TYPE_STRING,
                      G_TYPE_STRING,
                      CDK_TYPE_DRAG_ACTION,
                      G_TYPE_INT,
                      G_TYPE_INT);
    signals[HANDLE_URI_LIST] =
        g_signal_new ("handle_uri_list",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulTreeViewDragDestClass,
                                       handle_uri_list),
                      NULL, NULL,
                      baul_marshal_VOID__STRING_STRING_ENUM_INT_INT,
                      G_TYPE_NONE, 5,
                      G_TYPE_STRING,
                      G_TYPE_STRING,
                      CDK_TYPE_DRAG_ACTION,
                      G_TYPE_INT,
                      G_TYPE_INT);
    signals[HANDLE_TEXT] =
        g_signal_new ("handle_text",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulTreeViewDragDestClass,
                                       handle_text),
                      NULL, NULL,
                      baul_marshal_VOID__STRING_STRING_ENUM_INT_INT,
                      G_TYPE_NONE, 5,
                      G_TYPE_STRING,
                      G_TYPE_STRING,
                      CDK_TYPE_DRAG_ACTION,
                      G_TYPE_INT,
                      G_TYPE_INT);
    signals[HANDLE_RAW] =
        g_signal_new ("handle_raw",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulTreeViewDragDestClass,
                                       handle_raw),
                      NULL, NULL,
                      baul_marshal_VOID__POINTER_INT_STRING_STRING_ENUM_INT_INT,
                      G_TYPE_NONE, 7,
                      G_TYPE_POINTER,
                      G_TYPE_INT,
                      G_TYPE_STRING,
                      G_TYPE_STRING,
                      CDK_TYPE_DRAG_ACTION,
                      G_TYPE_INT,
                      G_TYPE_INT);
}



BaulTreeViewDragDest *
baul_tree_view_drag_dest_new (CtkTreeView *tree_view)
{
    BaulTreeViewDragDest *dest;
    CtkTargetList *targets;

    dest = g_object_new (BAUL_TYPE_TREE_VIEW_DRAG_DEST, NULL);

    dest->details->tree_view = tree_view;
    g_object_weak_ref (G_OBJECT (dest->details->tree_view),
                       tree_view_weak_notify, dest);

    ctk_drag_dest_set (CTK_WIDGET (tree_view),
                       0, drag_types, G_N_ELEMENTS (drag_types),
                       CDK_ACTION_MOVE | CDK_ACTION_COPY | CDK_ACTION_LINK | CDK_ACTION_ASK);

    targets = ctk_drag_dest_get_target_list (CTK_WIDGET (tree_view));
    ctk_target_list_add_text_targets (targets, BAUL_ICON_DND_TEXT);

    g_signal_connect_object (tree_view,
                             "drag_motion",
                             G_CALLBACK (drag_motion_callback),
                             dest, 0);
    g_signal_connect_object (tree_view,
                             "drag_leave",
                             G_CALLBACK (drag_leave_callback),
                             dest, 0);
    g_signal_connect_object (tree_view,
                             "drag_drop",
                             G_CALLBACK (drag_drop_callback),
                             dest, 0);
    g_signal_connect_object (tree_view,
                             "drag_data_received",
                             G_CALLBACK (drag_data_received_callback),
                             dest, 0);

    return dest;
}
