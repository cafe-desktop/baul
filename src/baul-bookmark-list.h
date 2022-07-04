/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Caja
 *
 * Copyright (C) 1999, 2000 Eazel, Inc.
 *
 * Caja is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Caja is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Authors: John Sullivan <sullivan@eazel.com>
 */

/* baul-bookmark-list.h - interface for centralized list of bookmarks.
 */

#ifndef BAUL_BOOKMARK_LIST_H
#define BAUL_BOOKMARK_LIST_H

#include <gio/gio.h>

#include <libbaul-private/baul-bookmark.h>

typedef struct CajaBookmarkList CajaBookmarkList;
typedef struct CajaBookmarkListClass CajaBookmarkListClass;

#define BAUL_TYPE_BOOKMARK_LIST baul_bookmark_list_get_type()
#define BAUL_BOOKMARK_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_BOOKMARK_LIST, CajaBookmarkList))
#define BAUL_BOOKMARK_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_BOOKMARK_LIST, CajaBookmarkListClass))
#define BAUL_IS_BOOKMARK_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_BOOKMARK_LIST))
#define BAUL_IS_BOOKMARK_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_BOOKMARK_LIST))
#define BAUL_BOOKMARK_LIST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_BOOKMARK_LIST, CajaBookmarkListClass))

struct CajaBookmarkList
{
    GObject object;

    GList *list;
    GFileMonitor *monitor;
    GQueue *pending_ops;
};

struct CajaBookmarkListClass
{
    GObjectClass parent_class;
    void (* contents_changed) (CajaBookmarkList *bookmarks);
};

GType                   baul_bookmark_list_get_type            (void);
CajaBookmarkList *  baul_bookmark_list_new                 (void);
void                    baul_bookmark_list_append              (CajaBookmarkList   *bookmarks,
        CajaBookmark *bookmark);
gboolean                baul_bookmark_list_contains            (CajaBookmarkList   *bookmarks,
        CajaBookmark *bookmark);
void                    baul_bookmark_list_delete_item_at      (CajaBookmarkList   *bookmarks,
        guint                   index);
void                    baul_bookmark_list_delete_items_with_uri (CajaBookmarkList *bookmarks,
        const char		   *uri);
void                    baul_bookmark_list_insert_item         (CajaBookmarkList   *bookmarks,
        CajaBookmark *bookmark,
        guint                   index);
guint                   baul_bookmark_list_length              (CajaBookmarkList   *bookmarks);
CajaBookmark *      baul_bookmark_list_item_at             (CajaBookmarkList   *bookmarks,
        guint                   index);
void                    baul_bookmark_list_move_item           (CajaBookmarkList *bookmarks,
        guint                 index,
        guint                 destination);
void                    baul_bookmark_list_set_window_geometry (CajaBookmarkList   *bookmarks,
        const char             *geometry);
const char *            baul_bookmark_list_get_window_geometry (CajaBookmarkList   *bookmarks);

#endif /* BAUL_BOOKMARK_LIST_H */
