/*
 *  baul-file-info.h - Information about a file
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

/* BaulFileInfo is an interface to the BaulFile object.  It
 * provides access to the asynchronous data in the BaulFile.
 * Extensions are passed objects of this type for operations. */

#ifndef BAUL_FILE_INFO_H
#define BAUL_FILE_INFO_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define BAUL_TYPE_FILE_INFO           (baul_file_info_get_type ())
#define BAUL_FILE_INFO(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_FILE_INFO, BaulFileInfo))
#define BAUL_IS_FILE_INFO(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_FILE_INFO))
#define BAUL_FILE_INFO_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAUL_TYPE_FILE_INFO, BaulFileInfoIface))

#ifndef BAUL_FILE_DEFINED
#define BAUL_FILE_DEFINED
/* Using BaulFile for the vtable to make implementing this in
 * BaulFile easier */
typedef struct BaulFile            BaulFile;
#endif

typedef BaulFile                   BaulFileInfo;
typedef struct _BaulFileInfoIface  BaulFileInfoIface;

/**
 * BaulFileInfoIface:
 * @g_iface: The parent interface.
 * @is_gone: Returns whether the file info is gone.
 *   See baul_file_info_is_gone() for details.
 * @get_name: Returns the file name as a string.
 *   See baul_file_info_get_name() for details.
 * @get_uri: Returns the file URI as a string.
 *   See baul_file_info_get_uri() for details.
 * @get_parent_uri: Returns the file parent URI as a string.
 *   See baul_file_info_get_parent_uri() for details.
 * @get_uri_scheme: Returns the file URI scheme as a string.
 *   See baul_file_info_get_uri_scheme() for details.
 * @get_mime_type: Returns the file mime type as a string.
 *   See baul_file_info_get_mime_type() for details.
 * @is_mime_type: Returns whether the file is the given mime type.
 *   See baul_file_info_is_mime_type() for details.
 * @is_directory: Returns whether the file is a directory.
 *   See baul_file_info_is_directory() for details.
 * @add_emblem: Adds an emblem to this file.
 *   See baul_file_info_add_emblem() for details.
 * @get_string_attribute: Returns the specified file attribute as a string.
 *   See baul_file_info_get_string_attribute() for details.
 * @add_string_attribute: Sets the specified string file attribute value.
 *   See baul_file_info_add_string_attribute() for details.
 * @invalidate_extension_info: Invalidates information of the file provided by extensions.
 *   See baul_file_info_invalidate_extension_info() for details.
 * @get_activation_uri: Returns the file activation URI as a string.
 *   See baul_file_info_get_activation_uri() for details.
 * @get_file_type: Returns the file type.
 *   See baul_file_info_get_file_type() for details.
 * @get_location: Returns the file location as a #GFile.
 *   See baul_file_info_get_location() for details.
 * @get_parent_location: Returns the file parent location as a #GFile.
 *   See baul_file_info_get_parent_location() for details.
 * @get_parent_info: Returns the file parent #BaulFileInfo.
 *   See baul_file_info_get_parent_info() for details.
 * @get_mount: Returns the file mount as a #GMount.
 *   See baul_file_info_get_mount() for details.
 * @can_write: Returns whether the file is writable.
 *   See baul_file_info_can_write() for details.
 *
 * Interface for extensions to get and modify information
 * about file objects.
 */

struct _BaulFileInfoIface {
    GTypeInterface g_iface;

    gboolean      (*is_gone)              (BaulFileInfo *file);

    char         *(*get_name)             (BaulFileInfo *file);
    char         *(*get_uri)              (BaulFileInfo *file);
    char         *(*get_parent_uri)       (BaulFileInfo *file);
    char         *(*get_uri_scheme)       (BaulFileInfo *file);

    char         *(*get_mime_type)        (BaulFileInfo *file);
    gboolean      (*is_mime_type)         (BaulFileInfo *file,
                                           const char   *mime_Type);
    gboolean      (*is_directory)         (BaulFileInfo *file);

    void          (*add_emblem)           (BaulFileInfo *file,
                                           const char   *emblem_name);
    char         *(*get_string_attribute) (BaulFileInfo *file,
                                           const char   *attribute_name);
    void          (*add_string_attribute) (BaulFileInfo *file,
                                           const char   *attribute_name,
                                           const char   *value);
    void          (*invalidate_extension_info) (BaulFileInfo *file);

    char         *(*get_activation_uri)   (BaulFileInfo *file);

    GFileType     (*get_file_type)        (BaulFileInfo *file);
    GFile        *(*get_location)         (BaulFileInfo *file);
    GFile        *(*get_parent_location)  (BaulFileInfo *file);
    BaulFileInfo *(*get_parent_info)      (BaulFileInfo *file);
    GMount       *(*get_mount)            (BaulFileInfo *file);
    gboolean      (*can_write)            (BaulFileInfo *file);
};

GList       *baul_file_info_list_copy             (GList        *files);
void         baul_file_info_list_free             (GList        *files);
GType        baul_file_info_get_type              (void);

/* Return true if the file has been deleted */
gboolean     baul_file_info_is_gone               (BaulFileInfo *file);

/* Name and Location */
GFileType    baul_file_info_get_file_type         (BaulFileInfo *file);
GFile        *baul_file_info_get_location         (BaulFileInfo *file);
char         *baul_file_info_get_name             (BaulFileInfo *file);
char         *baul_file_info_get_uri              (BaulFileInfo *file);
char         *baul_file_info_get_activation_uri   (BaulFileInfo *file);
GFile        *baul_file_info_get_parent_location  (BaulFileInfo *file);
char         *baul_file_info_get_parent_uri       (BaulFileInfo *file);
GMount       *baul_file_info_get_mount            (BaulFileInfo *file);
char         *baul_file_info_get_uri_scheme       (BaulFileInfo *file);
/* It's not safe to call this recursively multiple times, as it works
 * only for files already cached by Baul.
 */
BaulFileInfo *baul_file_info_get_parent_info      (BaulFileInfo *file);

/* File Type */
char         *baul_file_info_get_mime_type        (BaulFileInfo *file);
gboolean      baul_file_info_is_mime_type         (BaulFileInfo *file,
                                                   const char   *mime_type);
gboolean      baul_file_info_is_directory         (BaulFileInfo *file);
gboolean      baul_file_info_can_write            (BaulFileInfo *file);


/* Modifying the BaulFileInfo */
void          baul_file_info_add_emblem           (BaulFileInfo *file,
                                                   const char   *emblem_name);
char         *baul_file_info_get_string_attribute (BaulFileInfo *file,
                                                   const char   *attribute_name);
void          baul_file_info_add_string_attribute (BaulFileInfo *file,
                                                   const char   *attribute_name,
                                                   const char   *value);

/* Invalidating file info */
void          baul_file_info_invalidate_extension_info (BaulFileInfo *file);

BaulFileInfo *baul_file_info_lookup                (GFile       *location);
BaulFileInfo *baul_file_info_create                (GFile       *location);
BaulFileInfo *baul_file_info_lookup_for_uri        (const char  *uri);
BaulFileInfo *baul_file_info_create_for_uri        (const char  *uri);

G_END_DECLS

#endif
