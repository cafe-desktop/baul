/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-desktop-link.h: Class that handles the links on the desktop

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

#ifndef BAUL_DESKTOP_LINK_H
#define BAUL_DESKTOP_LINK_H

#include <gio/gio.h>

#include "baul-file.h"

#define BAUL_TYPE_DESKTOP_LINK baul_desktop_link_get_type()
#define BAUL_DESKTOP_LINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_DESKTOP_LINK, BaulDesktopLink))
#define BAUL_DESKTOP_LINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_DESKTOP_LINK, BaulDesktopLinkClass))
#define BAUL_IS_DESKTOP_LINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_DESKTOP_LINK))
#define BAUL_IS_DESKTOP_LINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_DESKTOP_LINK))
#define BAUL_DESKTOP_LINK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_DESKTOP_LINK, BaulDesktopLinkClass))

typedef struct _BaulDesktopLinkPrivate BaulDesktopLinkPrivate;

typedef struct
{
    GObject parent_slot;
    BaulDesktopLinkPrivate *details;
} BaulDesktopLink;

typedef struct
{
    GObjectClass parent_slot;
} BaulDesktopLinkClass;

typedef enum
{
    BAUL_DESKTOP_LINK_HOME,
    BAUL_DESKTOP_LINK_COMPUTER,
    BAUL_DESKTOP_LINK_TRASH,
    BAUL_DESKTOP_LINK_MOUNT,
    BAUL_DESKTOP_LINK_NETWORK
} BaulDesktopLinkType;

GType   baul_desktop_link_get_type (void);

BaulDesktopLink *   baul_desktop_link_new                     (BaulDesktopLinkType  type);
BaulDesktopLink *   baul_desktop_link_new_from_mount          (GMount                 *mount);
BaulDesktopLinkType baul_desktop_link_get_link_type           (BaulDesktopLink     *link);
char *                  baul_desktop_link_get_file_name           (BaulDesktopLink     *link);
char *                  baul_desktop_link_get_display_name        (BaulDesktopLink     *link);
GIcon *                 baul_desktop_link_get_icon                (BaulDesktopLink     *link);
GFile *                 baul_desktop_link_get_activation_location (BaulDesktopLink     *link);
char *                  baul_desktop_link_get_activation_uri      (BaulDesktopLink     *link);
gboolean                baul_desktop_link_get_date                (BaulDesktopLink     *link,
        BaulDateType         date_type,
        time_t                  *date);
GMount *                baul_desktop_link_get_mount               (BaulDesktopLink     *link);
gboolean                baul_desktop_link_can_rename              (BaulDesktopLink     *link);
gboolean                baul_desktop_link_rename                  (BaulDesktopLink     *link,
        const char              *name);


#endif /* BAUL_DESKTOP_LINK_H */
