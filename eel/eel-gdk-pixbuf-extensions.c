/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-cdk-pixbuf-extensions.c: Routines to augment what's in cdk-pixbuf.

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

#include <config.h>
#include "eel-cdk-pixbuf-extensions.h"

#include "eel-debug.h"
#include "eel-cdk-extensions.h"
#include "eel-glib-extensions.h"
#include "eel-graphic-effects.h"
#include "eel-lib-self-check-functions.h"
#include "eel-string.h"
#include <cdk-pixbuf/cdk-pixbuf.h>
#include <cdk/cdkprivate.h>
#include <cdk/cdkx.h>
#include <gio/gio.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define LOAD_BUFFER_SIZE 65536

GdkPixbuf *
eel_cdk_pixbuf_load (const char *uri)
{
    GdkPixbuf *pixbuf;
    GFile *file;
    GFileInputStream *stream;

    g_return_val_if_fail (uri != NULL, NULL);

    file = g_file_new_for_uri (uri);

    stream = g_file_read (file, NULL, NULL);

    g_object_unref (file);

    if (stream == NULL)
    {
        return NULL;
    }

    pixbuf = eel_cdk_pixbuf_load_from_stream (G_INPUT_STREAM (stream));

    g_object_unref (stream);

    return pixbuf;
}

GdkPixbuf *
eel_cdk_pixbuf_load_from_stream (GInputStream  *stream)
{
    return eel_cdk_pixbuf_load_from_stream_at_size (stream, -1);
}

static void
pixbuf_loader_size_prepared (GdkPixbufLoader *loader,
                             int              width,
                             int              height,
                             gpointer         desired_size_ptr)
{
    int size, desired_size;

    size = MAX (width, height);
    desired_size = GPOINTER_TO_INT (desired_size_ptr);

    if (size != desired_size)
    {
        float scale;

        scale = (float) desired_size / size;
        cdk_pixbuf_loader_set_size (loader,
                                    floor (scale * width + 0.5),
                                    floor (scale * height + 0.5));
    }
}

GdkPixbuf *
eel_cdk_pixbuf_load_from_stream_at_size (GInputStream  *stream,
        int            size)
{
    guchar buffer[LOAD_BUFFER_SIZE];
    gssize bytes_read;
    GdkPixbufLoader *loader;
    GdkPixbuf *pixbuf;
    gboolean got_eos;


    g_return_val_if_fail (stream != NULL, NULL);

    got_eos = FALSE;
    loader = cdk_pixbuf_loader_new ();

    if (size > 0)
    {
        g_signal_connect (loader, "size-prepared",
                          G_CALLBACK (pixbuf_loader_size_prepared),
                          GINT_TO_POINTER (size));
    }

    while (1)
    {
        bytes_read = g_input_stream_read (stream, buffer, sizeof (buffer),
                                          NULL, NULL);

        if (bytes_read < 0)
        {
            break;
        }
        if (bytes_read == 0)
        {
            got_eos = TRUE;
            break;
        }
        if (!cdk_pixbuf_loader_write (loader,
                                      buffer,
                                      bytes_read,
                                      NULL))
        {
            break;
        }
    }

    g_input_stream_close (stream, NULL, NULL);
    cdk_pixbuf_loader_close (loader, NULL);

    pixbuf = NULL;
    if (got_eos)
    {
        pixbuf = cdk_pixbuf_loader_get_pixbuf (loader);
        if (pixbuf != NULL)
        {
            g_object_ref (pixbuf);
        }
    }

    g_object_unref (loader);

    return pixbuf;
}

double
eel_cdk_scale_to_fit_factor (int width, int height,
                             int max_width, int max_height,
                             int *scaled_width, int *scaled_height)
{
    double scale_factor;

    scale_factor = MIN (max_width  / (double) width, max_height / (double) height);

    *scaled_width  = floor (width * scale_factor + .5);
    *scaled_height = floor (height * scale_factor + .5);

    return scale_factor;
}

/* Returns a scaled copy of pixbuf, preserving aspect ratio. The copy will
 * be scaled as large as possible without exceeding the specified width and height.
 */
GdkPixbuf *
eel_cdk_pixbuf_scale_to_fit (GdkPixbuf *pixbuf, int max_width, int max_height)
{
    int scaled_width;
    int scaled_height;

    eel_cdk_scale_to_fit_factor (cdk_pixbuf_get_width(pixbuf), cdk_pixbuf_get_height(pixbuf),
                                 max_width, max_height,
                                 &scaled_width, &scaled_height);

    return cdk_pixbuf_scale_simple (pixbuf, scaled_width, scaled_height, GDK_INTERP_BILINEAR);
}

/* Returns a copy of pixbuf scaled down, preserving aspect ratio, to fit
 * within the specified width and height. If it already fits, a copy of
 * the original, without scaling, is returned.
 */
GdkPixbuf *
eel_cdk_pixbuf_scale_down_to_fit (GdkPixbuf *pixbuf, int max_width, int max_height)
{
    int scaled_width;
    int scaled_height;

    double scale_factor;

    scale_factor = eel_cdk_scale_to_fit_factor (cdk_pixbuf_get_width(pixbuf), cdk_pixbuf_get_height(pixbuf),
                   max_width, max_height,
                   &scaled_width, &scaled_height);

    if (scale_factor >= 1.0)
    {
        return cdk_pixbuf_copy (pixbuf);
    }
    else
    {
        return eel_cdk_pixbuf_scale_down (pixbuf, scaled_width, scaled_height);
    }
}

