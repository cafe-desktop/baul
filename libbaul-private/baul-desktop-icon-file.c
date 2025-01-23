/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-desktop-icon-file.c: Subclass of BaulFile to help implement the
   virtual desktop icons.

   Copyright (C) 2003 Red Hat, Inc.

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

   Author: Alexander Larsson <alexl@redhat.com>
*/

#include <config.h>
#include <glib/gi18n.h>
#include <string.h>
#include <gio/gio.h>

#include <eel/eel-glib-extensions.h>

#include "baul-desktop-icon-file.h"
#include "baul-desktop-metadata.h"
#include "baul-desktop-directory-file.h"
#include "baul-directory-notify.h"
#include "baul-directory-private.h"
#include "baul-file-attributes.h"
#include "baul-file-private.h"
#include "baul-file-utilities.h"
#include "baul-file-operations.h"
#include "baul-desktop-directory.h"

struct _BaulDesktopIconFilePrivate
{
    BaulDesktopLink *link;
};

G_DEFINE_TYPE_WITH_PRIVATE (BaulDesktopIconFile, baul_desktop_icon_file, BAUL_TYPE_FILE)


static void
desktop_icon_file_monitor_add (BaulFile *file,
                               gconstpointer client,
                               BaulFileAttributes attributes)
{
    baul_directory_monitor_add_internal
    (file->details->directory, file,
     client, TRUE, attributes, NULL, NULL);
}

static void
desktop_icon_file_monitor_remove (BaulFile *file,
                                  gconstpointer client)
{
    baul_directory_monitor_remove_internal
    (file->details->directory, file, client);
}

static void
desktop_icon_file_call_when_ready (BaulFile *file,
                                   BaulFileAttributes attributes,
                                   BaulFileCallback callback,
                                   gpointer callback_data)
{
    baul_directory_call_when_ready_internal
    (file->details->directory, file,
     attributes, FALSE, NULL, callback, callback_data);
}

static void
desktop_icon_file_cancel_call_when_ready (BaulFile *file,
        BaulFileCallback callback,
        gpointer callback_data)
{
    baul_directory_cancel_callback_internal
    (file->details->directory, file,
     NULL, callback, callback_data);
}

static gboolean
desktop_icon_file_check_if_ready (BaulFile *file,
                                  BaulFileAttributes attributes)
{
    return baul_directory_check_if_ready_internal
           (file->details->directory, file,
            attributes);
}

static gboolean
desktop_icon_file_get_item_count (BaulFile *file G_GNUC_UNUSED,
				  guint    *count,
				  gboolean *count_unreadable)
{
    if (count != NULL)
    {
        *count = 0;
    }
    if (count_unreadable != NULL)
    {
        *count_unreadable = FALSE;
    }
    return TRUE;
}

static BaulRequestStatus
desktop_icon_file_get_deep_counts (BaulFile *file G_GNUC_UNUSED,
				   guint    *directory_count,
				   guint    *file_count,
				   guint    *unreadable_directory_count,
				   goffset  *total_size,
				   goffset  *total_size_on_disk)
{
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

    return BAUL_REQUEST_DONE;
}

static gboolean
desktop_icon_file_get_date (BaulFile *file,
                            BaulDateType date_type,
                            time_t *date)
{
    BaulDesktopIconFile *desktop_file;

    desktop_file = BAUL_DESKTOP_ICON_FILE (file);

    return baul_desktop_link_get_date (desktop_file->details->link,
                                       date_type, date);
}

static char *
desktop_icon_file_get_where_string (BaulFile *file G_GNUC_UNUSED)
{
    return g_strdup (_("on the desktop"));
}

static void
baul_desktop_icon_file_init (BaulDesktopIconFile *desktop_file)
{
    desktop_file->details =	baul_desktop_icon_file_get_instance_private (desktop_file);
}

static void
update_info_from_link (BaulDesktopIconFile *icon_file)
{
    BaulFile *file;
    BaulDesktopLink *link;
    char *display_name;
    GMount *mount;

    file = BAUL_FILE (icon_file);

    link = icon_file->details->link;

    if (link == NULL)
    {
        return;
    }

    g_clear_pointer (&file->details->mime_type, g_ref_string_release);
    file->details->mime_type = g_ref_string_new_intern ("application/x-baul-link");
    file->details->type = G_FILE_TYPE_SHORTCUT;
    file->details->size = 0;
    file->details->has_permissions = FALSE;
    file->details->can_read = TRUE;
    file->details->can_write = TRUE;

    file->details->can_mount = FALSE;
    file->details->can_unmount = FALSE;
    file->details->can_eject = FALSE;
    if (file->details->mount)
    {
        g_object_unref (file->details->mount);
    }
    mount = baul_desktop_link_get_mount (link);
    file->details->mount = mount;
    if (mount)
    {
        file->details->can_unmount = g_mount_can_unmount (mount);
        file->details->can_eject = g_mount_can_eject (mount);
    }

    file->details->file_info_is_up_to_date = TRUE;

    display_name = baul_desktop_link_get_display_name (link);
    baul_file_set_display_name (file,
                                display_name, NULL, TRUE);
    g_free (display_name);

    if (file->details->icon != NULL)
    {
        g_object_unref (file->details->icon);
    }
    file->details->icon = baul_desktop_link_get_icon (link);
    g_free (file->details->activation_uri);
    file->details->activation_uri = baul_desktop_link_get_activation_uri (link);
    file->details->got_link_info = TRUE;
    file->details->link_info_is_up_to_date = TRUE;

    file->details->directory_count = 0;
    file->details->got_directory_count = TRUE;
    file->details->directory_count_is_up_to_date = TRUE;
}

