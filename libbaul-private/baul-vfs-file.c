/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-vfs-file.c: Subclass of BaulFile to help implement the
   virtual trash directory.

   Copyright (C) 1999, 2000, 2001 Eazel, Inc.

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

#include <glib/gi18n.h>

#include <eel/eel-ctk-macros.h>

#include "baul-vfs-file.h"
#include "baul-directory-notify.h"
#include "baul-directory-private.h"
#include "baul-file-private.h"
#include "baul-autorun.h"

static void baul_vfs_file_init       (gpointer   object,
                                      gpointer   klass);
static void baul_vfs_file_class_init (gpointer   klass);

EEL_CLASS_BOILERPLATE (BaulVFSFile,
                       baul_vfs_file,
                       BAUL_TYPE_FILE)

static void
vfs_file_monitor_add (BaulFile *file,
                      gconstpointer client,
                      BaulFileAttributes attributes)
{
    baul_directory_monitor_add_internal
    (file->details->directory, file,
     client, TRUE, attributes, NULL, NULL);
}

static void
vfs_file_monitor_remove (BaulFile *file,
                         gconstpointer client)
{
    baul_directory_monitor_remove_internal
    (file->details->directory, file, client);
}

static void
vfs_file_call_when_ready (BaulFile *file,
                          BaulFileAttributes file_attributes,
                          BaulFileCallback callback,
                          gpointer callback_data)

{
    baul_directory_call_when_ready_internal
    (file->details->directory, file,
     file_attributes, FALSE, NULL, callback, callback_data);
}

static void
vfs_file_cancel_call_when_ready (BaulFile *file,
                                 BaulFileCallback callback,
                                 gpointer callback_data)
{
    baul_directory_cancel_callback_internal
    (file->details->directory, file,
     NULL, callback, callback_data);
}

static gboolean
vfs_file_check_if_ready (BaulFile *file,
                         BaulFileAttributes file_attributes)
{
    return baul_directory_check_if_ready_internal
           (file->details->directory, file,
            file_attributes);
}

static void
set_metadata_get_info_callback (GObject *source_object,
                                GAsyncResult *res,
                                gpointer callback_data)
{
    BaulFile *file;
    GFileInfo *new_info;
    GError *error;

    file = callback_data;

    error = NULL;
    new_info = g_file_query_info_finish (G_FILE (source_object), res, &error);
    if (new_info != NULL)
    {
        if (baul_file_update_info (file, new_info))
        {
            baul_file_changed (file);
        }
        g_object_unref (new_info);
    }
    baul_file_unref (file);
    if (error)
    {
        g_error_free (error);
    }
}

static void
set_metadata_callback (GObject *source_object,
                       GAsyncResult *result,
                       gpointer callback_data)
{
    BaulFile *file;
    GError *error;
    gboolean res;

    file = callback_data;

    error = NULL;
    res = g_file_set_attributes_finish (G_FILE (source_object),
                                        result,
                                        NULL,
                                        &error);

    if (res)
    {
        g_file_query_info_async (G_FILE (source_object),
                                 BAUL_FILE_DEFAULT_ATTRIBUTES,
                                 0,
                                 G_PRIORITY_DEFAULT,
                                 NULL,
                                 set_metadata_get_info_callback, file);
    }
    else
    {
        baul_file_unref (file);
        g_error_free (error);
    }
}

static void
vfs_file_set_metadata (BaulFile           *file,
                       const char             *key,
                       const char             *value)
{
    GFileInfo *info;
    GFile *location;
    char *gio_key;

    info = g_file_info_new ();

    gio_key = g_strconcat ("metadata::", key, NULL);
    if (value != NULL)
    {
        g_file_info_set_attribute_string (info, gio_key, value);
    }
    else
    {
        /* Unset the key */
        g_file_info_set_attribute (info, gio_key,
                                   G_FILE_ATTRIBUTE_TYPE_INVALID,
                                   NULL);
    }
    g_free (gio_key);

    location = baul_file_get_location (file);
    g_file_set_attributes_async (location,
                                 info,
                                 0,
                                 G_PRIORITY_DEFAULT,
                                 NULL,
                                 set_metadata_callback,
                                 baul_file_ref (file));
    g_object_unref (location);
    g_object_unref (info);
}

