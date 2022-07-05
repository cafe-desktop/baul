/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-mime-actions.h - uri-specific versions of mime action functions

   Copyright (C) 2000 Eazel, Inc.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Cafe Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Maciej Stachowiak <mjs@eazel.com>
*/

#ifndef BAUL_MIME_ACTIONS_H
#define BAUL_MIME_ACTIONS_H

#include <gio/gio.h>

#include "baul-file.h"
#include "baul-window-info.h"
#include "baul-window-slot-info.h"

BaulFileAttributes baul_mime_actions_get_required_file_attributes (void);

GAppInfo *             baul_mime_get_default_application_for_file     (BaulFile            *file);
GList *                baul_mime_get_applications_for_file            (BaulFile            *file);

GAppInfo *             baul_mime_get_default_application_for_files    (GList                   *files);
GList *                baul_mime_get_applications_for_files           (GList                   *file);

gboolean               baul_mime_has_any_applications_for_file        (BaulFile            *file);

gboolean               baul_mime_file_opens_in_view                   (BaulFile            *file);
gboolean               baul_mime_file_opens_in_external_app           (BaulFile            *file);
void                   baul_mime_activate_files                       (GtkWindow               *parent_window,
        BaulWindowSlotInfo  *slot_info,
        GList                   *files,
        const char              *launch_directory,
        BaulWindowOpenMode   mode,
        BaulWindowOpenFlags  flags,
        gboolean                 user_confirmation);
void                   baul_mime_activate_file                        (GtkWindow               *parent_window,
        BaulWindowSlotInfo  *slot_info,
        BaulFile            *file,
        const char              *launch_directory,
        BaulWindowOpenMode   mode,
        BaulWindowOpenFlags  flags);


#endif /* BAUL_MIME_ACTIONS_H */
