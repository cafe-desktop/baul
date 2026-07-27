/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-cdk-pixbuf-extensions.h: Routines to augment what's in cdk-pixbuf.

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

   Authors: Darin Adler <darin@eazel.com>
            Ramiro Estrugo <ramiro@eazel.com>
*/

#ifndef EEL_CDK_PIXBUF_EXTENSIONS_H
#define EEL_CDK_PIXBUF_EXTENSIONS_H

#include <cdk-pixbuf/cdk-pixbuf.h>
#include <cdk/cdk.h>
#include <gio/gio.h>

/* Loading a CdkPixbuf with a URI. */
CdkPixbuf *          eel_cdk_pixbuf_load                      (const char            *uri);
CdkPixbuf *          eel_cdk_pixbuf_load_from_stream          (GInputStream          *stream);
CdkPixbuf *          eel_cdk_pixbuf_load_from_stream_at_size  (GInputStream          *stream,
        int                    size);


CdkPixbuf *          eel_cdk_pixbuf_scale_down_to_fit         (CdkPixbuf             *pixbuf,
        int                    max_width,
        int                    max_height);
CdkPixbuf *          eel_cdk_pixbuf_scale_to_fit              (CdkPixbuf             *pixbuf,
        int                    max_width,
        int                    max_height);
double               eel_cdk_scale_to_fit_factor              (int                    width,
        int                    height,
        int                    max_width,
        int                    max_height,
        int                   *scaled_width,
        int                   *scaled_height);
CdkPixbuf *          eel_cdk_pixbuf_scale_to_min              (CdkPixbuf             *pixbuf,
        int                    min_width,
        int                    min_height);
double              eel_cdk_scale_to_min_factor               (int                   width,
        int                   height,
        int                   min_width,
        int                   min_height,
        int                   *scaled_width,
        int                   *scaled_height);


/* Save a pixbuf to a png file.  Return value indicates succss/TRUE or failure/FALSE */
gboolean             eel_cdk_pixbuf_save_to_file              (const CdkPixbuf       *pixbuf,
        const char            *file_name);
void                 eel_cdk_pixbuf_ref_if_not_null           (CdkPixbuf             *pixbuf_or_null);
void                 eel_cdk_pixbuf_unref_if_not_null         (CdkPixbuf             *pixbuf_or_null);


/* Scales large pixbufs down fast */
CdkPixbuf *          eel_cdk_pixbuf_scale_down                (CdkPixbuf *pixbuf,
        int dest_width,
        int dest_height);

#endif /* EEL_CDK_PIXBUF_EXTENSIONS_H */
