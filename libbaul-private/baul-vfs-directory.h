/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-vfs-directory.h: Subclass of BaulDirectory to implement the
   the case of a VFS directory.

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

#ifndef BAUL_VFS_DIRECTORY_H
#define BAUL_VFS_DIRECTORY_H

#include "baul-directory.h"

#define BAUL_TYPE_VFS_DIRECTORY baul_vfs_directory_get_type()
#define BAUL_VFS_DIRECTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_VFS_DIRECTORY, BaulVFSDirectory))
#define BAUL_VFS_DIRECTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_VFS_DIRECTORY, BaulVFSDirectoryClass))
#define BAUL_IS_VFS_DIRECTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_VFS_DIRECTORY))
#define BAUL_IS_VFS_DIRECTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_VFS_DIRECTORY))
#define BAUL_VFS_DIRECTORY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_VFS_DIRECTORY, BaulVFSDirectoryClass))

typedef struct BaulVFSDirectoryDetails BaulVFSDirectoryDetails;

typedef struct
{
    BaulDirectory parent_slot;
} BaulVFSDirectory;

typedef struct
{
    BaulDirectoryClass parent_slot;
} BaulVFSDirectoryClass;

GType   baul_vfs_directory_get_type (void);

#endif /* BAUL_VFS_DIRECTORY_H */
