/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* fm-error-reporting.h - interface for file manager functions that report
 			  errors to the user.

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

   Authors: John Sullivan <sullivan@eazel.com>
*/

#ifndef FM_ERROR_REPORTING_H
#define FM_ERROR_REPORTING_H

#include <ctk/ctk.h>

#include <libbaul-private/baul-file.h>

void fm_report_error_loading_directory	 (BaulFile   *file,
        GError         *error,
        CtkWindow	 *parent_window);
void fm_report_error_renaming_file       (BaulFile   *file,
        const char     *new_name,
        GError         *error,
        CtkWindow	 *parent_window);
void fm_report_error_setting_permissions (BaulFile   *file,
        GError         *error,
        CtkWindow	 *parent_window);
void fm_report_error_setting_owner       (BaulFile   *file,
        GError         *error,
        CtkWindow	 *parent_window);
void fm_report_error_setting_group       (BaulFile   *file,
        GError         *error,
        CtkWindow	 *parent_window);

/* FIXME bugzilla.gnome.org 42394: Should this file be renamed or should this function be moved? */
void fm_rename_file                      (BaulFile   *file,
        const char     *new_name,
        BaulFileOperationCallback callback,
        gpointer callback_data);

#endif /* FM_ERROR_REPORTING_H */
