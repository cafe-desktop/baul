/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Baul is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Baul is distributed in the hope that it will be useful,
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

/* This is the implementation of the property browser window, which
 * gives the user access to an extensible palette of properties which
 * can be dropped on various elements of the user interface to
 * customize them
 */

#include <config.h>
#include <math.h>

#include <libxml/parser.h>
#include <ctk/ctk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <atk/atkrelationset.h>

#include <eel/eel-cdk-extensions.h>
#include <eel/eel-gdk-pixbuf-extensions.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-ctk-extensions.h>
#include <eel/eel-image-table.h>
#include <eel/eel-labeled-image.h>
#include <eel/eel-stock-dialogs.h>
#include <eel/eel-vfs-extensions.h>
#include <eel/eel-xml-extensions.h>

#include <libbaul-private/baul-customization-data.h>
#include <libbaul-private/baul-directory.h>
#include <libbaul-private/baul-emblem-utils.h>
#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-file.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-metadata.h>
#include <libbaul-private/baul-signaller.h>

#include "baul-property-browser.h"

/* property types */

typedef enum
{
    BAUL_PROPERTY_NONE,
    BAUL_PROPERTY_PATTERN,
    BAUL_PROPERTY_COLOR,
    BAUL_PROPERTY_EMBLEM
} BaulPropertyType;

struct _BaulPropertyBrowserPrivate
{
    CtkWidget *container;

    CtkWidget *content_container;
    CtkWidget *content_frame;
    CtkWidget *content_table;

    CtkWidget *category_container;
    CtkWidget *category_box;

    CtkWidget *title_box;
    CtkWidget *title_label;
    CtkWidget *help_label;

    CtkWidget *bottom_box;

    CtkWidget *add_button;
    CtkWidget *add_button_image;
    CtkWidget *remove_button;
    CtkWidget *remove_button_image;

    CtkWidget *patterns_dialog;
    CtkWidget *colors_dialog;
    CtkWidget *emblems_dialog;

    CtkWidget *keyword;
    CtkWidget *emblem_image;
    CtkWidget *image_button;

    CtkWidget *color_picker;
    CtkWidget *color_name;

    GList *keywords;

    char *path;
    char *category;
    char *dragged_file;
    char *drag_type;
    char *image_path;
    char *filename;

    BaulPropertyType category_type;

    int category_position;

    CdkPixbuf *property_chit;

    gboolean remove_mode;
    gboolean keep_around;
    gboolean has_local;
};

static void     baul_property_browser_update_contents       (BaulPropertyBrowser       *property_browser);
static void     baul_property_browser_set_category          (BaulPropertyBrowser       *property_browser,
        const char                    *new_category);
static void     baul_property_browser_set_dragged_file      (BaulPropertyBrowser       *property_browser,
        const char                    *dragged_file_name);
static void     baul_property_browser_set_drag_type         (BaulPropertyBrowser       *property_browser,
        const char                    *new_drag_type);
static void     add_new_button_callback                         (CtkWidget                     *widget,
        BaulPropertyBrowser       *property_browser);
static void     cancel_remove_mode                              (BaulPropertyBrowser       *property_browser);
static void     done_button_callback                            (CtkWidget                     *widget,
        CtkWidget                     *property_browser);
static void     help_button_callback                            (CtkWidget                     *widget,
        CtkWidget                     *property_browser);
static void     remove_button_callback                          (CtkWidget                     *widget,
        BaulPropertyBrowser       *property_browser);
static gboolean baul_property_browser_delete_event_callback (CtkWidget                     *widget,
        CdkEvent                      *event,
        gpointer                       user_data);
static void     baul_property_browser_hide_callback         (CtkWidget                     *widget,
        gpointer                       user_data);
static void     baul_property_browser_drag_end              (CtkWidget                     *widget,
        CdkDragContext                *context);
static void     baul_property_browser_drag_begin            (CtkWidget                     *widget,
        CdkDragContext                *context);
static void     baul_property_browser_drag_data_get         (CtkWidget                     *widget,
        CdkDragContext                *context,
        CtkSelectionData              *selection_data,
        guint                          info,
        guint32                        time);
static void     emit_emblems_changed_signal                     (void);
static void     emblems_changed_callback                        (GObject                       *signaller,
        BaulPropertyBrowser       *property_browser);

/* misc utilities */
static void     element_clicked_callback                        (CtkWidget                     *image_table,
        CtkWidget                     *child,
        const EelImageTableEvent *event,
        gpointer                       callback_data);

static CdkPixbuf * make_drag_image                              (BaulPropertyBrowser       *property_browser,
        const char                    *file_name);
static CdkPixbuf * make_color_drag_image                        (BaulPropertyBrowser       *property_browser,
        const char                    *color_spec,
        gboolean                       trim_edges);


#define BROWSER_CATEGORIES_FILE_NAME "browser.xml"

#define PROPERTY_BROWSER_WIDTH 540
#define PROPERTY_BROWSER_HEIGHT 340
#define MAX_EMBLEM_HEIGHT 52
#define STANDARD_BUTTON_IMAGE_HEIGHT 42

#define MAX_ICON_WIDTH 63
#define MAX_ICON_HEIGHT 63
#define COLOR_SQUARE_SIZE 48

#define LABELED_IMAGE_SPACING 2
#define IMAGE_TABLE_X_SPACING 6
#define IMAGE_TABLE_Y_SPACING 4

#define ERASE_OBJECT_NAME "erase.png"

enum
{
    PROPERTY_TYPE
};

static CtkTargetEntry drag_types[] =
{
    { "text/uri-list",  0, PROPERTY_TYPE }
};


G_DEFINE_TYPE_WITH_PRIVATE (BaulPropertyBrowser, baul_property_browser, CTK_TYPE_WINDOW)


/* Destroy the three dialogs for adding patterns/colors/emblems if any of them
   exist. */
static void
baul_property_browser_destroy_dialogs (BaulPropertyBrowser *property_browser)
{
    if (property_browser->details->patterns_dialog)
    {
        ctk_widget_destroy (property_browser->details->patterns_dialog);
        property_browser->details->patterns_dialog = NULL;
    }
    if (property_browser->details->colors_dialog)
    {
        ctk_widget_destroy (property_browser->details->colors_dialog);
        property_browser->details->colors_dialog = NULL;
    }
    if (property_browser->details->emblems_dialog)
    {
        ctk_widget_destroy (property_browser->details->emblems_dialog);
        property_browser->details->emblems_dialog = NULL;
    }
}

static void
baul_property_browser_dispose (GObject *object)
{
    BaulPropertyBrowser *property_browser;

    property_browser = BAUL_PROPERTY_BROWSER (object);

    baul_property_browser_destroy_dialogs (property_browser);

    g_free (property_browser->details->path);
    g_free (property_browser->details->category);
    g_free (property_browser->details->dragged_file);
    g_free (property_browser->details->drag_type);

    g_list_free_full (property_browser->details->keywords, g_free);

    if (property_browser->details->property_chit)
    {
        g_object_unref (property_browser->details->property_chit);
    }

    G_OBJECT_CLASS (baul_property_browser_parent_class)->dispose (object);
}

/* initializing the class object by installing the operations we override */
static void
baul_property_browser_class_init (BaulPropertyBrowserClass *klass)
{
    CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

    G_OBJECT_CLASS (klass)->dispose = baul_property_browser_dispose;
    widget_class->drag_begin = baul_property_browser_drag_begin;
    widget_class->drag_data_get  = baul_property_browser_drag_data_get;
    widget_class->drag_end  = baul_property_browser_drag_end;
}

/* initialize the instance's fields, create the necessary subviews, etc. */

