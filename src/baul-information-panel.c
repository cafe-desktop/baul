/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 1999, 2000, 2001 Eazel, Inc.
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
 *
 */

#include <config.h>

#include <cdk-pixbuf/cdk-pixbuf.h>
#include <ctk/ctk.h>
#include <glib/gi18n.h>

#include <eel/eel-background.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-ctk-extensions.h>
#include <eel/eel-stock-dialogs.h>
#include <eel/eel-string.h>
#include <eel/eel-vfs-extensions.h>

#include <libbaul-private/baul-dnd.h>
#include <libbaul-private/baul-directory.h>
#include <libbaul-private/baul-file-dnd.h>
#include <libbaul-private/baul-file.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-keep-last-vertical-box.h>
#include <libbaul-private/baul-metadata.h>
#include <libbaul-private/baul-mime-actions.h>
#include <libbaul-private/baul-program-choosing.h>
#include <libbaul-private/baul-sidebar-provider.h>
#include <libbaul-private/baul-module.h>

#include "baul-information-panel.h"
#include "baul-sidebar-title.h"

struct _BaulInformationPanelPrivate
{
    CtkWidget *container;
    BaulWindowInfo *window;
    BaulSidebarTitle *title;
    CtkWidget *button_box_centerer;
    CtkWidget *button_box;
    gboolean has_buttons;
    BaulFile *file;
    guint file_changed_connection;
    gboolean background_connected;

    char *default_background_color;
    char *default_background_image;
    char *current_background_color;
    char *current_background_image;
};

/* button assignments */
#define CONTEXTUAL_MENU_BUTTON 3

static gboolean baul_information_panel_press_event           (CtkWidget                    *widget,
        GdkEventButton               *event);
static void     baul_information_panel_finalize              (GObject                      *object);
static void     baul_information_panel_drag_data_received    (CtkWidget                    *widget,
        GdkDragContext               *context,
        int                           x,
        int                           y,
        CtkSelectionData             *selection_data,
        guint                         info,
        guint                         time);
static void     baul_information_panel_read_defaults         (BaulInformationPanel     *information_panel);
static void     baul_information_panel_style_updated         (CtkWidget                    *widget);
static void     baul_information_panel_theme_changed         (GSettings   *settings,
                                                              const gchar *key,
                                                              gpointer     user_data);
static void     baul_information_panel_update_appearance     (BaulInformationPanel     *information_panel);
static void     baul_information_panel_update_buttons        (BaulInformationPanel     *information_panel);
static void     background_metadata_changed_callback             (BaulInformationPanel     *information_panel);
static void     baul_information_panel_iface_init            (BaulSidebarIface         *iface);
static void     sidebar_provider_iface_init                      (BaulSidebarProviderIface *iface);
static GType    baul_information_panel_provider_get_type     (void);

enum
{
    LOCATION_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* drag and drop definitions */

enum
{
    TARGET_URI_LIST,
    TARGET_COLOR,
    TARGET_BGIMAGE,
    TARGET_KEYWORD,
    TARGET_BACKGROUND_RESET,
    TARGET_CAFE_URI_LIST
};

static const CtkTargetEntry target_table[] =
{
    { "text/uri-list",  0, TARGET_URI_LIST },
    { "application/x-color", 0, TARGET_COLOR },
    { "property/bgimage", 0, TARGET_BGIMAGE },
    { "property/keyword", 0, TARGET_KEYWORD },
    { "x-special/cafe-reset-background", 0, TARGET_BACKGROUND_RESET },
    { "x-special/cafe-icon-list",  0, TARGET_CAFE_URI_LIST }
};

typedef enum
{
    NO_PART,
    BACKGROUND_PART,
    ICON_PART
} InformationPanelPart;

typedef struct
{
    GObject parent;
} BaulInformationPanelProvider;

typedef struct
{
    GObjectClass parent;
} BaulInformationPanelProviderClass;


G_DEFINE_TYPE_WITH_CODE (BaulInformationPanel, baul_information_panel, EEL_TYPE_BACKGROUND_BOX,
                         G_ADD_PRIVATE (BaulInformationPanel)
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_SIDEBAR,
                                 baul_information_panel_iface_init));

