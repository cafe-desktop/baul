/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2000, 2001 Eazel, Inc.
 *
 * Baul is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Andy Hertzfeld <andy@eazel.com>
 */

/* This is the sidebar title widget, which is the title part of the sidebar. */

#include <config.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <ctk/ctk.h>
#include <pango/pango.h>
#include <glib/gi18n.h>

#include <eel/eel-background.h>
#include <eel/eel-cdk-extensions.h>
#include <eel/eel-gdk-pixbuf-extensions.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-ctk-extensions.h>

#include <libbaul-private/baul-file-attributes.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-metadata.h>
#include <libbaul-private/baul-sidebar.h>

#include "baul-sidebar-title.h"
#include "baul-window.h"

/* maximum allowable size to be displayed as the title */
#define MAX_TITLE_SIZE 		256
#define MINIMUM_INFO_WIDTH	32
#define SIDEBAR_INFO_MARGIN	4

#define MORE_INFO_FONT_SIZE 	 12
#define MIN_TITLE_FONT_SIZE 	 12
#define TITLE_PADDING		  4

#define DEFAULT_LIGHT_INFO_COLOR "#FFFFFF"
#define DEFAULT_DARK_INFO_COLOR  "#2A2A2A"

static void                baul_sidebar_title_size_allocate     (CtkWidget             *widget,
        							 CtkAllocation         *allocation);
static void                update_icon                          (BaulSidebarTitle      *sidebar_title);
static CtkWidget *         sidebar_title_create_title_label     (void);
static CtkWidget *         sidebar_title_create_more_info_label (void);
static void		   update_all 				(BaulSidebarTitle      *sidebar_title);
static void		   update_more_info			(BaulSidebarTitle      *sidebar_title);
static void		   update_title_font			(BaulSidebarTitle      *sidebar_title);
static void                style_updated                        (CtkWidget             *widget);
static guint		   get_best_icon_size 			(BaulSidebarTitle      *sidebar_title);

enum
{
    LABEL_COLOR,
    LABEL_COLOR_HIGHLIGHT,
    LABEL_COLOR_ACTIVE,
    LABEL_COLOR_PRELIGHT,
    LABEL_INFO_COLOR,
    LABEL_INFO_COLOR_HIGHLIGHT,
    LABEL_INFO_COLOR_ACTIVE,
    LAST_LABEL_COLOR
};

struct _BaulSidebarTitlePrivate
{
    BaulFile		*file;
    guint		 file_changed_connection;
    gboolean		 monitoring_count;

    char		*title_text;
    CtkWidget		*icon;
    CtkWidget		*title_label;
    CtkWidget		*more_info_label;
    CtkWidget		*emblem_box;

    CdkRGBA		 label_colors [LAST_LABEL_COLOR];
    guint		 best_icon_size;
    gboolean		 determined_icon;
};

G_DEFINE_TYPE_WITH_PRIVATE (BaulSidebarTitle, baul_sidebar_title, CTK_TYPE_BOX)

static void
style_updated (CtkWidget *widget)
{
    BaulSidebarTitle *sidebar_title;

    g_return_if_fail (BAUL_IS_SIDEBAR_TITLE (widget));

    sidebar_title = BAUL_SIDEBAR_TITLE (widget);

    /* Update the dynamically-sized title font */
    update_title_font (sidebar_title);

}