static void
baul_property_browser_init (BaulPropertyBrowser *property_browser)
{
    CtkWidget *widget, *temp_box, *temp_hbox, *temp_frame, *vbox;
    CtkWidget *temp_button;
    CtkWidget *viewport;
    PangoAttrList *attrs;
    char *temp_str;

    widget = CTK_WIDGET (property_browser);

    property_browser->details = baul_property_browser_get_instance_private (property_browser);

    property_browser->details->category = g_strdup ("patterns");
    property_browser->details->category_type = BAUL_PROPERTY_PATTERN;

    /* load the chit frame */
    temp_str = baul_pixmap_file ("chit_frame.png");
    if (temp_str != NULL)
    {
        property_browser->details->property_chit = gdk_pixbuf_new_from_file (temp_str, NULL);
    }
    g_free (temp_str);

    /* set the initial size of the property browser */
    ctk_window_set_default_size (CTK_WINDOW (property_browser),
                                 PROPERTY_BROWSER_WIDTH,
                                 PROPERTY_BROWSER_HEIGHT);

    /* set the title and standard close accelerator */
    ctk_window_set_title (CTK_WINDOW (widget), _("Backgrounds and Emblems"));

    ctk_window_set_type_hint (CTK_WINDOW (widget), CDK_WINDOW_TYPE_HINT_DIALOG);

    CtkStyleContext *context;

    context = ctk_widget_get_style_context (CTK_WIDGET (property_browser));
    ctk_style_context_add_class (context, "baul-property-browser");

    /* create the main vbox. */
    vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
    ctk_container_set_border_width (CTK_CONTAINER (vbox), 12);
    ctk_widget_show (vbox);
    ctk_container_add (CTK_CONTAINER (property_browser), vbox);

    /* create the container box */
    property_browser->details->container = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
    ctk_widget_show (CTK_WIDGET (property_browser->details->container));
    ctk_box_pack_start (CTK_BOX (vbox),
                        property_browser->details->container,
                        TRUE, TRUE, 0);

    /* make the category container */
    property_browser->details->category_container = ctk_scrolled_window_new (NULL, NULL);
    property_browser->details->category_position = -1;

    viewport = ctk_viewport_new (NULL, NULL);
    ctk_widget_show (viewport);
    ctk_viewport_set_shadow_type(CTK_VIEWPORT(viewport), CTK_SHADOW_NONE);

    ctk_box_pack_start (CTK_BOX (property_browser->details->container),
                        property_browser->details->category_container, FALSE, FALSE, 0);
    ctk_widget_show (property_browser->details->category_container);
    ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (property_browser->details->category_container),
                                    CTK_POLICY_NEVER, CTK_POLICY_AUTOMATIC);
    ctk_scrolled_window_set_overlay_scrolling (CTK_SCROLLED_WINDOW (property_browser->details->category_container),
                                               FALSE);

    /* allocate a table to hold the category selector */
    property_browser->details->category_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
    ctk_container_add(CTK_CONTAINER(viewport), property_browser->details->category_box);
    ctk_container_add (CTK_CONTAINER (property_browser->details->category_container), viewport);
    ctk_widget_show (CTK_WIDGET (property_browser->details->category_box));

    /* make the content container vbox */
    property_browser->details->content_container = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
    ctk_widget_show (property_browser->details->content_container);
    ctk_box_pack_start (CTK_BOX (property_browser->details->container),
                        property_browser->details->content_container,
                        TRUE, TRUE, 0);

    /* create the title box */
    property_browser->details->title_box = ctk_event_box_new();

    ctk_widget_show(property_browser->details->title_box);
    ctk_box_pack_start (CTK_BOX(property_browser->details->content_container),
                        property_browser->details->title_box,
                        FALSE, FALSE, 0);

    temp_frame = ctk_frame_new(NULL);
    ctk_frame_set_shadow_type(CTK_FRAME(temp_frame), CTK_SHADOW_NONE);
    ctk_widget_show(temp_frame);
    ctk_container_add(CTK_CONTAINER(property_browser->details->title_box), temp_frame);

    temp_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
    ctk_widget_show(temp_hbox);

    ctk_container_add(CTK_CONTAINER(temp_frame), temp_hbox);

    /* add the title label */
    attrs = pango_attr_list_new ();
    pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_X_LARGE));
    pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
    property_browser->details->title_label = ctk_label_new ("");
    ctk_label_set_attributes (CTK_LABEL (property_browser->details->title_label), attrs);
    pango_attr_list_unref (attrs);

    ctk_widget_show(property_browser->details->title_label);
    ctk_box_pack_start (CTK_BOX(temp_hbox), property_browser->details->title_label, FALSE, FALSE, 0);

    /* add the help label */
    property_browser->details->help_label = ctk_label_new  ("");
    ctk_widget_show(property_browser->details->help_label);
    ctk_box_pack_end (CTK_BOX (temp_hbox), property_browser->details->help_label, FALSE, FALSE, 0);

    /* add the bottom box to hold the command buttons */
    temp_box = ctk_event_box_new();
    ctk_widget_show(temp_box);

    property_browser->details->bottom_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
    ctk_widget_show (property_browser->details->bottom_box);

    ctk_box_pack_end (CTK_BOX (vbox), temp_box, FALSE, FALSE, 0);
    ctk_container_add (CTK_CONTAINER (temp_box), property_browser->details->bottom_box);

    /* create the "help" button */
    temp_button = ctk_button_new_with_mnemonic (_("_Help"));
    ctk_button_set_image (CTK_BUTTON (temp_button), ctk_image_new_from_icon_name ("help-browser", CTK_ICON_SIZE_BUTTON));

    ctk_widget_show (temp_button);
    ctk_box_pack_start (CTK_BOX (property_browser->details->bottom_box), temp_button, FALSE, FALSE, 0);
    g_signal_connect_object (temp_button, "clicked", G_CALLBACK (help_button_callback), property_browser, 0);

    /* create the "close" button */
    temp_button = ctk_button_new_with_mnemonic (_("_Close"));
    ctk_button_set_image (CTK_BUTTON (temp_button), ctk_image_new_from_icon_name ("window-close", CTK_ICON_SIZE_BUTTON));

    ctk_widget_set_can_default (temp_button, TRUE);

    ctk_widget_show (temp_button);
    ctk_box_pack_end (CTK_BOX (property_browser->details->bottom_box), temp_button, FALSE, FALSE, 0);
    ctk_widget_grab_default (temp_button);
    ctk_widget_grab_focus (temp_button);
    g_signal_connect_object (temp_button, "clicked", G_CALLBACK (done_button_callback), property_browser, 0);

    /* create the "remove" button */
    property_browser->details->remove_button = ctk_button_new_with_mnemonic (_("_Remove..."));

    property_browser->details->remove_button_image = ctk_image_new_from_icon_name ("list-remove", CTK_ICON_SIZE_BUTTON);
    ctk_button_set_image (CTK_BUTTON (property_browser->details->remove_button),
                          property_browser->details->remove_button_image);
    ctk_widget_show_all (property_browser->details->remove_button);

    ctk_box_pack_end (CTK_BOX (property_browser->details->bottom_box),
                      property_browser->details->remove_button, FALSE, FALSE, 0);

    g_signal_connect_object (property_browser->details->remove_button, "clicked",
                             G_CALLBACK (remove_button_callback), property_browser, 0);

    /* now create the "add new" button */
    property_browser->details->add_button = ctk_button_new_with_mnemonic (_("Add new..."));

    property_browser->details->add_button_image = ctk_image_new_from_icon_name ("list-add", CTK_ICON_SIZE_BUTTON);
    ctk_button_set_image (CTK_BUTTON (property_browser->details->add_button),
                          property_browser->details->add_button_image);
    ctk_widget_show_all (property_browser->details->add_button);

    ctk_box_pack_end (CTK_BOX(property_browser->details->bottom_box),
                      property_browser->details->add_button, FALSE, FALSE, 0);

    g_signal_connect_object (property_browser->details->add_button, "clicked",
                             G_CALLBACK (add_new_button_callback), property_browser, 0);


    /* now create the actual content, with the category pane and the content frame */

    /* the actual contents are created when necessary */
    property_browser->details->content_frame = NULL;

    g_signal_connect (property_browser, "delete_event",
                      G_CALLBACK (baul_property_browser_delete_event_callback), NULL);
    g_signal_connect (property_browser, "hide",
                      G_CALLBACK (baul_property_browser_hide_callback), NULL);

    g_signal_connect_object (baul_signaller_get_current (),
                             "emblems_changed",
                             G_CALLBACK (emblems_changed_callback), property_browser, 0);

    /* initially, display the top level */
    baul_property_browser_set_path(property_browser, BROWSER_CATEGORIES_FILE_NAME);
}

/* create a new instance */
BaulPropertyBrowser *
baul_property_browser_new (CdkScreen *screen)
{
    BaulPropertyBrowser *browser;

    browser = BAUL_PROPERTY_BROWSER
              (ctk_widget_new (baul_property_browser_get_type (), NULL));

    ctk_window_set_screen (CTK_WINDOW (browser), screen);
    ctk_widget_show (CTK_WIDGET(browser));

    return browser;
}

/* show the main property browser */

void
baul_property_browser_show (CdkScreen *screen)
{
    static CtkWindow *browser = NULL;

    if (browser == NULL)
    {
        browser = CTK_WINDOW (baul_property_browser_new (screen));
        g_object_add_weak_pointer (G_OBJECT (browser),
                                   (gpointer *) &browser);
    }
    else
    {
        ctk_window_set_screen (browser, screen);
        ctk_window_present (browser);
    }
}

static gboolean
baul_property_browser_delete_event_callback (CtkWidget *widget,
        CdkEvent  *event,
        gpointer   user_data)
{
    /* Hide but don't destroy */
    ctk_widget_hide(widget);
    return TRUE;
}

static void
baul_property_browser_hide_callback (CtkWidget *widget,
                                     gpointer   user_data)
{
    BaulPropertyBrowser *property_browser;

    property_browser = BAUL_PROPERTY_BROWSER (widget);

    cancel_remove_mode (property_browser);

    /* Destroy the 3 dialogs to add new patterns/colors/emblems. */
    baul_property_browser_destroy_dialogs (property_browser);
}

/* remember the name of the dragged file */
static void
baul_property_browser_set_dragged_file (BaulPropertyBrowser *property_browser,
                                        const char *dragged_file_name)
{
    g_free (property_browser->details->dragged_file);
    property_browser->details->dragged_file = g_strdup (dragged_file_name);
}

/* remember the drag type */
static void
baul_property_browser_set_drag_type (BaulPropertyBrowser *property_browser,
                                     const char *new_drag_type)
{
    g_free (property_browser->details->drag_type);
    property_browser->details->drag_type = g_strdup (new_drag_type);
}

static void
baul_property_browser_drag_begin (CtkWidget *widget,
                                  CdkDragContext *context)
{
    BaulPropertyBrowser *property_browser;
    CtkWidget *child;
    CdkPixbuf *pixbuf;
    char *element_name;

    property_browser = BAUL_PROPERTY_BROWSER (widget);

    child = g_object_steal_data (G_OBJECT (property_browser), "dragged-image");
    g_return_if_fail (child != NULL);

    element_name = g_object_get_data (G_OBJECT (child), "property-name");
    g_return_if_fail (child != NULL);

    /* compute the offsets for dragging */
    if (strcmp (drag_types[0].target, "application/x-color") != 0)
    {
        /* it's not a color, so, for now, it must be an image */
        /* fiddle with the category to handle the "reset" case properly */
        char * save_category = property_browser->details->category;
        if (g_strcmp0 (property_browser->details->category, "colors") == 0)
        {
            property_browser->details->category = "patterns";
        }
        pixbuf = make_drag_image (property_browser, element_name);
        property_browser->details->category = save_category;
    }
    else
    {
        pixbuf = make_color_drag_image (property_browser, element_name, TRUE);
    }

    /* set the pixmap and mask for dragging */
    if (pixbuf != NULL)
    {
        int x_delta, y_delta;

        x_delta = gdk_pixbuf_get_width (pixbuf) / 2;
        y_delta = gdk_pixbuf_get_height (pixbuf) / 2;

        ctk_drag_set_icon_pixbuf
        (context,
         pixbuf,
         x_delta, y_delta);
        g_object_unref (pixbuf);
    }

}


/* drag and drop data get handler */

