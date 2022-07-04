/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-window-slot-info.h: Interface for baul window slots

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

#ifndef BAUL_WINDOW_SLOT_INFO_H
#define BAUL_WINDOW_SLOT_INFO_H

#include "baul-window-info.h"
#include "baul-view.h"


#define BAUL_TYPE_WINDOW_SLOT_INFO           (baul_window_slot_info_get_type ())
#define BAUL_WINDOW_SLOT_INFO(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_WINDOW_SLOT_INFO, BaulWindowSlotInfo))
#define BAUL_IS_WINDOW_SLOT_INFO(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_WINDOW_SLOT_INFO))
#define BAUL_WINDOW_SLOT_INFO_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAUL_TYPE_WINDOW_SLOT_INFO, BaulWindowSlotInfoIface))

typedef struct _BaulWindowSlotInfoIface BaulWindowSlotInfoIface;

struct _BaulWindowSlotInfoIface
{
    GTypeInterface g_iface;

    /* signals */

    /* emitted right after this slot becomes active.
     * Views should connect to this signal and merge their UI
     * into the main window.
     */
    void  (* active)  (BaulWindowSlotInfo *slot);
    /* emitted right before this slot becomes inactive.
     * Views should connect to this signal and unmerge their UI
     * from the main window.
     */
    void  (* inactive) (BaulWindowSlotInfo *slot);

    /* returns the window info associated with this slot */
    BaulWindowInfo * (* get_window) (BaulWindowSlotInfo *slot);

    /* Returns the number of selected items in the view */
    int  (* get_selection_count)  (BaulWindowSlotInfo    *slot);

    /* Returns a list of uris for th selected items in the view, caller frees it */
    GList *(* get_selection)      (BaulWindowSlotInfo    *slot);

    char * (* get_current_location)  (BaulWindowSlotInfo *slot);
    BaulView * (* get_current_view) (BaulWindowSlotInfo *slot);
    void   (* set_status)            (BaulWindowSlotInfo *slot,
                                      const char *status);
    char * (* get_title)             (BaulWindowSlotInfo *slot);

    void   (* open_location)      (BaulWindowSlotInfo *slot,
                                   GFile *location,
                                   BaulWindowOpenMode mode,
                                   BaulWindowOpenFlags flags,
                                   GList *selection,
                                   BaulWindowGoToCallback callback,
                                   gpointer user_data);
    void   (* make_hosting_pane_active) (BaulWindowSlotInfo *slot);
};


GType                             baul_window_slot_info_get_type            (void);
BaulWindowInfo *              baul_window_slot_info_get_window          (BaulWindowSlotInfo            *slot);
#define baul_window_slot_info_open_location(slot, location, mode, flags, selection) \
	baul_window_slot_info_open_location_full(slot, location, mode, \
						 flags, selection, NULL, NULL)

void                              baul_window_slot_info_open_location_full
	(BaulWindowSlotInfo *slot,
        GFile                             *location,
        BaulWindowOpenMode                 mode,
        BaulWindowOpenFlags                flags,
        GList                             *selection,
        BaulWindowGoToCallback		   callback,
        gpointer			   user_data);
void                              baul_window_slot_info_set_status          (BaulWindowSlotInfo            *slot,
        const char *status);
void                              baul_window_slot_info_make_hosting_pane_active (BaulWindowSlotInfo       *slot);

char *                            baul_window_slot_info_get_current_location (BaulWindowSlotInfo           *slot);
BaulView *                    baul_window_slot_info_get_current_view     (BaulWindowSlotInfo           *slot);
int                               baul_window_slot_info_get_selection_count  (BaulWindowSlotInfo           *slot);
GList *                           baul_window_slot_info_get_selection        (BaulWindowSlotInfo           *slot);
char *                            baul_window_slot_info_get_title            (BaulWindowSlotInfo           *slot);

#endif /* BAUL_WINDOW_SLOT_INFO_H */