static void
baul_sidebar_title_init (BaulSidebarTitle *sidebar_title)
{
    sidebar_title->details = baul_sidebar_title_get_instance_private (sidebar_title);

    ctk_orientable_set_orientation (CTK_ORIENTABLE (sidebar_title), CTK_ORIENTATION_VERTICAL);

    /* Create the icon */
    sidebar_title->details->icon = ctk_image_new ();
    ctk_box_pack_start (CTK_BOX (sidebar_title), sidebar_title->details->icon, 0, 0, 0);
    ctk_widget_show (sidebar_title->details->icon);

    /* Create the title label */
    sidebar_title->details->title_label = sidebar_title_create_title_label ();
    ctk_box_pack_start (CTK_BOX (sidebar_title), sidebar_title->details->title_label, 0, 0, 0);
    ctk_widget_show (sidebar_title->details->title_label);

    /* Create the more info label */
    sidebar_title->details->more_info_label = sidebar_title_create_more_info_label ();
    ctk_box_pack_start (CTK_BOX (sidebar_title), sidebar_title->details->more_info_label, 0, 0, 0);
    ctk_widget_show (sidebar_title->details->more_info_label);

    sidebar_title->details->emblem_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
    ctk_widget_show (sidebar_title->details->emblem_box);
    ctk_box_pack_start (CTK_BOX (sidebar_title), sidebar_title->details->emblem_box, 0, 0, 0);

    sidebar_title->details->best_icon_size = get_best_icon_size (sidebar_title);
    /* Keep track of changes in graphics trade offs */
    update_all (sidebar_title);

    /* initialize the label colors & fonts */
    style_updated (CTK_WIDGET (sidebar_title));

    g_signal_connect_swapped (baul_preferences,
                              "changed::" BAUL_PREFERENCES_SHOW_DIRECTORY_ITEM_COUNTS,
                              G_CALLBACK(update_more_info),
                              sidebar_title);
}

/* destroy by throwing away private storage */
static void
release_file (BaulSidebarTitle *sidebar_title)
{
    if (sidebar_title->details->file_changed_connection != 0)
    {
        if (g_signal_handler_is_connected(G_OBJECT (sidebar_title->details->file),
							  sidebar_title->details->file_changed_connection)){
             g_signal_handler_disconnect (sidebar_title->details->file,
                                     sidebar_title->details->file_changed_connection);
        }
        sidebar_title->details->file_changed_connection = 0;
    }

    if (sidebar_title->details->file != NULL)
    {
        baul_file_monitor_remove (sidebar_title->details->file, sidebar_title);
        baul_file_unref (sidebar_title->details->file);
        sidebar_title->details->file = NULL;
    }
}

static void
baul_sidebar_title_finalize (GObject *object)
{
    BaulSidebarTitle *sidebar_title;

    sidebar_title = BAUL_SIDEBAR_TITLE (object);

    if (sidebar_title->details)
    {
        release_file (sidebar_title);

        g_free (sidebar_title->details->title_text);
    }

    g_signal_handlers_disconnect_by_func (baul_preferences,
                                          update_more_info, sidebar_title);

    G_OBJECT_CLASS (baul_sidebar_title_parent_class)->finalize (object);
}

static void
baul_sidebar_title_class_init (BaulSidebarTitleClass *klass)
{
    CtkWidgetClass *widget_class;

    G_OBJECT_CLASS (klass)->finalize = baul_sidebar_title_finalize;

    widget_class = CTK_WIDGET_CLASS (klass);
    widget_class->size_allocate = baul_sidebar_title_size_allocate;
    widget_class->style_updated = style_updated;

    ctk_widget_class_install_style_property (widget_class,
            g_param_spec_boxed ("light_info_rgba",
                                "Light Info RGBA",
                                "Color used for information text against a dark background",
                                CDK_TYPE_RGBA,
                                G_PARAM_READABLE));

    ctk_widget_class_install_style_property (widget_class,
            g_param_spec_boxed ("dark_info_rgba",
                                "Dark Info RGBA",
                                "Color used for information text against a light background",
                                CDK_TYPE_RGBA,
                                G_PARAM_READABLE));
}

/* return a new index title object */
CtkWidget *
baul_sidebar_title_new (void)
{
    return ctk_widget_new (baul_sidebar_title_get_type (), NULL);
}

static void
setup_gc_with_fg (BaulSidebarTitle *sidebar_title, int idx, CdkRGBA *color)
{
    sidebar_title->details->label_colors[idx] = *color;
}

