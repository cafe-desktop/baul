/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
   baul-open-with-dialog.c: an open-with dialog

   Copyright (C) 2004 Novell, Inc.

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

   Authors: Dave Camp <dave@novell.com>
*/

#ifndef BAUL_OPEN_WITH_DIALOG_H
#define BAUL_OPEN_WITH_DIALOG_H

#include <gtk/gtk.h>
#include <gio/gio.h>

#define BAUL_TYPE_OPEN_WITH_DIALOG         (baul_open_with_dialog_get_type ())
#define BAUL_OPEN_WITH_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_OPEN_WITH_DIALOG, BaulOpenWithDialog))
#define BAUL_OPEN_WITH_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_OPEN_WITH_DIALOG, BaulOpenWithDialogClass))
#define BAUL_IS_OPEN_WITH_DIALOG(obj)      (G_TYPE_INSTANCE_CHECK_TYPE ((obj), BAUL_TYPE_OPEN_WITH_DIALOG)

typedef struct _BaulOpenWithDialog        BaulOpenWithDialog;
typedef struct _BaulOpenWithDialogClass   BaulOpenWithDialogClass;
typedef struct _BaulOpenWithDialogDetails BaulOpenWithDialogDetails;

struct _BaulOpenWithDialog
{
    GtkDialog parent;
    BaulOpenWithDialogDetails *details;
};

struct _BaulOpenWithDialogClass
{
    GtkDialogClass parent_class;

    void (*application_selected) (BaulOpenWithDialog *dialog,
                                  GAppInfo *application);
};

GType      baul_open_with_dialog_get_type (void);
GtkWidget* baul_open_with_dialog_new      (const char *uri,
        const char *mime_type,
        const char *extension);
GtkWidget* baul_add_application_dialog_new (const char *uri,
        const char *mime_type);
GtkWidget* baul_add_application_dialog_new_for_multiple_files (const char *extension,
        const char *mime_type);



#endif /* BAUL_OPEN_WITH_DIALOG_H */