static void
vfs_file_set_metadata_as_list (BaulFile           *file,
                               const char             *key,
                               char                  **value)
{
    GFile *location;
    GFileInfo *info;
    char *gio_key;

    info = g_file_info_new ();

    gio_key = g_strconcat ("metadata::", key, NULL);
    g_file_info_set_attribute_stringv (info, gio_key, value);
    g_free (gio_key);

    location = baul_file_get_location (file);
    g_file_set_attributes_async (location,
                                 info,
                                 0,
                                 G_PRIORITY_DEFAULT,
                                 NULL,
                                 set_metadata_callback,
                                 baul_file_ref (file));
    g_object_unref (info);
    g_object_unref (location);
}

static gboolean
vfs_file_get_item_count (BaulFile *file,
                         guint *count,
                         gboolean *count_unreadable)
{
    if (count_unreadable != NULL)
    {
        *count_unreadable = file->details->directory_count_failed;
    }
    if (!file->details->got_directory_count)
    {
        if (count != NULL)
        {
            *count = 0;
        }
        return FALSE;
    }
    if (count != NULL)
    {
        *count = file->details->directory_count;
    }
    return TRUE;
}

static BaulRequestStatus
vfs_file_get_deep_counts (BaulFile *file,
                          guint *directory_count,
                          guint *file_count,
                          guint *unreadable_directory_count,
                          goffset *total_size,
                          goffset *total_size_on_disk)
{
    GFileType type;

    if (directory_count != NULL)
    {
        *directory_count = 0;
    }
    if (file_count != NULL)
    {
        *file_count = 0;
    }
    if (unreadable_directory_count != NULL)
    {
        *unreadable_directory_count = 0;
    }
    if (total_size != NULL)
    {
        *total_size = 0;
    }
    if (total_size_on_disk != NULL)
    {
        *total_size_on_disk = 0;
    }

    if (!baul_file_is_directory (file))
    {
        return BAUL_REQUEST_DONE;
    }

    if (file->details->deep_counts_status != BAUL_REQUEST_NOT_STARTED)
    {
        if (directory_count != NULL)
        {
            *directory_count = file->details->deep_directory_count;
        }
        if (file_count != NULL)
        {
            *file_count = file->details->deep_file_count;
        }
        if (unreadable_directory_count != NULL)
        {
            *unreadable_directory_count = file->details->deep_unreadable_count;
        }
        if (total_size != NULL)
        {
            *total_size = file->details->deep_size;
        }
        if (total_size_on_disk != NULL)
        {
            *total_size_on_disk = file->details->deep_size_on_disk;
        }
        return file->details->deep_counts_status;
    }

    /* For directories, or before we know the type, we haven't started. */
    type = baul_file_get_file_type (file);
    if (type == G_FILE_TYPE_UNKNOWN
            || type == G_FILE_TYPE_DIRECTORY)
    {
        return BAUL_REQUEST_NOT_STARTED;
    }

    /* For other types, we are done, and the zeros are permanent. */
    return BAUL_REQUEST_DONE;
}