G_DEFINE_TYPE_WITH_CODE (BaulInformationPanelProvider, baul_information_panel_provider, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_SIDEBAR_PROVIDER,
                                 sidebar_provider_iface_init));


static const char *
baul_information_panel_get_sidebar_id (BaulSidebar *sidebar)
{
    return BAUL_INFORMATION_PANEL_ID;
}

static char *
baul_information_panel_get_tab_label (BaulSidebar *sidebar)
{
    return g_strdup (_("Information"));
}

static char *
baul_information_panel_get_tab_tooltip (BaulSidebar *sidebar)
{
    return g_strdup (_("Show Information"));
}

static GdkPixbuf *
baul_information_panel_get_tab_icon (BaulSidebar *sidebar)
{
    return NULL;
}

static void
baul_information_panel_is_visible_changed (BaulSidebar *sidebar,
        gboolean         is_visible)
{
    /* Do nothing */
}

static void
baul_information_panel_iface_init (BaulSidebarIface *iface)
{
    iface->get_sidebar_id = baul_information_panel_get_sidebar_id;
    iface->get_tab_label = baul_information_panel_get_tab_label;
    iface->get_tab_tooltip = baul_information_panel_get_tab_tooltip;
    iface->get_tab_icon = baul_information_panel_get_tab_icon;
    iface->is_visible_changed = baul_information_panel_is_visible_changed;
}

/* initializing the class object by installing the operations we override */
static void
baul_information_panel_class_init (BaulInformationPanelClass *klass)
{
    CtkWidgetClass *widget_class;
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    widget_class = CTK_WIDGET_CLASS (klass);

    gobject_class->finalize = baul_information_panel_finalize;

    widget_class->drag_data_received  = baul_information_panel_drag_data_received;
    widget_class->button_press_event  = baul_information_panel_press_event;
    widget_class->style_updated = baul_information_panel_style_updated;

    /* add the "location changed" signal */
    signals[LOCATION_CHANGED] = g_signal_new
                                ("location_changed",
                                 G_TYPE_FROM_CLASS (klass),
                                 G_SIGNAL_RUN_LAST,
                                 G_STRUCT_OFFSET (BaulInformationPanelClass,
                                         location_changed),
                                 NULL, NULL,
                                 g_cclosure_marshal_VOID__STRING,
                                 G_TYPE_NONE, 1, G_TYPE_STRING);
}

/* utility routine to allocate the box the holds the command buttons */
static void
make_button_box (BaulInformationPanel *information_panel)
{
    information_panel->details->button_box_centerer = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);

    ctk_box_pack_start (CTK_BOX (information_panel->details->container),
                        information_panel->details->button_box_centerer, TRUE, TRUE, 0);

    information_panel->details->button_box = baul_keep_last_vertical_box_new (4);
    ctk_container_set_border_width (CTK_CONTAINER (information_panel->details->button_box), 8);
    ctk_widget_show (information_panel->details->button_box);
    ctk_box_pack_start (CTK_BOX (information_panel->details->button_box_centerer),
                        information_panel->details->button_box,
                        TRUE, TRUE, 0);
    information_panel->details->has_buttons = FALSE;
}

/* initialize the instance's fields, create the necessary subviews, etc. */

