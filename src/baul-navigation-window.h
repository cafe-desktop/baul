/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/*
 *  Baul
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 1999, 2000, 2001 Eazel, Inc.
 *  Copyright (C) 2003 Ximian, Inc.
 *
 *  Baul is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Baul is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Authors: Elliot Lee <sopwith@redhat.com>
 *           Darin Adler <darin@bentspoon.com>
 *
 */
/* baul-navigation-window.h: Interface of the navigation window object */

#ifndef BAUL_NAVIGATION_WINDOW_H
#define BAUL_NAVIGATION_WINDOW_H

#include <ctk/ctk.h>

#include <eel/eel-glib-extensions.h>

#include <libbaul-private/baul-bookmark.h>
#include <libbaul-private/baul-sidebar.h>

#include "baul-application.h"
#include "baul-information-panel.h"
#include "baul-side-pane.h"
#include "baul-window.h"

#define BAUL_TYPE_NAVIGATION_WINDOW baul_navigation_window_get_type()
#define BAUL_NAVIGATION_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_NAVIGATION_WINDOW, BaulNavigationWindow))
#define BAUL_NAVIGATION_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_NAVIGATION_WINDOW, BaulNavigationWindowClass))
#define BAUL_IS_NAVIGATION_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_NAVIGATION_WINDOW))
#define BAUL_IS_NAVIGATION_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_NAVIGATION_WINDOW))
#define BAUL_NAVIGATION_WINDOW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_NAVIGATION_WINDOW, BaulNavigationWindowClass))

typedef struct _BaulNavigationWindow        BaulNavigationWindow;
typedef struct _BaulNavigationWindowClass   BaulNavigationWindowClass;
typedef struct _BaulNavigationWindowPrivate BaulNavigationWindowPrivate;


struct _BaulNavigationWindow
{
    BaulWindow parent_object;

    BaulNavigationWindowPrivate *details;

    /** UI stuff **/
    BaulSidePane *sidebar;

    /* Current views stuff */
    GList *sidebar_panels;
};


struct _BaulNavigationWindowClass
{
    BaulWindowClass parent_spot;
};

GType    baul_navigation_window_get_type             (void);
void     baul_navigation_window_allow_back           (BaulNavigationWindow *window,
        gboolean                  allow);
void     baul_navigation_window_allow_forward        (BaulNavigationWindow *window,
        gboolean                  allow);
void     baul_navigation_window_clear_back_list      (BaulNavigationWindow *window);
void     baul_navigation_window_clear_forward_list   (BaulNavigationWindow *window);
void     baul_forget_history                         (void);
gint     baul_navigation_window_get_base_page_index  (BaulNavigationWindow *window);
void     baul_navigation_window_hide_toolbar         (BaulNavigationWindow *window);
void     baul_navigation_window_show_toolbar         (BaulNavigationWindow *window);
gboolean baul_navigation_window_toolbar_showing      (BaulNavigationWindow *window);
void     baul_navigation_window_hide_sidebar         (BaulNavigationWindow *window);
void     baul_navigation_window_show_sidebar         (BaulNavigationWindow *window);
gboolean baul_navigation_window_sidebar_showing      (BaulNavigationWindow *window);
void     baul_navigation_window_add_sidebar_panel    (BaulNavigationWindow *window,
        BaulSidebar          *sidebar_panel);
void     baul_navigation_window_remove_sidebar_panel (BaulNavigationWindow *window,
        BaulSidebar          *sidebar_panel);
void     baul_navigation_window_hide_status_bar      (BaulNavigationWindow *window);
void     baul_navigation_window_show_status_bar      (BaulNavigationWindow *window);
gboolean baul_navigation_window_status_bar_showing   (BaulNavigationWindow *window);
void     baul_navigation_window_back_or_forward      (BaulNavigationWindow *window,
        gboolean                  back,
        guint                     distance,
        gboolean                  new_tab);
void     baul_navigation_window_show_search          (BaulNavigationWindow *window);
void     baul_navigation_window_unset_focus_widget   (BaulNavigationWindow *window);
void     baul_navigation_window_hide_search          (BaulNavigationWindow *window);
void     baul_navigation_window_set_search_button	 (BaulNavigationWindow *window,
        gboolean		    state);
void     baul_navigation_window_restore_focus_widget (BaulNavigationWindow *window);
void     baul_navigation_window_split_view_on        (BaulNavigationWindow *window);
void     baul_navigation_window_split_view_off       (BaulNavigationWindow *window);
gboolean baul_navigation_window_split_view_showing   (BaulNavigationWindow *window);

gboolean baul_navigation_window_is_in_temporary_navigation_bar (GtkWidget *widget,
        BaulNavigationWindow *window);
gboolean baul_navigation_window_is_in_temporary_search_bar (GtkWidget *widget,
        BaulNavigationWindow *window);

#endif
