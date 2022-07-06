/*
 *  Baul
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 1999, 2000, 2001 Eazel, Inc.
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
/* baul-window.h: Interface of the main window object */

#ifndef BAUL_WINDOW_H
#define BAUL_WINDOW_H

#include <ctk/ctk.h>

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
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_WINDOW, BaulWindow))
#define BAUL_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_WINDOW, BaulWindowClass))
#define BAUL_IS_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_WINDOW))
#define BAUL_IS_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_WINDOW))
#define BAUL_WINDOW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_WINDOW, BaulWindowClass))

#ifndef BAUL_WINDOW_DEFINED
#define BAUL_WINDOW_DEFINED
typedef struct BaulWindow BaulWindow;
#endif

#ifndef BAUL_WINDOW_SLOT_DEFINED
#define BAUL_WINDOW_SLOT_DEFINED
typedef struct BaulWindowSlot BaulWindowSlot;
#endif

typedef struct _BaulWindowPane      BaulWindowPane;

typedef struct BaulWindowSlotClass BaulWindowSlotClass;
typedef enum BaulWindowOpenSlotFlags BaulWindowOpenSlotFlags;

typedef enum
{
    BAUL_WINDOW_NOT_SHOWN,
    BAUL_WINDOW_POSITION_SET,
    BAUL_WINDOW_SHOULD_SHOW
} BaulWindowShowState;

enum BaulWindowOpenSlotFlags
{
    BAUL_WINDOW_OPEN_SLOT_NONE = 0,
    BAUL_WINDOW_OPEN_SLOT_APPEND = 1
};

typedef struct _BaulWindowPrivate BaulWindowPrivate;

typedef struct
{
    CtkWindowClass parent_spot;

    BaulWindowType window_type;
    const char *bookmarks_placeholder;

    /* Function pointers for overriding, without corresponding signals */

    char * (* get_title) (BaulWindow *window);
    void   (* sync_title) (BaulWindow *window,
                           BaulWindowSlot *slot);
    BaulIconInfo * (* get_icon) (BaulWindow *window,
                                 BaulWindowSlot *slot);

    void   (* sync_allow_stop) (BaulWindow *window,
                                BaulWindowSlot *slot);
    void   (* set_allow_up) (BaulWindow *window, gboolean allow);
    void   (* reload)              (BaulWindow *window);
    void   (* prompt_for_location) (BaulWindow *window, const char *initial);
    void   (* get_min_size) (BaulWindow *window, guint *default_width, guint *default_height);
    void   (* get_default_size) (BaulWindow *window, guint *default_width, guint *default_height);
    void   (* close) (BaulWindow *window);

    BaulWindowSlot * (* open_slot) (BaulWindowPane *pane,
                                    BaulWindowOpenSlotFlags flags);
    void                 (* close_slot) (BaulWindowPane *pane,
                                         BaulWindowSlot *slot);
    void                 (* set_active_slot) (BaulWindowPane *pane,
            BaulWindowSlot *slot);

    /* Signals used only for keybindings */
    gboolean (* go_up) (BaulWindow *window, gboolean close);
} BaulWindowClass;

struct BaulWindow
{
    CtkWindow parent_object;

    BaulWindowPrivate *details;

    BaulApplication *application;
};

GType            baul_window_get_type             (void);
void             baul_window_show_window          (BaulWindow    *window);
void             baul_window_close                (BaulWindow    *window);

void             baul_window_connect_content_view (BaulWindow    *window,
        BaulView      *view);
void             baul_window_disconnect_content_view (BaulWindow    *window,
        BaulView      *view);

void             baul_window_go_to                (BaulWindow    *window,
        GFile             *location);
void             baul_window_go_to_tab            (BaulWindow    *window,
        GFile             *location);
void             baul_window_go_to_full           (BaulWindow    *window,
        GFile             *location,
        BaulWindowGoToCallback callback,
        gpointer           user_data);
void             baul_window_go_to_with_selection (BaulWindow    *window,
        GFile             *location,
        GList             *new_selection);
void             baul_window_go_home              (BaulWindow    *window);
void             baul_window_new_tab              (BaulWindow    *window);
void             baul_window_new_window           (BaulWindow    *window);
void             baul_window_go_up                (BaulWindow    *window,
        gboolean           close_behind,
        gboolean           new_tab);
void             baul_window_prompt_for_location  (BaulWindow    *window,
        const char        *initial);
void             baul_window_display_error        (BaulWindow    *window,
        const char        *error_msg);
void		 baul_window_reload		      (BaulWindow	 *window);

void             baul_window_allow_reload         (BaulWindow    *window,
        gboolean           allow);
void             baul_window_allow_up             (BaulWindow    *window,
        gboolean           allow);
void             baul_window_allow_stop           (BaulWindow    *window,
        gboolean           allow);
CtkUIManager *   baul_window_get_ui_manager       (BaulWindow    *window);

#endif
