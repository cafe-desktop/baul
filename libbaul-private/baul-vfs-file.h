/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-vfs-file.h: Subclass of CajaFile to implement the
   the case of a VFS file.

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

#ifndef BAUL_VFS_FILE_H
#define BAUL_VFS_FILE_H

#include "baul-file.h"

#define BAUL_TYPE_VFS_FILE baul_vfs_file_get_type()
#define BAUL_VFS_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_VFS_FILE, CajaVFSFile))
#define BAUL_VFS_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_VFS_FILE, CajaVFSFileClass))
#define BAUL_IS_VFS_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_VFS_FILE))
#define BAUL_IS_VFS_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_VFS_FILE))
#define BAUL_VFS_FILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_VFS_FILE, CajaVFSFileClass))

typedef struct CajaVFSFileDetails CajaVFSFileDetails;

typedef struct
{
    CajaFile parent_slot;
} CajaVFSFile;

typedef struct
{
    CajaFileClass parent_slot;
} CajaVFSFileClass;

GType   baul_vfs_file_get_type (void);

#endif /* BAUL_VFS_FILE_H */