static void
baul_property_browser_drag_data_get (CtkWidget *widget,
                                     CdkDragContext *context,
                                     CtkSelectionData *selection_data,
                                     guint info,
                                     guint32 time)
{
    char  *image_file_name, *image_file_uri;
    gboolean is_reset;
    BaulPropertyBrowser *property_browser = BAUL_PROPERTY_BROWSER(widget);
    CdkAtom target;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (context != NULL);

    target = ctk_selection_data_get_target (selection_data);

    switch (info)
    {
    case PROPERTY_TYPE:
        /* formulate the drag data based on the drag type.  Eventually, we will
           probably select the behavior from properties in the category xml definition,
           but for now we hardwire it to the drag_type */

        is_reset = FALSE;
        if (strcmp (property_browser->details->drag_type,
                    "property/keyword") == 0)
        {
            char *keyword_str = eel_filename_strip_extension (property_browser->details->dragged_file);
            ctk_selection_data_set (selection_data, target, 8, keyword_str, strlen (keyword_str));
            g_free (keyword_str);
            return;
        }
        else if (strcmp (property_browser->details->drag_type,
                         "application/x-color") == 0)
        {
            CdkColor color;
            guint16 colorArray[4];

            /* handle the "reset" case as an image */
            if (g_strcmp0 (property_browser->details->dragged_file, RESET_IMAGE_NAME) != 0)
            {
                cdk_color_parse (property_browser->details->dragged_file, &color);

                colorArray[0] = color.red;
                colorArray[1] = color.green;
                colorArray[2] = color.blue;
                colorArray[3] = 0xffff;

                ctk_selection_data_set(selection_data,
                                       target, 16, (const char *) &colorArray[0], 8);
                return;
            }
            else
            {
                is_reset = TRUE;
            }

        }

        image_file_name = g_strdup_printf ("%s/%s/%s",
                                           BAUL_DATADIR,
                                           is_reset ? "patterns" : property_browser->details->category,
                                           property_browser->details->dragged_file);

        if (!g_file_test (image_file_name, G_FILE_TEST_EXISTS))
        {
            char *user_directory;
            g_free (image_file_name);

            user_directory = baul_get_user_directory ();
            image_file_name = g_strdup_printf ("%s/%s/%s",
                                               user_directory,
                                               property_browser->details->category,
                                               property_browser->details->dragged_file);

            g_free (user_directory);
        }

        image_file_uri = g_filename_to_uri (image_file_name, NULL, NULL);
        ctk_selection_data_set (selection_data, target, 8, image_file_uri, strlen (image_file_uri));
        g_free (image_file_name);
        g_free (image_file_uri);

        break;
    default:
        g_assert_not_reached ();
    }
}

/* drag and drop end handler, where we destroy ourselves, since the transaction is complete */

static void
baul_property_browser_drag_end (CtkWidget *widget, CdkDragContext *context)
{
    BaulPropertyBrowser *property_browser = BAUL_PROPERTY_BROWSER(widget);
    if (!property_browser->details->keep_around)
    {
        ctk_widget_hide (CTK_WIDGET (widget));
    }
}

/* utility routine to check if the passed-in uri is an image file */
static gboolean
ensure_file_is_image (GFile *file)
{
    GFileInfo *info;
    const char *mime_type;
    gboolean ret;

    info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL, NULL);
    if (info == NULL)
    {
        return FALSE;
    }

    mime_type = g_file_info_get_content_type (info);
    if (mime_type == NULL)
    {
        return FALSE;
    }

    ret = (g_content_type_is_a (mime_type, "image/*") &&
           !g_content_type_equals (mime_type, "image/svg") &&
           !g_content_type_equals (mime_type, "image/svg+xml"));

    g_object_unref (info);

    return ret;
}

/* create the appropriate pixbuf for the passed in file */

static CdkPixbuf *
make_drag_image (BaulPropertyBrowser *property_browser, const char* file_name)
{
    CdkPixbuf *pixbuf, *orig_pixbuf;
    char *image_file_name;
    gboolean is_reset;

    if (property_browser->details->category_type == BAUL_PROPERTY_EMBLEM)
    {
        if (strcmp (file_name, "erase") == 0)
        {
            pixbuf = NULL;

            image_file_name = baul_pixmap_file (ERASE_OBJECT_NAME);
            if (image_file_name != NULL)
            {
                pixbuf = gdk_pixbuf_new_from_file (image_file_name, NULL);
            }
            g_free (image_file_name);
        }
        else
        {
            char *icon_name;
            BaulIconInfo *info;

            icon_name = baul_emblem_get_icon_name_from_keyword (file_name);
            info = baul_icon_info_lookup_from_name (icon_name, BAUL_ICON_SIZE_STANDARD, 1);
            pixbuf = baul_icon_info_get_pixbuf_at_size (info, BAUL_ICON_SIZE_STANDARD);
            g_object_unref (info);
            g_free (icon_name);
        }
        return pixbuf;
    }

    image_file_name = g_strdup_printf ("%s/%s/%s",
                                       BAUL_DATADIR,
                                       property_browser->details->category,
                                       file_name);

    if (!g_file_test (image_file_name, G_FILE_TEST_EXISTS))
    {
        char *user_directory;
        g_free (image_file_name);

        user_directory = baul_get_user_directory ();

        image_file_name = g_strdup_printf ("%s/%s/%s",
                                           user_directory,
                                           property_browser->details->category,
                                           file_name);

        g_free (user_directory);
    }

    orig_pixbuf = gdk_pixbuf_new_from_file_at_scale (image_file_name,
                  MAX_ICON_WIDTH, MAX_ICON_HEIGHT,
                  TRUE,
                  NULL);

    g_free (image_file_name);

    if (orig_pixbuf == NULL)
    {
        return NULL;
    }

    is_reset = g_strcmp0 (file_name, RESET_IMAGE_NAME) == 0;

    if (strcmp (property_browser->details->category, "patterns") == 0 &&
            property_browser->details->property_chit != NULL)
    {
        pixbuf = baul_customization_make_pattern_chit (orig_pixbuf, property_browser->details->property_chit, TRUE, is_reset);
    }
    else
    {
        pixbuf = eel_gdk_pixbuf_scale_down_to_fit (orig_pixbuf, MAX_ICON_WIDTH, MAX_ICON_HEIGHT);
    }

    g_object_unref (orig_pixbuf);

    return pixbuf;
}


/* create a pixbuf and fill it with a color */

static CdkPixbuf*
make_color_drag_image (BaulPropertyBrowser *property_browser, const char *color_spec, gboolean trim_edges)
{
    CdkPixbuf *color_square;
    CdkPixbuf *ret;
    int row, col, stride;
    char *pixels;
    CdkColor color;

    color_square = gdk_pixbuf_new (CDK_COLORSPACE_RGB, TRUE, 8, COLOR_SQUARE_SIZE, COLOR_SQUARE_SIZE);

    cdk_color_parse (color_spec, &color);
    color.red >>= 8;
    color.green >>= 8;
    color.blue >>= 8;

    pixels = gdk_pixbuf_get_pixels (color_square);
    stride = gdk_pixbuf_get_rowstride (color_square);

    /* loop through and set each pixel */
    for (row = 0; row < COLOR_SQUARE_SIZE; row++)
    {
        char *row_pixels;

        row_pixels =  (pixels + (row * stride));

        for (col = 0; col < COLOR_SQUARE_SIZE; col++)
        {
            *row_pixels++ = color.red;
            *row_pixels++ = color.green;
            *row_pixels++ = color.blue;
            *row_pixels++ = 255;
        }
    }

    g_assert (color_square != NULL);

    if (property_browser->details->property_chit != NULL)
    {
        ret = baul_customization_make_pattern_chit (color_square,
                property_browser->details->property_chit,
                trim_edges, FALSE);
        g_object_unref (color_square);
    }
    else
    {
        ret = color_square;
    }

    return ret;
}

/* this callback handles button presses on the category widget. It maintains the active state */

static void
category_toggled_callback (CtkWidget *widget, char *category_name)
{
    BaulPropertyBrowser *property_browser;

    property_browser = BAUL_PROPERTY_BROWSER (g_object_get_data (G_OBJECT (widget), "user_data"));

    /* exit remove mode when the user switches categories, since there might be nothing to remove
       in the new category */
    property_browser->details->remove_mode = FALSE;

    baul_property_browser_set_category (property_browser, category_name);
}

static xmlDocPtr
read_browser_xml (BaulPropertyBrowser *property_browser)
{
    char *path;
    xmlDocPtr document;

    path = baul_get_data_file_path (property_browser->details->path);
    if (path == NULL)
    {
        return NULL;
    }
    document = xmlParseFile (path);
    g_free (path);
    return document;
}

static void
write_browser_xml (BaulPropertyBrowser *property_browser,
                   xmlDocPtr document)
{
    char *user_directory, *path;

    user_directory = baul_get_user_directory ();
    path = g_build_filename (user_directory, property_browser->details->path, NULL);
    g_free (user_directory);
    xmlSaveFile (path, document);
    g_free (path);
}

static xmlNodePtr
get_color_category (xmlDocPtr document)
{
    return eel_xml_get_root_child_by_name_and_property (document, "category", "name", "colors");
}

/* routines to remove specific category types.  First, handle colors */

static void
remove_color (BaulPropertyBrowser *property_browser, const char* color_name)
{
    /* load the local xml file to remove the color */
    xmlDocPtr document;
    xmlNodePtr cur_node, color_node;
    gboolean match;
    char *name;

    document = read_browser_xml (property_browser);
    if (document == NULL)
    {
        return;
    }

    /* find the colors category */
    cur_node = get_color_category (document);
    if (cur_node != NULL)
    {
        /* loop through the colors to find one that matches */
        for (color_node = eel_xml_get_children (cur_node);
                color_node != NULL;
                color_node = color_node->next)
        {

            if (color_node->type != XML_ELEMENT_NODE)
            {
                continue;
            }

            name = xmlGetProp (color_node, "name");
            match = name != NULL
                    && strcmp (name, color_name) == 0;
            xmlFree (name);

            if (match)
            {
                xmlUnlinkNode (color_node);
                xmlFreeNode (color_node);
                write_browser_xml (property_browser, document);
                break;
            }
        }
    }

    xmlFreeDoc (document);
}

/* remove the pattern matching the passed in name */

