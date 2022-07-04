/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-desktop-directory-file.h: Subclass of BaulFile to implement the
   the case of the desktop directory

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

#ifndef BAUL_DESKTOP_DIRECTORY_FILE_H
#define BAUL_DESKTOP_DIRECTORY_FILE_H

#include "baul-file.h"

#define BAUL_TYPE_DESKTOP_DIRECTORY_FILE baul_desktop_directory_file_get_type()
#define BAUL_DESKTOP_DIRECTORY_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_DESKTOP_DIRECTORY_FILE, BaulDesktopDirectoryFile))
#define BAUL_DESKTOP_DIRECTORY_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_DESKTOP_DIRECTORY_FILE, BaulDesktopDirectoryFileClass))
#define BAUL_IS_DESKTOP_DIRECTORY_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_DESKTOP_DIRECTORY_FILE))
#define BAUL_IS_DESKTOP_DIRECTORY_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_DESKTOP_DIRECTORY_FILE))
#define BAUL_DESKTOP_DIRECTORY_FILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_DESKTOP_DIRECTORY_FILE, BaulDesktopDirectoryFileClass))

typedef struct BaulDesktopDirectoryFileDetails BaulDesktopDirectoryFileDetails;

typedef struct
{
    BaulFile parent_slot;
    BaulDesktopDirectoryFileDetails *details;
} BaulDesktopDirectoryFile;

typedef struct
{
    BaulFileClass parent_slot;
} BaulDesktopDirectoryFileClass;

GType    baul_desktop_directory_file_get_type    (void);

#endif /* BAUL_DESKTOP_DIRECTORY_FILE_H */
