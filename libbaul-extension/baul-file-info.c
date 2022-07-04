/*
 *  baul-file-info.c - Information about a file
 *
 *  Copyright (C) 2003 Novell, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <config.h>
#include "baul-file-info.h"
#include "baul-extension-private.h"

BaulFileInfo *(*baul_file_info_getter) (GFile *location, gboolean create);

/**
 * SECTION:baul-file-info
 * @title: BaulFileInfo
 * @short_description: File interface for baul extensions
 * @include: libbaul-extension/baul-file-info.h
 *
 * #BaulFileInfo provides methods to get and modify information
 * about file objects in the file manager.
 */

/**
 * baul_file_info_list_copy:
 * @files: (element-type BaulFileInfo): the files to copy
 *
 * Returns: (element-type BaulFileInfo) (transfer full): a copy of @files.
 *  Use #baul_file_info_list_free to free the list and unref its contents.
 */
GList *
baul_file_info_list_copy (GList *files)
{
    GList *ret;
    GList *l;

    ret = g_list_copy (files);
    for (l = ret; l != NULL; l = l->next)
    {
        g_object_ref (G_OBJECT (l->data));
    }

    return ret;
}

/**
 * baul_file_info_list_free:
 * @files: (element-type BaulFileInfo): a list created with
 *   #baul_file_info_list_copy
 *
 */
void
baul_file_info_list_free (GList *files)
{
    GList *l;

    for (l = files; l != NULL; l = l->next)
    {
        g_object_unref (G_OBJECT (l->data));
    }

    g_list_free (files);
}

static void
baul_file_info_base_init (gpointer g_class)
{
}

GType
baul_file_info_get_type (void)
{
    static GType type = 0;

    if (!type) {
        const GTypeInfo info = {
            sizeof (BaulFileInfoIface),
            baul_file_info_base_init,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            NULL
        };

        type = g_type_register_static (G_TYPE_INTERFACE,
                                       "BaulFileInfo",
                                       &info, 0);
        g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }

    return type;
}

gboolean
baul_file_info_is_gone (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), FALSE);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->is_gone != NULL, FALSE);

    return BAUL_FILE_INFO_GET_IFACE (file)->is_gone (file);
}

GFileType
baul_file_info_get_file_type (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), G_FILE_TYPE_UNKNOWN);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->get_file_type != NULL, G_FILE_TYPE_UNKNOWN);

    return BAUL_FILE_INFO_GET_IFACE (file)->get_file_type (file);
}

char *
baul_file_info_get_name (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), NULL);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->get_name != NULL, NULL);

    return BAUL_FILE_INFO_GET_IFACE (file)->get_name (file);
}

/**
 * baul_file_info_get_location:
 * @file: a #BaulFileInfo
 *
 * Returns: (transfer full): a #GFile for the location of @file
 */
GFile *
baul_file_info_get_location (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), NULL);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->get_location != NULL, NULL);

    return BAUL_FILE_INFO_GET_IFACE (file)->get_location (file);
}

char *
baul_file_info_get_uri (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), NULL);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->get_uri != NULL, NULL);

    return BAUL_FILE_INFO_GET_IFACE (file)->get_uri (file);
}

char *
baul_file_info_get_activation_uri (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), NULL);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->get_activation_uri != NULL, NULL);

    return BAUL_FILE_INFO_GET_IFACE (file)->get_activation_uri (file);
}

/**
 * baul_file_info_get_parent_location:
 * @file: a #BaulFileInfo
 *
 * Returns: (allow-none) (transfer full): a #GFile for the parent location of @file,
 *   or %NULL if @file has no parent
 */
GFile *
baul_file_info_get_parent_location (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), NULL);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->get_parent_location != NULL, NULL);

    return BAUL_FILE_INFO_GET_IFACE (file)->get_parent_location (file);
}

char *
baul_file_info_get_parent_uri (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), NULL);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->get_parent_uri != NULL, NULL);

    return BAUL_FILE_INFO_GET_IFACE (file)->get_parent_uri (file);
}

/**
 * baul_file_info_get_parent_info:
 * @file: a #BaulFileInfo
 *
 * Returns: (allow-none) (transfer full): a #BaulFileInfo for the parent of @file,
 *   or %NULL if @file has no parent
 */
BaulFileInfo *
baul_file_info_get_parent_info (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), NULL);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->get_parent_info != NULL, NULL);

    return BAUL_FILE_INFO_GET_IFACE (file)->get_parent_info (file);
}