static void
remove_pattern(BaulPropertyBrowser *property_browser, const char* pattern_name)
{
    char *pattern_path;
    char *user_directory;

    user_directory = baul_get_user_directory ();

    /* build the pathname of the pattern */
    pattern_path = g_build_filename (user_directory,
                                     "patterns",
                                     pattern_name,
                                     NULL);
    g_free (user_directory);

    /* delete the pattern from the pattern directory */
    if (g_unlink (pattern_path) != 0)
    {
        char *message = g_strdup_printf (_("Sorry, but pattern %s could not be deleted."), pattern_name);
        char *detail = _("Check that you have permission to delete the pattern.");
        eel_show_error_dialog (message, detail, CTK_WINDOW (property_browser));
        g_free (message);
    }

    g_free (pattern_path);
}

/* remove the emblem matching the passed in name */

static void
remove_emblem (BaulPropertyBrowser *property_browser, const char* emblem_name)
{
    /* delete the emblem from the emblem directory */
    if (baul_emblem_remove_emblem (emblem_name) == FALSE)
    {
        char *message = g_strdup_printf (_("Sorry, but emblem %s could not be deleted."), emblem_name);
        char *detail = _("Check that you have permission to delete the emblem.");
        eel_show_error_dialog (message, detail, CTK_WINDOW (property_browser));
        g_free (message);
    }
    else
    {
        emit_emblems_changed_signal ();
    }
}

/* handle removing the passed in element */

static void
baul_property_browser_remove_element (BaulPropertyBrowser *property_browser, EelLabeledImage *child)
{
    const char *element_name;
    char *color_name;

    element_name = g_object_get_data (G_OBJECT (child), "property-name");

    /* lookup category and get mode, then case out and handle the modes */
    switch (property_browser->details->category_type)
    {
    case BAUL_PROPERTY_PATTERN:
        remove_pattern (property_browser, element_name);
        break;
    case BAUL_PROPERTY_COLOR:
        color_name = eel_labeled_image_get_text (child);
        remove_color (property_browser, color_name);
        g_free (color_name);
        break;
    case BAUL_PROPERTY_EMBLEM:
        remove_emblem (property_browser, element_name);
        break;
    default:
        break;
    }
}

static void
update_preview_cb (CtkFileChooser *fc,
                   CtkImage *preview)
{
    char *filename;

    filename = ctk_file_chooser_get_preview_filename (fc);

    if (filename)
    {
        CdkPixbuf *pixbuf;

        pixbuf = gdk_pixbuf_new_from_file (filename, NULL);

        ctk_file_chooser_set_preview_widget_active (fc, pixbuf != NULL);

        if (pixbuf)
        {
            ctk_image_set_from_pixbuf (preview, pixbuf);
            g_object_unref (pixbuf);
        }

        g_free (filename);
    }
}

static void
icon_button_clicked_cb (CtkButton *b,
                        BaulPropertyBrowser *browser)
{
    CtkWidget *dialog;
    CtkFileFilter *filter;
    CtkWidget *preview;
    int res;

    dialog = eel_file_chooser_dialog_new (_("Select an Image File for the New Emblem"),
                                          CTK_WINDOW (browser),
                                          CTK_FILE_CHOOSER_ACTION_OPEN,
                                          "process-stop", CTK_RESPONSE_CANCEL,
                                          "document-open", CTK_RESPONSE_ACCEPT,
                                          NULL);
    ctk_file_chooser_set_current_folder (CTK_FILE_CHOOSER (dialog),
                                         DATADIR "/pixmaps");
    filter = ctk_file_filter_new ();
    ctk_file_filter_add_pixbuf_formats (filter);
    ctk_file_chooser_set_filter (CTK_FILE_CHOOSER (dialog), filter);

    preview = ctk_image_new ();
    ctk_file_chooser_set_preview_widget (CTK_FILE_CHOOSER (dialog),
                                         preview);
    g_signal_connect (dialog, "update-preview",
                      G_CALLBACK (update_preview_cb), preview);

    res = ctk_dialog_run (CTK_DIALOG (dialog));

    if (res == CTK_RESPONSE_ACCEPT)
    {
        /* update the image */
        g_free (browser->details->filename);
        browser->details->filename = ctk_file_chooser_get_filename (CTK_FILE_CHOOSER (dialog));
        ctk_image_set_from_file (CTK_IMAGE (browser->details->image_button), browser->details->filename);
    }

    ctk_widget_destroy (dialog);
}

/* here's where we create the emblem dialog */
static CtkWidget*
baul_emblem_dialog_new (BaulPropertyBrowser *property_browser)
{
    CtkWidget *widget;
    CtkWidget *button;
    CtkWidget *dialog;
    CtkWidget *label;
    CtkWidget *grid = ctk_grid_new ();

    dialog = ctk_dialog_new ();
    ctk_window_set_title (CTK_WINDOW (dialog), _("Create a New Emblem"));
    ctk_window_set_transient_for (CTK_WINDOW (dialog), CTK_WINDOW (property_browser));

    eel_dialog_add_button (CTK_DIALOG (dialog),
                           _("_Cancel"),
                           "process-stop",
                           CTK_RESPONSE_CANCEL);

    eel_dialog_add_button (CTK_DIALOG (dialog),
                           _("_OK"),
                           "ctk-ok",
                           CTK_RESPONSE_OK);

    /* install the grid in the dialog */
    ctk_container_set_border_width (CTK_CONTAINER (grid), 5);
    ctk_grid_set_row_spacing (CTK_GRID (grid), 6);
    ctk_grid_set_column_spacing (CTK_GRID (grid), 12);
    ctk_widget_show (grid);

    ctk_window_set_resizable (CTK_WINDOW (dialog), TRUE);
    ctk_container_set_border_width (CTK_CONTAINER (dialog), 5);
    ctk_box_set_spacing (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dialog))), 2);
    ctk_window_set_resizable (CTK_WINDOW (dialog), FALSE);
    ctk_box_pack_start (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dialog))), grid, TRUE, TRUE, 0);
    ctk_dialog_set_default_response (CTK_DIALOG(dialog), CTK_RESPONSE_OK);

    /* make the keyword label and field */

    widget = ctk_label_new_with_mnemonic(_("_Keyword:"));
    ctk_label_set_xalign (CTK_LABEL (widget), 0.0);
    ctk_widget_show(widget);
    ctk_grid_attach(CTK_GRID(grid), widget, 0, 0, 1, 1);

    property_browser->details->keyword = ctk_entry_new ();
    ctk_entry_set_activates_default (CTK_ENTRY (property_browser->details->keyword), TRUE);
    ctk_entry_set_max_length (CTK_ENTRY (property_browser->details->keyword), 24);
    ctk_widget_show(property_browser->details->keyword);
    ctk_grid_attach(CTK_GRID(grid), property_browser->details->keyword, 1, 0, 1, 1);
    ctk_widget_grab_focus(property_browser->details->keyword);
    ctk_label_set_mnemonic_widget (CTK_LABEL (widget),
                                   CTK_WIDGET (property_browser->details->keyword));

    /* default image is the generic emblem */
    g_free (property_browser->details->image_path);
    property_browser->details->image_path = g_build_filename (BAUL_PIXMAPDIR, "emblems.png", NULL);

    /* set up a file chooser to pick the image file */
    label = ctk_label_new_with_mnemonic (_("_Image:"));
    ctk_label_set_xalign (CTK_LABEL (label), 0.0);
    ctk_widget_show (label);
    ctk_grid_attach (CTK_GRID(grid), label, 0, 1, 1, 1);

    widget = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
    ctk_widget_show (widget);

    button = ctk_button_new ();
    property_browser->details->image_button = ctk_image_new_from_file (property_browser->details->image_path);
    ctk_button_set_image (CTK_BUTTON (button), property_browser->details->image_button);
    g_signal_connect (button, "clicked", G_CALLBACK (icon_button_clicked_cb),
                      property_browser);
    ctk_label_set_mnemonic_widget (CTK_LABEL (label), button);

    ctk_widget_show (button);
    ctk_grid_attach (CTK_GRID (grid), widget, 1, 1, 1, 1);
    ctk_box_pack_start (CTK_BOX (widget), button, FALSE, FALSE, 0);

    return dialog;
}

/* create the color selection dialog */

static CtkWidget*
baul_color_selection_dialog_new (BaulPropertyBrowser *property_browser)
{
    CtkWidget *widget;
    CtkWidget *dialog;

    CtkWidget *grid = ctk_grid_new ();

    dialog = ctk_dialog_new ();
    ctk_window_set_title (CTK_WINDOW (dialog), _("Create a New Color:"));
    ctk_window_set_transient_for (CTK_WINDOW (dialog), CTK_WINDOW (property_browser));

    eel_dialog_add_button (CTK_DIALOG (dialog),
                           _("_Cancel"),
                           "process-stop",
                           CTK_RESPONSE_CANCEL);

    eel_dialog_add_button (CTK_DIALOG (dialog),
                           _("_OK"),
                           "ctk-ok",
                           CTK_RESPONSE_OK);

    /* install the grid in the dialog */
    ctk_widget_show (grid);
    ctk_box_pack_start (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dialog))), grid, TRUE, TRUE, 0);

    ctk_dialog_set_default_response (CTK_DIALOG(dialog), CTK_RESPONSE_OK);

    /* make the name label and field */

    widget = ctk_label_new_with_mnemonic(_("Color _name:"));
    ctk_widget_show(widget);
    ctk_grid_attach(CTK_GRID(grid), widget, 0, 0, 1, 1);

    property_browser->details->color_name = ctk_entry_new ();
    ctk_entry_set_activates_default (CTK_ENTRY (property_browser->details->color_name), TRUE);
    ctk_entry_set_max_length (CTK_ENTRY (property_browser->details->color_name), 24);
    ctk_widget_grab_focus (property_browser->details->color_name);
    ctk_label_set_mnemonic_widget (CTK_LABEL (widget), property_browser->details->color_name);
    ctk_widget_show(property_browser->details->color_name);
    ctk_grid_attach(CTK_GRID(grid), property_browser->details->color_name, 1, 0, 1, 1);
    ctk_widget_grab_focus(property_browser->details->color_name);

    /* default image is the generic emblem */
    g_free(property_browser->details->image_path);

    widget = ctk_label_new_with_mnemonic(_("Color _value:"));
    ctk_widget_show(widget);
    ctk_grid_attach(CTK_GRID(grid), widget, 0, 1, 1, 1);

    property_browser->details->color_picker = ctk_color_button_new ();
    ctk_widget_show (property_browser->details->color_picker);
    ctk_label_set_mnemonic_widget (CTK_LABEL (widget), property_browser->details->color_picker);

    ctk_grid_attach(CTK_GRID(grid), property_browser->details->color_picker, 1, 1, 1, 1);

    return dialog;
}

