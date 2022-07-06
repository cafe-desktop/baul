/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
   baul-mime-application-chooser.c: Manages applications for mime types

   Copyright (C) 2004 Novell, Inc.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but APPLICATIONOUT ANY WARRANTY; applicationout even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along application the Cafe Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Dave Camp <dave@novell.com>
*/

#ifndef BAUL_MIME_APPLICATION_CHOOSER_H
#define BAUL_MIME_APPLICATION_CHOOSER_H

#include <ctk/ctk.h>

#define BAUL_TYPE_MIME_APPLICATION_CHOOSER         (baul_mime_application_chooser_get_type ())
#define BAUL_MIME_APPLICATION_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_MIME_APPLICATION_CHOOSER, BaulMimeApplicationChooser))
#define BAUL_MIME_APPLICATION_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_MIME_APPLICATION_CHOOSER, BaulMimeApplicationChooserClass))
#define BAUL_IS_MIME_APPLICATION_CHOOSER(obj)      (G_TYPE_INSTANCE_CHECK_TYPE ((obj), BAUL_TYPE_MIME_APPLICATION_CHOOSER)

typedef struct _BaulMimeApplicationChooser        BaulMimeApplicationChooser;
typedef struct _BaulMimeApplicationChooserClass   BaulMimeApplicationChooserClass;
typedef struct _BaulMimeApplicationChooserDetails BaulMimeApplicationChooserDetails;

struct _BaulMimeApplicationChooser
{
    GtkBox parent;
    BaulMimeApplicationChooserDetails *details;
};

struct _BaulMimeApplicationChooserClass
{
    GtkBoxClass parent_class;
};

GType      baul_mime_application_chooser_get_type (void);
GtkWidget* baul_mime_application_chooser_new      (const char *uri,
        const char *mime_type);
GtkWidget* baul_mime_application_chooser_new_for_multiple_files (GList *uris,
        const char *mime_type);

#endif /* BAUL_MIME_APPLICATION_CHOOSER_H */