static void
baul_information_panel_init (BaulInformationPanel *information_panel)
{
    information_panel->details = baul_information_panel_get_instance_private (information_panel);

    /* load the default background */
    baul_information_panel_read_defaults (information_panel);

    /* enable mouse tracking */
    ctk_widget_add_events (CTK_WIDGET (information_panel), GDK_POINTER_MOTION_MASK);

    /* create the container box */
    information_panel->details->container = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
    ctk_container_set_border_width (CTK_CONTAINER (information_panel->details->container), 0);
    ctk_widget_show (information_panel->details->container);
    ctk_container_add (CTK_CONTAINER (information_panel),
                       information_panel->details->container);

    /* allocate and install the index title widget */
    information_panel->details->title = BAUL_SIDEBAR_TITLE (baul_sidebar_title_new ());
    ctk_widget_show (CTK_WIDGET (information_panel->details->title));
    ctk_box_pack_start (CTK_BOX (information_panel->details->container),
                        CTK_WIDGET (information_panel->details->title),
                        FALSE, FALSE, 8);

    /* allocate and install the command button container */
    make_button_box (information_panel);

    /* add a callback for when the theme changes */
    g_signal_connect (baul_preferences,
              "changed::" BAUL_PREFERENCES_SIDE_PANE_BACKGROUND_SET,
              G_CALLBACK(baul_information_panel_theme_changed),
              information_panel);
    g_signal_connect (baul_preferences,
              "changed::" BAUL_PREFERENCES_SIDE_PANE_BACKGROUND_COLOR,
              G_CALLBACK(baul_information_panel_theme_changed),
              information_panel);
    g_signal_connect (baul_preferences,
              "changed::" BAUL_PREFERENCES_SIDE_PANE_BACKGROUND_URI,
              G_CALLBACK(baul_information_panel_theme_changed),
              information_panel);

    /* prepare ourselves to receive dropped objects */
    ctk_drag_dest_set (CTK_WIDGET (information_panel),
                       CTK_DEST_DEFAULT_MOTION | CTK_DEST_DEFAULT_HIGHLIGHT | CTK_DEST_DEFAULT_DROP,
                       target_table, G_N_ELEMENTS (target_table),
                       GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK);
}

static void
baul_information_panel_finalize (GObject *object)
{
    BaulInformationPanel *information_panel;

    information_panel = BAUL_INFORMATION_PANEL (object);

    if (information_panel->details->file != NULL)
    {
        baul_file_monitor_remove (information_panel->details->file, information_panel);
        baul_file_unref (information_panel->details->file);
    }

    g_free (information_panel->details->default_background_color);
    g_free (information_panel->details->default_background_image);
    g_free (information_panel->details->current_background_color);
    g_free (information_panel->details->current_background_image);

    g_signal_handlers_disconnect_by_func (baul_preferences,
                                          baul_information_panel_theme_changed,
                                          information_panel);

    G_OBJECT_CLASS (baul_information_panel_parent_class)->finalize (object);
}

/* callback to handle resetting the background */
static void
reset_background_callback (CtkWidget *menu_item, CtkWidget *information_panel)
{
    EelBackground *background;

    background = eel_get_widget_background (information_panel);
    if (background != NULL)
    {
        eel_background_reset (background);
    }
}

static gboolean
information_panel_has_background (BaulInformationPanel *information_panel)
{
    EelBackground *background;
    gboolean has_background;
    char *color;
    char *image;

    background = eel_get_widget_background (CTK_WIDGET(information_panel));

    color = eel_background_get_color (background);
    image = eel_background_get_image_uri (background);

    has_background = (color || image);

    return has_background;
}

/* create the context menu */
static CtkWidget *
baul_information_panel_create_context_menu (BaulInformationPanel *information_panel)
{
    CtkWidget *menu, *menu_item;

    menu = ctk_menu_new ();
    ctk_menu_set_screen (CTK_MENU (menu),
                         ctk_widget_get_screen (CTK_WIDGET (information_panel)));

    /* add the reset background item, possibly disabled */
    menu_item = ctk_menu_item_new_with_mnemonic (_("Use _Default Background"));
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (CTK_MENU_SHELL (menu), menu_item);
    ctk_widget_set_sensitive (menu_item, information_panel_has_background (information_panel));
    g_signal_connect_object (menu_item, "activate",
                             G_CALLBACK (reset_background_callback), information_panel, 0);

    return menu;
}

