/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-saved-search-file.h: Subclass of BaulVFSFile to implement the
   the case of a Saved Search file.

   Copyright (C) 2005 Red Hat, Inc

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

   Author: Alexander Larsson
*/

#ifndef BAUL_SAVED_SEARCH_FILE_H
#define BAUL_SAVED_SEARCH_FILE_H

#include "baul-vfs-file.h"

#define BAUL_TYPE_SAVED_SEARCH_FILE baul_saved_search_file_get_type()
#define BAUL_SAVED_SEARCH_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_SAVED_SEARCH_FILE, BaulSavedSearchFile))
#define BAUL_SAVED_SEARCH_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_SAVED_SEARCH_FILE, BaulSavedSearchFileClass))
#define BAUL_IS_SAVED_SEARCH_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_SAVED_SEARCH_FILE))
#define BAUL_IS_SAVED_SEARCH_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_SAVED_SEARCH_FILE))
#define BAUL_SAVED_SEARCH_FILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_SAVED_SEARCH_FILE, BaulSavedSearchFileClass))


typedef struct BaulSavedSearchFileDetails BaulSavedSearchFileDetails;

typedef struct
{
    BaulFile parent_slot;
} BaulSavedSearchFile;

typedef struct
{
    BaulFileClass parent_slot;
} BaulSavedSearchFileClass;

GType   baul_saved_search_file_get_type (void);

#endif /* BAUL_SAVED_SEARCH_FILE_H */
