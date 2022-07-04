/*
 *  Caja
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 1999, 2000, 2001 Eazel, Inc.
 *
 *  Caja is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Caja is distributed in the hope that it will be useful,
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
/* baul-window.h: Interface of the main window object */

#ifndef BAUL_WINDOW_H
#define BAUL_WINDOW_H

#include <gtk/gtk.h>

#include <eel/eel-glib-extensions.h>

#include <libbaul-private/baul-bookmark.h>
#include <libbaul-private/baul-entry.h>
#include <libbaul-private/baul-window-info.h>
#include <libbaul-private/baul-search-directory.h>

#include "baul-application.h"
#include "baul-information-panel.h"
#include "baul-side-pane.h"

#define BAUL_TYPE_WINDOW baul_window_get_type()
#define BAUL_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_WINDOW, CajaWindow))
#define BAUL_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_WINDOW, CajaWindowClass))
#define BAUL_IS_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_WINDOW))
#define BAUL_IS_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_WINDOW))
#define BAUL_WINDOW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_WINDOW, CajaWindowClass))

#ifndef BAUL_WINDOW_DEFINED
#define BAUL_WINDOW_DEFINED
typedef struct CajaWindow CajaWindow;
#endif

#ifndef BAUL_WINDOW_SLOT_DEFINED
#define BAUL_WINDOW_SLOT_DEFINED
typedef struct CajaWindowSlot CajaWindowSlot;
#endif

typedef struct _CajaWindowPane      CajaWindowPane;

typedef struct CajaWindowSlotClass CajaWindowSlotClass;
typedef enum CajaWindowOpenSlotFlags CajaWindowOpenSlotFlags;

typedef enum
{
    BAUL_WINDOW_NOT_SHOWN,
    BAUL_WINDOW_POSITION_SET,
    BAUL_WINDOW_SHOULD_SHOW
} CajaWindowShowState;

enum CajaWindowOpenSlotFlags
{
    BAUL_WINDOW_OPEN_SLOT_NONE = 0,
    BAUL_WINDOW_OPEN_SLOT_APPEND = 1
};

typedef struct _CajaWindowPrivate CajaWindowPrivate;

typedef struct
{
    GtkWindowClass parent_spot;

    CajaWindowType window_type;
    const char *bookmarks_placeholder;

    /* Function pointers for overriding, without corresponding signals */

    char * (* get_title) (CajaWindow *window);
    void   (* sync_title) (CajaWindow *window,
                           CajaWindowSlot *slot);
    CajaIconInfo * (* get_icon) (CajaWindow *window,
                                 CajaWindowSlot *slot);

    void   (* sync_allow_stop) (CajaWindow *window,
                                CajaWindowSlot *slot);
    void   (* set_allow_up) (CajaWindow *window, gboolean allow);
    void   (* reload)              (CajaWindow *window);
    void   (* prompt_for_location) (CajaWindow *window, const char *initial);
    void   (* get_min_size) (CajaWindow *window, guint *default_width, guint *default_height);
    void   (* get_default_size) (CajaWindow *window, guint *default_width, guint *default_height);
    void   (* close) (CajaWindow *window);

    CajaWindowSlot * (* open_slot) (CajaWindowPane *pane,
                                    CajaWindowOpenSlotFlags flags);
    void                 (* close_slot) (CajaWindowPane *pane,
                                         CajaWindowSlot *slot);
    void                 (* set_active_slot) (CajaWindowPane *pane,
            CajaWindowSlot *slot);

    /* Signals used only for keybindings */
    gboolean (* go_up) (CajaWindow *window, gboolean close);
} CajaWindowClass;

struct CajaWindow
{
    GtkWindow parent_object;

    CajaWindowPrivate *details;

    CajaApplication *application;
};

GType            baul_window_get_type             (void);
void             baul_window_show_window          (CajaWindow    *window);
void             baul_window_close                (CajaWindow    *window);

void             baul_window_connect_content_view (CajaWindow    *window,
        CajaView      *view);
void             baul_window_disconnect_content_view (CajaWindow    *window,
        CajaView      *view);

void             baul_window_go_to                (CajaWindow    *window,
        GFile             *location);
void             baul_window_go_to_tab            (CajaWindow    *window,
        GFile             *location);
void             baul_window_go_to_full           (CajaWindow    *window,
        GFile             *location,
        CajaWindowGoToCallback callback,
        gpointer           user_data);
void             baul_window_go_to_with_selection (CajaWindow    *window,
        GFile             *location,
        GList             *new_selection);
void             baul_window_go_home              (CajaWindow    *window);
void             baul_window_new_tab              (CajaWindow    *window);
void             baul_window_new_window           (CajaWindow    *window);
void             baul_window_go_up                (CajaWindow    *window,
        gboolean           close_behind,
        gboolean           new_tab);
void             baul_window_prompt_for_location  (CajaWindow    *window,
        const char        *initial);
void             baul_window_display_error        (CajaWindow    *window,
        const char        *error_msg);
void		 baul_window_reload		      (CajaWindow	 *window);

void             baul_window_allow_reload         (CajaWindow    *window,
        gboolean           allow);
void             baul_window_allow_up             (CajaWindow    *window,
        gboolean           allow);
void             baul_window_allow_stop           (CajaWindow    *window,
        gboolean           allow);
GtkUIManager *   baul_window_get_ui_manager       (CajaWindow    *window);

#endif