static gboolean
vfs_file_get_date (BaulFile *file,
                   BaulDateType date_type,
                   time_t *date)
{
    switch (date_type)
    {
    case BAUL_DATE_TYPE_CHANGED:
        /* Before we have info on a file, the date is unknown. */
        if (file->details->ctime == 0)
        {
            return FALSE;
        }
        if (date != NULL)
        {
            *date = file->details->ctime;
        }
        return TRUE;
    case BAUL_DATE_TYPE_ACCESSED:
        /* Before we have info on a file, the date is unknown. */
        if (file->details->atime == 0)
        {
            return FALSE;
        }
        if (date != NULL)
        {
            *date = file->details->atime;
        }
        return TRUE;
    case BAUL_DATE_TYPE_MODIFIED:
        /* Before we have info on a file, the date is unknown. */
        if (file->details->mtime == 0)
        {
            return FALSE;
        }
        if (date != NULL)
        {
            *date = file->details->mtime;
        }
        return TRUE;
    case BAUL_DATE_TYPE_TRASHED:
        /* Before we have info on a file, the date is unknown. */
        if (file->details->trash_time == 0)
        {
            return FALSE;
        }
        if (date != NULL)
        {
            *date = file->details->trash_time;
        }
        return TRUE;
    case BAUL_DATE_TYPE_PERMISSIONS_CHANGED:
        /* Before we have info on a file, the date is unknown. */
        if (file->details->mtime == 0 || file->details->ctime == 0)
        {
            return FALSE;
        }
        /* mtime is when the contents changed; ctime is when the
         * contents or the permissions (inc. owner/group) changed.
         * So we can only know when the permissions changed if mtime
         * and ctime are different.
         */
        if (file->details->mtime == file->details->ctime)
        {
            return FALSE;
        }
        if (date != NULL)
        {
            *date = file->details->ctime;
        }
        return TRUE;
    }
    return FALSE;
}

static char *
vfs_file_get_where_string (BaulFile *file)
{
    return baul_file_get_parent_uri_for_display (file);
}

static void
vfs_file_mount_callback (GObject *source_object,
                         GAsyncResult *res,
                         gpointer callback_data)
{
    BaulFileOperation *op;
    GFile *mounted_on;
    GError *error;

    op = callback_data;

    error = NULL;
    mounted_on = g_file_mount_mountable_finish (G_FILE (source_object),
                 res, &error);
    baul_file_operation_complete (op, mounted_on, error);
    if (mounted_on)
    {
        g_object_unref (mounted_on);
    }
    if (error)
    {
        g_error_free (error);
    }
}


static void
vfs_file_mount (BaulFile                   *file,
                GMountOperation                *mount_op,
                GCancellable                   *cancellable,
                BaulFileOperationCallback   callback,
                gpointer                        callback_data)
{
    BaulFileOperation *op;
    GError *error;
    GFile *location;

    if (file->details->type != G_FILE_TYPE_MOUNTABLE)
    {
        if (callback)
        {
            error = NULL;
            g_set_error_literal (&error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                                 _("This file cannot be mounted"));
            callback (file, NULL, error, callback_data);
            g_error_free (error);
        }
        return;
    }

    op = baul_file_operation_new (file, callback, callback_data);
    if (cancellable)
    {
        g_object_unref (op->cancellable);
        op->cancellable = g_object_ref (cancellable);
    }

    location = baul_file_get_location (file);
    g_file_mount_mountable (location,
                            0,
                            mount_op,
                            op->cancellable,
                            vfs_file_mount_callback,
                            op);
    g_object_unref (location);
}

static void
vfs_file_unmount_callback (GObject *source_object,
                           GAsyncResult *res,
                           gpointer callback_data)
{
    BaulFileOperation *op;
    gboolean unmounted;
    GError *error;

    op = callback_data;

    error = NULL;
    unmounted = g_file_unmount_mountable_with_operation_finish (G_FILE (source_object),
                res, &error);

    if (!unmounted &&
            error->domain == G_IO_ERROR &&
            (error->code == G_IO_ERROR_FAILED_HANDLED ||
             error->code == G_IO_ERROR_CANCELLED))
    {
        g_error_free (error);
        error = NULL;
    }

    baul_file_operation_complete (op, G_FILE (source_object), error);
    if (error)
    {
        g_error_free (error);
    }
}

