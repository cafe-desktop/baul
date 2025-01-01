/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* baul-icon-info.c
 * Copyright (C) 2007  Red Hat, Inc.,  Alexander Larsson <alexl@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <string.h>
#include "baul-icon-info.h"
#include "baul-icon-names.h"
#include "baul-default-file-icon.h"
#include <ctk/ctk.h>
#include <gio/gio.h>

struct _BaulIconInfo
{
    GObject parent;

    gboolean sole_owner;
    gint64 last_use_time;
    GdkPixbuf *pixbuf;

    gint n_attach_points;
    CdkPoint *attach_points;
    char *icon_name;

    gint  orig_scale;
};

struct _BaulIconInfoClass
{
    GObjectClass parent_class;
};

static void schedule_reap_cache (void);

G_DEFINE_TYPE (BaulIconInfo,
               baul_icon_info,
               G_TYPE_OBJECT);

static void
baul_icon_info_init (BaulIconInfo *icon)
{
    icon->last_use_time = g_get_monotonic_time ();
    icon->sole_owner = TRUE;
}

gboolean
baul_icon_info_is_fallback (BaulIconInfo  *icon)
{
    return icon->pixbuf == NULL;
}

static void
pixbuf_toggle_notify (gpointer      info,
                      GObject      *object,
                      gboolean      is_last_ref)
{
    BaulIconInfo  *icon = info;

    if (is_last_ref)
    {
        icon->sole_owner = TRUE;
        g_object_remove_toggle_ref (object,
                                    pixbuf_toggle_notify,
                                    info);
        icon->last_use_time = g_get_monotonic_time ();
        schedule_reap_cache ();
    }
}

static void
baul_icon_info_finalize (GObject *object)
{
    BaulIconInfo *icon;

    icon = BAUL_ICON_INFO (object);

    if (!icon->sole_owner && icon->pixbuf)
    {
        g_object_remove_toggle_ref (G_OBJECT (icon->pixbuf),
                                    pixbuf_toggle_notify,
                                    icon);
    }

    if (icon->pixbuf)
    {
        g_object_unref (icon->pixbuf);
    }
    g_free (icon->attach_points);
    g_free (icon->icon_name);

    G_OBJECT_CLASS (baul_icon_info_parent_class)->finalize (object);
}

static void
baul_icon_info_class_init (BaulIconInfoClass *icon_info_class)
{
    GObjectClass *gobject_class;

    gobject_class = (GObjectClass *) icon_info_class;

    gobject_class->finalize = baul_icon_info_finalize;

}

BaulIconInfo *
baul_icon_info_new_for_pixbuf (GdkPixbuf *pixbuf,
                               gint       scale)
{
    BaulIconInfo *icon;

    icon = g_object_new (BAUL_TYPE_ICON_INFO, NULL);

    if (pixbuf)
    {
        icon->pixbuf = g_object_ref (pixbuf);
    }

    icon->orig_scale = scale;

    return icon;
}

static BaulIconInfo *
baul_icon_info_new_for_icon_info (CtkIconInfo *icon_info,
                                  gint         scale)
{
    BaulIconInfo *icon;
    const char *filename;
    char *basename;

    icon = g_object_new (BAUL_TYPE_ICON_INFO, NULL);

    icon->pixbuf = ctk_icon_info_load_icon (icon_info, NULL);

    filename = ctk_icon_info_get_filename (icon_info);

    if (filename != NULL)
    {
        char *p;

        basename = g_path_get_basename (filename);
        p = strrchr (basename, '.');
        if (p)
        {
            *p = 0;
        }
        icon->icon_name = basename;
    }

    icon->orig_scale = scale;

    return icon;
}


typedef struct
{
    GIcon *icon;
    int scale;
    int size;
} IconKey;

static GHashTable *loadable_icon_cache = NULL;
static GHashTable *themed_icon_cache = NULL;
static guint reap_cache_timeout = 0;

#define MICROSEC_PER_SEC ((guint64)1000000L)

static guint time_now;

static gboolean
reap_old_icon (gpointer  key G_GNUC_UNUSED,
	       gpointer  value,
	       gpointer  user_info)
{
    BaulIconInfo *icon = value;
    gboolean *reapable_icons_left = user_info;

    if (icon->sole_owner)
    {
        if (time_now - icon->last_use_time > 30 * MICROSEC_PER_SEC)
        {
            /* This went unused 30 secs ago. reap */
            return TRUE;
        }
        else
        {
            /* We can reap this soon */
            *reapable_icons_left = TRUE;
        }
    }

    return FALSE;
}

