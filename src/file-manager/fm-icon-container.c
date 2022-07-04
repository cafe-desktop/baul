/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* fm-icon-container.h - the container widget for file manager icons

   Copyright (C) 2002 Sun Microsystems, Inc.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Author: Michael Meeks <michael@ximian.com>
*/
#include <config.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gio/gio.h>

#include <eel/eel-glib-extensions.h>

#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-file-attributes.h>
#include <libbaul-private/baul-thumbnails.h>
#include <libbaul-private/baul-desktop-icon-file.h>

#include "fm-icon-container.h"

#define ICON_TEXT_ATTRIBUTES_NUM_ITEMS		3
#define ICON_TEXT_ATTRIBUTES_DEFAULT_TOKENS	"size,date_modified,type"

G_DEFINE_TYPE (FMIconContainer, fm_icon_container, BAUL_TYPE_ICON_CONTAINER);

static GQuark attribute_none_q;

static FMIconView *
get_icon_view (BaulIconContainer *container)
{
    /* Type unsafe comparison for performance */
    return ((FMIconContainer *)container)->view;
}

static BaulIconInfo *
fm_icon_container_get_icon_images (BaulIconContainer *container,
                                   BaulIconData      *data,
                                   int                    size,
                                   GList                **emblem_pixbufs,
                                   char                 **embedded_text,
                                   gboolean               for_drag_accept,
                                   gboolean               need_large_embeddded_text,
                                   gboolean              *embedded_text_needs_loading,
                                   gboolean              *has_window_open)
{
    FMIconView *icon_view;
    BaulFile *file;
    gboolean use_embedding;
    BaulFileIconFlags flags;
    guint emblem_size;
    gint scale;

    file = (BaulFile *) data;

    g_assert (BAUL_IS_FILE (file));
    icon_view = get_icon_view (container);
    g_return_val_if_fail (icon_view != NULL, NULL);

    use_embedding = FALSE;
    if (embedded_text)
    {
        *embedded_text = baul_file_peek_top_left_text (file, need_large_embeddded_text, embedded_text_needs_loading);
        use_embedding = *embedded_text != NULL;
    }

    if (emblem_pixbufs != NULL)
    {
        emblem_size = baul_icon_get_emblem_size_for_icon_size (size);
        /* don't return images larger than the actual icon size */
        emblem_size = MIN (emblem_size, size);

        if (emblem_size > 0)
        {
            char **emblems_to_ignore;

            emblems_to_ignore = fm_directory_view_get_emblem_names_to_exclude
                                (FM_DIRECTORY_VIEW (icon_view));
            *emblem_pixbufs = baul_file_get_emblem_pixbufs (file,
                              emblem_size,
                              FALSE,
                              emblems_to_ignore);
            g_strfreev (emblems_to_ignore);
        }
    }

    *has_window_open = baul_file_has_open_window (file);

    flags = BAUL_FILE_ICON_FLAGS_USE_MOUNT_ICON_AS_EMBLEM;
    if (!fm_icon_view_is_compact (icon_view) ||
            baul_icon_container_get_zoom_level (container) > BAUL_ZOOM_LEVEL_STANDARD)
    {
        flags |= BAUL_FILE_ICON_FLAGS_USE_THUMBNAILS;
        if (fm_icon_view_is_compact (icon_view))
        {
            flags |= BAUL_FILE_ICON_FLAGS_FORCE_THUMBNAIL_SIZE;
        }
    }

    if (use_embedding)
    {
        flags |= BAUL_FILE_ICON_FLAGS_EMBEDDING_TEXT;
    }
    if (for_drag_accept)
    {
        flags |= BAUL_FILE_ICON_FLAGS_FOR_DRAG_ACCEPT;
    }

    scale = gtk_widget_get_scale_factor (GTK_WIDGET (icon_view));

    return baul_file_get_icon (file, size, scale, flags);
}

static char *
fm_icon_container_get_icon_description (BaulIconContainer *container,
                                        BaulIconData      *data)
{
    BaulFile *file;
    char *mime_type;
    const char *description;

    file = BAUL_FILE (data);
    g_assert (BAUL_IS_FILE (file));

    if (BAUL_IS_DESKTOP_ICON_FILE (file))
    {
        return NULL;
    }

    mime_type = baul_file_get_mime_type (file);
    description = g_content_type_get_description (mime_type);
    g_free (mime_type);
    return g_strdup (description);
}

