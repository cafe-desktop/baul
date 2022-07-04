/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-desktop-directory.h: Subclass of CajaDirectory to implement
   a virtual directory consisting of the desktop directory and the desktop
   icons

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

#ifndef BAUL_DESKTOP_DIRECTORY_H
#define BAUL_DESKTOP_DIRECTORY_H

#include "baul-directory.h"

#define BAUL_TYPE_DESKTOP_DIRECTORY baul_desktop_directory_get_type()
#define BAUL_DESKTOP_DIRECTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_DESKTOP_DIRECTORY, CajaDesktopDirectory))
#define BAUL_DESKTOP_DIRECTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_DESKTOP_DIRECTORY, CajaDesktopDirectoryClass))
#define BAUL_IS_DESKTOP_DIRECTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_DESKTOP_DIRECTORY))
#define BAUL_IS_DESKTOP_DIRECTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_DESKTOP_DIRECTORY))
#define BAUL_DESKTOP_DIRECTORY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_DESKTOP_DIRECTORY, CajaDesktopDirectoryClass))

typedef struct CajaDesktopDirectoryDetails CajaDesktopDirectoryDetails;

typedef struct
{
    CajaDirectory parent_slot;
    CajaDesktopDirectoryDetails *details;
} CajaDesktopDirectory;

typedef struct
{
    CajaDirectoryClass parent_slot;

} CajaDesktopDirectoryClass;

GType   baul_desktop_directory_get_type             (void);
CajaDirectory * baul_desktop_directory_get_real_directory   (CajaDesktopDirectory *desktop_directory);

#endif /* BAUL_DESKTOP_DIRECTORY_H */
