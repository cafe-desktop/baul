/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* CajaUndoStackManager - Manages undo of file operations (header)
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
#include <gtk/gtk.h>
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
} CajaUndoStackActionType;

typedef struct _CajaUndoStackActionData CajaUndoStackActionData;

typedef struct _CajaUndoStackMenuData CajaUndoStackMenuData;

struct _CajaUndoStackMenuData {
  char* undo_label;
  char* undo_description;
  char* redo_label;
  char* redo_description;
};

/* End action structures */

typedef void
(*CajaUndostackFinishCallback)(gpointer data);

typedef struct _CajaUndoStackManagerPrivate CajaUndoStackManagerPrivate;

typedef struct _CajaUndoStackManager
{
  GObject parent_instance;

  CajaUndoStackManagerPrivate* priv;

} CajaUndoStackManager;

typedef struct _CajaUndoStackManagerClass
{
  GObjectClass parent_class;

} CajaUndoStackManagerClass;

#define TYPE_BAUL_UNDOSTACK_MANAGER (baul_undostack_manager_get_type())

#define BAUL_UNDOSTACK_MANAGER(object) \
 (G_TYPE_CHECK_INSTANCE_CAST((object), TYPE_BAUL_UNDOSTACK_MANAGER, CajaUndoStackManager))

#define BAUL_UNDOSTACK_MANAGER_CLASS(klass) \
 (G_TYPE_CHECK_CLASS_CAST((klass), TYPE_BAUL_UNDOSTACK_MANAGER, CajaUndoStackManagerClass))

#define IS_BAUL_UNDOSTACK_MANAGER(object) \
 (G_TYPE_CHECK_INSTANCE_TYPE((object), TYPE_BAUL_UNDOSTACK_MANAGER))

#define IS_BAUL_UNDOSTACK_MANAGER_CLASS(klass) \
 (G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_BAUL_UNDOSTACK_MANAGER))

#define BAUL_UNDOSTACK_MANAGER_GET_CLASS(object) \
 (G_TYPE_INSTANCE_GET_CLASS((object), TYPE_BAUL_UNDOSTACK_MANAGER, CajaUndoStackManagerClass))

GType
baul_undostack_manager_get_type (void);

void
baul_undostack_manager_add_action(CajaUndoStackManager* manager,
    CajaUndoStackActionData* action);

void
baul_undostack_manager_undo(CajaUndoStackManager* manager,
    GtkWidget *parent_view, CajaUndostackFinishCallback cb);

void
baul_undostack_manager_redo(CajaUndoStackManager* manager,
    GtkWidget *parent_view, CajaUndostackFinishCallback cb);

CajaUndoStackActionData*
baul_undostack_manager_data_new(CajaUndoStackActionType type,
    gint items_count);

gboolean
baul_undostack_manager_is_undo_redo(CajaUndoStackManager* manager);

void
baul_undostack_manager_trash_has_emptied(CajaUndoStackManager* manager);

CajaUndoStackManager*
baul_undostack_manager_instance(void);

void
baul_undostack_manager_data_set_src_dir(CajaUndoStackActionData* data,
    GFile* src);

void
baul_undostack_manager_data_set_dest_dir(CajaUndoStackActionData* data,
    GFile* dest);

void
baul_undostack_manager_data_add_origin_target_pair(
    CajaUndoStackActionData* data, GFile* origin, GFile* target);

void
baul_undostack_manager_data_set_create_data(
    CajaUndoStackActionData* data, char* target_uri, char* template_uri);

void
baul_undostack_manager_data_set_rename_information(
    CajaUndoStackActionData* data, GFile* old_file, GFile* new_file);

guint64
baul_undostack_manager_get_file_modification_time(GFile* file);

void
baul_undostack_manager_data_add_trashed_file(
    CajaUndoStackActionData* data, GFile* file, guint64 mtime);

void
baul_undostack_manager_request_menu_update(CajaUndoStackManager* manager);

void
baul_undostack_manager_data_add_file_permissions(
    CajaUndoStackActionData* data, GFile* file, guint32 permission);

void
baul_undostack_manager_data_set_recursive_permissions(
    CajaUndoStackActionData* data, guint32 file_permissions, guint32 file_mask,
	guint32 dir_permissions, guint32 dir_mask);

void
baul_undostack_manager_data_set_file_permissions(
    CajaUndoStackActionData* data, char* uri, guint32 current_permissions, guint32 new_permissions);

void
baul_undostack_manager_data_set_owner_change_information(
    CajaUndoStackActionData* data, char* uri, const char* current_user, const char* new_user);

void
baul_undostack_manager_data_set_group_change_information(
    CajaUndoStackActionData* data, char* uri, const char* current_group, const char* new_group);

#endif /* BAUL_UNDOSTACK_MANAGER_H */