static void
fm_icon_container_start_monitor_top_left (BaulIconContainer *container,
        BaulIconData      *data,
        gconstpointer          client,
        gboolean               large_text)
{
    BaulFile *file;
    BaulFileAttributes attributes;

    file = (BaulFile *) data;

    g_assert (BAUL_IS_FILE (file));

    attributes = BAUL_FILE_ATTRIBUTE_TOP_LEFT_TEXT;
    if (large_text)
    {
        attributes |= BAUL_FILE_ATTRIBUTE_LARGE_TOP_LEFT_TEXT;
    }
    baul_file_monitor_add (file, client, attributes);
}

static void
fm_icon_container_stop_monitor_top_left (BaulIconContainer *container,
        BaulIconData      *data,
        gconstpointer          client)
{
    BaulFile *file;

    file = (BaulFile *) data;

    g_assert (BAUL_IS_FILE (file));

    baul_file_monitor_remove (file, client);
}

static void
fm_icon_container_prioritize_thumbnailing (BaulIconContainer *container,
        BaulIconData      *data)
{
    BaulFile *file;

    file = (BaulFile *) data;

    g_assert (BAUL_IS_FILE (file));

    if (baul_file_is_thumbnailing (file))
    {
        char *uri;

        uri = baul_file_get_uri (file);
        baul_thumbnail_prioritize (uri);
        g_free (uri);
    }
}

/*
 * Get the preference for which caption text should appear
 * beneath icons.
 */
static GQuark *
fm_icon_container_get_icon_text_attributes_from_preferences (void)
{
    static GQuark *attributes = NULL;

    if (attributes == NULL)
    {
        eel_g_settings_add_auto_strv_as_quarks (baul_icon_view_preferences,
                                                BAUL_PREFERENCES_ICON_VIEW_CAPTIONS,
                                                &attributes);
    }

    /* We don't need to sanity check the attributes list even though it came
     * from preferences.
     *
     * There are 2 ways that the values in the list could be bad.
     *
     * 1) The user picks "bad" values.  "bad" values are those that result in
     *    there being duplicate attributes in the list.
     *
     * 2) Value stored in MateConf are tampered with.  Its possible physically do
     *    this by pulling the rug underneath MateConf and manually editing its
     *    config files.  Its also possible to use a third party MateConf key
     *    editor and store garbage for the keys in question.
     *
     * Thankfully, the Baul preferences machinery deals with both of
     * these cases.
     *
     * In the first case, the preferences dialog widgetry prevents
     * duplicate attributes by making "bad" choices insensitive.
     *
     * In the second case, the preferences getter (and also the auto storage) for
     * string_array values are always valid members of the enumeration associated
     * with the preference.
     *
     * So, no more error checking on attributes is needed here and we can return
     * a the auto stored value.
     */
    return attributes;
}

static int
quarkv_length (GQuark *attributes)
{
    int i;
    i = 0;
    while (attributes[i] != 0)
    {
        i++;
    }
    return i;
}

/**
 * fm_icon_view_get_icon_text_attribute_names:
 *
 * Get a list representing which text attributes should be displayed
 * beneath an icon. The result is dependent on zoom level and possibly
 * user configuration. Don't free the result.
 * @view: FMIconView to query.
 *
 **/
static GQuark *
fm_icon_container_get_icon_text_attribute_names (BaulIconContainer *container,
        int *len)
{
    GQuark *attributes;
    int piece_count;

    const int pieces_by_level[] =
    {
        0,	/* BAUL_ZOOM_LEVEL_SMALLEST */
        0,	/* BAUL_ZOOM_LEVEL_SMALLER */
        0,	/* BAUL_ZOOM_LEVEL_SMALL */
        1,	/* BAUL_ZOOM_LEVEL_STANDARD */
        2,	/* BAUL_ZOOM_LEVEL_LARGE */
        2,	/* BAUL_ZOOM_LEVEL_LARGER */
        3	/* BAUL_ZOOM_LEVEL_LARGEST */
    };

    piece_count = pieces_by_level[baul_icon_container_get_zoom_level (container)];

    attributes = fm_icon_container_get_icon_text_attributes_from_preferences ();

    *len = MIN (piece_count, quarkv_length (attributes));

    return attributes;
}

/* This callback returns the text, both the editable part, and the
 * part below that is not editable.
 */