void
baul_sidebar_title_select_text_color (BaulSidebarTitle *sidebar_title,
                                      EelBackground    *background)
{
    CdkRGBA *light_info_color, *dark_info_color;
    CtkStyleContext *style;
    CdkRGBA color;
    CdkRGBA *c;

    g_assert (BAUL_IS_SIDEBAR_TITLE (sidebar_title));
    g_return_if_fail (ctk_widget_get_realized (CTK_WIDGET (sidebar_title)));

    /* read the info colors from the current theme; use a reasonable default if undefined */
    style = ctk_widget_get_style_context (CTK_WIDGET (sidebar_title));
    ctk_style_context_get_style (style,
                                 "light_info_color", &light_info_color,
                                 "dark_info_color", &dark_info_color,
                                 NULL);

    if (!light_info_color)
    {
        light_info_color = g_malloc (sizeof (CdkRGBA));
        cdk_rgba_parse (light_info_color, DEFAULT_LIGHT_INFO_COLOR);
    }

    if (!dark_info_color)
    {
        dark_info_color = g_malloc (sizeof (CdkRGBA));
        cdk_rgba_parse (dark_info_color, DEFAULT_DARK_INFO_COLOR);
    }

    ctk_style_context_get_color (style, CTK_STATE_FLAG_SELECTED, &color);
    setup_gc_with_fg (sidebar_title, LABEL_COLOR_HIGHLIGHT, &color);

    ctk_style_context_get_color (style, CTK_STATE_FLAG_ACTIVE, &color);
    setup_gc_with_fg (sidebar_title, LABEL_COLOR_ACTIVE, &color);

    ctk_style_context_get_color (style, CTK_STATE_FLAG_PRELIGHT, &color);
    setup_gc_with_fg (sidebar_title, LABEL_COLOR_PRELIGHT, &color);

    ctk_style_context_get (style, CTK_STATE_FLAG_SELECTED,
                           CTK_STYLE_PROPERTY_BACKGROUND_COLOR,
                           &c, NULL);
    color = *c;

    setup_gc_with_fg (sidebar_title, LABEL_INFO_COLOR_HIGHLIGHT,
                      eel_cdk_rgba_is_dark (&color) ? light_info_color : dark_info_color);

    ctk_style_context_get (style, CTK_STATE_FLAG_ACTIVE,
                           CTK_STYLE_PROPERTY_BACKGROUND_COLOR,
                           &c, NULL);
    color = *c;

    setup_gc_with_fg (sidebar_title, LABEL_INFO_COLOR_ACTIVE,
                      eel_cdk_rgba_is_dark (&color) ? light_info_color : dark_info_color);


    /* If EelBackground is not set in the widget, we can safely
     * use the foreground color from the theme, because it will
     * always be displayed against the ctk background */
    if (!eel_background_is_set(background))
    {
        ctk_style_context_get_color (style, CTK_STATE_FLAG_NORMAL, &color);
        setup_gc_with_fg (sidebar_title, LABEL_COLOR, &color);

        ctk_style_context_get (style, CTK_STATE_FLAG_NORMAL,
                               CTK_STYLE_PROPERTY_BACKGROUND_COLOR,
                               &color, NULL);
        color = *c;

        setup_gc_with_fg (sidebar_title, LABEL_INFO_COLOR,
                          eel_cdk_rgba_is_dark (&color) ?
                          light_info_color : dark_info_color);
    }
    else if (eel_background_is_dark (background))
    {
        CdkRGBA tmp;

        cdk_rgba_parse (&tmp, "EFEFEF");
        setup_gc_with_fg (sidebar_title, LABEL_COLOR, &tmp);
        setup_gc_with_fg (sidebar_title, LABEL_INFO_COLOR, light_info_color);
    }
    else     /* converse */
    {
        CdkRGBA tmp;

        cdk_rgba_parse (&tmp, "000000");
        setup_gc_with_fg (sidebar_title, LABEL_COLOR, &tmp);
        setup_gc_with_fg (sidebar_title, LABEL_INFO_COLOR, dark_info_color);
    }

    cdk_rgba_free (c);
    cdk_rgba_free (dark_info_color);
    cdk_rgba_free (light_info_color);
}

static char*
get_property_from_component (BaulSidebarTitle *sidebar_title G_GNUC_UNUSED,
			     const char       *property G_GNUC_UNUSED)
{
    /* There used to be a way to get icon and summary_text from main view,
     *  but its not used right now, so this sas stubbed out for now
     */
    return NULL;
}

static guint
get_best_icon_size (BaulSidebarTitle *sidebar_title)
{
    gint width;
    CtkAllocation allocation;

    ctk_widget_get_allocation (CTK_WIDGET (sidebar_title), &allocation);
    width = allocation.width - TITLE_PADDING;

    if (width < 0)
    {
        /* use smallest available icon size */
        return baul_icon_get_smaller_icon_size (0);
    }
    else
    {
        return baul_icon_get_smaller_icon_size ((guint) width);
    }
}