static gboolean
reap_cache (gpointer data G_GNUC_UNUSED)
{
    gboolean reapable_icons_left;

    reapable_icons_left = TRUE;

    time_now = g_get_monotonic_time ();

    if (loadable_icon_cache)
    {
        g_hash_table_foreach_remove (loadable_icon_cache,
                                     reap_old_icon,
                                     &reapable_icons_left);
    }

    if (themed_icon_cache)
    {
        g_hash_table_foreach_remove (themed_icon_cache,
                                     reap_old_icon,
                                     &reapable_icons_left);
    }

    if (reapable_icons_left)
    {
        return TRUE;
    }
    else
    {
        reap_cache_timeout = 0;
        return FALSE;
    }
}

static void
schedule_reap_cache (void)
{
    if (reap_cache_timeout == 0)
    {
        reap_cache_timeout = g_timeout_add_seconds_full (0, 5,
                             reap_cache,
                             NULL, NULL);
    }
}

void
baul_icon_info_clear_caches (void)
{
    if (loadable_icon_cache)
    {
        g_hash_table_remove_all (loadable_icon_cache);
    }

    if (themed_icon_cache)
    {
        g_hash_table_remove_all (themed_icon_cache);
    }
}

static guint
icon_key_hash (IconKey *key)
{
    return g_icon_hash (key->icon) ^ key->size;
}

static gboolean
icon_key_equal (const IconKey *a,
                         const IconKey *b)
{
    return a->size == b->size &&
           a->scale == b->scale &&
           g_icon_equal (a->icon, b->icon);
}

static IconKey *
icon_key_new (GIcon *icon,
	      int    scale G_GNUC_UNUSED,
	      int    size)
{
    IconKey *key;

    key = g_slice_new (IconKey);
    key->icon = g_object_ref (icon);
    key->size = size;

    return key;
}

static void
icon_key_free (IconKey *key)
{
    g_object_unref (key->icon);
    g_slice_free (IconKey, key);
}

BaulIconInfo *
baul_icon_info_lookup (GIcon *icon,
                       int size,
                       int scale)
{
    CtkIconTheme *icon_theme;
    CtkIconInfo *ctkicon_info;

    BaulIconInfo *icon_info;

    icon_theme = ctk_icon_theme_get_default ();

    if (G_IS_LOADABLE_ICON (icon)) {
        GdkPixbuf *pixbuf;
        IconKey lookup_key;
        IconKey *key;
        GInputStream *stream;

        if (loadable_icon_cache == NULL) {
            loadable_icon_cache = g_hash_table_new_full ((GHashFunc) icon_key_hash,
                                                         (GEqualFunc) icon_key_equal,
                                                         (GDestroyNotify) icon_key_free,
                                                         (GDestroyNotify) g_object_unref);
        }

        lookup_key.icon = icon;
        lookup_key.scale = scale;
        lookup_key.size = size * scale;

        icon_info = g_hash_table_lookup (loadable_icon_cache, &lookup_key);
        if (icon_info)
        {
            return g_object_ref (icon_info);
        }

        pixbuf = NULL;
        stream = g_loadable_icon_load (G_LOADABLE_ICON (icon),
                                       size * scale,
                                       NULL, NULL, NULL);
        if (stream)
        {
            pixbuf = gdk_pixbuf_new_from_stream_at_scale (stream,
                                                          size * scale, size * scale,
                                                          TRUE,
                                                          NULL, NULL);
            g_input_stream_close (stream, NULL, NULL);
            g_object_unref (stream);
        }

        if (!pixbuf) {
            ctkicon_info = ctk_icon_theme_lookup_icon_for_scale (icon_theme,
                                                                 "text-x-generic",
                                                                 size,
                                                                 scale,
                                                                 CTK_ICON_LOOKUP_FORCE_SIZE);
            pixbuf = ctk_icon_info_load_icon (ctkicon_info, NULL);
        }


        icon_info = baul_icon_info_new_for_pixbuf (pixbuf, scale);

        key = icon_key_new (icon, scale, size);
        g_hash_table_insert (loadable_icon_cache, key, icon_info);
        g_clear_object (&pixbuf);

        return g_object_ref (icon_info);
    }   else  {
        IconKey lookup_key;
        IconKey *key;
        if (themed_icon_cache == NULL) {
            themed_icon_cache = g_hash_table_new_full ((GHashFunc) icon_key_hash,
                                                       (GEqualFunc) icon_key_equal,
                                                       (GDestroyNotify) icon_key_free,
                                                       (GDestroyNotify) g_object_unref);
        }
        lookup_key.icon = icon;
        lookup_key.scale = scale;
        lookup_key.size = size;

        icon_info = g_hash_table_lookup (themed_icon_cache, &lookup_key);
        if (icon_info) {
            return g_object_ref (icon_info);
        }

        ctkicon_info = NULL;

        ctkicon_info = ctk_icon_theme_lookup_by_gicon_for_scale (icon_theme,
                                                                 icon,
                                                                 size,
                                                                 scale,
                                                                 CTK_ICON_LOOKUP_FORCE_SIZE);

        if (!ctkicon_info) {
            ctkicon_info = ctk_icon_theme_lookup_icon_for_scale (icon_theme,
                                                                 "text-x-generic",
                                                                 size,
                                                                 scale,
                                                                 CTK_ICON_LOOKUP_FORCE_SIZE);
        }

        icon_info = baul_icon_info_new_for_icon_info (ctkicon_info, scale);
        g_object_unref (ctkicon_info);

        key = icon_key_new (icon,scale, size);
        g_hash_table_insert (themed_icon_cache, key, icon_info);

        return g_object_ref (icon_info);
    }

}

