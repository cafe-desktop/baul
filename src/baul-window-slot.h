/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-window-slot.h: Baul window slot

   Copyright (C) 2008 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Author: Christian Neumair <cneumair@gnome.org>
*/

#ifndef BAUL_WINDOW_SLOT_H
#define BAUL_WINDOW_SLOT_H

#include "baul-window-pane.h"
#include "baul-query-editor.h"
#include <glib/gi18n.h>

#define BAUL_TYPE_WINDOW_SLOT	 (baul_window_slot_get_type())
#define BAUL_WINDOW_SLOT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BAUL_TYPE_WINDOW_SLOT, BaulWindowSlotClass))
#define BAUL_WINDOW_SLOT(obj)	 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_WINDOW_SLOT, BaulWindowSlot))
#define BAUL_IS_WINDOW_SLOT(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_WINDOW_SLOT))
#define BAUL_IS_WINDOW_SLOT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BAUL_TYPE_WINDOW_SLOT))
#define BAUL_WINDOW_SLOT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BAUL_TYPE_WINDOW_SLOT, BaulWindowSlotClass))

typedef enum
{
    BAUL_LOCATION_CHANGE_STANDARD,
    BAUL_LOCATION_CHANGE_BACK,
    BAUL_LOCATION_CHANGE_FORWARD,
    BAUL_LOCATION_CHANGE_RELOAD,
    BAUL_LOCATION_CHANGE_REDIRECT,
    BAUL_LOCATION_CHANGE_FALLBACK
} BaulLocationChangeType;

struct BaulWindowSlotClass
{
    GObjectClass parent_class;

    /* wrapped BaulWindowInfo signals, for overloading */
    void (* active)   (BaulWindowSlot *slot);
    void (* inactive) (BaulWindowSlot *slot);

    void (* update_query_editor) (BaulWindowSlot *slot);
};

/* Each BaulWindowSlot corresponds to
 * a location in the window for displaying
 * a BaulView.
 *
 * For navigation windows, this would be a
 * tab, while spatial windows only have one slot.
 */
struct BaulWindowSlot
{
    GObject parent;

    BaulWindowPane *pane;

    /* content_box contains
     *  1) an event box containing extra_location_widgets
     *  2) the view box for the content view
     */
    GtkWidget *content_box;
    GtkWidget *extra_location_frame;
    GtkWidget *extra_location_widgets;
    GtkWidget *view_box;

    BaulView *content_view;
    BaulView *new_content_view;

    /* Information about bookmarks */
    BaulBookmark *current_location_bookmark;
    BaulBookmark *last_location_bookmark;

    /* Current location. */
    GFile *location;
    char *title;
    char *status_text;

    BaulFile *viewed_file;
    gboolean viewed_file_seen;
    gboolean viewed_file_in_trash;

    gboolean allow_stop;

    BaulQueryEditor *query_editor;

    /* New location. */
    BaulLocationChangeType location_change_type;
    guint location_change_distance;
    GFile *pending_location;
    char *pending_scroll_to;
    GList *pending_selection;
    BaulFile *determine_view_file;
    GCancellable *mount_cancellable;
    GError *mount_error;
    gboolean tried_mount;
    BaulWindowGoToCallback open_callback;
    gpointer open_callback_user_data;

    GCancellable *find_mount_cancellable;

    gboolean visible;
};

GType   baul_window_slot_get_type (void);

char *  baul_window_slot_get_title			   (BaulWindowSlot *slot);
void    baul_window_slot_update_title		   (BaulWindowSlot *slot);
void    baul_window_slot_update_icon		   (BaulWindowSlot *slot);
void    baul_window_slot_update_query_editor	   (BaulWindowSlot *slot);

GFile * baul_window_slot_get_location		   (BaulWindowSlot *slot);
char *  baul_window_slot_get_location_uri		   (BaulWindowSlot *slot);

void    baul_window_slot_close			   (BaulWindowSlot *slot);
void    baul_window_slot_reload			   (BaulWindowSlot *slot);

void			baul_window_slot_open_location	      (BaulWindowSlot	*slot,
        GFile			*location,
        gboolean			 close_behind);
void			baul_window_slot_open_location_with_selection (BaulWindowSlot	    *slot,
        GFile		    *location,
        GList		    *selection,
        gboolean		     close_behind);
void			baul_window_slot_open_location_full       (BaulWindowSlot	*slot,
        GFile			*location,
        BaulWindowOpenMode	 mode,
        BaulWindowOpenFlags	 flags,
        GList			*new_selection,
        BaulWindowGoToCallback   callback,
        gpointer		 user_data);
void			baul_window_slot_stop_loading	      (BaulWindowSlot	*slot);

void			baul_window_slot_set_content_view	      (BaulWindowSlot	*slot,
        const char		*id);
const char	       *baul_window_slot_get_content_view_id      (BaulWindowSlot	*slot);
gboolean		baul_window_slot_content_view_matches_iid (BaulWindowSlot	*slot,
        const char		*iid);

void                    baul_window_slot_connect_content_view     (BaulWindowSlot       *slot,
        BaulView             *view);
void                    baul_window_slot_disconnect_content_view  (BaulWindowSlot       *slot,
        BaulView             *view);

#define baul_window_slot_go_to(slot,location, new_tab) \
	baul_window_slot_open_location_full(slot, location, BAUL_WINDOW_OPEN_ACCORDING_TO_MODE, \
						(new_tab ? BAUL_WINDOW_OPEN_FLAG_NEW_TAB : 0), \
						NULL, NULL, NULL)

#define baul_window_slot_go_to_full(slot, location, new_tab, callback, user_data) \
	baul_window_slot_open_location_full(slot, location, BAUL_WINDOW_OPEN_ACCORDING_TO_MODE, \
						(new_tab ? BAUL_WINDOW_OPEN_FLAG_NEW_TAB : 0), \
						NULL, callback, user_data)

#define baul_window_slot_go_to_with_selection(slot,location,new_selection) \
	baul_window_slot_open_location_with_selection(slot, location, new_selection, FALSE)

void    baul_window_slot_go_home			   (BaulWindowSlot *slot,
        gboolean            new_tab);
void    baul_window_slot_go_up			   (BaulWindowSlot *slot,
        gboolean           close_behind);

void    baul_window_slot_set_content_view_widget	   (BaulWindowSlot *slot,
        BaulView       *content_view);
void    baul_window_slot_set_viewed_file		   (BaulWindowSlot *slot,
        BaulFile      *file);
void    baul_window_slot_set_allow_stop		   (BaulWindowSlot *slot,
        gboolean	    allow_stop);
void    baul_window_slot_set_status			   (BaulWindowSlot *slot,
        const char	 *status);

void    baul_window_slot_add_extra_location_widget     (BaulWindowSlot *slot,
        GtkWidget       *widget);
void    baul_window_slot_remove_extra_location_widgets (BaulWindowSlot *slot);

void    baul_window_slot_add_current_location_to_history_list (BaulWindowSlot *slot);

void    baul_window_slot_is_in_active_pane (BaulWindowSlot *slot, gboolean is_active);

#endif /* BAUL_WINDOW_SLOT_H */