void
baul_desktop_icon_file_update (BaulDesktopIconFile *icon_file)
{
    BaulFile *file;

    update_info_from_link (icon_file);
    file = BAUL_FILE (icon_file);
    baul_file_changed (file);
}

void
baul_desktop_icon_file_remove (BaulDesktopIconFile *icon_file)
{
    BaulFile *file;
    GList list;

    icon_file->details->link = NULL;

    file = BAUL_FILE (icon_file);

    /* ref here because we might be removing the last ref when we
     * mark the file gone below, but we need to keep a ref at
     * least long enough to send the change notification.
     */
    baul_file_ref (file);

    file->details->is_gone = TRUE;

    list.data = file;
    list.next = NULL;
    list.prev = NULL;

    baul_directory_remove_file (file->details->directory, file);
    baul_directory_emit_change_signals (file->details->directory, &list);

    baul_file_unref (file);
}

BaulDesktopIconFile *
baul_desktop_icon_file_new (BaulDesktopLink *link)
{
    BaulFile *file;
    BaulDirectory *directory;
    BaulDesktopIconFile *icon_file;
    GList list;
    char *name;

    directory = baul_directory_get_by_uri (EEL_DESKTOP_URI);

    file = BAUL_FILE (g_object_new (BAUL_TYPE_DESKTOP_ICON_FILE, NULL));

#ifdef BAUL_FILE_DEBUG_REF
    printf("%10p ref'd\n", file);
    eazel_dump_stack_trace ("\t", 10);
#endif

    file->details->directory = directory;

    icon_file = BAUL_DESKTOP_ICON_FILE (file);
    icon_file->details->link = link;

    name = baul_desktop_link_get_file_name (link);
    file->details->name = g_ref_string_new (name);
    g_free (name);

    update_info_from_link (icon_file);

    baul_desktop_update_metadata_from_keyfile (file, file->details->name);

    baul_directory_add_file (directory, file);

    list.data = file;
    list.next = NULL;
    list.prev = NULL;
    baul_directory_emit_files_added (directory, &list);

    return icon_file;
}

/* Note: This can return NULL if the link was recently removed (i.e. unmounted) */
BaulDesktopLink *
baul_desktop_icon_file_get_link (BaulDesktopIconFile *icon_file)
{
    if (icon_file->details->link)
        return g_object_ref (icon_file->details->link);
    else
        return NULL;
}

static void
baul_desktop_icon_file_unmount (BaulFile                 *file,
				GMountOperation          *mount_op G_GNUC_UNUSED,
				GCancellable             *cancellable G_GNUC_UNUSED,
				BaulFileOperationCallback callback G_GNUC_UNUSED,
				gpointer                  callback_data G_GNUC_UNUSED)
{
    BaulDesktopIconFile *desktop_file;

    desktop_file = BAUL_DESKTOP_ICON_FILE (file);
    if (desktop_file)
    {
        GMount *mount;

        mount = baul_desktop_link_get_mount (desktop_file->details->link);
        if (mount != NULL)
        {
            baul_file_operations_unmount_mount (NULL, mount, FALSE, TRUE);
        }
    }

}

static void
baul_desktop_icon_file_eject (BaulFile                 *file,
			      GMountOperation          *mount_op G_GNUC_UNUSED,
			      GCancellable             *cancellable G_GNUC_UNUSED,
			      BaulFileOperationCallback callback G_GNUC_UNUSED,
			      gpointer                  callback_data G_GNUC_UNUSED)
{
    BaulDesktopIconFile *desktop_file;

    desktop_file = BAUL_DESKTOP_ICON_FILE (file);
    if (desktop_file)
    {
        GMount *mount;

        mount = baul_desktop_link_get_mount (desktop_file->details->link);
        if (mount != NULL)
        {
            baul_file_operations_unmount_mount (NULL, mount, TRUE, TRUE);
        }
    }
}

static void
baul_desktop_icon_file_set_metadata (BaulFile           *file,
                                     const char             *key,
                                     const char             *value)
{
    baul_desktop_set_metadata_string (file, file->details->name, key, value);
}

static void
baul_desktop_icon_file_set_metadata_as_list (BaulFile           *file,
        const char             *key,
        char                  **value)
{
    baul_desktop_set_metadata_stringv (file, file->details->name, key, (const gchar **) value);
}

static void
baul_desktop_icon_file_class_init (BaulDesktopIconFileClass *klass)
{
    BaulFileClass *file_class;

    file_class = BAUL_FILE_CLASS (klass);

    file_class->default_file_type = G_FILE_TYPE_DIRECTORY;

    file_class->monitor_add = desktop_icon_file_monitor_add;
    file_class->monitor_remove = desktop_icon_file_monitor_remove;
    file_class->call_when_ready = desktop_icon_file_call_when_ready;
    file_class->cancel_call_when_ready = desktop_icon_file_cancel_call_when_ready;
    file_class->check_if_ready = desktop_icon_file_check_if_ready;
    file_class->get_item_count = desktop_icon_file_get_item_count;
    file_class->get_deep_counts = desktop_icon_file_get_deep_counts;
    file_class->get_date = desktop_icon_file_get_date;
    file_class->get_where_string = desktop_icon_file_get_where_string;
    file_class->set_metadata = baul_desktop_icon_file_set_metadata;
    file_class->set_metadata_as_list = baul_desktop_icon_file_set_metadata_as_list;
    file_class->unmount = baul_desktop_icon_file_unmount;
    file_class->eject = baul_desktop_icon_file_eject;
}