BaulIconInfo *
baul_icon_info_lookup_from_name (const char *name,
                                 int size,
                                 int scale)
{
    GIcon *icon;
    BaulIconInfo *info;

    icon = g_themed_icon_new (name);
    info = baul_icon_info_lookup (icon, size, scale);
    g_object_unref (icon);
    return info;
}

BaulIconInfo *
baul_icon_info_lookup_from_path (const char *path,
                                 int size,
                                 int scale)
{
    GFile *icon_file;
    GIcon *icon;
    BaulIconInfo *info;

    icon_file = g_file_new_for_path (path);
    icon = g_file_icon_new (icon_file);
    info = baul_icon_info_lookup (icon, size, scale);
    g_object_unref (icon);
    g_object_unref (icon_file);
    return info;
}

GdkPixbuf *
baul_icon_info_get_pixbuf_nodefault (BaulIconInfo  *icon)
{
    GdkPixbuf *res;

    if (icon->pixbuf == NULL)
    {
        res = NULL;
    }
    else
    {
        res = g_object_ref (icon->pixbuf);

        if (icon->sole_owner)
        {
            icon->sole_owner = FALSE;
            g_object_add_toggle_ref (G_OBJECT (res),
                                     pixbuf_toggle_notify,
                                     icon);
        }
    }

    return res;
}

cairo_surface_t *
baul_icon_info_get_surface_nodefault (BaulIconInfo *icon)
{
    GdkPixbuf *pixbuf;
    cairo_surface_t *surface;

    pixbuf = baul_icon_info_get_pixbuf_nodefault (icon);
    surface = cdk_cairo_surface_create_from_pixbuf (pixbuf, icon->orig_scale, NULL);
    g_object_unref (pixbuf);

    return surface;
}

GdkPixbuf *
baul_icon_info_get_pixbuf (BaulIconInfo *icon)
{
    GdkPixbuf *res;

    res = baul_icon_info_get_pixbuf_nodefault (icon);
    if (res == NULL)
    {
        res = gdk_pixbuf_new_from_data (baul_default_file_icon,
                                        GDK_COLORSPACE_RGB,
                                        TRUE,
                                        8,
                                        baul_default_file_icon_width,
                                        baul_default_file_icon_height,
                                        baul_default_file_icon_width * 4, /* stride */
                                        NULL, /* don't destroy info */
                                        NULL);
    }

    return res;
}

cairo_surface_t *
baul_icon_info_get_surface (BaulIconInfo *icon)
{
    GdkPixbuf *pixbuf;
    cairo_surface_t *surface;

    pixbuf = baul_icon_info_get_pixbuf (icon);
    surface = cdk_cairo_surface_create_from_pixbuf (pixbuf, icon->orig_scale, NULL);
    g_object_unref (pixbuf);

    return surface;
}