static void
fm_icon_container_get_icon_text (BaulIconContainer *container,
                                 BaulIconData      *data,
                                 char                 **editable_text,
                                 char                 **additional_text,
                                 gboolean               include_invisible)
{
    GQuark *attributes;
    char *text_array[4];
    int i, j, num_attributes;
    FMIconView *icon_view;
    BaulFile *file;
    gboolean use_additional;

    file = BAUL_FILE (data);

    g_assert (BAUL_IS_FILE (file));
    g_assert (editable_text != NULL);
    icon_view = get_icon_view (container);
    g_return_if_fail (icon_view != NULL);

    use_additional = (additional_text != NULL);

    /* In the smallest zoom mode, no text is drawn. */
    if (baul_icon_container_get_zoom_level (container) == BAUL_ZOOM_LEVEL_SMALLEST &&
            !include_invisible)
    {
        *editable_text = NULL;
    }
    else
    {
        /* Strip the suffix for baul object xml files. */
        *editable_text = baul_file_get_display_name (file);
    }

    if (!use_additional)
    {
        return;
    }

    if (fm_icon_view_is_compact (icon_view))
    {
        *additional_text = NULL;
        return;
    }

    if (BAUL_IS_DESKTOP_ICON_FILE (file))
    {
        /* Don't show the normal extra information for desktop icons, it doesn't
         * make sense. */
        *additional_text = NULL;
        return;
    }

    /* Handle link files specially. */
    if (baul_file_is_baul_link (file))
    {
        /* FIXME bugzilla.gnome.org 42531: Does sync. I/O and works only locally. */
        *additional_text = NULL;
        if (baul_file_is_local (file))
        {
            char *actual_uri;
            gchar *description;

            actual_uri = baul_file_get_uri (file);
            description = baul_link_local_get_additional_text (actual_uri);

            if (description)
                *additional_text = g_strdup_printf (" \n%s\n ", description);

            g_free (description);
            g_free (actual_uri);
        }
        /* Don't show the normal extra information for desktop files, it doesn't
         * make sense. */
        return;
    }

    /* Find out what attributes go below each icon. */
    attributes = fm_icon_container_get_icon_text_attribute_names (container,
                 &num_attributes);

    /* Get the attributes. */
    j = 0;
    for (i = 0; i < num_attributes; ++i)
    {
        if (attributes[i] == attribute_none_q)
        {
            continue;
        }

        text_array[j++] =
            baul_file_get_string_attribute_with_default_q (file, attributes[i]);
    }
    text_array[j] = NULL;

    /* Return them. */
    if (j == 0)
    {
        *additional_text = NULL;
    }
    else if (j == 1)
    {
        /* Only one item, avoid the strdup + free */
        *additional_text = text_array[0];
    }
    else
    {
        *additional_text = g_strjoinv ("\n", text_array);

        for (i = 0; i < j; i++)
        {
            g_free (text_array[i]);
        }
    }
}

/* Sort as follows:
 *   0) computer link
 *   1) home link
 *   2) network link
 *   3) mount links
 *   4) other
 *   5) trash link
 */
typedef enum
{
    SORT_COMPUTER_LINK,
    SORT_HOME_LINK,
    SORT_NETWORK_LINK,
    SORT_MOUNT_LINK,
    SORT_OTHER,
    SORT_TRASH_LINK
} SortCategory;

static SortCategory
get_sort_category (BaulFile *file)
{
    SortCategory category;

    category = SORT_OTHER;

    if (BAUL_IS_DESKTOP_ICON_FILE (file))
    {
        BaulDesktopLink *link;

        link = baul_desktop_icon_file_get_link (BAUL_DESKTOP_ICON_FILE (file));

        if (link != NULL)
        {
            switch (baul_desktop_link_get_link_type (link))
            {
            case BAUL_DESKTOP_LINK_COMPUTER:
                category = SORT_COMPUTER_LINK;
                break;
            case BAUL_DESKTOP_LINK_HOME:
                category = SORT_HOME_LINK;
                break;
            case BAUL_DESKTOP_LINK_MOUNT:
                category = SORT_MOUNT_LINK;
                break;
            case BAUL_DESKTOP_LINK_TRASH:
                category = SORT_TRASH_LINK;
                break;
            case BAUL_DESKTOP_LINK_NETWORK:
                category = SORT_NETWORK_LINK;
                break;
            default:
                category = SORT_OTHER;
                break;
            }
            g_object_unref (link);
        }
    }

    return category;
}

static int
fm_desktop_icon_container_icons_compare (BaulIconContainer *container,
        BaulIconData      *data_a,
        BaulIconData      *data_b)
{
    BaulFile *file_a;
    BaulFile *file_b;
    FMDirectoryView *directory_view;
    SortCategory category_a, category_b;

    file_a = (BaulFile *) data_a;
    file_b = (BaulFile *) data_b;

    directory_view = FM_DIRECTORY_VIEW (FM_ICON_CONTAINER (container)->view);
    g_return_val_if_fail (directory_view != NULL, 0);

    category_a = get_sort_category (file_a);
    category_b = get_sort_category (file_b);

    if (category_a == category_b)
    {
        return baul_file_compare_for_sort
               (file_a, file_b, BAUL_FILE_SORT_BY_DISPLAY_NAME,
                fm_directory_view_should_sort_directories_first (directory_view),
                FALSE);
    }

    if (category_a < category_b)
    {
        return -1;
    }
    else
    {
        return +1;
    }
}

