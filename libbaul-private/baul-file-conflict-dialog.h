/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-file-conflict-dialog: dialog that handles file conflicts
   during transfer operations.

   Copyright (C) 2008, Cosimo Cecchi

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

   Authors: Cosimo Cecchi <cosimoc@gnome.org>
*/

#ifndef BAUL_FILE_CONFLICT_DIALOG_H
#define BAUL_FILE_CONFLICT_DIALOG_H

#include <glib-object.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#define BAUL_TYPE_FILE_CONFLICT_DIALOG \
	(baul_file_conflict_dialog_get_type ())
#define BAUL_FILE_CONFLICT_DIALOG(o) \
	(G_TYPE_CHECK_INSTANCE_CAST ((o), BAUL_TYPE_FILE_CONFLICT_DIALOG,\
				     CajaFileConflictDialog))
#define BAUL_FILE_CONFLICT_DIALOG_CLASS(k) \
	(G_TYPE_CHECK_CLASS_CAST((k), BAUL_TYPE_FILE_CONFLICT_DIALOG,\
				 CajaFileConflictDialogClass))
#define BAUL_IS_FILE_CONFLICT_DIALOG(o) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((o), BAUL_TYPE_FILE_CONFLICT_DIALOG))
#define BAUL_IS_FILE_CONFLICT_DIALOG_CLASS(k) \
	(G_TYPE_CHECK_CLASS_TYPE ((k), BAUL_TYPE_FILE_CONFLICT_DIALOG))
#define BAUL_FILE_CONFLICT_DIALOG_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS ((o), BAUL_TYPE_FILE_CONFLICT_DIALOG,\
				    CajaFileConflictDialogClass))

typedef struct _CajaFileConflictDialog        CajaFileConflictDialog;
typedef struct _CajaFileConflictDialogClass   CajaFileConflictDialogClass;
typedef struct _CajaFileConflictDialogPrivate CajaFileConflictDialogPrivate;

struct _CajaFileConflictDialog
{
    GtkDialog parent;
    CajaFileConflictDialogPrivate *details;
};

struct _CajaFileConflictDialogClass
{
    GtkDialogClass parent_class;
};

enum
{
    CONFLICT_RESPONSE_SKIP = 1,
    CONFLICT_RESPONSE_REPLACE = 2,
    CONFLICT_RESPONSE_RENAME = 3,
};

GType baul_file_conflict_dialog_get_type (void) G_GNUC_CONST;

GtkWidget* baul_file_conflict_dialog_new              (GtkWindow *parent,
        GFile *source,
        GFile *destination,
        GFile *dest_dir);
char*      baul_file_conflict_dialog_get_new_name     (CajaFileConflictDialog *dialog);
gboolean   baul_file_conflict_dialog_get_apply_to_all (CajaFileConflictDialog *dialog);

#endif /* BAUL_FILE_CONFLICT_DIALOG_H */