/* set up the default backgrounds and images */
static void
baul_information_panel_read_defaults (BaulInformationPanel *information_panel)
{
    gboolean background_set;
    char *background_color, *background_image;

    background_set = g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_SIDE_PANE_BACKGROUND_SET);

    background_color = NULL;
    background_image = NULL;
    if (background_set)
    {
        background_color = g_settings_get_string (baul_preferences, BAUL_PREFERENCES_SIDE_PANE_BACKGROUND_COLOR);
        background_image = g_settings_get_string (baul_preferences, BAUL_PREFERENCES_SIDE_PANE_BACKGROUND_URI);
    }

    g_free (information_panel->details->default_background_color);
    information_panel->details->default_background_color = NULL;
    g_free (information_panel->details->default_background_image);
    information_panel->details->default_background_image = NULL;

    if (background_color && strlen (background_color))
    {
        information_panel->details->default_background_color = g_strdup (background_color);
    }

    /* set up the default background image */

    if (background_image && strlen (background_image))
    {
        information_panel->details->default_background_image = g_strdup (background_image);
    }

    g_free (background_color);
    g_free (background_image);
}

/* handler for handling theme changes */

static void
baul_information_panel_theme_changed (GSettings   *settings,
                                      const gchar *key,
                                      gpointer user_data)
{
    BaulInformationPanel *information_panel;

    information_panel = BAUL_INFORMATION_PANEL (user_data);
    baul_information_panel_read_defaults (information_panel);
    baul_information_panel_update_appearance (information_panel);
    ctk_widget_queue_draw (CTK_WIDGET (information_panel)) ;
}

/* hit testing */

static InformationPanelPart
hit_test (BaulInformationPanel *information_panel,
          int x, int y)
{
    CtkAllocation *allocation;
    gboolean bg_hit;

    if (baul_sidebar_title_hit_test_icon (information_panel->details->title, x, y))
    {
        return ICON_PART;
    }

    allocation = g_new0 (CtkAllocation, 1);
    ctk_widget_get_allocation (CTK_WIDGET (information_panel), allocation);

    bg_hit = allocation != NULL
             && x >= allocation->x && y >= allocation->y
             && x < allocation->x + allocation->width
             && y < allocation->y + allocation->height;
    g_free (allocation);

    if (bg_hit)
    {
        return BACKGROUND_PART;
    }

    return NO_PART;
}

/* utility to test if a uri refers to a local image */
static gboolean
uri_is_local_image (const char *uri)
{
    GdkPixbuf *pixbuf;
    char *image_path;

    image_path = g_filename_from_uri (uri, NULL, NULL);
    if (image_path == NULL)
    {
        return FALSE;
    }

    pixbuf = cdk_pixbuf_new_from_file (image_path, NULL);
    g_free (image_path);

    if (pixbuf == NULL)
    {
        return FALSE;
    }
    g_object_unref (pixbuf);
    return TRUE;
}

static void
receive_dropped_uri_list (BaulInformationPanel *information_panel,
                          GdkDragAction action,
                          int x, int y,
                          CtkSelectionData *selection_data)
{
    char **uris;
    gboolean exactly_one;
    CtkWindow *window;

    uris = g_uri_list_extract_uris ((gchar *) ctk_selection_data_get_data (selection_data));
    exactly_one = uris[0] != NULL && (uris[1] == NULL || uris[1][0] == '\0');
    window = CTK_WINDOW (ctk_widget_get_toplevel (CTK_WIDGET (information_panel)));

    switch (hit_test (information_panel, x, y))
    {
    case NO_PART:
    case BACKGROUND_PART:
        /* FIXME bugzilla.gnome.org 42507: Does this work for all images, or only background images?
         * Other views handle background images differently from other URIs.
         */
        if (exactly_one && uri_is_local_image (uris[0]))
        {
            if (action == GDK_ACTION_ASK)
            {
                action = baul_drag_drop_background_ask (CTK_WIDGET (information_panel),
                             BAUL_DND_ACTION_SET_AS_BACKGROUND | BAUL_DND_ACTION_SET_AS_GLOBAL_BACKGROUND);
            }

            if (action > 0)
            {
                EelBackground *background;

                background = eel_get_widget_background (CTK_WIDGET (information_panel));
                eel_background_set_dropped_image (background, action, uris[0]);
            }
        }
        else if (exactly_one)
        {
            g_signal_emit (information_panel, signals[LOCATION_CHANGED], 0, uris[0]);
        }
        break;
    case ICON_PART:
        /* handle images dropped on the logo specially */

        if (!exactly_one)
        {
            eel_show_error_dialog (
                _("You cannot assign more than one custom icon at a time."),
                _("Please drag just one image to set a custom icon."),
                window);
            break;
        }

        if (uri_is_local_image (uris[0]))
        {
            if (information_panel->details->file != NULL)
            {
                baul_file_set_metadata (information_panel->details->file,
                                        BAUL_METADATA_KEY_CUSTOM_ICON,
                                        NULL,
                                        uris[0]);
                baul_file_set_metadata (information_panel->details->file,
                                        BAUL_METADATA_KEY_ICON_SCALE,
                                        NULL,
                                        NULL);
            }
        }
        else
        {
            GFile *f;

            f = g_file_new_for_uri (uris[0]);
            if (!g_file_is_native (f))
            {
                eel_show_error_dialog (
                    _("The file that you dropped is not local."),
                    _("You can only use local images as custom icons."),
                    window);

            }
            else
            {
                eel_show_error_dialog (
                    _("The file that you dropped is not an image."),
                    _("You can only use images as custom icons."),
                    window);
            }
            g_object_unref (f);
        }
        break;
    }

    g_strfreev (uris);
}

