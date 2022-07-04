/*
 *  baul-menu.h - Menus exported by BaulMenuProvider objects.
 *
 *  Copyright (C) 2005 Raffaele Sandrini
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
 *  Author:  Raffaele Sandrini <rasa@gmx.ch>
 *
 */

#include <config.h>
#include "baul-menu.h"
#include "baul-extension-i18n.h"

#include <glib.h>

/**
 * SECTION:baul-menu
 * @title: BaulMenu
 * @short_description: Menu descriptor object
 * @include: libbaul-extension/baul-menu.h
 *
 * #BaulMenu is an object that describes a submenu in a file manager
 * menu. Extensions can provide #BaulMenu objects by attaching them to
 * #BaulMenuItem objects, using baul_menu_item_set_submenu().
 */


struct _BaulMenuPrivate {
    GList *item_list;
};

G_DEFINE_TYPE_WITH_PRIVATE (BaulMenu, baul_menu, G_TYPE_OBJECT);

void
baul_menu_append_item (BaulMenu *menu, BaulMenuItem *item)
{
    g_return_if_fail (menu != NULL);
    g_return_if_fail (item != NULL);

    menu->priv->item_list = g_list_append (menu->priv->item_list, g_object_ref (item));
}

/**
 * baul_menu_get_items:
 * @menu: a #BaulMenu
 *
 * Returns: (element-type BaulMenuItem) (transfer full): the provided #BaulMenuItem list
 */
GList *
baul_menu_get_items (BaulMenu *menu)
{
    GList *item_list;

    g_return_val_if_fail (menu != NULL, NULL);

    item_list = g_list_copy (menu->priv->item_list);
    g_list_foreach (item_list, (GFunc)g_object_ref, NULL);

    return item_list;
}

/**
 * baul_menu_item_list_free:
 * @item_list: (element-type BaulMenuItem): a list of #BaulMenuItem
 *
 */
void
baul_menu_item_list_free (GList *item_list)
{
    g_return_if_fail (item_list != NULL);

    g_list_foreach (item_list, (GFunc)g_object_unref, NULL);
    g_list_free (item_list);
}

/* Type initialization */

static void
baul_menu_finalize (GObject *object)
{
    BaulMenu *menu = BAUL_MENU (object);

    if (menu->priv->item_list) {
        g_list_free (menu->priv->item_list);
    }

    G_OBJECT_CLASS (baul_menu_parent_class)->finalize (object);
}

static void
baul_menu_init (BaulMenu *menu)
{
    menu->priv = baul_menu_get_instance_private (menu);

    menu->priv->item_list = NULL;
}

static void
baul_menu_class_init (BaulMenuClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = baul_menu_finalize;
}

/* public constructors */

BaulMenu *
baul_menu_new (void)
{
    BaulMenu *obj;

    obj = BAUL_MENU (g_object_new (BAUL_TYPE_MENU, NULL));

    return obj;
}