/* add the newly selected file to the browser images */
static void
add_pattern_to_browser (CtkDialog *dialog, gint response_id, gpointer data)
{
    char *directory_path, *destination_name;
    char *basename;
    char *user_directory;
    GFile *dest, *selected;

    BaulPropertyBrowser *property_browser = BAUL_PROPERTY_BROWSER (data);

    if (response_id != CTK_RESPONSE_ACCEPT)
    {
        ctk_widget_hide (CTK_WIDGET (dialog));
        return;
    }

    selected = ctk_file_chooser_get_file (CTK_FILE_CHOOSER (dialog));

    /* don't allow the user to change the reset image */
    basename = g_file_get_basename (selected);
    if (basename && g_strcmp0 (basename, RESET_IMAGE_NAME) == 0)
    {
        eel_show_error_dialog (_("Sorry, but you cannot replace the reset image."),
                               _("Reset is a special image that cannot be deleted."),
                               NULL);
        g_object_unref (selected);
        g_free (basename);
        return;
    }


    user_directory = baul_get_user_directory ();

    /* copy the image file to the patterns directory */
    directory_path = g_build_filename (user_directory, "patterns", NULL);
    g_free (user_directory);
    destination_name = g_build_filename (directory_path, basename, NULL);

    /* make the directory if it doesn't exist */
    if (!g_file_test (directory_path, G_FILE_TEST_EXISTS))
    {
        g_mkdir_with_parents (directory_path, 0775);
    }

    dest = g_file_new_for_path (destination_name);

    g_free (destination_name);
    g_free (directory_path);

    if (!g_file_copy (selected, dest,
                      0,
                      NULL, NULL, NULL, NULL))
    {
        char *message = g_strdup_printf (_("Sorry, but the pattern %s could not be installed."), basename);
        eel_show_error_dialog (message, NULL, CTK_WINDOW (property_browser));
        g_free (message);
    }
    g_object_unref (selected);
    g_object_unref (dest);
    g_free (basename);


    /* update the property browser's contents to show the new one */
    baul_property_browser_update_contents (property_browser);

    ctk_widget_hide (CTK_WIDGET (dialog));
}

/* here's where we initiate adding a new pattern by putting up a file selector */

static void
add_new_pattern (BaulPropertyBrowser *property_browser)
{
    CtkWidget *dialog;

    if (property_browser->details->patterns_dialog)
    {
        ctk_window_present (CTK_WINDOW (property_browser->details->patterns_dialog));
    }
    else
    {
        CtkFileFilter *filter;
        CtkWidget *preview;

        property_browser->details->patterns_dialog = dialog =
                    eel_file_chooser_dialog_new (_("Select an Image File to Add as a Pattern"),
                            CTK_WINDOW (property_browser),
                            CTK_FILE_CHOOSER_ACTION_OPEN,
                            "process-stop", CTK_RESPONSE_CANCEL,
                            "document-open", CTK_RESPONSE_ACCEPT,
                            NULL);
        ctk_file_chooser_set_current_folder (CTK_FILE_CHOOSER (dialog),
                                             DATADIR "/baul/patterns/");
        filter = ctk_file_filter_new ();
        ctk_file_filter_add_pixbuf_formats (filter);
        ctk_file_chooser_set_filter (CTK_FILE_CHOOSER (dialog), filter);

        ctk_file_chooser_set_local_only (CTK_FILE_CHOOSER (dialog), FALSE);

        preview = ctk_image_new ();
        ctk_file_chooser_set_preview_widget (CTK_FILE_CHOOSER (dialog),
                                             preview);
        g_signal_connect (dialog, "update-preview",
                          G_CALLBACK (update_preview_cb), preview);

        g_signal_connect (dialog, "response",
                          G_CALLBACK (add_pattern_to_browser),
                          property_browser);

        ctk_widget_show (CTK_WIDGET (dialog));

        if (property_browser->details->patterns_dialog)
            eel_add_weak_pointer (&property_browser->details->patterns_dialog);
    }
}

/* here's where we add the passed in color to the file that defines the colors */

static void
add_color_to_file (BaulPropertyBrowser *property_browser, const char *color_spec, const char *color_name)
{
    xmlNodePtr cur_node, new_color_node, children_node;
    xmlDocPtr document;
    gboolean color_name_exists = FALSE;

    document = read_browser_xml (property_browser);
    if (document == NULL)
    {
        return;
    }

    /* find the colors category */
    cur_node = get_color_category (document);
    if (cur_node != NULL)
    {
        /* check if theres already a color whith that name */
        children_node = cur_node->xmlChildrenNode;
        while (children_node != NULL)
        {
            xmlChar *child_color_name;

            child_color_name = xmlGetProp (children_node, "name");
            if (xmlStrcmp (color_name, child_color_name) == 0)
            {
                color_name_exists = TRUE;
                xmlFree (child_color_name);
                break;
            }
            xmlFree (child_color_name);

            children_node = children_node->next;
        }

        /* add a new color node */
        if (!color_name_exists)
        {
            new_color_node = xmlNewChild (cur_node, NULL, "color", NULL);
            xmlNodeSetContent (new_color_node, color_spec);
            xmlSetProp (new_color_node, "local", "1");
            xmlSetProp (new_color_node, "name", color_name);

            write_browser_xml (property_browser, document);
        }
        else
        {
            eel_show_error_dialog (_("The color cannot be installed."),
                                   _("Sorry, but you must specify an unused color name for the new color."),
                                   CTK_WINDOW (property_browser));
        }
    }

    xmlFreeDoc (document);
}

/* handle the OK button being pushed on the color selection dialog */
static void
add_color_to_browser (CtkWidget *widget, gint which_button, gpointer data)
{
    BaulPropertyBrowser *property_browser = BAUL_PROPERTY_BROWSER (data);

    if (which_button == CTK_RESPONSE_OK)
    {
        char * color_spec;
        const char *color_name;
        char *stripped_color_name;
        CdkColor color;

        ctk_color_button_get_color (CTK_COLOR_BUTTON (property_browser->details->color_picker), &color);
        color_spec = cdk_color_to_string (&color);

        color_name = ctk_entry_get_text (CTK_ENTRY (property_browser->details->color_name));
        stripped_color_name = g_strstrip (g_strdup (color_name));
        if (strlen (stripped_color_name) == 0)
        {
            eel_show_error_dialog (_("The color cannot be installed."),
                                   _("Sorry, but you must specify a non-blank name for the new color."),
                                   CTK_WINDOW (property_browser));

        }
        else
        {
            add_color_to_file (property_browser, color_spec, stripped_color_name);
            baul_property_browser_update_contents(property_browser);
        }
        g_free (stripped_color_name);
        g_free (color_spec);
    }

    ctk_widget_destroy(property_browser->details->colors_dialog);
    property_browser->details->colors_dialog = NULL;
}

/* create the color selection dialog, pre-set with the color that was just selected */
static void
show_color_selection_window (CtkWidget *widget, gpointer data)
{
    CdkColor color;
    BaulPropertyBrowser *property_browser = BAUL_PROPERTY_BROWSER (data);

    ctk_color_selection_get_current_color (CTK_COLOR_SELECTION
                                           (ctk_color_selection_dialog_get_color_selection (CTK_COLOR_SELECTION_DIALOG (property_browser->details->colors_dialog))),
                                           &color);
    ctk_widget_destroy (property_browser->details->colors_dialog);

    /* allocate a new color selection dialog */
    property_browser->details->colors_dialog = baul_color_selection_dialog_new (property_browser);

    /* set the color to the one picked by the selector */
    ctk_color_button_set_color (CTK_COLOR_BUTTON (property_browser->details->color_picker), &color);

    /* connect the signals to the new dialog */

    eel_add_weak_pointer (&property_browser->details->colors_dialog);

    g_signal_connect_object (property_browser->details->colors_dialog, "response",
                             G_CALLBACK (add_color_to_browser), property_browser, 0);
    ctk_window_set_position (CTK_WINDOW (property_browser->details->colors_dialog), CTK_WIN_POS_MOUSE);
    ctk_widget_show (CTK_WIDGET(property_browser->details->colors_dialog));
}


/* here's the routine to add a new color, by putting up a color selector */

static void
add_new_color (BaulPropertyBrowser *property_browser)
{
    if (property_browser->details->colors_dialog)
    {
        ctk_window_present (CTK_WINDOW (property_browser->details->colors_dialog));
    }
    else
    {
        CtkColorSelectionDialog *color_dialog;
        CtkWidget *ok_button, *cancel_button, *help_button;

        property_browser->details->colors_dialog = ctk_color_selection_dialog_new (_("Select a Color to Add"));
        color_dialog = CTK_COLOR_SELECTION_DIALOG (property_browser->details->colors_dialog);

        eel_add_weak_pointer (&property_browser->details->colors_dialog);

        g_object_get (color_dialog, "ok-button", &ok_button,
                      "cancel-button", &cancel_button,
                      "help-button", &help_button, NULL);

        g_signal_connect_object (ok_button, "clicked",
                                 G_CALLBACK (show_color_selection_window), property_browser, 0);
        g_signal_connect_object (cancel_button, "clicked",
                                 G_CALLBACK (ctk_widget_destroy), color_dialog, G_CONNECT_SWAPPED);
        ctk_widget_hide (help_button);

        ctk_window_set_position (CTK_WINDOW (color_dialog), CTK_WIN_POS_MOUSE);
        ctk_widget_show (CTK_WIDGET(color_dialog));
    }
}

