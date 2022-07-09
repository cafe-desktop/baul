/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* BaulUndoStackManager - Manages undo of file operations (header)
 *
 * Copyright (C) 2007-2010 Amos Brocco
 * Copyright (C) 2011 Stefano Karapetsas
 *
 * Authors: Amos Brocco <amos.brocco@unifr.ch>,
 *          Stefano Karapetsas <stefano@karapetsas.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef BAUL_UNDOSTACK_MANAGER_H
#define BAUL_UNDOSTACK_MANAGER_H

#include <glib.h>
#include <glib-object.h>
#include <ctk/ctk.h>
#include <gio/gio.h>

/* Begin action structures */

typedef enum
{
  BAUL_UNDOSTACK_COPY,
  BAUL_UNDOSTACK_DUPLICATE,
  BAUL_UNDOSTACK_MOVE,
  BAUL_UNDOSTACK_RENAME,
  BAUL_UNDOSTACK_CREATEEMPTYFILE,
  BAUL_UNDOSTACK_CREATEFILEFROMTEMPLATE,
  BAUL_UNDOSTACK_CREATEFOLDER,
  BAUL_UNDOSTACK_MOVETOTRASH,
  BAUL_UNDOSTACK_CREATELINK,
  BAUL_UNDOSTACK_DELETE,
  BAUL_UNDOSTACK_RESTOREFROMTRASH,
  BAUL_UNDOSTACK_SETPERMISSIONS,
  BAUL_UNDOSTACK_RECURSIVESETPERMISSIONS,
  BAUL_UNDOSTACK_CHANGEOWNER,
  BAUL_UNDOSTACK_CHANGEGROUP
} BaulUndoStackActionType;

typedef struct _BaulUndoStackActionData BaulUndoStackActionData;

typedef struct _BaulUndoStackMenuData BaulUndoStackMenuData;

struct _BaulUndoStackMenuData {
  char* undo_label;
  char* undo_description;
  char* redo_label;
  char* redo_description;
};

/* End action structures */

typedef void
(*BaulUndostackFinishCallback)(gpointer data);

typedef struct _BaulUndoStackManagerPrivate BaulUndoStackManagerPrivate;

typedef struct _BaulUndoStackManager
{
  GObject parent_instance;

  BaulUndoStackManagerPrivate* priv;

} BaulUndoStackManager;

typedef struct _BaulUndoStackManagerClass
{
  GObjectClass parent_class;

} BaulUndoStackManagerClass;

#define TYPE_BAUL_UNDOSTACK_MANAGER (baul_undostack_manager_get_type())

#define BAUL_UNDOSTACK_MANAGER(object) \
 (G_TYPE_CHECK_INSTANCE_CAST((object), TYPE_BAUL_UNDOSTACK_MANAGER, BaulUndoStackManager))

#define BAUL_UNDOSTACK_MANAGER_CLASS(klass) \
 (G_TYPE_CHECK_CLASS_CAST((klass), TYPE_BAUL_UNDOSTACK_MANAGER, BaulUndoStackManagerClass))

#define IS_BAUL_UNDOSTACK_MANAGER(object) \
 (G_TYPE_CHECK_INSTANCE_TYPE((object), TYPE_BAUL_UNDOSTACK_MANAGER))

#define IS_BAUL_UNDOSTACK_MANAGER_CLASS(klass) \
 (G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_BAUL_UNDOSTACK_MANAGER))

#define BAUL_UNDOSTACK_MANAGER_GET_CLASS(object) \
 (G_TYPE_INSTANCE_GET_CLASS((object), TYPE_BAUL_UNDOSTACK_MANAGER, BaulUndoStackManagerClass))

GType
baul_undostack_manager_get_type (void);

void
baul_undostack_manager_add_action(BaulUndoStackManager* manager,
    BaulUndoStackActionData* action);

void
baul_undostack_manager_undo(BaulUndoStackManager* manager,
    CtkWidget *parent_view, BaulUndostackFinishCallback cb);

void
baul_undostack_manager_redo(BaulUndoStackManager* manager,
    CtkWidget *parent_view, BaulUndostackFinishCallback cb);

BaulUndoStackActionData*
baul_undostack_manager_data_new(BaulUndoStackActionType type,
    gint items_count);

gboolean
baul_undostack_manager_is_undo_redo(BaulUndoStackManager* manager);

void
baul_undostack_manager_trash_has_emptied(BaulUndoStackManager* manager);

BaulUndoStackManager*
baul_undostack_manager_instance(void);

void
baul_undostack_manager_data_set_src_dir(BaulUndoStackActionData* data,
    GFile* src);

void
baul_undostack_manager_data_set_dest_dir(BaulUndoStackActionData* data,
    GFile* dest);

void
baul_undostack_manager_data_add_origin_target_pair(
    BaulUndoStackActionData* data, GFile* origin, GFile* target);

void
baul_undostack_manager_data_set_create_data(
    BaulUndoStackActionData* data, char* target_uri, char* template_uri);

void
baul_undostack_manager_data_set_rename_information(
    BaulUndoStackActionData* data, GFile* old_file, GFile* new_file);

guint64
baul_undostack_manager_get_file_modification_time(GFile* file);

void
baul_undostack_manager_data_add_trashed_file(
    BaulUndoStackActionData* data, GFile* file, guint64 mtime);

void
baul_undostack_manager_request_menu_update(BaulUndoStackManager* manager);

void
baul_undostack_manager_data_add_file_permissions(
    BaulUndoStackActionData* data, GFile* file, guint32 permission);

void
baul_undostack_manager_data_set_recursive_permissions(
    BaulUndoStackActionData* data, guint32 file_permissions, guint32 file_mask,
	guint32 dir_permissions, guint32 dir_mask);

void
baul_undostack_manager_data_set_file_permissions(
    BaulUndoStackActionData* data, char* uri, guint32 current_permissions, guint32 new_permissions);

void
baul_undostack_manager_data_set_owner_change_information(
    BaulUndoStackActionData* data, char* uri, const char* current_user, const char* new_user);

void
baul_undostack_manager_data_set_group_change_information(
    BaulUndoStackActionData* data, char* uri, const char* current_group, const char* new_group);

#endif /* BAUL_UNDOSTACK_MANAGER_H */