static void
receive_dropped_color (BaulInformationPanel *information_panel,
                       GdkDragAction action,
                       int x, int y,
                       CtkSelectionData *selection_data)
{
    guint16 *channels;
    char color_spec[8];

    if (ctk_selection_data_get_length (selection_data) != 8 ||
            ctk_selection_data_get_format (selection_data) != 16)
    {
        g_warning ("received invalid color data");
        return;
    }

    channels = (guint16 *) ctk_selection_data_get_data (selection_data);
    g_snprintf (color_spec, sizeof (color_spec),
                "#%02X%02X%02X", channels[0] >> 8, channels[1] >> 8, channels[2] >> 8);

    switch (hit_test (information_panel, x, y))
    {
    case NO_PART:
        g_warning ("dropped color, but not on any part of information_panel");
        break;
    case ICON_PART:
    case BACKGROUND_PART:
        if (action == GDK_ACTION_ASK)
        {
            action = baul_drag_drop_background_ask (CTK_WIDGET (information_panel),
                         BAUL_DND_ACTION_SET_AS_BACKGROUND | BAUL_DND_ACTION_SET_AS_GLOBAL_BACKGROUND);
        }

        if (action > 0)
        {
            EelBackground *background;

            background = eel_get_widget_background (CTK_WIDGET (information_panel));
            eel_background_set_dropped_color (background, CTK_WIDGET (information_panel),
                                              action, x, y, selection_data);
        }

        break;
    }
}

/* handle receiving a dropped keyword */

static void
receive_dropped_keyword (BaulInformationPanel *information_panel,
                         int x, int y,
                         CtkSelectionData *selection_data)
{
    baul_drag_file_receive_dropped_keyword (information_panel->details->file,
                                            ctk_selection_data_get_data (selection_data));

    /* regenerate the display */
    baul_information_panel_update_appearance (information_panel);
}

static void
baul_information_panel_drag_data_received (CtkWidget *widget, GdkDragContext *context,
        int x, int y,
        CtkSelectionData *selection_data,
        guint info, guint time)
{
    BaulInformationPanel *information_panel;
    EelBackground *background;

    g_return_if_fail (BAUL_IS_INFORMATION_PANEL (widget));

    information_panel = BAUL_INFORMATION_PANEL (widget);

    switch (info)
    {
    case TARGET_CAFE_URI_LIST:
    case TARGET_URI_LIST:
        receive_dropped_uri_list (information_panel,
                                  cdk_drag_context_get_selected_action (context), x, y, selection_data);
        break;
    case TARGET_COLOR:
        receive_dropped_color (information_panel,
                               cdk_drag_context_get_selected_action (context), x, y, selection_data);
        break;
    case TARGET_BGIMAGE:
        if (hit_test (information_panel, x, y) == BACKGROUND_PART)
            receive_dropped_uri_list (information_panel,
                                      cdk_drag_context_get_selected_action (context), x, y, selection_data);
        break;
    case TARGET_BACKGROUND_RESET:
        background = eel_get_widget_background ( CTK_WIDGET (information_panel));
        if (background != NULL)
        {
            eel_background_reset (background);
        }
        break;
    case TARGET_KEYWORD:
        receive_dropped_keyword (information_panel, x, y, selection_data);
        break;
    default:
        g_warning ("unknown drop type");
    }
}