double
eel_cdk_scale_to_min_factor (int width, int height,
                             int min_width, int min_height,
                             int *scaled_width, int *scaled_height)
{
    double scale_factor;

    scale_factor = MAX (min_width / (double) width, min_height / (double) height);

    *scaled_width  = floor (width * scale_factor + .5);
    *scaled_height = floor (height * scale_factor + .5);

    return scale_factor;
}

/* Returns a scaled copy of pixbuf, preserving aspect ratio. The copy will
 * be scaled as small as possible without going under the specified width and height.
 */
GdkPixbuf *
eel_cdk_pixbuf_scale_to_min (GdkPixbuf *pixbuf, int min_width, int min_height)
{
    int scaled_width;
    int scaled_height;

    eel_cdk_scale_to_min_factor (cdk_pixbuf_get_width(pixbuf), cdk_pixbuf_get_height(pixbuf),
                                 min_width, min_height,
                                 &scaled_width, &scaled_height);

    return cdk_pixbuf_scale_simple (pixbuf, scaled_width, scaled_height, GDK_INTERP_BILINEAR);
}

gboolean
eel_cdk_pixbuf_save_to_file (const GdkPixbuf *pixbuf,
                             const char *file_name)
{
    return cdk_pixbuf_save ((GdkPixbuf *) pixbuf,
                            file_name, "png", NULL, NULL);
}

void
eel_cdk_pixbuf_ref_if_not_null (GdkPixbuf *pixbuf_or_null)
{
    if (pixbuf_or_null != NULL)
    {
        g_object_ref (pixbuf_or_null);
    }
}

void
eel_cdk_pixbuf_unref_if_not_null (GdkPixbuf *pixbuf_or_null)
{
    if (pixbuf_or_null != NULL)
    {
        g_object_unref (pixbuf_or_null);
    }
}

GdkPixbuf *
eel_cdk_pixbuf_scale_down (GdkPixbuf *pixbuf,
                           int dest_width,
                           int dest_height)
{
    int source_width, source_height;
    int s_y1, s_x2;
    int s_yfrac;
    int dx, dx_frac, dy, dy_frac;
    div_t ddx, ddy;
    int x, y;
    int r, g, b, a;
    int n_pixels;
    gboolean has_alpha;
    guchar *dest, *src, *xsrc, *src_pixels;
    GdkPixbuf *dest_pixbuf;
    int pixel_stride;
    int source_rowstride, dest_rowstride;

    if (dest_width == 0 || dest_height == 0)
    {
        return NULL;
    }

    source_width = cdk_pixbuf_get_width (pixbuf);
    source_height = cdk_pixbuf_get_height (pixbuf);

    g_assert (source_width >= dest_width);
    g_assert (source_height >= dest_height);

    ddx = div (source_width, dest_width);
    dx = ddx.quot;
    dx_frac = ddx.rem;

    ddy = div (source_height, dest_height);
    dy = ddy.quot;
    dy_frac = ddy.rem;

    has_alpha = cdk_pixbuf_get_has_alpha (pixbuf);
    source_rowstride = cdk_pixbuf_get_rowstride (pixbuf);
    src_pixels = cdk_pixbuf_get_pixels (pixbuf);

    dest_pixbuf = cdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8,
                                  dest_width, dest_height);
    dest = cdk_pixbuf_get_pixels (dest_pixbuf);
    dest_rowstride = cdk_pixbuf_get_rowstride (dest_pixbuf);

    pixel_stride = (has_alpha)?4:3;

    s_y1 = 0;
    s_yfrac = -dest_height/2;
    while (s_y1 < source_height)
    {
        int s_x1, s_y2;
        int s_xfrac;

        s_y2 = s_y1 + dy;
        s_yfrac += dy_frac;
        if (s_yfrac > 0)
        {
            s_y2++;
            s_yfrac -= dest_height;
        }

        s_x1 = 0;
        s_xfrac = -dest_width/2;
        while (s_x1 < source_width)
        {
            s_x2 = s_x1 + dx;
            s_xfrac += dx_frac;
            if (s_xfrac > 0)
            {
                s_x2++;
                s_xfrac -= dest_width;
            }

            /* Average block of [x1,x2[ x [y1,y2[ and store in dest */
            r = g = b = a = 0;
            n_pixels = 0;

            src = src_pixels + s_y1 * source_rowstride + s_x1 * pixel_stride;
            for (y = s_y1; y < s_y2; y++)
            {
                xsrc = src;
                if (has_alpha)
                {
                    for (x = 0; x < s_x2-s_x1; x++)
                    {
                        n_pixels++;

                        r += xsrc[3] * xsrc[0];
                        g += xsrc[3] * xsrc[1];
                        b += xsrc[3] * xsrc[2];
                        a += xsrc[3];
                        xsrc += 4;
                    }
                }
                else
                {
                    for (x = 0; x < s_x2-s_x1; x++)
                    {
                        n_pixels++;
                        r += *xsrc++;
                        g += *xsrc++;
                        b += *xsrc++;
                    }
                }
                src += source_rowstride;
            }

            if (has_alpha)
            {
                if (a != 0)
                {
                    *dest++ = r / a;
                    *dest++ = g / a;
                    *dest++ = b / a;
                    *dest++ = a / n_pixels;
                }
                else
                {
                    *dest++ = 0;
                    *dest++ = 0;
                    *dest++ = 0;
                    *dest++ = 0;
                }
            }
            else
            {
                if (n_pixels != 0)
                {
                    *dest++ = r / n_pixels;
                    *dest++ = g / n_pixels;
                    *dest++ = b / n_pixels;
                }
                else
                {
                    *dest++ = 0;
                    *dest++ = 0;
                    *dest++ = 0;
                }
            }

            s_x1 = s_x2;
        }
        s_y1 = s_y2;
        dest += dest_rowstride - dest_width * pixel_stride;
    }

    return dest_pixbuf;
}