/* here's where we handle clicks in the emblem dialog buttons */
static void
emblem_dialog_clicked (CtkWidget *dialog, int which_button, BaulPropertyBrowser *property_browser)
{
    char *emblem_path;

    if (which_button == CTK_RESPONSE_OK)
    {
        const char *new_keyword;
        char *stripped_keyword;
        GFile *emblem_file;
        CdkPixbuf *pixbuf;

        /* update the image path from the file entry */
        if (property_browser->details->filename)
        {
            emblem_path = property_browser->details->filename;
            emblem_file = g_file_new_for_path (emblem_path);
            if (ensure_file_is_image (emblem_file))
            {
                g_free (property_browser->details->image_path);
                property_browser->details->image_path = emblem_path;
            }
            else
            {
                char *message = g_strdup_printf
                                (_("Sorry, but \"%s\" is not a usable image file."), emblem_path);
                eel_show_error_dialog (_("The file is not an image."), message, CTK_WINDOW (property_browser));
                g_free (message);
                g_free (emblem_path);
                emblem_path = NULL;
                g_object_unref (emblem_file);
                return;
            }
            g_object_unref (emblem_file);
        }

        emblem_file = g_file_new_for_path (property_browser->details->image_path);
        pixbuf = baul_emblem_load_pixbuf_for_emblem (emblem_file);
        g_object_unref (emblem_file);

        if (pixbuf == NULL)
        {
            char *message = g_strdup_printf
                            (_("Sorry, but \"%s\" is not a usable image file."), property_browser->details->image_path);
            eel_show_error_dialog (_("The file is not an image."), message, CTK_WINDOW (property_browser));
            g_free (message);
        }

        new_keyword = ctk_entry_get_text(CTK_ENTRY(property_browser->details->keyword));
        if (new_keyword == NULL)
        {
            stripped_keyword = NULL;
        }
        else
        {
            stripped_keyword = g_strstrip (g_strdup (new_keyword));
        }


        baul_emblem_install_custom_emblem (pixbuf,
                                           stripped_keyword,
                                           stripped_keyword,
                                           CTK_WINDOW (property_browser));
        if (pixbuf != NULL)
            g_object_unref (pixbuf);

        baul_emblem_refresh_list ();

        emit_emblems_changed_signal ();

        g_free (stripped_keyword);
    }

    ctk_widget_destroy (dialog);

    property_browser->details->keyword = NULL;
    property_browser->details->emblem_image = NULL;
    property_browser->details->filename = NULL;
}

/* here's the routine to add a new emblem, by putting up an emblem dialog */

static void
add_new_emblem (BaulPropertyBrowser *property_browser)
{
    if (property_browser->details->emblems_dialog)
    {
        ctk_window_present (CTK_WINDOW (property_browser->details->emblems_dialog));
    }
    else
    {
        property_browser->details->emblems_dialog = baul_emblem_dialog_new (property_browser);

        eel_add_weak_pointer (&property_browser->details->emblems_dialog);

        g_signal_connect_object (property_browser->details->emblems_dialog, "response",
                                 G_CALLBACK (emblem_dialog_clicked), property_browser, 0);
        ctk_window_set_position (CTK_WINDOW (property_browser->details->emblems_dialog), CTK_WIN_POS_MOUSE);
        ctk_widget_show (CTK_WIDGET(property_browser->details->emblems_dialog));
    }
}

/* cancelremove mode */
static void
cancel_remove_mode (BaulPropertyBrowser *property_browser)
{
    if (property_browser->details->remove_mode)
    {
        property_browser->details->remove_mode = FALSE;
        baul_property_browser_update_contents(property_browser);
        ctk_widget_show (property_browser->details->help_label);
    }
}

/* handle the add_new button */

static void
add_new_button_callback(CtkWidget *widget, BaulPropertyBrowser *property_browser)
{
    /* handle remove mode, where we act as a cancel button */
    if (property_browser->details->remove_mode)
    {
        cancel_remove_mode (property_browser);
        return;
    }

    switch (property_browser->details->category_type)
    {
    case BAUL_PROPERTY_PATTERN:
        add_new_pattern (property_browser);
        break;
    case BAUL_PROPERTY_COLOR:
        add_new_color (property_browser);
        break;
    case BAUL_PROPERTY_EMBLEM:
        add_new_emblem (property_browser);
        break;
    default:
        break;
    }
}

/* handle the "done" button */
static void
done_button_callback (CtkWidget *widget, CtkWidget *property_browser)
{
    cancel_remove_mode (BAUL_PROPERTY_BROWSER (property_browser));
    ctk_widget_hide (property_browser);
}

/* handle the "help" button */
static void
help_button_callback (CtkWidget *widget, CtkWidget *property_browser)
{
    GError *error = NULL;

    ctk_show_uri_on_window (CTK_WINDOW (property_browser),
                            "help:cafe-user-guide/gosbaul-50",
                            ctk_get_current_event_time (), &error);

    if (error)
    {
        CtkWidget *dialog;

        dialog = ctk_message_dialog_new (CTK_WINDOW (property_browser),
                                         CTK_DIALOG_DESTROY_WITH_PARENT,
                                         CTK_MESSAGE_ERROR,
                                         CTK_BUTTONS_OK,
                                         _("There was an error displaying help: \n%s"),
                                         error->message);

        g_signal_connect (G_OBJECT (dialog),
                          "response", G_CALLBACK (ctk_widget_destroy),
                          NULL);
        ctk_window_set_resizable (CTK_WINDOW (dialog), FALSE);
        ctk_widget_show (dialog);
        g_error_free (error);
    }
}

/* handle the "remove" button */
static void
remove_button_callback(CtkWidget *widget, BaulPropertyBrowser *property_browser)
{
    if (property_browser->details->remove_mode)
    {
        return;
    }

    property_browser->details->remove_mode = TRUE;
    ctk_widget_hide (property_browser->details->help_label);
    baul_property_browser_update_contents(property_browser);
}

/* this callback handles clicks on the image or color based content content elements */

static void
element_clicked_callback (CtkWidget *image_table,
                          CtkWidget *child,
                          const EelImageTableEvent *event,
                          gpointer callback_data)
{
    BaulPropertyBrowser *property_browser;
    CtkTargetList *target_list;
    const char *element_name;

    g_return_if_fail (EEL_IS_IMAGE_TABLE (image_table));
    g_return_if_fail (EEL_IS_LABELED_IMAGE (child));
    g_return_if_fail (event != NULL);
    g_return_if_fail (BAUL_IS_PROPERTY_BROWSER (callback_data));
    g_return_if_fail (g_object_get_data (G_OBJECT (child), "property-name") != NULL);

    element_name = g_object_get_data (G_OBJECT (child), "property-name");
    property_browser = BAUL_PROPERTY_BROWSER (callback_data);

    /* handle remove mode by removing the element */
    if (property_browser->details->remove_mode)
    {
        baul_property_browser_remove_element (property_browser, EEL_LABELED_IMAGE (child));
        property_browser->details->remove_mode = FALSE;
        baul_property_browser_update_contents (property_browser);
        ctk_widget_show (property_browser->details->help_label);
        return;
    }

    /* set up the drag and drop type corresponding to the category */
    drag_types[0].target = property_browser->details->drag_type;

    /* treat the reset property in the colors section specially */
    if (g_strcmp0 (element_name, RESET_IMAGE_NAME) == 0)
    {
        drag_types[0].target = "x-special/cafe-reset-background";
    }

    target_list = ctk_target_list_new (drag_types, G_N_ELEMENTS (drag_types));
    baul_property_browser_set_dragged_file(property_browser, element_name);

    g_object_set_data (G_OBJECT (property_browser), "dragged-image", child);

    ctk_drag_begin_with_coordinates (CTK_WIDGET (property_browser),
                                     target_list,
                                     CDK_ACTION_ASK | CDK_ACTION_MOVE | CDK_ACTION_COPY,
                                     event->button,
                                     event->event,
                                     event->x,
                                     event->y);

    ctk_target_list_unref (target_list);

    /* optionally (if the shift key is down) hide the property browser - it will later be destroyed when the drag ends */
    property_browser->details->keep_around = (event->state & CDK_SHIFT_MASK) == 0;
    if (! property_browser->details->keep_around)
    {
        ctk_widget_hide (CTK_WIDGET (property_browser));
    }
}

static void
labeled_image_configure (EelLabeledImage *labeled_image)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    eel_labeled_image_set_spacing (labeled_image, LABELED_IMAGE_SPACING);
}

/* Make a color tile for a property */
static CtkWidget *
labeled_image_new (const char *text,
                   CdkPixbuf *pixbuf,
                   const char *property_name,
                   double scale_factor)
{
    CtkWidget *labeled_image;

    labeled_image = eel_labeled_image_new (text, pixbuf);
    labeled_image_configure (EEL_LABELED_IMAGE (labeled_image));

    if (property_name != NULL)
    {
        g_object_set_data_full (G_OBJECT (labeled_image),
                                "property-name",
                                g_strdup (property_name),
                                g_free);
    }

    return labeled_image;
}