static int
fm_icon_container_compare_icons (BaulIconContainer *container,
                                 BaulIconData      *icon_a,
                                 BaulIconData      *icon_b)
{
    FMIconView *icon_view;

    icon_view = get_icon_view (container);
    g_return_val_if_fail (icon_view != NULL, 0);

    if (FM_ICON_CONTAINER (container)->sort_for_desktop)
    {
        return fm_desktop_icon_container_icons_compare
               (container, icon_a, icon_b);
    }

    /* Type unsafe comparisons for performance */
    return fm_icon_view_compare_files (icon_view,
                                       (BaulFile *)icon_a,
                                       (BaulFile *)icon_b);
}

static int
fm_icon_container_compare_icons_by_name (BaulIconContainer *container,
        BaulIconData      *icon_a,
        BaulIconData      *icon_b)
{
    return baul_file_compare_for_sort
           (BAUL_FILE (icon_a),
            BAUL_FILE (icon_b),
            BAUL_FILE_SORT_BY_DISPLAY_NAME,
            FALSE, FALSE);
}

static void
fm_icon_container_freeze_updates (BaulIconContainer *container)
{
    FMIconView *icon_view;
    icon_view = get_icon_view (container);
    g_return_if_fail (icon_view != NULL);
    fm_directory_view_freeze_updates (FM_DIRECTORY_VIEW (icon_view));
}

static void
fm_icon_container_unfreeze_updates (BaulIconContainer *container)
{
    FMIconView *icon_view;
    icon_view = get_icon_view (container);
    g_return_if_fail (icon_view != NULL);
    fm_directory_view_unfreeze_updates (FM_DIRECTORY_VIEW (icon_view));
}

static void
fm_icon_container_dispose (GObject *object)
{
    FMIconContainer *icon_container;

    icon_container = FM_ICON_CONTAINER (object);

    icon_container->view = NULL;

    G_OBJECT_CLASS (fm_icon_container_parent_class)->dispose (object);
}

static void
fm_icon_container_class_init (FMIconContainerClass *klass)
{
    BaulIconContainerClass *ic_class;

    ic_class = &klass->parent_class;

    attribute_none_q = g_quark_from_static_string ("none");

    ic_class->get_icon_text = fm_icon_container_get_icon_text;
    ic_class->get_icon_images = fm_icon_container_get_icon_images;
    ic_class->get_icon_description = fm_icon_container_get_icon_description;
    ic_class->start_monitor_top_left = fm_icon_container_start_monitor_top_left;
    ic_class->stop_monitor_top_left = fm_icon_container_stop_monitor_top_left;
    ic_class->prioritize_thumbnailing = fm_icon_container_prioritize_thumbnailing;

    ic_class->compare_icons = fm_icon_container_compare_icons;
    ic_class->compare_icons_by_name = fm_icon_container_compare_icons_by_name;
    ic_class->freeze_updates = fm_icon_container_freeze_updates;
    ic_class->unfreeze_updates = fm_icon_container_unfreeze_updates;

    G_OBJECT_CLASS (klass)->dispose = fm_icon_container_dispose;
}

static void
fm_icon_container_init (FMIconContainer *icon_container)
{
    gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (icon_container)),
                                 GTK_STYLE_CLASS_VIEW);
}

BaulIconContainer *
fm_icon_container_construct (FMIconContainer *icon_container, FMIconView *view)
{
    AtkObject *atk_obj;

    g_return_val_if_fail (FM_IS_ICON_VIEW (view), NULL);

    icon_container->view = view;
    atk_obj = gtk_widget_get_accessible (GTK_WIDGET (icon_container));
    atk_object_set_name (atk_obj, _("Icon View"));

    return BAUL_ICON_CONTAINER (icon_container);
}

BaulIconContainer *
fm_icon_container_new (FMIconView *view)
{
    return fm_icon_container_construct
           (g_object_new (FM_TYPE_ICON_CONTAINER, NULL),
            view);
}

void
fm_icon_container_set_sort_desktop (FMIconContainer *container,
                                    gboolean         desktop)
{
    container->sort_for_desktop = desktop;
}
