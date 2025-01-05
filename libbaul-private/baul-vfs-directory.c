/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-vfs-directory.c: Subclass of BaulDirectory to help implement the
   virtual trash directory.

   Copyright (C) 1999, 2000 Eazel, Inc.

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

   Author: Darin Adler <darin@bentspoon.com>
*/

#include <config.h>

#include <eel/eel-ctk-macros.h>

#include "baul-vfs-directory.h"
#include "baul-directory-private.h"
#include "baul-file-private.h"

static void baul_vfs_directory_init       (gpointer   object,
        gpointer   klass);
static void baul_vfs_directory_class_init (gpointer   klass);

EEL_CLASS_BOILERPLATE (BaulVFSDirectory,
                       baul_vfs_directory,
                       BAUL_TYPE_DIRECTORY)

static void
baul_vfs_directory_init (gpointer object G_GNUC_UNUSED,
			 gpointer klass G_GNUC_UNUSED)
{
}

static gboolean
vfs_contains_file (BaulDirectory *directory,
                   BaulFile *file)
{
    g_assert (BAUL_IS_VFS_DIRECTORY (directory));
    g_assert (BAUL_IS_FILE (file));

    return file->details->directory == directory;
}

static void
vfs_call_when_ready (BaulDirectory *directory,
                     BaulFileAttributes file_attributes,
                     gboolean wait_for_file_list,
                     BaulDirectoryCallback callback,
                     gpointer callback_data)
{
    g_assert (BAUL_IS_VFS_DIRECTORY (directory));

    baul_directory_call_when_ready_internal
    (directory,
     NULL,
     file_attributes,
     wait_for_file_list,
     callback,
     NULL,
     callback_data);
}

static void
vfs_cancel_callback (BaulDirectory *directory,
                     BaulDirectoryCallback callback,
                     gpointer callback_data)
{
    g_assert (BAUL_IS_VFS_DIRECTORY (directory));

    baul_directory_cancel_callback_internal
    (directory,
     NULL,
     callback,
     NULL,
     callback_data);
}

static void
vfs_file_monitor_add (BaulDirectory *directory,
                      gconstpointer client,
                      gboolean monitor_hidden_files,
                      BaulFileAttributes file_attributes,
                      BaulDirectoryCallback callback,
                      gpointer callback_data)
{
    g_assert (BAUL_IS_VFS_DIRECTORY (directory));
    g_assert (client != NULL);

    baul_directory_monitor_add_internal
    (directory, NULL,
     client,
     monitor_hidden_files,
     file_attributes,
     callback, callback_data);
}

static void
vfs_file_monitor_remove (BaulDirectory *directory,
                         gconstpointer client)
{
    g_assert (BAUL_IS_VFS_DIRECTORY (directory));
    g_assert (client != NULL);

    baul_directory_monitor_remove_internal (directory, NULL, client);
}

static void
vfs_force_reload (BaulDirectory *directory)
{
    BaulFileAttributes all_attributes;

    g_assert (BAUL_IS_DIRECTORY (directory));

    all_attributes = baul_file_get_all_attributes ();
    baul_directory_force_reload_internal (directory,
                                          all_attributes);
}

static gboolean
vfs_are_all_files_seen (BaulDirectory *directory)
{
    g_assert (BAUL_IS_VFS_DIRECTORY (directory));

    return directory->details->directory_loaded;
}

static gboolean
vfs_is_not_empty (BaulDirectory *directory)
{
    g_assert (BAUL_IS_VFS_DIRECTORY (directory));
    g_assert (baul_directory_is_anyone_monitoring_file_list (directory));

    return directory->details->file_list != NULL;
}

static void
baul_vfs_directory_class_init (gpointer klass)
{
    BaulDirectoryClass *directory_class;

    directory_class = BAUL_DIRECTORY_CLASS (klass);

    directory_class->contains_file = vfs_contains_file;
    directory_class->call_when_ready = vfs_call_when_ready;
    directory_class->cancel_callback = vfs_cancel_callback;
    directory_class->file_monitor_add = vfs_file_monitor_add;
    directory_class->file_monitor_remove = vfs_file_monitor_remove;
    directory_class->force_reload = vfs_force_reload;
    directory_class->are_all_files_seen = vfs_are_all_files_seen;
    directory_class->is_not_empty = vfs_is_not_empty;
}