static void
vfs_file_unmount (BaulFile                   *file,
                  GMountOperation                *mount_op,
                  GCancellable                   *cancellable,
                  BaulFileOperationCallback   callback,
                  gpointer                        callback_data)
{
    BaulFileOperation *op;
    GFile *location;

    op = baul_file_operation_new (file, callback, callback_data);
    if (cancellable)
    {
        g_object_unref (op->cancellable);
        op->cancellable = g_object_ref (cancellable);
    }

    location = baul_file_get_location (file);
    g_file_unmount_mountable_with_operation (location,
            G_MOUNT_UNMOUNT_NONE,
            mount_op,
            op->cancellable,
            vfs_file_unmount_callback,
            op);
    g_object_unref (location);
}

static void
vfs_file_eject_callback (GObject *source_object,
                         GAsyncResult *res,
                         gpointer callback_data)
{
    BaulFileOperation *op;
    gboolean ejected;
    GError *error;

    op = callback_data;

    error = NULL;
    ejected = g_file_eject_mountable_with_operation_finish (G_FILE (source_object),
              res, &error);

    if (!ejected &&
            error->domain == G_IO_ERROR &&
            (error->code == G_IO_ERROR_FAILED_HANDLED ||
             error->code == G_IO_ERROR_CANCELLED))
    {
        g_error_free (error);
        error = NULL;
    }

    baul_file_operation_complete (op, G_FILE (source_object), error);
    if (error)
    {
        g_error_free (error);
    }
}

static void
vfs_file_eject (BaulFile                   *file,
                GMountOperation                *mount_op,
                GCancellable                   *cancellable,
                BaulFileOperationCallback   callback,
                gpointer                        callback_data)
{
    BaulFileOperation *op;
    GFile *location;

    op = baul_file_operation_new (file, callback, callback_data);
    if (cancellable)
    {
        g_object_unref (op->cancellable);
        op->cancellable = g_object_ref (cancellable);
    }

    location = baul_file_get_location (file);
    g_file_eject_mountable_with_operation (location,
                                           G_MOUNT_UNMOUNT_NONE,
                                           mount_op,
                                           op->cancellable,
                                           vfs_file_eject_callback,
                                           op);
    g_object_unref (location);
}

static void
vfs_file_start_callback (GObject *source_object,
                         GAsyncResult *res,
                         gpointer callback_data)
{
    BaulFileOperation *op;
    gboolean started;
    GError *error;

    op = callback_data;

    error = NULL;
    started = g_file_start_mountable_finish (G_FILE (source_object),
              res, &error);

    if (!started &&
            error->domain == G_IO_ERROR &&
            (error->code == G_IO_ERROR_FAILED_HANDLED ||
             error->code == G_IO_ERROR_CANCELLED))
    {
        g_error_free (error);
        error = NULL;
    }

    baul_file_operation_complete (op, G_FILE (source_object), error);
    if (error)
    {
        g_error_free (error);
    }
}


static void
vfs_file_start (BaulFile                   *file,
                GMountOperation                *mount_op,
                GCancellable                   *cancellable,
                BaulFileOperationCallback   callback,
                gpointer                        callback_data)
{
    BaulFileOperation *op;
    GError *error;
    GFile *location;

    if (file->details->type != G_FILE_TYPE_MOUNTABLE)
    {
        if (callback)
        {
            error = NULL;
            g_set_error_literal (&error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                                 _("This file cannot be started"));
            callback (file, NULL, error, callback_data);
            g_error_free (error);
        }
        return;
    }

    op = baul_file_operation_new (file, callback, callback_data);
    if (cancellable)
    {
        g_object_unref (op->cancellable);
        op->cancellable = g_object_ref (cancellable);
    }

    location = baul_file_get_location (file);
    g_file_start_mountable (location,
                            0,
                            mount_op,
                            op->cancellable,
                            vfs_file_start_callback,
                            op);
    g_object_unref (location);
}

