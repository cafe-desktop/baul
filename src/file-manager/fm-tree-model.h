/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Copyright (C) 2002 Anders Carlsson
 * Copyright (C) 2002 Bent Spoon Software
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Anders Carlsson <andersca@gnu.org>
 */

/* fm-tree-model.h - Model for the tree view */

#ifndef FM_TREE_MODEL_H
#define FM_TREE_MODEL_H

#include <glib-object.h>

#include <ctk/ctk.h>
#include <gio/gio.h>

#include <libbaul-private/baul-file.h>

#define FM_TYPE_TREE_MODEL fm_tree_model_get_type()
#define FM_TREE_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), FM_TYPE_TREE_MODEL, FMTreeModel))
#define FM_TREE_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), FM_TYPE_TREE_MODEL, FMTreeModelClass))
#define FM_IS_TREE_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FM_TYPE_TREE_MODEL))
#define FM_IS_TREE_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), FM_TYPE_TREE_MODEL))
#define FM_TREE_MODEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), FM_TYPE_TREE_MODEL, FMTreeModelClass))

enum
{
    FM_TREE_MODEL_DISPLAY_NAME_COLUMN,
    FM_TREE_MODEL_CLOSED_SURFACE_COLUMN,
    FM_TREE_MODEL_OPEN_SURFACE_COLUMN,
    FM_TREE_MODEL_FONT_STYLE_COLUMN,
    FM_TREE_MODEL_NUM_COLUMNS
};

typedef struct FMTreeModelDetails FMTreeModelDetails;

typedef struct
{
    GObject parent;
    FMTreeModelDetails *details;
} FMTreeModel;

typedef struct
{
    GObjectClass parent_class;

    void         (* row_loaded)      (FMTreeModel *tree_model,
                                      CtkTreeIter       *iter);
} FMTreeModelClass;

GType              fm_tree_model_get_type                  (void);
FMTreeModel *fm_tree_model_new                       (void);
void               fm_tree_model_set_show_hidden_files     (FMTreeModel *model,
        gboolean           show_hidden_files);
void               fm_tree_model_set_show_only_directories (FMTreeModel *model,
        gboolean           show_only_directories);
BaulFile *     fm_tree_model_iter_get_file             (FMTreeModel *model,
        CtkTreeIter       *iter);
void               fm_tree_model_add_root_uri              (FMTreeModel *model,
        const char        *root_uri,
        const char        *display_name,
        GIcon             *icon,
        GMount            *mount);
void               fm_tree_model_remove_root_uri           (FMTreeModel *model,
        const char        *root_uri);
gboolean           fm_tree_model_iter_is_root              (FMTreeModel *model,
        CtkTreeIter *iter);
int                fm_tree_model_iter_compare_roots        (FMTreeModel *model,
        CtkTreeIter *iter_a,
        CtkTreeIter *iter_b);
gboolean           fm_tree_model_file_get_iter             (FMTreeModel *model,
        CtkTreeIter *iter,
        BaulFile *file,
        CtkTreeIter *currentIter);

GMount *         fm_tree_model_get_mount_for_root_node_file
(FMTreeModel  *model,
 BaulFile *file);
void             fm_tree_model_set_highlight_for_files    (FMTreeModel *model,
        GList *files);

#endif /* FM_TREE_MODEL_H */
