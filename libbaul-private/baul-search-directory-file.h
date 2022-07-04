/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-search-directory-file.h: Subclass of BaulFile to implement the
   the case of the search directory

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

#ifndef BAUL_SEARCH_DIRECTORY_FILE_H
#define BAUL_SEARCH_DIRECTORY_FILE_H

#include "baul-file.h"

#define BAUL_TYPE_SEARCH_DIRECTORY_FILE baul_search_directory_file_get_type()
#define BAUL_SEARCH_DIRECTORY_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_SEARCH_DIRECTORY_FILE, BaulSearchDirectoryFile))
#define BAUL_SEARCH_DIRECTORY_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_SEARCH_DIRECTORY_FILE, BaulSearchDirectoryFileClass))
#define BAUL_IS_SEARCH_DIRECTORY_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_SEARCH_DIRECTORY_FILE))
#define BAUL_IS_SEARCH_DIRECTORY_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_SEARCH_DIRECTORY_FILE))
#define BAUL_SEARCH_DIRECTORY_FILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_SEARCH_DIRECTORY_FILE, BaulSearchDirectoryFileClass))

typedef struct BaulSearchDirectoryFileDetails BaulSearchDirectoryFileDetails;

typedef struct
{
    BaulFile parent_slot;
    BaulSearchDirectoryFileDetails *details;
} BaulSearchDirectoryFile;

typedef struct
{
    BaulFileClass parent_slot;
} BaulSearchDirectoryFileClass;

GType   baul_search_directory_file_get_type (void);
void    baul_search_directory_file_update_display_name (BaulSearchDirectoryFile *search_file);

#endif /* BAUL_SEARCH_DIRECTORY_FILE_H */