/* handle the context menu if necessary */
static gboolean
baul_information_panel_press_event (CtkWidget *widget, GdkEventButton *event)
{
    BaulInformationPanel *information_panel;

    if (ctk_widget_get_window (widget) != event->window)
    {
        return FALSE;
    }

    information_panel = BAUL_INFORMATION_PANEL (widget);

    /* handle the context menu */
    if (event->button == CONTEXTUAL_MENU_BUTTON)
    {
        CtkWidget *menu;

        menu = baul_information_panel_create_context_menu (information_panel);
        eel_pop_up_context_menu (CTK_MENU(menu),
                                 event);
    }
    return TRUE;
}

static CtkWindow *
baul_information_panel_get_window (BaulInformationPanel *information_panel)
{
    CtkWidget *result;

    result = ctk_widget_get_ancestor (CTK_WIDGET (information_panel), CTK_TYPE_WINDOW);

    return result == NULL ? NULL : CTK_WINDOW (result);
}

static void
command_button_callback (CtkWidget *button, GAppInfo *application)
{
    BaulInformationPanel *information_panel;
    GList files;

    information_panel = BAUL_INFORMATION_PANEL (g_object_get_data (G_OBJECT (button), "user_data"));

    files.next = NULL;
    files.prev = NULL;
    files.data = information_panel->details->file;
    baul_launch_application (application, &files,
                             baul_information_panel_get_window (information_panel));
}

/* interpret commands for buttons specified by metadata. Handle some built-in ones explicitly, or fork
   a shell to handle general ones */
/* for now, we don't have any of these */
static void
metadata_button_callback (CtkWidget *button, const char *command_str)
{
    //BaulInformationPanel *self = BAUL_INFORMATION_PANEL (g_object_get_data (G_OBJECT (button), "user_data"));
}

/* utility routine that allocates the command buttons from the command list */

static void
add_command_button (BaulInformationPanel *information_panel, GAppInfo *application)
{
    char *temp_str;
    CtkWidget *temp_button, *label;

    /* There's always at least the "Open with..." button */
    information_panel->details->has_buttons = TRUE;

    temp_str = g_strdup_printf (_("Open With %s"), g_app_info_get_display_name (application));
    temp_button = ctk_button_new_with_label (temp_str);
    label = ctk_bin_get_child (CTK_BIN (temp_button));
    ctk_label_set_ellipsize (CTK_LABEL (label), PANGO_ELLIPSIZE_START);
    g_free (temp_str);
    ctk_box_pack_start (CTK_BOX (information_panel->details->button_box),
                        temp_button,
                        FALSE, FALSE,
                        0);

    g_signal_connect_data (temp_button,
                           "clicked",
                           G_CALLBACK (command_button_callback),
                           g_object_ref (application),
                           (GClosureNotify)g_object_unref,
                           0);

    g_object_set_data (G_OBJECT (temp_button), "user_data", information_panel);

    ctk_widget_show (temp_button);
}

/* utility to construct command buttons for the information_panel from the passed in metadata string */

