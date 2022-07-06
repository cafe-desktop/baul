/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-ctk-extensions.h - interface for new functions that operate on
  			       ctk classes. Perhaps some of these should be
  			       rolled into ctk someday.

   Copyright (C) 1999, 2000, 2001 Eazel, Inc.

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

   Authors: John Sullivan <sullivan@eazel.com>
            Ramiro Estrugo <ramiro@eazel.com>
*/

#ifndef EEL_CTK_EXTENSIONS_H
#define EEL_CTK_EXTENSIONS_H

#include <ctk/ctk.h>
#include "eel-cdk-extensions.h"

/* CtkWindow */
void                  eel_ctk_window_set_initial_geometry             (CtkWindow            *window,
        EelGdkGeometryFlags   geometry_flags,
        int                   left,
        int                   top,
        guint                 width,
        guint                 height);
void                  eel_ctk_window_set_initial_geometry_from_string (CtkWindow            *window,
        const char           *geometry_string,
        guint                 minimum_width,
        guint                 minimum_height,
        gboolean		     ignore_position);
char *                eel_ctk_window_get_geometry_string              (CtkWindow            *window);


/* CtkMenu and CtkMenuItem */
void                  eel_pop_up_context_menu                         (CtkMenu              *menu,
        GdkEventButton       *event);
CtkMenuItem *         eel_ctk_menu_append_separator                   (CtkMenu              *menu);
CtkMenuItem *         eel_ctk_menu_insert_separator                   (CtkMenu              *menu,
        int                   index);

/* CtkMenuToolButton */
CtkWidget *           eel_ctk_menu_tool_button_get_button             (CtkMenuToolButton    *tool_button);

/* CtkLabel */
void                  eel_ctk_label_make_bold                         (CtkLabel             *label);

/* CtkTreeView */
void                  eel_ctk_tree_view_set_activate_on_single_click  (CtkTreeView          *tree_view,
                                                                       gboolean              should_activate);

/* CtkMessageDialog */
void                  eel_ctk_message_dialog_set_details_label        (CtkMessageDialog     *dialog,
                                                                       const gchar          *details_text);

CtkWidget *           eel_image_menu_item_new_from_icon               (const gchar          *icon_name,
                                                                       const gchar          *label_name);

CtkWidget *           eel_image_menu_item_new_from_surface              (cairo_surface_t    *icon_surface,
                                                                         const gchar        *label_name);

gboolean              eel_dialog_page_scroll_event_callback           (CtkWidget            *widget,
                                                                       GdkEventScroll       *event,
                                                                       CtkWindow            *window);
#endif /* EEL_CTK_EXTENSIONS_H */
