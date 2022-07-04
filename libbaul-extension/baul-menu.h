/*
 *  baul-menu.h - Menus exported by BaulMenuProvider objects.
 *
 *  Copyright (C) 2005 Raffaele Sandrini
 *  Copyright (C) 2003 Novell, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Author:  Dave Camp <dave@ximian.com>
 *           Raffaele Sandrini <rasa@gmx.ch>
 *
 */

#ifndef BAUL_MENU_H
#define BAUL_MENU_H

#include <glib-object.h>
#include "baul-extension-types.h"

G_BEGIN_DECLS

/* BaulMenu defines */
#define BAUL_TYPE_MENU         (baul_menu_get_type ())
#define BAUL_MENU(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BAUL_TYPE_MENU, BaulMenu))
#define BAUL_MENU_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BAUL_TYPE_MENU, BaulMenuClass))
#define BAUL_IS_MENU(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BAUL_TYPE_MENU))
#define BAUL_IS_MENU_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BAUL_TYPE_MENU))
#define BAUL_MENU_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BAUL_TYPE_MENU, BaulMenuClass))
/* BaulMenuItem defines */
#define BAUL_TYPE_MENU_ITEM            (baul_menu_item_get_type())
#define BAUL_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_MENU_ITEM, BaulMenuItem))
#define BAUL_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_MENU_ITEM, BaulMenuItemClass))
#define BAUL_MENU_IS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_MENU_ITEM))
#define BAUL_MENU_IS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), BAUL_TYPE_MENU_ITEM))
#define BAUL_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BAUL_TYPE_MENU_ITEM, BaulMenuItemClass))

/* BaulMenu types */
typedef struct _BaulMenu		BaulMenu;
typedef struct _BaulMenuPrivate	BaulMenuPrivate;
typedef struct _BaulMenuClass	BaulMenuClass;
/* BaulMenuItem types */
typedef struct _BaulMenuItem        BaulMenuItem;
typedef struct _BaulMenuItemDetails BaulMenuItemDetails;
typedef struct _BaulMenuItemClass   BaulMenuItemClass;

/* BaulMenu structs */
struct _BaulMenu {
    GObject parent;
    BaulMenuPrivate *priv;
};

struct _BaulMenuClass {
    GObjectClass parent_class;
};

/* BaulMenuItem structs */
struct _BaulMenuItem {
    GObject parent;

    BaulMenuItemDetails *details;
};

struct _BaulMenuItemClass {
    GObjectClass parent;

    void (*activate) (BaulMenuItem *item);
};

/* BaulMenu methods */
GType     baul_menu_get_type       (void);
BaulMenu *baul_menu_new            (void);

void      baul_menu_append_item    (BaulMenu     *menu,
                                    BaulMenuItem *item);
GList    *baul_menu_get_items      (BaulMenu *menu);
void      baul_menu_item_list_free (GList *item_list);

/* BaulMenuItem methods */
GType         baul_menu_item_get_type    (void);
BaulMenuItem *baul_menu_item_new         (const char   *name,
                                          const char   *label,
                                          const char   *tip,
                                          const char   *icon);

void          baul_menu_item_activate    (BaulMenuItem *item);
void          baul_menu_item_set_submenu (BaulMenuItem *item,
                                          BaulMenu     *menu);

/* BaulMenuItem has the following properties:
 *   name (string)        - the identifier for the menu item
 *   label (string)       - the user-visible label of the menu item
 *   tip (string)         - the tooltip of the menu item
 *   icon (string)        - the name of the icon to display in the menu item
 *   sensitive (boolean)  - whether the menu item is sensitive or not
 *   priority (boolean)   - used for toolbar items, whether to show priority
 *                          text.
 *   menu (BaulMenu)      - The menu belonging to this item. May be null.
 */

G_END_DECLS

#endif /* BAUL_MENU_H */
