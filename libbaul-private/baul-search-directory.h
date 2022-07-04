/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-search-directory.h: Subclass of BaulDirectory to implement
   a virtual directory consisting of the search directory and the search
   icons

   Copyright (C) 2005 Novell, Inc

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
*/

#ifndef BAUL_SEARCH_DIRECTORY_H
#define BAUL_SEARCH_DIRECTORY_H

#include "baul-directory.h"
#include "baul-query.h"

#define BAUL_TYPE_SEARCH_DIRECTORY baul_search_directory_get_type()
#define BAUL_SEARCH_DIRECTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_SEARCH_DIRECTORY, BaulSearchDirectory))
#define BAUL_SEARCH_DIRECTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_SEARCH_DIRECTORY, BaulSearchDirectoryClass))
#define BAUL_IS_SEARCH_DIRECTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_SEARCH_DIRECTORY))
#define BAUL_IS_SEARCH_DIRECTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_SEARCH_DIRECTORY))
#define BAUL_SEARCH_DIRECTORY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_SEARCH_DIRECTORY, BaulSearchDirectoryClass))

typedef struct BaulSearchDirectoryDetails BaulSearchDirectoryDetails;

typedef struct
{
    BaulDirectory parent_slot;
    BaulSearchDirectoryDetails *details;
} BaulSearchDirectory;

typedef struct
{
    BaulDirectoryClass parent_slot;
} BaulSearchDirectoryClass;

GType   baul_search_directory_get_type             (void);

char   *baul_search_directory_generate_new_uri     (void);

BaulSearchDirectory *baul_search_directory_new_from_saved_search (const char *uri);

gboolean       baul_search_directory_is_saved_search (BaulSearchDirectory *search);
gboolean       baul_search_directory_is_modified     (BaulSearchDirectory *search);
gboolean       baul_search_directory_is_indexed      (BaulSearchDirectory *search);
void           baul_search_directory_save_search     (BaulSearchDirectory *search);
void           baul_search_directory_save_to_file    (BaulSearchDirectory *search,
        const char              *save_file_uri);

BaulQuery *baul_search_directory_get_query       (BaulSearchDirectory *search);
void           baul_search_directory_set_query       (BaulSearchDirectory *search,
        BaulQuery           *query);

#endif /* BAUL_SEARCH_DIRECTORY_H */