static void
vfs_file_stop_callback (GObject *source_object,
                        GAsyncResult *res,
                        gpointer callback_data)
{
    BaulFileOperation *op;
    gboolean stopped;
    GError *error;

    op = callback_data;

    error = NULL;
    stopped = g_file_stop_mountable_finish (G_FILE (source_object),
                                            res, &error);

    if (!stopped &&
            error->domain == G_IO_ERROR &&
            (error->code == G_IO_ERROR_FAILED_HANDLED ||
             error->code == G_IO_ERROR_CANCELLED))
    {
        g_error_free (error);
        error = NULL;
    }

    baul_file_operation_complete (op, G_FILE (source_object), error);
    if (error)
    {
        g_error_free (error);
    }
}

static void
vfs_file_stop (BaulFile                   *file,
               GMountOperation                *mount_op,
               GCancellable                   *cancellable,
               BaulFileOperationCallback   callback,
               gpointer                        callback_data)
{
    BaulFileOperation *op;
    GFile *location;

    op = baul_file_operation_new (file, callback, callback_data);
    if (cancellable)
    {
        g_object_unref (op->cancellable);
        op->cancellable = g_object_ref (cancellable);
    }

    location = baul_file_get_location (file);
    g_file_stop_mountable (location,
                           G_MOUNT_UNMOUNT_NONE,
                           mount_op,
                           op->cancellable,
                           vfs_file_stop_callback,
                           op);
    g_object_unref (location);
}

static void
vfs_file_poll_callback (GObject *source_object,
                        GAsyncResult *res,
                        gpointer callback_data)
{
    BaulFileOperation *op;
    gboolean stopped;
    GError *error;

    op = callback_data;

    error = NULL;
    stopped = g_file_poll_mountable_finish (G_FILE (source_object),
                                            res, &error);

    if (!stopped &&
            error->domain == G_IO_ERROR &&
            (error->code == G_IO_ERROR_FAILED_HANDLED ||
             error->code == G_IO_ERROR_CANCELLED))
    {
        g_error_free (error);
        error = NULL;
    }

    baul_file_operation_complete (op, G_FILE (source_object), error);
    if (error)
    {
        g_error_free (error);
    }
}

static void
vfs_file_poll_for_media (BaulFile *file)
{
    BaulFileOperation *op;
    GFile *location;

    op = baul_file_operation_new (file, NULL, NULL);

    location = baul_file_get_location (file);
    g_file_poll_mountable (location,
                           op->cancellable,
                           vfs_file_poll_callback,
                           op);
    g_object_unref (location);
}

static void
baul_vfs_file_init (gpointer object G_GNUC_UNUSED,
		    gpointer klass G_GNUC_UNUSED)
{
}

static void
baul_vfs_file_class_init (gpointer klass)
{
    BaulFileClass *file_class;

    file_class = BAUL_FILE_CLASS (klass);

    file_class->monitor_add = vfs_file_monitor_add;
    file_class->monitor_remove = vfs_file_monitor_remove;
    file_class->call_when_ready = vfs_file_call_when_ready;
    file_class->cancel_call_when_ready = vfs_file_cancel_call_when_ready;
    file_class->check_if_ready = vfs_file_check_if_ready;
    file_class->get_item_count = vfs_file_get_item_count;
    file_class->get_deep_counts = vfs_file_get_deep_counts;
    file_class->get_date = vfs_file_get_date;
    file_class->get_where_string = vfs_file_get_where_string;
    file_class->set_metadata = vfs_file_set_metadata;
    file_class->set_metadata_as_list = vfs_file_set_metadata_as_list;
    file_class->mount = vfs_file_mount;
    file_class->unmount = vfs_file_unmount;
    file_class->eject = vfs_file_eject;
    file_class->start = vfs_file_start;
    file_class->stop = vfs_file_stop;
    file_class->poll_for_media = vfs_file_poll_for_media;
}
