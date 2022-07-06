/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-file-operations: execute file operations.

   Copyright (C) 1999, 2000 Free Software Foundation
   Copyright (C) 2000, 2001 Eazel, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Ettore Perazzoli <ettore@gnu.org>,
            Pavel Cisler <pavel@eazel.com>
*/

#ifndef BAUL_FILE_OPERATIONS_H
#define BAUL_FILE_OPERATIONS_H

#include <ctk/ctk.h>
#include <gio/gio.h>

typedef void (* BaulCopyCallback)      (GHashTable *debuting_uris,
                                        gpointer    callback_data);
typedef void (* BaulCreateCallback)    (GFile      *new_file,
                                        gpointer    callback_data);
typedef void (* BaulOpCallback)        (gpointer    callback_data);
typedef void (* BaulDeleteCallback)    (GHashTable *debuting_uris,
                                        gboolean    user_cancel,
                                        gpointer    callback_data);
typedef void (* BaulMountCallback)     (GVolume    *volume,
                                        GObject    *callback_data_object);
typedef void (* BaulUnmountCallback)   (gpointer    callback_data);

/* FIXME: int copy_action should be an enum */

void baul_file_operations_copy_move   (const GList               *item_uris,
                                       GArray                    *relative_item_points,
                                       const char                *target_dir_uri,
                                       CdkDragAction              copy_action,
                                       CtkWidget                 *parent_view,
                                       BaulCopyCallback       done_callback,
                                       gpointer                   done_callback_data);
void baul_file_operations_empty_trash (CtkWidget                 *parent_view);
void baul_file_operations_new_folder  (CtkWidget                 *parent_view,
                                       CdkPoint                  *target_point,
                                       const char                *parent_dir_uri,
                                       BaulCreateCallback     done_callback,
                                       gpointer                   done_callback_data);
void baul_file_operations_new_file    (CtkWidget                 *parent_view,
                                       CdkPoint                  *target_point,
                                       const char                *parent_dir,
                                       const char                *target_filename,
                                       const char                *initial_contents,
                                       int                        length,
                                       BaulCreateCallback     done_callback,
                                       gpointer                   data);
void baul_file_operations_new_file_from_template (CtkWidget               *parent_view,
        CdkPoint                *target_point,
        const char              *parent_dir,
        const char              *target_filename,
        const char              *template_uri,
        BaulCreateCallback   done_callback,
        gpointer                 data);

void baul_file_operations_delete          (GList                  *files,
        CtkWindow              *parent_window,
        BaulDeleteCallback  done_callback,
        gpointer                done_callback_data);
void baul_file_operations_trash_or_delete (GList                  *files,
        CtkWindow              *parent_window,
        BaulDeleteCallback  done_callback,
        gpointer                done_callback_data);

void baul_file_set_permissions_recursive (const char                     *directory,
        guint32                         file_permissions,
        guint32                         file_mask,
        guint32                         folder_permissions,
        guint32                         folder_mask,
        BaulOpCallback              callback,
        gpointer                        callback_data);

void baul_file_operations_unmount_mount (CtkWindow                      *parent_window,
        GMount                         *mount,
        gboolean                        eject,
        gboolean                        check_trash);
void baul_file_operations_unmount_mount_full (CtkWindow                 *parent_window,
        GMount                    *mount,
        gboolean                   eject,
        gboolean                   check_trash,
        BaulUnmountCallback    callback,
        gpointer                   callback_data);
void baul_file_operations_mount_volume  (CtkWindow                      *parent_window,
        GVolume                        *volume,
        gboolean                        allow_autorun);
void baul_file_operations_mount_volume_full (CtkWindow                      *parent_window,
        GVolume                        *volume,
        gboolean                        allow_autorun,
        BaulMountCallback           mount_callback,
        GObject                        *mount_callback_data_object);

void baul_file_operations_copy      (GList                *files,
                                     GArray               *relative_item_points,
                                     GFile                *target_dir,
                                     CtkWindow            *parent_window,
                                     BaulCopyCallback  done_callback,
                                     gpointer              done_callback_data);
void baul_file_operations_move      (GList                *files,
                                     GArray               *relative_item_points,
                                     GFile                *target_dir,
                                     CtkWindow            *parent_window,
                                     BaulCopyCallback  done_callback,
                                     gpointer              done_callback_data);
void baul_file_operations_duplicate (GList                *files,
                                     GArray               *relative_item_points,
                                     CtkWindow            *parent_window,
                                     BaulCopyCallback  done_callback,
                                     gpointer              done_callback_data);
void baul_file_operations_link      (GList                *files,
                                     GArray               *relative_item_points,
                                     GFile                *target_dir,
                                     CtkWindow            *parent_window,
                                     BaulCopyCallback  done_callback,
                                     gpointer              done_callback_data);
void baul_file_mark_desktop_file_trusted (GFile           *file,
        CtkWindow        *parent_window,
        gboolean          interactive,
        BaulOpCallback done_callback,
        gpointer          done_callback_data);

void baul_application_notify_unmount_show (const gchar *message);

#endif /* BAUL_FILE_OPERATIONS_H */