/* set up the icon image */
static void
update_icon (BaulSidebarTitle *sidebar_title)
{
    cairo_surface_t *surface;
    char *icon_name;
    gboolean leave_surface_unchanged;
    gint icon_scale;

    leave_surface_unchanged = FALSE;

    /* see if the current content view is specifying an icon */
    icon_name = get_property_from_component (sidebar_title, "icon_name");
    icon_scale = ctk_widget_get_scale_factor (CTK_WIDGET (sidebar_title));

    surface = NULL;

    if (icon_name != NULL && icon_name[0] != '\0')
    {
        BaulIconInfo *info;

        info = baul_icon_info_lookup_from_name (icon_name, BAUL_ICON_SIZE_LARGE, icon_scale);
        surface = baul_icon_info_get_surface_at_size (info, BAUL_ICON_SIZE_LARGE);
        g_object_unref (info);
    }
    else if (sidebar_title->details->file != NULL &&
             baul_file_check_if_ready (sidebar_title->details->file,
                                       BAUL_FILE_ATTRIBUTES_FOR_ICON))
    {
        surface = baul_file_get_icon_surface (sidebar_title->details->file,
                                              sidebar_title->details->best_icon_size * icon_scale,
                                              TRUE,
                                              icon_scale,
                                              BAUL_FILE_ICON_FLAGS_USE_THUMBNAILS |
                                              BAUL_FILE_ICON_FLAGS_USE_MOUNT_ICON_AS_EMBLEM);
    }
    else if (sidebar_title->details->determined_icon)
    {
        /* We used to know the icon for this file, but now the file says it isn't
         * ready. This means that some file info has been invalidated, which
         * doesn't necessarily mean that the previously-determined icon is
         * wrong (in fact, in practice it usually doesn't mean that). Keep showing
         * the one we last determined for now.
         */
        leave_surface_unchanged = TRUE;
    }

    g_free (icon_name);

    if (!leave_surface_unchanged)
    {
        ctk_image_set_from_surface (CTK_IMAGE (sidebar_title->details->icon), surface);
    }

    if (surface != NULL)
    {
        sidebar_title->details->determined_icon = TRUE;
        cairo_surface_destroy (surface);
    }
}

