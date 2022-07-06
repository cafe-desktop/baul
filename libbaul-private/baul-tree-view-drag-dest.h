/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
 *
 * Baul is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Baul is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Dave Camp <dave@ximian.com>
 */

/* baul-tree-view-drag-dest.h: Handles drag and drop for treeviews which
 *                                 contain a hierarchy of files
 */

#ifndef BAUL_TREE_VIEW_DRAG_DEST_H
#define BAUL_TREE_VIEW_DRAG_DEST_H

#include <ctk/ctk.h>

#include "baul-file.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BAUL_TYPE_TREE_VIEW_DRAG_DEST	(baul_tree_view_drag_dest_get_type ())
#define BAUL_TREE_VIEW_DRAG_DEST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_TREE_VIEW_DRAG_DEST, BaulTreeViewDragDest))
#define BAUL_TREE_VIEW_DRAG_DEST_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_TREE_VIEW_DRAG_DEST, BaulTreeViewDragDestClass))
#define BAUL_IS_TREE_VIEW_DRAG_DEST(obj)		(G_TYPE_INSTANCE_CHECK_TYPE ((obj), BAUL_TYPE_TREE_VIEW_DRAG_DEST))
#define BAUL_IS_TREE_VIEW_DRAG_DEST_CLASS(klass)	(G_TYPE_CLASS_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_TREE_VIEW_DRAG_DEST))

    typedef struct _BaulTreeViewDragDest        BaulTreeViewDragDest;
    typedef struct _BaulTreeViewDragDestClass   BaulTreeViewDragDestClass;
    typedef struct _BaulTreeViewDragDestDetails BaulTreeViewDragDestDetails;

    struct _BaulTreeViewDragDest
    {
        GObject parent;

        BaulTreeViewDragDestDetails *details;
    };

    struct _BaulTreeViewDragDestClass
    {
        GObjectClass parent;

        char *(*get_root_uri) (BaulTreeViewDragDest *dest);
        BaulFile *(*get_file_for_path) (BaulTreeViewDragDest *dest,
                                        CtkTreePath *path);
        void (*move_copy_items) (BaulTreeViewDragDest *dest,
                                 const GList *item_uris,
                                 const char *target_uri,
                                 GdkDragAction action,
                                 int x,
                                 int y);
        void (* handle_netscape_url) (BaulTreeViewDragDest *dest,
                                      const char *url,
                                      const char *target_uri,
                                      GdkDragAction action,
                                      int x,
                                      int y);
        void (* handle_uri_list) (BaulTreeViewDragDest *dest,
                                  const char *uri_list,
                                  const char *target_uri,
                                  GdkDragAction action,
                                  int x,
                                  int y);
        void (* handle_text)    (BaulTreeViewDragDest *dest,
                                 const char *text,
                                 const char *target_uri,
                                 GdkDragAction action,
                                 int x,
                                 int y);
        void (* handle_raw)    (BaulTreeViewDragDest *dest,
                                char *raw_data,
                                int length,
                                const char *target_uri,
                                const char *direct_save_uri,
                                GdkDragAction action,
                                int x,
                                int y);
    };

    GType                     baul_tree_view_drag_dest_get_type (void);
    BaulTreeViewDragDest *baul_tree_view_drag_dest_new      (CtkTreeView *tree_view);

#ifdef __cplusplus
}
#endif

#endif