static void
add_buttons_from_metadata (BaulInformationPanel *information_panel, const char *button_data)
{
    char **terms;
    char *button_name, *command_string;
    const char *term;
    int index;
    CtkWidget *temp_button;

    /* split the button specification into a set of terms */
    button_name = NULL;
    terms = g_strsplit (button_data, ";", 0);

    /* for each term, either create a button or attach a property to one */
    for (index = 0; (term = terms[index]) != NULL; index++)
    {
        char *current_term, *temp_str;

        current_term = g_strdup (term);
        temp_str = strchr (current_term, '=');
        if (temp_str)
        {
            *temp_str = '\0';
            if (!g_ascii_strcasecmp (current_term, "button"))
            {
                if (button_name)
                    g_free (button_name);

                button_name = g_strdup (temp_str + 1);
            }
            else if (!g_ascii_strcasecmp (current_term, "script"))
            {
                if (button_name != NULL)
                {
                    temp_button = ctk_button_new_with_label (button_name);
                    ctk_box_pack_start (CTK_BOX (information_panel->details->button_box),
                                        temp_button,
                                        FALSE, FALSE,
                                        0);
                    information_panel->details->has_buttons = TRUE;
                    command_string = g_strdup (temp_str + 1);

                    g_signal_connect_data (temp_button,
                                           "clicked",
                                           G_CALLBACK (metadata_button_callback),
                                           command_string,
                                           (GClosureNotify)g_free,
                                           0);

                    g_object_set_data (G_OBJECT (temp_button), "user_data", information_panel);

                    ctk_widget_show (temp_button);
                }
            }
        }
        g_free(current_term);
    }
    g_free (button_name);
    g_strfreev (terms);
}

/*
 * baul_information_panel_update_buttons:
 *
 * Update the list of program-launching buttons based on the current uri.
 */
static void
baul_information_panel_update_buttons (BaulInformationPanel *information_panel)
{
    char *button_data;

    /* dispose of any existing buttons */
    if (information_panel->details->has_buttons)
    {
        ctk_container_remove (CTK_CONTAINER (information_panel->details->container),
                              information_panel->details->button_box_centerer);
        make_button_box (information_panel);
    }

    /* create buttons from file metadata if necessary */
    button_data = baul_file_get_metadata (information_panel->details->file,
                                          BAUL_METADATA_KEY_SIDEBAR_BUTTONS,
                                          NULL);
    if (button_data)
    {
        add_buttons_from_metadata (information_panel, button_data);
        g_free(button_data);
    }

    /* Make a button for the default application */
    if (baul_mime_has_any_applications_for_file (information_panel->details->file) &&
            !baul_file_is_directory (information_panel->details->file))
    {
        GAppInfo *default_app;

        default_app =
            baul_mime_get_default_application_for_file (information_panel->details->file);
        add_command_button (information_panel, default_app);
        g_object_unref (default_app);
    }

    ctk_widget_show (information_panel->details->button_box_centerer);
}

static void
baul_information_panel_update_appearance (BaulInformationPanel *information_panel)
{
    ctk_style_context_add_class (ctk_widget_get_style_context (CTK_WIDGET (information_panel)),
                                 CTK_STYLE_CLASS_VIEW);
}

static void
background_metadata_changed_callback (BaulInformationPanel *information_panel)
{
    BaulFileAttributes attributes;
    gboolean ready;

    attributes = baul_mime_actions_get_required_file_attributes ();
    ready = baul_file_check_if_ready (information_panel->details->file, attributes);

    if (ready)
    {
        baul_information_panel_update_appearance (information_panel);

        /* set up the command buttons */
        baul_information_panel_update_buttons (information_panel);
    }
}

/* here is the key routine that populates the information_panel with the appropriate information when the uri changes */

static void
baul_information_panel_set_uri (BaulInformationPanel *information_panel,
                                const char* new_uri,
                                const char* initial_title)
{
    BaulFile *file;
    BaulFileAttributes attributes;

    g_return_if_fail (BAUL_IS_INFORMATION_PANEL (information_panel));
    g_return_if_fail (new_uri != NULL);
    g_return_if_fail (initial_title != NULL);

    /* there's nothing to do if the uri is the same as the current one */
    if (information_panel->details->file != NULL &&
            baul_file_matches_uri (information_panel->details->file, new_uri))
    {
        return;
    }

    if (information_panel->details->file != NULL)
    {
        g_signal_handler_disconnect (information_panel->details->file,
                                     information_panel->details->file_changed_connection);
        baul_file_monitor_remove (information_panel->details->file, information_panel);
    }

    file = baul_file_get_by_uri (new_uri);

    baul_file_unref (information_panel->details->file);
    information_panel->details->file = file;

    information_panel->details->file_changed_connection =
        g_signal_connect_object (information_panel->details->file, "changed",
                                 G_CALLBACK (background_metadata_changed_callback),
                                 information_panel, G_CONNECT_SWAPPED);

    attributes = baul_mime_actions_get_required_file_attributes ();
    baul_file_monitor_add (information_panel->details->file, information_panel, attributes);

    background_metadata_changed_callback (information_panel);

    /* tell the title widget about it */
    baul_sidebar_title_set_file (information_panel->details->title,
                                 information_panel->details->file,
                                 initial_title);
}