/**
 * baul_file_info_get_mount:
 * @file: a #BaulFileInfo
 *
 * Returns: (allow-none) (transfer full): a #GMount for the mount of @file,
 *   or %NULL if @file has no mount
 */
GMount *
baul_file_info_get_mount (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), NULL);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->get_mount != NULL, NULL);

    return BAUL_FILE_INFO_GET_IFACE (file)->get_mount (file);
}

char *
baul_file_info_get_uri_scheme (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), NULL);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->get_uri_scheme != NULL, NULL);

    return BAUL_FILE_INFO_GET_IFACE (file)->get_uri_scheme (file);
}

char *
baul_file_info_get_mime_type (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), NULL);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->get_mime_type != NULL, NULL);

    return BAUL_FILE_INFO_GET_IFACE (file)->get_mime_type (file);
}

gboolean
baul_file_info_is_mime_type (BaulFileInfo *file,
                             const char *mime_type)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), FALSE);
    g_return_val_if_fail (mime_type != NULL, FALSE);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->is_mime_type != NULL, FALSE);

    return BAUL_FILE_INFO_GET_IFACE (file)->is_mime_type (file,
           mime_type);
}

gboolean
baul_file_info_is_directory (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), FALSE);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->is_directory != NULL, FALSE);

    return BAUL_FILE_INFO_GET_IFACE (file)->is_directory (file);
}

gboolean
baul_file_info_can_write (BaulFileInfo *file)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), FALSE);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->can_write != NULL, FALSE);

    return BAUL_FILE_INFO_GET_IFACE (file)->can_write (file);
}

void
baul_file_info_add_emblem (BaulFileInfo *file,
                           const char *emblem_name)
{
    g_return_if_fail (BAUL_IS_FILE_INFO (file));
    g_return_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->add_emblem != NULL);

    BAUL_FILE_INFO_GET_IFACE (file)->add_emblem (file, emblem_name);
}

char *
baul_file_info_get_string_attribute (BaulFileInfo *file,
                                     const char *attribute_name)
{
    g_return_val_if_fail (BAUL_IS_FILE_INFO (file), NULL);
    g_return_val_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->get_string_attribute != NULL, NULL);
    g_return_val_if_fail (attribute_name != NULL, NULL);

    return BAUL_FILE_INFO_GET_IFACE (file)->get_string_attribute
           (file, attribute_name);
}

void
baul_file_info_add_string_attribute (BaulFileInfo *file,
                                     const char *attribute_name,
                                     const char *value)
{
    g_return_if_fail (BAUL_IS_FILE_INFO (file));
    g_return_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->add_string_attribute != NULL);
    g_return_if_fail (attribute_name != NULL);
    g_return_if_fail (value != NULL);

    BAUL_FILE_INFO_GET_IFACE (file)->add_string_attribute
        (file, attribute_name, value);
}

void
baul_file_info_invalidate_extension_info (BaulFileInfo *file)
{
    g_return_if_fail (BAUL_IS_FILE_INFO (file));
    g_return_if_fail (BAUL_FILE_INFO_GET_IFACE (file)->invalidate_extension_info != NULL);

    BAUL_FILE_INFO_GET_IFACE (file)->invalidate_extension_info (file);
}

/**
 * baul_file_info_lookup:
 * @location: the location to lookup the file info for
 *
 * Returns: (transfer full): a #BaulFileInfo
 */
BaulFileInfo *
baul_file_info_lookup (GFile *location)
{
    return baul_file_info_getter (location, FALSE);
}

/**
 * baul_file_info_create:
 * @location: the location to create the file info for
 *
 * Returns: (transfer full): a #BaulFileInfo
 */
BaulFileInfo *
baul_file_info_create (GFile *location)
{
    return baul_file_info_getter (location, TRUE);
}

/**
 * baul_file_info_lookup_for_uri:
 * @uri: the URI to lookup the file info for
 *
 * Returns: (transfer full): a #BaulFileInfo
 */
BaulFileInfo *
baul_file_info_lookup_for_uri (const char *uri)
{
    GFile *location;
    BaulFile *file;

    location = g_file_new_for_uri (uri);
    file = baul_file_info_lookup (location);
    g_object_unref (location);

    return file;
}

/**
 * baul_file_info_create_for_uri:
 * @uri: the URI to lookup the file info for
 *
 * Returns: (transfer full): a #BaulFileInfo
 */
BaulFileInfo *
baul_file_info_create_for_uri (const char *uri)
{
    GFile *location;
    BaulFile *file;

    location = g_file_new_for_uri (uri);
    file = baul_file_info_create (location);
    g_object_unref (location);

    return file;
}