static void
make_properties_from_directories (BaulPropertyBrowser *property_browser)
{
    char *object_name;
    char *object_label;
    CdkPixbuf *object_pixbuf;
    EelImageTable *image_table;
    CtkWidget *reset_object = NULL;
    GList *icons, *l;
    CtkWidget *property_image;
    guint num_images;

    g_return_if_fail (BAUL_IS_PROPERTY_BROWSER (property_browser));
    g_return_if_fail (EEL_IS_IMAGE_TABLE (property_browser->details->content_table));

    image_table = EEL_IMAGE_TABLE (property_browser->details->content_table);

    if (property_browser->details->category_type == BAUL_PROPERTY_EMBLEM)
    {
        BaulIconInfo *info = NULL;

        g_list_free_full (property_browser->details->keywords, g_free);
        property_browser->details->keywords = NULL;

        icons = baul_emblem_list_available ();

        property_browser->details->has_local = FALSE;
        l = icons;
        while (l != NULL)
        {
            char *icon_name;
            char *keyword;

            icon_name = (char *)l->data;
            l = l->next;

            if (!baul_emblem_should_show_in_list (icon_name))
            {
                continue;
            }

            object_name = baul_emblem_get_keyword_from_icon_name (icon_name);
            if (baul_emblem_can_remove_emblem (object_name))
            {
                property_browser->details->has_local = TRUE;
            }
            else if (property_browser->details->remove_mode)
            {
                g_free (object_name);
                continue;
            }
            info = baul_icon_info_lookup_from_name (icon_name, BAUL_ICON_SIZE_STANDARD, 1);
            object_pixbuf = baul_icon_info_get_pixbuf_at_size (info, BAUL_ICON_SIZE_STANDARD);
            object_label = g_strdup (baul_icon_info_get_display_name (info));
            g_object_unref (info);

            if (object_label == NULL)
            {
                object_label = g_strdup (object_name);
            }

            property_image = labeled_image_new (object_label, object_pixbuf, object_name, PANGO_SCALE_LARGE);
            eel_labeled_image_set_fixed_image_height (EEL_LABELED_IMAGE (property_image), MAX_EMBLEM_HEIGHT);

            keyword = eel_filename_strip_extension (object_name);
            property_browser->details->keywords = g_list_prepend (property_browser->details->keywords,
                                                  keyword);

            ctk_container_add (CTK_CONTAINER (image_table), property_image);
            ctk_widget_show (property_image);

            g_free (object_name);
            g_free (object_label);
            if (object_pixbuf != NULL)
            {
                g_object_unref (object_pixbuf);
            }
        }
        g_list_free_full (icons, g_free);
    }
    else
    {
        BaulCustomizationData *customization_data;

        customization_data = baul_customization_data_new (property_browser->details->category,
                             !property_browser->details->remove_mode,
                             MAX_ICON_WIDTH,
                             MAX_ICON_HEIGHT);
        if (customization_data == NULL)
        {
            return;
        }

        /* interate through the set of objects and display each */
        while (baul_customization_data_get_next_element_for_display (customization_data,
                &object_name,
                &object_pixbuf,
                &object_label))
        {

            property_image = labeled_image_new (object_label, object_pixbuf, object_name, PANGO_SCALE_LARGE);

            ctk_container_add (CTK_CONTAINER (image_table), property_image);
            ctk_widget_show (property_image);

            /* Keep track of ERASE objects to place them prominently later */
            if (property_browser->details->category_type == BAUL_PROPERTY_PATTERN
                    && !g_strcmp0 (object_name, RESET_IMAGE_NAME))
            {
                g_assert (reset_object == NULL);
                reset_object = property_image;
            }

            ctk_widget_show (property_image);

            g_free (object_name);
            g_free (object_label);
            if (object_pixbuf != NULL)
            {
                g_object_unref (object_pixbuf);
            }
        }

        property_browser->details->has_local = baul_customization_data_private_data_was_displayed (customization_data);
        baul_customization_data_destroy (customization_data);
    }

    /*
     * We place ERASE objects (for emblems) at the end with a blank in between.
     */
    if (property_browser->details->category_type == BAUL_PROPERTY_EMBLEM)
    {
        CtkWidget *blank;
        char *path;

        blank = eel_image_table_add_empty_image (image_table);
        labeled_image_configure (EEL_LABELED_IMAGE (blank));


        num_images = eel_wrap_table_get_num_children (EEL_WRAP_TABLE (image_table));
        g_assert (num_images > 0);
        eel_wrap_table_reorder_child (EEL_WRAP_TABLE (image_table),
                                      blank,
                                      num_images - 1);

        ctk_widget_show (blank);

        object_pixbuf = NULL;

        path = baul_pixmap_file (ERASE_OBJECT_NAME);
        if (path != NULL)
        {
            object_pixbuf = gdk_pixbuf_new_from_file (path, NULL);
        }
        g_free (path);
        property_image = labeled_image_new (_("Erase"), object_pixbuf, "erase", PANGO_SCALE_LARGE);
        eel_labeled_image_set_fixed_image_height (EEL_LABELED_IMAGE (property_image), MAX_EMBLEM_HEIGHT);

        ctk_container_add (CTK_CONTAINER (image_table), property_image);
        ctk_widget_show (property_image);

        eel_wrap_table_reorder_child (EEL_WRAP_TABLE (image_table),
                                      property_image, -1);

        if (object_pixbuf != NULL)
        {
            g_object_unref (object_pixbuf);
        }
    }

    /*
     * We place RESET objects (for colors and patterns) at the beginning.
     */
    if (reset_object != NULL)
    {
        g_assert (EEL_IS_LABELED_IMAGE (reset_object));
        eel_wrap_table_reorder_child (EEL_WRAP_TABLE (image_table),
                                      reset_object,
                                      0);
    }

}

/* utility routine to add a reset property in the first position */
static void
add_reset_property (BaulPropertyBrowser *property_browser)
{
    char *reset_image_file_name;
    CtkWidget *reset_image;
    CdkPixbuf *reset_pixbuf, *reset_chit;

    reset_chit = NULL;

    reset_image_file_name = g_strdup_printf ("%s/%s/%s", BAUL_DATADIR, "patterns", RESET_IMAGE_NAME);
    reset_pixbuf = gdk_pixbuf_new_from_file (reset_image_file_name, NULL);
    if (reset_pixbuf != NULL && property_browser->details->property_chit != NULL)
    {
        reset_chit = baul_customization_make_pattern_chit (reset_pixbuf, property_browser->details->property_chit, FALSE, TRUE);
    }

    g_free (reset_image_file_name);

    reset_image = labeled_image_new (_("Reset"), reset_chit != NULL ? reset_chit : reset_pixbuf, RESET_IMAGE_NAME, PANGO_SCALE_MEDIUM);
    ctk_container_add (CTK_CONTAINER (property_browser->details->content_table), reset_image);
    eel_wrap_table_reorder_child (EEL_WRAP_TABLE (property_browser->details->content_table),
                                  reset_image,
                                  0);
    ctk_widget_show (reset_image);

    if (reset_pixbuf != NULL)
    {
        g_object_unref (reset_pixbuf);
    }

    if (reset_chit != NULL)
    {
        g_object_unref (reset_chit);
    }
}

/* generate properties from the children of the passed in node */
/* for now, we just handle color nodes */

static void
make_properties_from_xml_node (BaulPropertyBrowser *property_browser,
                               xmlNodePtr node)
{
    xmlNodePtr child_node;
    CdkPixbuf *pixbuf;
    CtkWidget *new_property;
    char *deleted, *local, *color, *name;

    gboolean local_only = property_browser->details->remove_mode;

    /* add a reset property in the first slot */
    if (!property_browser->details->remove_mode)
    {
        add_reset_property (property_browser);
    }

    property_browser->details->has_local = FALSE;

    for (child_node = eel_xml_get_children (node);
            child_node != NULL;
            child_node = child_node->next)
    {

        if (child_node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        /* We used to mark colors that were removed with the "deleted" attribute.
         * To prevent these colors from suddenly showing up now, this legacy remains.
         */
        deleted = xmlGetProp (child_node, "deleted");
        local = xmlGetProp (child_node, "local");

        if (deleted == NULL && (!local_only || local != NULL))
        {
            if (local != NULL)
            {
                property_browser->details->has_local = TRUE;
            }

            color = xmlNodeGetContent (child_node);
            name = eel_xml_get_property_translated (child_node, "name");

            /* make the image from the color spec */
            pixbuf = make_color_drag_image (property_browser, color, FALSE);

            /* make the tile from the pixmap and name */
            new_property = labeled_image_new (name, pixbuf, color, PANGO_SCALE_LARGE);

            ctk_container_add (CTK_CONTAINER (property_browser->details->content_table), new_property);
            ctk_widget_show (new_property);

            g_object_unref (pixbuf);
            xmlFree (color);
            xmlFree (name);
        }

        xmlFree (local);
        xmlFree (deleted);
    }
}

/* make_category generates widgets corresponding all of the objects in the passed in directory */
static void
make_category(BaulPropertyBrowser *property_browser, const char* path, const char* mode, xmlNodePtr node, const char *description)
{

    /* set up the description in the help label */
    ctk_label_set_text (CTK_LABEL (property_browser->details->help_label), description);

    /* case out on the mode */
    if (strcmp (mode, "directory") == 0)
        make_properties_from_directories (property_browser);
    else if (strcmp (mode, "inline") == 0)
        make_properties_from_xml_node (property_browser, node);

}

/* Create a category button */
static CtkWidget *
property_browser_category_button_new (const char *display_name,
                                      const char *image)
{
    CtkWidget *button;
    char *file_name;

    g_return_val_if_fail (display_name != NULL, NULL);
    g_return_val_if_fail (image != NULL, NULL);

    file_name = baul_pixmap_file (image);
    if (file_name != NULL)
    {
        button = eel_labeled_image_radio_button_new_from_file_name (display_name, file_name);
    }
    else
    {
        button = eel_labeled_image_radio_button_new (display_name, NULL);
    }

    ctk_toggle_button_set_mode (CTK_TOGGLE_BUTTON (button), FALSE);

    /* We also want all of the buttons to be the same height */
    eel_labeled_image_set_fixed_image_height (EEL_LABELED_IMAGE (ctk_bin_get_child (CTK_BIN (button))), STANDARD_BUTTON_IMAGE_HEIGHT);

    g_free (file_name);

    return button;
}

/* this is a utility routine to generate a category link widget and install it in the browser */
static void
make_category_link (BaulPropertyBrowser *property_browser,
                    const char *name,
                    const char *display_name,
                    const char *image,
                    CtkRadioButton **group)
{
    CtkWidget *button;

    g_return_if_fail (name != NULL);
    g_return_if_fail (image != NULL);
    g_return_if_fail (display_name != NULL);
    g_return_if_fail (BAUL_IS_PROPERTY_BROWSER (property_browser));

    button = property_browser_category_button_new (display_name, image);
    ctk_widget_show (button);

    if (*group)
    {
        ctk_radio_button_set_group (CTK_RADIO_BUTTON (button),
                                    ctk_radio_button_get_group (*group));
    }
    else
    {
        *group = CTK_RADIO_BUTTON (button);
    }

    /* if the button represents the current category, highlight it */
    if (property_browser->details->category &&
            strcmp (property_browser->details->category, name) == 0)
        ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);

    /* Place it in the category box */
    ctk_box_pack_start (CTK_BOX (property_browser->details->category_box),
                        button, FALSE, FALSE, 0);

    property_browser->details->category_position += 1;

    /* add a signal to handle clicks */
    g_object_set_data (G_OBJECT(button), "user_data", property_browser);
    g_signal_connect_data
    (button, "toggled",
     G_CALLBACK (category_toggled_callback),
     g_strdup (name), (GClosureNotify) g_free, 0);
}