GdkPixbuf *
baul_icon_info_get_pixbuf_nodefault_at_size (BaulIconInfo  *icon,
        gsize              forced_size)
{
    GdkPixbuf *pixbuf, *scaled_pixbuf;
    int w, h, s;
    double scale;

    pixbuf = baul_icon_info_get_pixbuf_nodefault (icon);

    if (pixbuf == NULL)
        return NULL;

    w = gdk_pixbuf_get_width (pixbuf) / icon->orig_scale;
    h = gdk_pixbuf_get_height (pixbuf) / icon->orig_scale;
    s = MAX (w, h);
    if (s == forced_size)
    {
        return pixbuf;
    }

    scale = (double)forced_size / s;
    scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                    w * scale, h * scale,
                    GDK_INTERP_BILINEAR);
    g_object_unref (pixbuf);
    return scaled_pixbuf;
}

cairo_surface_t *
baul_icon_info_get_surface_nodefault_at_size (BaulIconInfo *icon,
                                              gsize         forced_size)
{
    GdkPixbuf *pixbuf;
    cairo_surface_t *surface;

    pixbuf = baul_icon_info_get_pixbuf_nodefault_at_size (icon, forced_size);
    /*catch the case of baul_icon_info_get_pixbuf_nodefault_at_size returning NULL */
    if (!pixbuf)
        return NULL;
    surface = cdk_cairo_surface_create_from_pixbuf (pixbuf, icon->orig_scale, NULL);
    g_object_unref (pixbuf);

    return surface;
}

GdkPixbuf *
baul_icon_info_get_pixbuf_at_size (BaulIconInfo  *icon,
                                   gsize              forced_size)
{
    GdkPixbuf *pixbuf, *scaled_pixbuf;
    int w, h, s;
    double scale;

    pixbuf = baul_icon_info_get_pixbuf (icon);

    if (pixbuf == NULL)
        return NULL;

    w = gdk_pixbuf_get_width (pixbuf) / icon->orig_scale;
    h = gdk_pixbuf_get_height (pixbuf) / icon->orig_scale;
    s = MAX (w, h);
    if (s == forced_size)
    {
        return pixbuf;
    }

    scale = (double)forced_size / s;
    scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                    w * scale, h * scale,
                    GDK_INTERP_BILINEAR);
    g_object_unref (pixbuf);
    return scaled_pixbuf;
}

cairo_surface_t *
baul_icon_info_get_surface_at_size (BaulIconInfo *icon,
                                    gsize         forced_size)
{
    GdkPixbuf *pixbuf;
    cairo_surface_t *surface;

    pixbuf = baul_icon_info_get_pixbuf_at_size (icon, forced_size);
    surface = cdk_cairo_surface_create_from_pixbuf (pixbuf, icon->orig_scale, NULL);
    g_object_unref (pixbuf);

    return surface;
}

gboolean
baul_icon_info_get_attach_points (BaulIconInfo  *icon,
                                  CdkPoint         **points,
                                  gint              *n_points)
{
    *n_points = icon->n_attach_points;
    *points = icon->attach_points;
    return icon->n_attach_points != 0;
}

const char* baul_icon_info_get_used_name(BaulIconInfo* icon)
{
    return icon->icon_name;
}

/* Return nominal icon size for given zoom level.
 * @zoom_level: zoom level for which to find matching icon size.
 *
 * Return value: icon size between BAUL_ICON_SIZE_SMALLEST and
 * BAUL_ICON_SIZE_LARGEST, inclusive.
 */
guint
baul_get_icon_size_for_zoom_level (BaulZoomLevel zoom_level)
{
    switch (zoom_level)
    {
    case BAUL_ZOOM_LEVEL_SMALLEST:
        return BAUL_ICON_SIZE_SMALLEST;
    case BAUL_ZOOM_LEVEL_SMALLER:
        return BAUL_ICON_SIZE_SMALLER;
    case BAUL_ZOOM_LEVEL_SMALL:
        return BAUL_ICON_SIZE_SMALL;
    case BAUL_ZOOM_LEVEL_STANDARD:
        return BAUL_ICON_SIZE_STANDARD;
    case BAUL_ZOOM_LEVEL_LARGE:
        return BAUL_ICON_SIZE_LARGE;
    case BAUL_ZOOM_LEVEL_LARGER:
        return BAUL_ICON_SIZE_LARGER;
    case BAUL_ZOOM_LEVEL_LARGEST:
        return BAUL_ICON_SIZE_LARGEST;
    }
    g_return_val_if_reached (BAUL_ICON_SIZE_STANDARD);
}