static void
override_title_font (CtkWidget   *widget,
                     const gchar *font)
{
    gchar          *css;
    CtkCssProvider *provider;
    gchar          *tempsize;

    provider = ctk_css_provider_new ();
    tempsize = g_strdup (font);

    g_strreverse (tempsize);
    g_strcanon (tempsize, "1234567890", '\0');
    g_strreverse (tempsize);

    gchar tempfont [strlen (font)];
    g_strlcpy (tempfont, font, sizeof (tempfont));
    tempfont [strlen (font) - strlen (tempsize)] = 0;

    css = g_strdup_printf ("label { font-family: %s; font-size: %spt; }", tempfont, tempsize);
    ctk_css_provider_load_from_data (provider, css, -1, NULL);
    g_free (css);

    ctk_style_context_add_provider (ctk_widget_get_style_context (widget),
                                    CTK_STYLE_PROVIDER (provider),
                                    CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (provider);
}

static void
update_title_font (BaulSidebarTitle *sidebar_title)
{
    int available_width, width;
    int max_fit_font_size, max_style_font_size;
    CtkStyleContext *context;
    CtkStateFlags    state;
    CtkAllocation allocation;
    PangoFontDescription *title_font, *tmp_font;
    PangoLayout *layout;

    /* Make sure theres work to do */
    if (sidebar_title->details->title_text == NULL
        || strlen (sidebar_title->details->title_text) < 1)
    {
        return;
    }

    ctk_widget_get_allocation (CTK_WIDGET (sidebar_title), &allocation);
    available_width = allocation.width - TITLE_PADDING;

    /* No work to do */
    if (available_width <= 0)
    {
        return;
    }

    context = ctk_widget_get_style_context (CTK_WIDGET (sidebar_title));
    state = ctk_widget_get_state_flags (CTK_WIDGET (sidebar_title));
    ctk_style_context_get (context, state, CTK_STYLE_PROPERTY_FONT, &title_font, NULL);
    max_style_font_size = pango_font_description_get_size (title_font) * 1.8 / PANGO_SCALE;
    if (max_style_font_size < MIN_TITLE_FONT_SIZE + 1)
    {
        max_style_font_size = MIN_TITLE_FONT_SIZE + 1;
    }

    /* Calculate largest-fitting font size */
    layout = pango_layout_new (ctk_widget_get_pango_context (sidebar_title->details->title_label));
    pango_layout_set_text (layout, sidebar_title->details->title_text, -1);
    pango_layout_set_font_description (layout, title_font);
    tmp_font = pango_font_description_new ();

    max_fit_font_size = max_style_font_size;
    for (; max_fit_font_size >= MIN_TITLE_FONT_SIZE; max_fit_font_size--)
    {
        pango_font_description_set_size (tmp_font, max_fit_font_size * PANGO_SCALE);
        pango_layout_set_font_description (layout, tmp_font);
        pango_layout_get_pixel_size (layout, &width, NULL);

        if (width <= available_width)
            break;
    }

    pango_font_description_free (tmp_font);
    g_object_unref (layout);

    pango_font_description_set_size (title_font, max_fit_font_size * PANGO_SCALE);
    pango_font_description_set_weight (title_font, PANGO_WEIGHT_BOLD);

    override_title_font (sidebar_title->details->title_label, pango_font_description_to_string (title_font));

    pango_font_description_free (title_font);
}

static void
update_title (BaulSidebarTitle *sidebar_title)
{
    CtkLabel *label;
    const char *text;

    label = CTK_LABEL (sidebar_title->details->title_label);
    text = sidebar_title->details->title_text;

    if (g_strcmp0 (text, ctk_label_get_text (label)) == 0)
    {
        return;
    }
    ctk_label_set_text (label, text);
    update_title_font (sidebar_title);
}

static void
append_and_eat (GString *string, const char *separator, char *new_string)
{
    if (new_string == NULL)
    {
        return;
    }
    if (separator != NULL)
    {
        g_string_append (string, separator);
    }
    g_string_append (string, new_string);
    g_free (new_string);
}

static int
measure_width_callback (const char *string, gpointer callback_data)
{
    PangoLayout *layout;
    int width;

    layout = PANGO_LAYOUT (callback_data);
    pango_layout_set_text (layout, string, -1);
    pango_layout_get_pixel_size (layout, &width, NULL);
    return width;
}

static void
update_more_info (BaulSidebarTitle *sidebar_title)
{
    BaulFile *file;
    GString *info_string;
    char *component_info;
    CtkAllocation allocation;

    file = sidebar_title->details->file;

    /* allow components to specify the info if they wish to */
    component_info = get_property_from_component (sidebar_title, "summary_info");
    if (component_info != NULL && strlen (component_info) > 0)
    {
        info_string = g_string_new (component_info);
        g_free (component_info);
    }
    else
    {
        char *type_string;
        int sidebar_width;

        info_string = g_string_new (NULL);

        type_string = NULL;

        if (file != NULL && baul_file_should_show_type (file))
        {
            type_string = baul_file_get_string_attribute (file, "type");
        }

        if (type_string != NULL)
        {
            append_and_eat (info_string, NULL, type_string);
            append_and_eat (info_string, ", ",
                            baul_file_get_string_attribute (file, "size"));
        }
        else
        {
            append_and_eat (info_string, NULL,
                            baul_file_get_string_attribute (file, "size"));
        }

        ctk_widget_get_allocation (CTK_WIDGET (sidebar_title), &allocation);
        sidebar_width = allocation.width - 2 * SIDEBAR_INFO_MARGIN;
        if (sidebar_width > MINIMUM_INFO_WIDTH)
        {
            char *date_modified_str;
            PangoLayout *layout;

            layout = pango_layout_copy (ctk_label_get_layout (CTK_LABEL (sidebar_title->details->more_info_label)));
            pango_layout_set_width (layout, -1);
            date_modified_str = baul_file_fit_modified_date_as_string
                                (file, sidebar_width, measure_width_callback, NULL, layout);
            g_object_unref (layout);
            append_and_eat (info_string, "\n", date_modified_str);
        }
    }
    ctk_label_set_text (CTK_LABEL (sidebar_title->details->more_info_label),
                        info_string->str);

    g_string_free (info_string, TRUE);
}

/* add a pixbuf to the emblem box */
static void
add_emblem (BaulSidebarTitle *sidebar_title, GdkPixbuf *pixbuf)
{
    CtkWidget *image_widget;

    image_widget = ctk_image_new_from_pixbuf (pixbuf);
    ctk_widget_show (image_widget);
    ctk_container_add (CTK_CONTAINER (sidebar_title->details->emblem_box), image_widget);
}

static void
update_emblems (BaulSidebarTitle *sidebar_title)
{
    GList *pixbufs, *p;
    GdkPixbuf *pixbuf = NULL;

    /* exit if we don't have the file yet */
    if (sidebar_title->details->file == NULL)
    {
        return;
    }

    /* First, deallocate any existing ones */
    ctk_container_foreach (CTK_CONTAINER (sidebar_title->details->emblem_box),
                           (CtkCallback) ctk_widget_destroy,
                           NULL);

    /* fetch the emblem icons from metadata */
    pixbufs = baul_file_get_emblem_pixbufs (sidebar_title->details->file,
                                            baul_icon_get_emblem_size_for_icon_size (BAUL_ICON_SIZE_STANDARD),
                                            FALSE,
                                            NULL);

    /* loop through the list of emblems, installing them in the box */
    for (p = pixbufs; p != NULL; p = p->next)
    {
        pixbuf = p->data;
        add_emblem (sidebar_title, pixbuf);
        g_object_unref (pixbuf);
    }
    g_list_free (pixbufs);
}

/* return the filename text */
char *
baul_sidebar_title_get_text (BaulSidebarTitle *sidebar_title)
{
    return g_strdup (sidebar_title->details->title_text);
}

/* set up the filename text */
void
baul_sidebar_title_set_text (BaulSidebarTitle *sidebar_title,
                             const char* new_text)
{
    g_free (sidebar_title->details->title_text);

    /* truncate the title to a reasonable size */
    if (new_text && strlen (new_text) > MAX_TITLE_SIZE)
    {
        sidebar_title->details->title_text = g_strndup (new_text, MAX_TITLE_SIZE);
    }
    else
    {
        sidebar_title->details->title_text = g_strdup (new_text);
    }
    /* Recompute the displayed text. */
    update_title (sidebar_title);
}

static gboolean
item_count_ready (BaulSidebarTitle *sidebar_title)
{
    return sidebar_title->details->file != NULL
           && baul_file_get_directory_item_count
           (sidebar_title->details->file, NULL, NULL) != 0;
}

static void
monitor_add (BaulSidebarTitle *sidebar_title)
{
    BaulFileAttributes attributes;

    /* Monitor the things needed to get the right icon. Don't
     * monitor a directory's item count at first even though the
     * "size" attribute is based on that, because the main view
     * will get it for us in most cases, and in other cases it's
     * OK to not show the size -- if we did monitor it, we'd be in
     * a race with the main view and could cause it to have to
     * load twice. Once we have a size, though, we want to monitor
     * the size to guarantee it stays up to date.
     */

    sidebar_title->details->monitoring_count = item_count_ready (sidebar_title);

    attributes = BAUL_FILE_ATTRIBUTES_FOR_ICON | BAUL_FILE_ATTRIBUTE_INFO;
    if (sidebar_title->details->monitoring_count)
    {
        attributes |= BAUL_FILE_ATTRIBUTE_DIRECTORY_ITEM_COUNT;
    }

    baul_file_monitor_add (sidebar_title->details->file, sidebar_title, attributes);
}

static void
update_all (BaulSidebarTitle *sidebar_title)
{
    update_icon (sidebar_title);

    update_title (sidebar_title);
    update_more_info (sidebar_title);

    update_emblems (sidebar_title);

    /* Redo monitor once the count is ready. */
    if (!sidebar_title->details->monitoring_count && item_count_ready (sidebar_title))
    {
        baul_file_monitor_remove (sidebar_title->details->file, sidebar_title);
        monitor_add (sidebar_title);
    }
}

void
baul_sidebar_title_set_file (BaulSidebarTitle *sidebar_title,
                             BaulFile         *file,
                             const char           *initial_text)
{
    if (file != sidebar_title->details->file)
    {
        release_file (sidebar_title);
        sidebar_title->details->file = file;
        sidebar_title->details->determined_icon = FALSE;
        baul_file_ref (sidebar_title->details->file);

        /* attach file */
        if (file != NULL)
        {
            sidebar_title->details->file_changed_connection =
                g_signal_connect_object
                (sidebar_title->details->file, "changed",
                 G_CALLBACK (update_all), sidebar_title, G_CONNECT_SWAPPED);
            monitor_add (sidebar_title);
        }
    }

    g_free (sidebar_title->details->title_text);
    sidebar_title->details->title_text = g_strdup (initial_text);

    update_all (sidebar_title);
}

static void
baul_sidebar_title_size_allocate (CtkWidget *widget,
                                  CtkAllocation *allocation)
{
    BaulSidebarTitle *sidebar_title;
    guint16 old_width;
    guint best_icon_size;
    gint scale;
    CtkAllocation old_allocation, new_allocation;

    sidebar_title = BAUL_SIDEBAR_TITLE (widget);
    scale = ctk_widget_get_scale_factor (widget);

    ctk_widget_get_allocation (widget, &old_allocation);
    old_width = old_allocation.width;

    CTK_WIDGET_CLASS (baul_sidebar_title_parent_class)->size_allocate (widget, allocation);

    ctk_widget_get_allocation (widget, &new_allocation);

    if (old_width != new_allocation.width)
    {
        best_icon_size = get_best_icon_size (sidebar_title) / scale;
        if (best_icon_size != sidebar_title->details->best_icon_size)
        {
            sidebar_title->details->best_icon_size = best_icon_size;
            update_icon (sidebar_title);
        }

        /* update the title font and info format as the size changes. */
        update_title_font (sidebar_title);
        update_more_info (sidebar_title);
    }
}

gboolean
baul_sidebar_title_hit_test_icon (BaulSidebarTitle *sidebar_title, int x, int y)
{
    CtkAllocation *allocation;
    gboolean icon_hit;

    g_return_val_if_fail (BAUL_IS_SIDEBAR_TITLE (sidebar_title), FALSE);

    allocation = g_new0 (CtkAllocation, 1);
    ctk_widget_get_allocation (CTK_WIDGET (sidebar_title->details->icon), allocation);
    g_return_val_if_fail (allocation != NULL, FALSE);

    icon_hit = x >= allocation->x && y >= allocation->y
               && x < allocation->x + allocation->width
               && y < allocation->y + allocation->height;
    g_free (allocation);

    return icon_hit;
}

static CtkWidget *
sidebar_title_create_title_label (void)
{
    CtkWidget *title_label;

    title_label = ctk_label_new ("");
    eel_ctk_label_make_bold (CTK_LABEL (title_label));
    ctk_label_set_line_wrap (CTK_LABEL (title_label), TRUE);
    ctk_label_set_justify (CTK_LABEL (title_label), CTK_JUSTIFY_CENTER);
    ctk_label_set_selectable (CTK_LABEL (title_label), TRUE);
    ctk_label_set_ellipsize (CTK_LABEL (title_label), PANGO_ELLIPSIZE_END);

    return title_label;
}

static CtkWidget *
sidebar_title_create_more_info_label (void)
{
    CtkWidget *more_info_label;
    PangoAttrList *attrs;

    attrs = pango_attr_list_new ();
    pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_SMALL));

    more_info_label = ctk_label_new ("");

    ctk_label_set_attributes (CTK_LABEL (more_info_label), attrs);
    pango_attr_list_unref (attrs);

    ctk_label_set_justify (CTK_LABEL (more_info_label), CTK_JUSTIFY_CENTER);
    ctk_label_set_selectable (CTK_LABEL (more_info_label), TRUE);
    ctk_label_set_ellipsize (CTK_LABEL (more_info_label), PANGO_ELLIPSIZE_END);

    return more_info_label;
}