/* update_contents populates the property browser with information specified by the path and other state variables */
void
baul_property_browser_update_contents (BaulPropertyBrowser *property_browser)
{
    xmlNodePtr cur_node;
    xmlDocPtr document;
    CtkWidget *viewport;
    CtkRadioButton *group;
    gboolean got_categories;
    char *name, *image, *type, *description, *display_name, *path, *mode;

    /* load the xml document corresponding to the path and selection */
    document = read_browser_xml (property_browser);
    if (document == NULL)
    {
        return;
    }

    /* remove the existing content box, if any, and allocate a new one */
    if (property_browser->details->content_frame)
    {
        ctk_widget_destroy(property_browser->details->content_frame);
    }

    /* allocate a new container, with a scrollwindow and viewport */
    property_browser->details->content_frame = ctk_scrolled_window_new (NULL, NULL);
    ctk_widget_set_vexpand (property_browser->details->content_frame, TRUE);
    ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (property_browser->details->content_frame),
                                         CTK_SHADOW_IN);
    viewport = ctk_viewport_new (NULL, NULL);
    ctk_widget_show(viewport);
    ctk_viewport_set_shadow_type(CTK_VIEWPORT(viewport), CTK_SHADOW_IN);
    ctk_container_add (CTK_CONTAINER (property_browser->details->content_container), property_browser->details->content_frame);
    ctk_widget_show (property_browser->details->content_frame);
    ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (property_browser->details->content_frame),
                                    CTK_POLICY_NEVER, CTK_POLICY_AUTOMATIC);
    ctk_scrolled_window_set_overlay_scrolling (CTK_SCROLLED_WINDOW (property_browser->details->content_frame),
                                               FALSE);

    /* allocate a table to hold the content widgets */
    property_browser->details->content_table = eel_image_table_new (TRUE);
    eel_wrap_table_set_x_spacing (EEL_WRAP_TABLE (property_browser->details->content_table),
                                  IMAGE_TABLE_X_SPACING);
    eel_wrap_table_set_y_spacing (EEL_WRAP_TABLE (property_browser->details->content_table),
                                  IMAGE_TABLE_Y_SPACING);

    g_signal_connect_object (property_browser->details->content_table, "child_pressed",
                             G_CALLBACK (element_clicked_callback), property_browser, 0);

    ctk_container_add(CTK_CONTAINER(viewport), property_browser->details->content_table);
    ctk_container_add (CTK_CONTAINER (property_browser->details->content_frame), viewport);
    ctk_widget_show (CTK_WIDGET (property_browser->details->content_table));

    /* iterate through the xml file to generate the widgets */
    got_categories = property_browser->details->category_position >= 0;
    if (!got_categories)
    {
        property_browser->details->category_position = 0;
    }

    group = NULL;
    for (cur_node = eel_xml_get_children (xmlDocGetRootElement (document));
            cur_node != NULL;
            cur_node = cur_node->next)
    {

        if (cur_node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if (strcmp (cur_node->name, "category") == 0)
        {
            name = xmlGetProp (cur_node, "name");

            if (property_browser->details->category != NULL
                    && strcmp (property_browser->details->category, name) == 0)
            {
                path = xmlGetProp (cur_node, "path");
                mode = xmlGetProp (cur_node, "mode");
                description = eel_xml_get_property_translated (cur_node, "description");
                type = xmlGetProp (cur_node, "type");

                make_category (property_browser,
                               path,
                               mode,
                               cur_node,
                               description);
                baul_property_browser_set_drag_type (property_browser, type);

                xmlFree (path);
                xmlFree (mode);
                xmlFree (description);
                xmlFree (type);
            }

            if (!got_categories)
            {
                display_name = eel_xml_get_property_translated (cur_node, "display_name");
                image = xmlGetProp (cur_node, "image");

                make_category_link (property_browser,
                                    name,
                                    display_name,
                                    image,
                                    &group);

                xmlFree (display_name);
                xmlFree (image);
            }

            xmlFree (name);
        }
    }

    /* release the  xml document and we're done */
    xmlFreeDoc (document);

    /* update the title and button */

    if (property_browser->details->category == NULL)
    {
        ctk_label_set_text (CTK_LABEL (property_browser->details->title_label), _("Select a Category:"));
        ctk_widget_hide(property_browser->details->add_button);
        ctk_widget_hide(property_browser->details->remove_button);

    }
    else
    {
        const char *text;
        char *label_text;
        char *icon_name;

        if (property_browser->details->remove_mode)
        {
            icon_name = "process-stop";
            text = _("C_ancel Remove");
        }
        else
        {
            icon_name = "list-add";
            /* FIXME: Using spaces to add padding is not good design. */
            switch (property_browser->details->category_type)
            {
            case BAUL_PROPERTY_PATTERN:
                text = _("_Add a New Pattern...");
                break;
            case BAUL_PROPERTY_COLOR:
                text = _("_Add a New Color...");
                break;
            case BAUL_PROPERTY_EMBLEM:
                text = _("_Add a New Emblem...");
                break;
            default:
                text = NULL;
                break;
            }
        }

        /* enable the "add new" button and update it's name and icon */
        ctk_image_set_from_icon_name (CTK_IMAGE(property_browser->details->add_button_image), icon_name,
                                      CTK_ICON_SIZE_BUTTON);

        if (text != NULL)
        {
            ctk_button_set_label (CTK_BUTTON (property_browser->details->add_button), text);

        }
        ctk_widget_show (property_browser->details->add_button);


        if (property_browser->details->remove_mode)
        {

            switch (property_browser->details->category_type)
            {
            case BAUL_PROPERTY_PATTERN:
                label_text = g_strdup (_("Click on a pattern to remove it"));
                break;
            case BAUL_PROPERTY_COLOR:
                label_text = g_strdup (_("Click on a color to remove it"));
                break;
            case BAUL_PROPERTY_EMBLEM:
                label_text = g_strdup (_("Click on an emblem to remove it"));
                break;
            default:
                label_text = NULL;
                break;
            }
        }
        else
        {
            switch (property_browser->details->category_type)
            {
            case BAUL_PROPERTY_PATTERN:
                label_text = g_strdup (_("Patterns:"));
                break;
            case BAUL_PROPERTY_COLOR:
                label_text = g_strdup (_("Colors:"));
                break;
            case BAUL_PROPERTY_EMBLEM:
                label_text = g_strdup (_("Emblems:"));
                break;
            default:
                label_text = NULL;
                break;
            }
        }

        if (label_text)
        {
            ctk_label_set_text_with_mnemonic
            (CTK_LABEL (property_browser->details->title_label), label_text);
        }
        g_free(label_text);

        /* enable the remove button (if necessary) and update its name */

        /* case out instead of substituting to provide flexibilty for other languages */
        /* FIXME: Using spaces to add padding is not good design. */
        switch (property_browser->details->category_type)
        {
        case BAUL_PROPERTY_PATTERN:
            text = _("_Remove a Pattern...");
            break;
        case BAUL_PROPERTY_COLOR:
            text = _("_Remove a Color...");
            break;
        case BAUL_PROPERTY_EMBLEM:
            text = _("_Remove an Emblem...");
            break;
        default:
            text = NULL;
            break;
        }

        if (property_browser->details->remove_mode
                || !property_browser->details->has_local)
            ctk_widget_hide(property_browser->details->remove_button);
        else
            ctk_widget_show(property_browser->details->remove_button);
        if (text != NULL)
        {
            ctk_button_set_label (CTK_BUTTON (property_browser->details->remove_button), text);
        }
    }
}

/* set the category and regenerate contents as necessary */

static void
baul_property_browser_set_category (BaulPropertyBrowser *property_browser,
                                    const char *new_category)
{
    /* there's nothing to do if the category is the same as the current one */
    if (g_strcmp0 (property_browser->details->category, new_category) == 0)
    {
        return;
    }

    g_free (property_browser->details->category);
    property_browser->details->category = g_strdup (new_category);

    /* set up the property type enum */
    if (g_strcmp0 (new_category, "patterns") == 0)
    {
        property_browser->details->category_type = BAUL_PROPERTY_PATTERN;
    }
    else if (g_strcmp0 (new_category, "colors") == 0)
    {
        property_browser->details->category_type = BAUL_PROPERTY_COLOR;
    }
    else if (g_strcmp0 (new_category, "emblems") == 0)
    {
        property_browser->details->category_type = BAUL_PROPERTY_EMBLEM;
    }
    else
    {
        property_browser->details->category_type = BAUL_PROPERTY_NONE;
    }

    /* populate the per-uri box with the info */
    baul_property_browser_update_contents (property_browser);
}


/* here is the routine that populates the property browser with the appropriate information
   when the path changes */

void
baul_property_browser_set_path (BaulPropertyBrowser *property_browser,
                                const char *new_path)
{
    /* there's nothing to do if the uri is the same as the current one */
    if (g_strcmp0 (property_browser->details->path, new_path) == 0)
    {
        return;
    }

    g_free (property_browser->details->path);
    property_browser->details->path = g_strdup (new_path);

    /* populate the per-uri box with the info */
    baul_property_browser_update_contents (property_browser);
}

static void
emblems_changed_callback (GObject *signaller,
                          BaulPropertyBrowser *property_browser)
{
    baul_property_browser_update_contents (property_browser);
}


static void
emit_emblems_changed_signal (void)
{
    g_signal_emit_by_name (baul_signaller_get_current (), "emblems_changed");
}
