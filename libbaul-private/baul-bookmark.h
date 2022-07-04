/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-bookmark.h - interface for individual bookmarks.

   Copyright (C) 1999, 2000 Eazel, Inc.

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

   Authors: John Sullivan <sullivan@eazel.com>
*/

#ifndef BAUL_BOOKMARK_H
#define BAUL_BOOKMARK_H

#include <gtk/gtk.h>
#include <gio/gio.h>
typedef struct BaulBookmark BaulBookmark;

#define BAUL_TYPE_BOOKMARK baul_bookmark_get_type()
#define BAUL_BOOKMARK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_BOOKMARK, BaulBookmark))
#define BAUL_BOOKMARK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_BOOKMARK, BaulBookmarkClass))
#define BAUL_IS_BOOKMARK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_BOOKMARK))
#define BAUL_IS_BOOKMARK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_BOOKMARK))
#define BAUL_BOOKMARK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_BOOKMARK, BaulBookmarkClass))

typedef struct BaulBookmarkDetails BaulBookmarkDetails;

struct BaulBookmark
{
    GObject object;
    BaulBookmarkDetails *details;
};

struct BaulBookmarkClass
{
    GObjectClass parent_class;

    /* Signals that clients can connect to. */

    /* The appearance_changed signal is emitted when the bookmark's
     * name or icon has changed.
     */
    void	(* appearance_changed) (BaulBookmark *bookmark);

    /* The contents_changed signal is emitted when the bookmark's
     * URI has changed.
     */
    void	(* contents_changed) (BaulBookmark *bookmark);
};

typedef struct BaulBookmarkClass BaulBookmarkClass;

GType                 baul_bookmark_get_type               (void);
BaulBookmark *    baul_bookmark_new                    (GFile *location,
        const char *name,
        gboolean has_custom_name,
        GIcon *icon);
BaulBookmark *    baul_bookmark_copy                   (BaulBookmark      *bookmark);
char *                baul_bookmark_get_name               (BaulBookmark      *bookmark);
GFile *               baul_bookmark_get_location           (BaulBookmark      *bookmark);
char *                baul_bookmark_get_uri                (BaulBookmark      *bookmark);
GIcon *               baul_bookmark_get_icon               (BaulBookmark      *bookmark);
gboolean	      baul_bookmark_get_has_custom_name    (BaulBookmark      *bookmark);
gboolean              baul_bookmark_set_name               (BaulBookmark      *bookmark,
        const char            *new_name);
gboolean              baul_bookmark_uri_known_not_to_exist (BaulBookmark      *bookmark);
int                   baul_bookmark_compare_with           (gconstpointer          a,
        gconstpointer          b);
int                   baul_bookmark_compare_uris           (gconstpointer          a,
        gconstpointer          b);

void                  baul_bookmark_set_scroll_pos         (BaulBookmark      *bookmark,
        const char            *uri);
char *                baul_bookmark_get_scroll_pos         (BaulBookmark      *bookmark);


/* Helper functions for displaying bookmarks */
cairo_surface_t *     baul_bookmark_get_surface            (BaulBookmark      *bookmark,
        GtkIconSize            icon_size);
GtkWidget *           baul_bookmark_menu_item_new          (BaulBookmark      *bookmark);

#endif /* BAUL_BOOKMARK_H */
