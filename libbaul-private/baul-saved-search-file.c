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
#include <config.h>
#include "baul-saved-search-file.h"
#include "baul-file-private.h"

G_DEFINE_TYPE(BaulSavedSearchFile, baul_saved_search_file, BAUL_TYPE_VFS_FILE)


static void
baul_saved_search_file_init (BaulSavedSearchFile *search_file G_GNUC_UNUSED)
{
}

static void
baul_saved_search_file_class_init (BaulSavedSearchFileClass * klass)
{
    BaulFileClass *file_class;

    file_class = BAUL_FILE_CLASS (klass);

    file_class->default_file_type = G_FILE_TYPE_DIRECTORY;
}