static void
title_changed_callback (BaulWindowInfo *window,
                        char               *new_title,
                        BaulInformationPanel *panel)
{
    baul_sidebar_title_set_text (panel->details->title,
                                 new_title);
}

/* ::style_set handler for the information_panel */
static void
baul_information_panel_style_updated (CtkWidget *widget)
{
    BaulInformationPanel *information_panel;

    information_panel = BAUL_INFORMATION_PANEL (widget);

    baul_information_panel_theme_changed (NULL, NULL, information_panel);
}

static void
loading_uri_callback (BaulWindowInfo *window,
                      char               *uri,
                      BaulInformationPanel *panel)
{
    BaulWindowSlotInfo *slot;
    char *title;

    slot = baul_window_info_get_active_slot (window);

    title = baul_window_slot_info_get_title (slot);
    baul_information_panel_set_uri (panel,
                                    uri,
                                    title);
    g_free (title);
}

static void
selection_changed_callback (BaulWindowInfo *window,
                            BaulInformationPanel *panel)
{
    int selection_count;
    GList *selection;
    char *uri, *name;

    selection = baul_window_info_get_selection (window);
    selection_count = g_list_length (selection);

    if (selection_count == 1)
    {
        GFile *selected;
        BaulFile *file;

        selection = baul_window_info_get_selection (window);
        selected = selection->data;

        /* this should never fail here, as we're displaying the file */
        file = baul_file_get_existing (selected);
        uri = baul_file_get_uri (file);
        name = baul_file_get_display_name (file);

        baul_file_unref (file);
    }
    else
    {
        uri = baul_window_info_get_current_location (window);
        name = baul_window_info_get_title (window);
    }

    baul_information_panel_set_uri (panel, uri, name);

    g_list_free_full (selection, g_object_unref);
    g_free (uri);
    g_free (name);
}

static void
baul_information_panel_set_parent_window (BaulInformationPanel *panel,
        BaulWindowInfo *window)
{
    gpointer slot;
    char *title, *location;

    panel->details->window = window;

    g_signal_connect_object (window, "loading_uri",
                             G_CALLBACK (loading_uri_callback), panel, 0);
    g_signal_connect_object (window, "title_changed",
                             G_CALLBACK (title_changed_callback), panel, 0);
    g_signal_connect_object (window, "selection-changed",
                             G_CALLBACK (selection_changed_callback), panel, 0);

    slot = baul_window_info_get_active_slot (window);

    title = baul_window_slot_info_get_title (slot);
    location = baul_window_slot_info_get_current_location (slot);
    baul_information_panel_set_uri (panel,
                                    location,
                                    title);
    g_free (location);
    g_free (title);
}

static BaulSidebar *
baul_information_panel_create (BaulSidebarProvider *provider,
                               BaulWindowInfo *window)
{
    BaulInformationPanel *panel;

    panel = g_object_new (baul_information_panel_get_type (), NULL);
    baul_information_panel_set_parent_window (panel, window);
    g_object_ref_sink (panel);

    return BAUL_SIDEBAR (panel);
}

static void
sidebar_provider_iface_init (BaulSidebarProviderIface *iface)
{
    iface->create = baul_information_panel_create;
}

static void
baul_information_panel_provider_init (BaulInformationPanelProvider *sidebar)
{
}

static void
baul_information_panel_provider_class_init (BaulInformationPanelProviderClass *class)
{
}

void
baul_information_panel_register (void)
{
    baul_module_add_type (baul_information_panel_provider_get_type ());
}

