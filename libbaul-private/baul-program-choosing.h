/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-program-choosing.h - functions for selecting and activating
 				 programs for opening/viewing particular files.

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

   Author: John Sullivan <sullivan@eazel.com>
*/

#ifndef BAUL_PROGRAM_CHOOSING_H
#define BAUL_PROGRAM_CHOOSING_H

#include <ctk/ctk.h>
#include <gio/gio.h>

#include "baul-file.h"

typedef void (*BaulApplicationChoiceCallback) (GAppInfo                      *application,
        gpointer			  callback_data);

void baul_launch_application                 (GAppInfo                          *application,
        GList                             *files,
        CtkWindow                         *parent_window);
void baul_launch_application_by_uri          (GAppInfo                          *application,
        GList                             *uris,
        CtkWindow                         *parent_window);
void baul_launch_application_from_command    (CdkScreen                         *screen,
        const char                        *name,
        const char                        *command_string,
        gboolean                           use_terminal,
        ...) G_GNUC_NULL_TERMINATED;
void baul_launch_application_from_command_array (CdkScreen                         *screen,
        const char                        *name,
        const char                        *command_string,
        gboolean                           use_terminal,
        const char * const *               parameters);
void baul_launch_desktop_file		 (CdkScreen                         *screen,
                                      const char                        *desktop_file_uri,
                                      const GList                       *parameter_uris,
                                      CtkWindow                         *parent_window);

#endif /* BAUL_PROGRAM_CHOOSING_H */
