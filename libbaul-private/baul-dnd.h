/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-dnd.h - Common Drag & drop handling code shared by the icon container
   and the list view.

   Copyright (C) 2000 Eazel, Inc.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Pavel Cisler <pavel@eazel.com>,
	    Ettore Perazzoli <ettore@gnu.org>
*/

#ifndef BAUL_DND_H
#define BAUL_DND_H

#include <gtk/gtk.h>

#include "baul-window-slot-info.h"

/* Drag & Drop target names. */
#define BAUL_ICON_DND_MATE_ICON_LIST_TYPE	"x-special/cafe-icon-list"
#define BAUL_ICON_DND_URI_LIST_TYPE		"text/uri-list"
#define BAUL_ICON_DND_NETSCAPE_URL_TYPE	"_NETSCAPE_URL"
#define BAUL_ICON_DND_COLOR_TYPE		"application/x-color"
#define BAUL_ICON_DND_BGIMAGE_TYPE		"property/bgimage"
#define BAUL_ICON_DND_KEYWORD_TYPE		"property/keyword"
#define BAUL_ICON_DND_RESET_BACKGROUND_TYPE "x-special/cafe-reset-background"
#define BAUL_ICON_DND_ROOTWINDOW_DROP_TYPE	"application/x-rootwindow-drop"
#define BAUL_ICON_DND_XDNDDIRECTSAVE_TYPE	"XdndDirectSave0" /* XDS Protocol Type */
#define BAUL_ICON_DND_RAW_TYPE	"application/octet-stream"

/* Item of the drag selection list */
typedef struct
{
    char *uri;
    gboolean got_icon_position;
    int icon_x, icon_y;
    int icon_width, icon_height;
} BaulDragSelectionItem;

/* Standard Drag & Drop types. */
typedef enum
{
    BAUL_ICON_DND_MATE_ICON_LIST,
    BAUL_ICON_DND_URI_LIST,
    BAUL_ICON_DND_NETSCAPE_URL,
    BAUL_ICON_DND_COLOR,
    BAUL_ICON_DND_BGIMAGE,
    BAUL_ICON_DND_KEYWORD,
    BAUL_ICON_DND_TEXT,
    BAUL_ICON_DND_RESET_BACKGROUND,
    BAUL_ICON_DND_XDNDDIRECTSAVE,
    BAUL_ICON_DND_RAW,
    BAUL_ICON_DND_ROOTWINDOW_DROP
} BaulIconDndTargetType;

typedef enum
{
    BAUL_DND_ACTION_FIRST = GDK_ACTION_ASK << 1,
    BAUL_DND_ACTION_SET_AS_BACKGROUND = BAUL_DND_ACTION_FIRST << 0,
    BAUL_DND_ACTION_SET_AS_FOLDER_BACKGROUND = BAUL_DND_ACTION_FIRST << 1,
    BAUL_DND_ACTION_SET_AS_GLOBAL_BACKGROUND = BAUL_DND_ACTION_FIRST << 2
} BaulDndAction;

/* drag&drop-related information. */
typedef struct
{
    GtkTargetList *target_list;

    /* Stuff saved at "receive data" time needed later in the drag. */
    gboolean got_drop_data_type;
    BaulIconDndTargetType data_type;
    GtkSelectionData *selection_data;
    char *direct_save_uri;

    /* Start of the drag, in window coordinates. */
    int start_x, start_y;

    /* List of BaulDragSelectionItems, representing items being dragged, or NULL
     * if data about them has not been received from the source yet.
     */
    GList *selection_list;

    /* has the drop occured ? */
    gboolean drop_occured;

    /* whether or not need to clean up the previous dnd data */
    gboolean need_to_destroy;

    /* autoscrolling during dragging */
    int auto_scroll_timeout_id;
    gboolean waiting_to_autoscroll;
    gint64 start_auto_scroll_in;

} BaulDragInfo;

typedef struct
{
    /* NB: the following elements are managed by us */
    gboolean have_data;
    gboolean have_valid_data;

    gboolean drop_occured;

    unsigned int info;
    union
    {
        GList *selection_list;
        GList *uri_list;
        char *netscape_url;
    } data;

    /* NB: the following elements are managed by the caller of
     *   baul_drag_slot_proxy_init() */

    /* a fixed location, or NULL to use slot's location */
    GFile *target_location;
    /* a fixed slot, or NULL to use the window's active slot */
    BaulWindowSlotInfo *target_slot;
} BaulDragSlotProxyInfo;

typedef void		(* BaulDragEachSelectedItemDataGet)	(const char *url,
        int x, int y, int w, int h,
        gpointer data);
typedef void		(* BaulDragEachSelectedItemIterator)	(BaulDragEachSelectedItemDataGet iteratee,
        gpointer iterator_context,
        gpointer data);

void			    baul_drag_init				(BaulDragInfo		      *drag_info,
        const GtkTargetEntry		      *drag_types,
        int				       drag_type_count,
        gboolean			       add_text_targets);
void			    baul_drag_finalize			(BaulDragInfo		      *drag_info);
BaulDragSelectionItem  *baul_drag_selection_item_new		(void);
void			    baul_drag_destroy_selection_list	(GList				      *selection_list);
GList			   *baul_drag_build_selection_list		(GtkSelectionData		      *data);

GList *			    baul_drag_uri_list_from_selection_list	(const GList			      *selection_list);

GList *			    baul_drag_uri_list_from_array		(const char			     **uris);

gboolean		    baul_drag_items_local			(const char			      *target_uri,
        const GList			      *selection_list);
gboolean		    baul_drag_uris_local			(const char			      *target_uri,
        const GList			      *source_uri_list);
gboolean		    baul_drag_items_on_desktop		(const GList			      *selection_list);
void			    baul_drag_default_drop_action_for_icons (GdkDragContext			      *context,
        const char			      *target_uri,
        const GList			      *items,
        int				      *action);
GdkDragAction		    baul_drag_default_drop_action_for_netscape_url (GdkDragContext			     *context);
GdkDragAction		    baul_drag_default_drop_action_for_uri_list     (GdkDragContext			     *context,
        const char			     *target_uri_string);
gboolean		    baul_drag_drag_data_get			(GtkWidget			      *widget,
        GdkDragContext			      *context,
        GtkSelectionData		      *selection_data,
        guint				       info,
        guint32			       time,
        gpointer			       container_context,
        BaulDragEachSelectedItemIterator  each_selected_item_iterator);
int			    baul_drag_modifier_based_action		(int				       default_action,
        int				       non_default_action);

GdkDragAction		    baul_drag_drop_action_ask		(GtkWidget			      *widget,
        GdkDragAction			       possible_actions);
GdkDragAction		    baul_drag_drop_background_ask		(GtkWidget			      *widget,
        GdkDragAction			       possible_actions);

gboolean		    baul_drag_autoscroll_in_scroll_region	(GtkWidget			      *widget);
void			    baul_drag_autoscroll_calculate_delta	(GtkWidget			      *widget,
        float				      *x_scroll_delta,
        float				      *y_scroll_delta);
void			    baul_drag_autoscroll_start		(BaulDragInfo		      *drag_info,
        GtkWidget			      *widget,
        GSourceFunc			       callback,
        gpointer			       user_data);
void			    baul_drag_autoscroll_stop		(BaulDragInfo		      *drag_info);

gboolean		    baul_drag_selection_includes_special_link (GList			      *selection_list);

void                        baul_drag_slot_proxy_init               (GtkWidget *widget,
        BaulDragSlotProxyInfo *drag_info);

#endif