float
baul_get_relative_icon_size_for_zoom_level (BaulZoomLevel zoom_level)
{
    return (float)baul_get_icon_size_for_zoom_level (zoom_level) / BAUL_ICON_SIZE_STANDARD;
}

guint
baul_icon_get_larger_icon_size (guint size)
{
    if (size < BAUL_ICON_SIZE_SMALLEST)
    {
        return BAUL_ICON_SIZE_SMALLEST;
    }
    if (size < BAUL_ICON_SIZE_SMALLER)
    {
        return BAUL_ICON_SIZE_SMALLER;
    }
    if (size < BAUL_ICON_SIZE_SMALL)
    {
        return BAUL_ICON_SIZE_SMALL;
    }
    if (size < BAUL_ICON_SIZE_STANDARD)
    {
        return BAUL_ICON_SIZE_STANDARD;
    }
    if (size < BAUL_ICON_SIZE_LARGE)
    {
        return BAUL_ICON_SIZE_LARGE;
    }
    if (size < BAUL_ICON_SIZE_LARGER)
    {
        return BAUL_ICON_SIZE_LARGER;
    }
    return BAUL_ICON_SIZE_LARGEST;
}

guint
baul_icon_get_smaller_icon_size (guint size)
{
    if (size > BAUL_ICON_SIZE_LARGEST)
    {
        return BAUL_ICON_SIZE_LARGEST;
    }
    if (size > BAUL_ICON_SIZE_LARGER)
    {
        return BAUL_ICON_SIZE_LARGER;
    }
    if (size > BAUL_ICON_SIZE_LARGE)
    {
        return BAUL_ICON_SIZE_LARGE;
    }
    if (size > BAUL_ICON_SIZE_STANDARD)
    {
        return BAUL_ICON_SIZE_STANDARD;
    }
    if (size > BAUL_ICON_SIZE_SMALL)
    {
        return BAUL_ICON_SIZE_SMALL;
    }
    if (size > BAUL_ICON_SIZE_SMALLER)
    {
        return BAUL_ICON_SIZE_SMALLER;
    }
    return BAUL_ICON_SIZE_SMALLEST;
}

gint
baul_get_icon_size_for_stock_size (CtkIconSize size)
{
    gint w, h;

    if (ctk_icon_size_lookup (size, &w, &h))
    {
        return MAX (w, h);
    }
    return BAUL_ZOOM_LEVEL_STANDARD;
}


guint
baul_icon_get_emblem_size_for_icon_size (guint size)
{
    if (size >= 96)
        return 48;
    if (size >= 64)
        return 32;
    if (size >= 48)
        return 24;
    if (size >= 24)
        return 16;
    if (size >= 16)
        return 12;

    return 0; /* no emblems for smaller sizes */
}

gboolean
baul_icon_theme_can_render (GThemedIcon *icon)
{
	CtkIconTheme *icon_theme;
	const gchar * const *names;
	gint idx;

	names = g_themed_icon_get_names (icon);

	icon_theme = ctk_icon_theme_get_default ();

	for (idx = 0; names[idx] != NULL; idx++) {
		if (ctk_icon_theme_has_icon (icon_theme, names[idx])) {
			return TRUE;
		}
	}

	return FALSE;
}

GIcon *
baul_user_special_directory_get_gicon (GUserDirectory directory)
{

	#define ICON_CASE(x) \
		case G_USER_DIRECTORY_ ## x:\
			return g_themed_icon_new (BAUL_ICON_FOLDER_ ## x);

	switch (directory) {

		ICON_CASE (DESKTOP);
		ICON_CASE (DOCUMENTS);
		ICON_CASE (DOWNLOAD);
		ICON_CASE (MUSIC);
		ICON_CASE (PICTURES);
		ICON_CASE (PUBLIC_SHARE);
		ICON_CASE (TEMPLATES);
		ICON_CASE (VIDEOS);

	default:
		return g_themed_icon_new ("folder");
	}

	#undef ICON_CASE
}
