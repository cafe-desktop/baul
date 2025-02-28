/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* baul-pathbar.c
 * Copyright (C) 2004  Red Hat, Inc.,  Jonathan Blandford <jrb@gnome.org>
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

#include <ctk/ctk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <libbaul-private/baul-file.h>
#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-icon-names.h>
#include <libbaul-private/baul-trash-monitor.h>
#include <libbaul-private/baul-dnd.h>
#include <libbaul-private/baul-icon-dnd.h>

#include "baul-pathbar.h"

enum
{
    PATH_CLICKED,
    PATH_EVENT,
    LAST_SIGNAL
};

typedef enum
{
    NORMAL_BUTTON,
    ROOT_BUTTON,
    HOME_BUTTON,
    DESKTOP_BUTTON,
    MOUNT_BUTTON,
    DEFAULT_LOCATION_BUTTON,
} ButtonType;

#define BUTTON_DATA(x) ((ButtonData *)(x))

#define SCROLL_TIMEOUT           150
#define INITIAL_SCROLL_TIMEOUT   300

static guint path_bar_signals [LAST_SIGNAL] = { 0 };

static gboolean desktop_is_home;

#define BAUL_PATH_BAR_ICON_SIZE 16

typedef struct _ButtonData ButtonData;

struct _ButtonData
{
    CtkWidget *button;
    ButtonType type;
    char *dir_name;
    GFile *path;
    BaulFile *file;
    unsigned int file_changed_signal_id;

    /* custom icon */
    cairo_surface_t *custom_icon;

    /* flag to indicate its the base folder in the URI */
    gboolean is_base_dir;

    CtkWidget *image;
    CtkWidget *label;
    guint ignore_changes : 1;
    guint file_is_hidden : 1;
    guint fake_root : 1;

    BaulDragSlotProxyInfo drag_info;
};

G_DEFINE_TYPE (BaulPathBar,
               baul_path_bar,
               CTK_TYPE_CONTAINER);

static void     baul_path_bar_finalize                 (GObject         *object);
static void     baul_path_bar_dispose                  (GObject         *object);

static void     baul_path_bar_get_preferred_width      (CtkWidget        *widget,
        						gint             *minimum,
        						gint             *natural);
static void     baul_path_bar_get_preferred_height     (CtkWidget        *widget,
        						gint             *minimum,
        						gint             *natural);

static void     baul_path_bar_unmap                    (CtkWidget       *widget);
static void     baul_path_bar_size_allocate            (CtkWidget       *widget,
        CtkAllocation   *allocation);
static void     baul_path_bar_add                      (CtkContainer    *container,
        CtkWidget       *widget);
static void     baul_path_bar_remove                   (CtkContainer    *container,
        CtkWidget       *widget);
static void     baul_path_bar_forall                   (CtkContainer    *container,
        gboolean         include_internals,
        CtkCallback      callback,
        gpointer         callback_data);
static void     baul_path_bar_scroll_up                (BaulPathBar *path_bar);
static void     baul_path_bar_scroll_down              (BaulPathBar *path_bar);
static gboolean baul_path_bar_scroll                   (CtkWidget       *path_bar,
        CdkEventScroll  *scroll);
static void     baul_path_bar_stop_scrolling           (BaulPathBar *path_bar);
static gboolean baul_path_bar_slider_button_press      (CtkWidget       *widget,
        CdkEventButton  *event,
        BaulPathBar *path_bar);
static gboolean baul_path_bar_slider_button_release    (CtkWidget       *widget,
        CdkEventButton  *event,
        BaulPathBar *path_bar);
static void     baul_path_bar_grab_notify              (CtkWidget       *widget,
        gboolean         was_grabbed);
static void     baul_path_bar_state_changed            (CtkWidget       *widget,
        CtkStateType     previous_state);

static void     baul_path_bar_style_updated            (CtkWidget       *widget);

static void     baul_path_bar_screen_changed           (CtkWidget       *widget,
        CdkScreen       *previous_screen);
static void     baul_path_bar_check_icon_theme         (BaulPathBar *path_bar);
static void     baul_path_bar_update_button_appearance (ButtonData      *button_data);
static void     baul_path_bar_update_button_state      (ButtonData      *button_data,
        gboolean         current_dir);
static gboolean baul_path_bar_update_path              (BaulPathBar *path_bar,
        GFile           *file_path,
        gboolean         emit_signal);


static CtkWidget *
get_slider_button (BaulPathBar  *path_bar,
                   const gchar  *arrow_type)
{
    CtkWidget *button;

    button = ctk_button_new ();
    ctk_widget_set_focus_on_click (button, FALSE);
    ctk_widget_add_events (button, CDK_SCROLL_MASK);
    ctk_container_add (CTK_CONTAINER (button),
                       ctk_image_new_from_icon_name (arrow_type, CTK_ICON_SIZE_MENU));
    ctk_container_add (CTK_CONTAINER (path_bar), button);
    ctk_widget_show_all (button);

    return button;
}

static void
update_button_types (BaulPathBar *path_bar)
{
    GList *list;
    GFile *path = NULL;

    for (list = path_bar->button_list; list; list = list->next)
    {
        ButtonData *button_data;
        button_data = BUTTON_DATA (list->data);
        if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button_data->button)))
        {
            /*Increase the reference count on path so it does not get cleared
             *by baul_path_bar_clear_buttons during baul_path_bar_update_path
             */
            path = g_object_ref (button_data->path);
            break;
        }
    }
    if (path != NULL)
    {
        baul_path_bar_update_path (path_bar, path, TRUE);
        g_object_unref (path);
    }
}


static void
desktop_location_changed_callback (gpointer user_data)
{
    BaulPathBar *path_bar;

    path_bar = BAUL_PATH_BAR (user_data);

    g_object_unref (path_bar->desktop_path);
    g_object_unref (path_bar->home_path);
    path_bar->desktop_path = baul_get_desktop_location ();
    path_bar->home_path = g_file_new_for_path (g_get_home_dir ());
    desktop_is_home = g_file_equal (path_bar->home_path, path_bar->desktop_path);

    update_button_types (path_bar);
}

static void
trash_state_changed_cb (BaulTrashMonitor *monitor G_GNUC_UNUSED,
			gboolean          state G_GNUC_UNUSED,
			BaulPathBar      *path_bar)
{
    GFile *file;
    GList *list;
    gint scale;

    file = g_file_new_for_uri ("trash:///");
    scale = ctk_widget_get_scale_factor (CTK_WIDGET (path_bar));
    for (list = path_bar->button_list; list; list = list->next)
    {
        ButtonData *button_data;
        button_data = BUTTON_DATA (list->data);
        if (g_file_equal (file, button_data->path))
        {
            GIcon *icon;
            BaulIconInfo *icon_info;
            cairo_surface_t *surface;

            icon = baul_trash_monitor_get_icon ();
            icon_info = baul_icon_info_lookup (icon, BAUL_PATH_BAR_ICON_SIZE, scale);
            surface = baul_icon_info_get_surface_at_size (icon_info, BAUL_PATH_BAR_ICON_SIZE);
            ctk_image_set_from_surface (CTK_IMAGE (button_data->image), surface);
        }
    }
    g_object_unref (file);
}

static gboolean
slider_timeout (gpointer user_data)
{
    BaulPathBar *path_bar;

    path_bar = BAUL_PATH_BAR (user_data);

    path_bar->drag_slider_timeout = 0;

    if (ctk_widget_get_visible (CTK_WIDGET (path_bar)))
    {
        if (path_bar->drag_slider_timeout_for_up_button)
        {
            baul_path_bar_scroll_up (path_bar);
        }
        else
        {
            baul_path_bar_scroll_down (path_bar);
        }
    }

    return FALSE;
}

static void
baul_path_bar_slider_drag_motion (CtkWidget      *widget,
				  CdkDragContext *context G_GNUC_UNUSED,
				  int             x G_GNUC_UNUSED,
				  int             y G_GNUC_UNUSED,
				  unsigned int    time G_GNUC_UNUSED,
				  gpointer        user_data)
{
    BaulPathBar *path_bar;
    unsigned int timeout;

    path_bar = BAUL_PATH_BAR (user_data);

    if (path_bar->drag_slider_timeout == 0)
    {
        CtkSettings *settings;

        settings = ctk_widget_get_settings (widget);

        g_object_get (settings, "ctk-timeout-expand", &timeout, NULL);
        path_bar->drag_slider_timeout =
            g_timeout_add (timeout,
                           slider_timeout,
                           path_bar);

        path_bar->drag_slider_timeout_for_up_button =
            widget == path_bar->up_slider_button;
    }
}

static void
baul_path_bar_slider_drag_leave (CtkWidget      *widget G_GNUC_UNUSED,
				 CdkDragContext *context G_GNUC_UNUSED,
				 unsigned int    time G_GNUC_UNUSED,
				 gpointer        user_data)
{
    BaulPathBar *path_bar;

    path_bar = BAUL_PATH_BAR (user_data);

    if (path_bar->drag_slider_timeout != 0)
    {
        g_source_remove (path_bar->drag_slider_timeout);
        path_bar->drag_slider_timeout = 0;
    }
}

static void
baul_path_bar_init (BaulPathBar *path_bar)
{
    char *p;
    CtkStyleContext *context;

    context = ctk_widget_get_style_context (CTK_WIDGET (path_bar));
    ctk_style_context_add_class (context, "baul-pathbar");

    ctk_widget_set_has_window (CTK_WIDGET (path_bar), FALSE);
    ctk_widget_set_redraw_on_allocate (CTK_WIDGET (path_bar), FALSE);

    path_bar->spacing = 3;
    path_bar->up_slider_button = get_slider_button (path_bar, "pan-start-symbolic");
    path_bar->down_slider_button = get_slider_button (path_bar, "pan-end-symbolic");
    ctk_style_context_add_class (ctk_widget_get_style_context (CTK_WIDGET (path_bar->up_slider_button)),
                                 "slider-button");
    ctk_style_context_add_class (ctk_widget_get_style_context (CTK_WIDGET (path_bar->down_slider_button)),
                                 "slider-button");

    path_bar->icon_size = BAUL_PATH_BAR_ICON_SIZE;

    p = baul_get_desktop_directory ();
    path_bar->desktop_path = g_file_new_for_path (p);
    g_free (p);
    path_bar->home_path = g_file_new_for_path (g_get_home_dir ());
    path_bar->root_path = g_file_new_for_path ("/");
    path_bar->current_path = NULL;
    path_bar->current_button_data = NULL;

    desktop_is_home = g_file_equal (path_bar->home_path, path_bar->desktop_path);

    g_signal_connect_swapped (baul_preferences, "changed::" BAUL_PREFERENCES_DESKTOP_IS_HOME_DIR,
                              G_CALLBACK(desktop_location_changed_callback),
                              path_bar);

    g_signal_connect_swapped (path_bar->up_slider_button, "clicked", G_CALLBACK (baul_path_bar_scroll_up), path_bar);
    g_signal_connect_swapped (path_bar->down_slider_button, "clicked", G_CALLBACK (baul_path_bar_scroll_down), path_bar);

    g_signal_connect (path_bar->up_slider_button, "button_press_event", G_CALLBACK (baul_path_bar_slider_button_press), path_bar);
    g_signal_connect (path_bar->up_slider_button, "button_release_event", G_CALLBACK (baul_path_bar_slider_button_release), path_bar);
    g_signal_connect (path_bar->down_slider_button, "button_press_event", G_CALLBACK (baul_path_bar_slider_button_press), path_bar);
    g_signal_connect (path_bar->down_slider_button, "button_release_event", G_CALLBACK (baul_path_bar_slider_button_release), path_bar);

    ctk_drag_dest_set (CTK_WIDGET (path_bar->up_slider_button),
                       0, NULL, 0, 0);
    ctk_drag_dest_set_track_motion (CTK_WIDGET (path_bar->up_slider_button), TRUE);
    g_signal_connect (path_bar->up_slider_button,
                      "drag-motion",
                      G_CALLBACK (baul_path_bar_slider_drag_motion),
                      path_bar);
    g_signal_connect (path_bar->up_slider_button,
                      "drag-leave",
                      G_CALLBACK (baul_path_bar_slider_drag_leave),
                      path_bar);

    ctk_drag_dest_set (CTK_WIDGET (path_bar->down_slider_button),
                       0, NULL, 0, 0);
    ctk_drag_dest_set_track_motion (CTK_WIDGET (path_bar->up_slider_button), TRUE);
    g_signal_connect (path_bar->down_slider_button,
                      "drag-motion",
                      G_CALLBACK (baul_path_bar_slider_drag_motion),
                      path_bar);
    g_signal_connect (path_bar->down_slider_button,
                      "drag-leave",
                      G_CALLBACK (baul_path_bar_slider_drag_leave),
                      path_bar);

    g_signal_connect (baul_trash_monitor_get (),
                      "trash_state_changed",
                      G_CALLBACK (trash_state_changed_cb),
                      path_bar);
}

static void
baul_path_bar_class_init (BaulPathBarClass *path_bar_class)
{
    GObjectClass *gobject_class;
    CtkWidgetClass *widget_class;
    CtkContainerClass *container_class;

    gobject_class = (GObjectClass *) path_bar_class;
    widget_class = (CtkWidgetClass *) path_bar_class;
    container_class = (CtkContainerClass *) path_bar_class;

    gobject_class->finalize = baul_path_bar_finalize;
    gobject_class->dispose = baul_path_bar_dispose;

    widget_class->get_preferred_height = baul_path_bar_get_preferred_height;
    widget_class->get_preferred_width = baul_path_bar_get_preferred_width;

    widget_class->unmap = baul_path_bar_unmap;
    widget_class->size_allocate = baul_path_bar_size_allocate;
    widget_class->style_updated = baul_path_bar_style_updated;

    widget_class->screen_changed = baul_path_bar_screen_changed;
    widget_class->grab_notify = baul_path_bar_grab_notify;
    widget_class->state_changed = baul_path_bar_state_changed;
    widget_class->scroll_event = baul_path_bar_scroll;

    container_class->add = baul_path_bar_add;
    container_class->forall = baul_path_bar_forall;
    container_class->remove = baul_path_bar_remove;

    path_bar_signals [PATH_CLICKED] =
        g_signal_new ("path-clicked",
                      G_OBJECT_CLASS_TYPE (path_bar_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (BaulPathBarClass, path_clicked),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1,
                      G_TYPE_FILE);

    path_bar_signals [PATH_EVENT] =
       g_signal_new ("path-event",
                      G_OBJECT_CLASS_TYPE (path_bar_class),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulPathBarClass, path_event),
                      NULL, NULL, NULL,
                      G_TYPE_BOOLEAN, 2,
                      G_TYPE_FILE,
                      CDK_TYPE_EVENT);

    ctk_container_class_handle_border_width (container_class);
}


static void
baul_path_bar_finalize (GObject *object)
{
    BaulPathBar *path_bar;

    path_bar = BAUL_PATH_BAR (object);

    baul_path_bar_stop_scrolling (path_bar);

    if (path_bar->drag_slider_timeout != 0)
    {
        g_source_remove (path_bar->drag_slider_timeout);
        path_bar->drag_slider_timeout = 0;
    }

    g_list_free (path_bar->button_list);
    if (path_bar->root_path)
    {
        g_object_unref (path_bar->root_path);
        path_bar->root_path = NULL;
    }
    if (path_bar->home_path)
    {
        g_object_unref (path_bar->home_path);
        path_bar->home_path = NULL;
    }
    if (path_bar->desktop_path)
    {
        g_object_unref (path_bar->desktop_path);
        path_bar->desktop_path = NULL;
    }

    g_signal_handlers_disconnect_by_func (baul_trash_monitor_get (),
                                          trash_state_changed_cb, path_bar);
    g_signal_handlers_disconnect_by_func (baul_preferences,
                                          desktop_location_changed_callback,
                                          path_bar);

    G_OBJECT_CLASS (baul_path_bar_parent_class)->finalize (object);
}

/* Removes the settings signal handler.  It's safe to call multiple times */
static void
remove_settings_signal (BaulPathBar *path_bar,
                        CdkScreen  *screen)
{
    if (path_bar->settings_signal_id)
    {
        CtkSettings *settings;

        settings = ctk_settings_get_for_screen (screen);
        g_signal_handler_disconnect (settings,
                                     path_bar->settings_signal_id);
        path_bar->settings_signal_id = 0;
    }
}

static void
baul_path_bar_dispose (GObject *object)
{
    remove_settings_signal (BAUL_PATH_BAR (object), ctk_widget_get_screen (CTK_WIDGET (object)));

    G_OBJECT_CLASS (baul_path_bar_parent_class)->dispose (object);
}

/* Size requisition:
 *
 * Ideally, our size is determined by another widget, and we are just filling
 * available space.
 */

static void
baul_path_bar_get_preferred_width (CtkWidget *widget,
    			       gint      *minimum,
    			       gint      *natural)
{
    BaulPathBar *path_bar;
    GList *list;
    gint child_height;
    gint height;
    gint child_min, child_nat;
    gint slider_width;
    ButtonData *button_data = NULL;

    path_bar = BAUL_PATH_BAR (widget);

    *minimum = *natural = 0;
    height = 0;

    for (list = path_bar->button_list; list; list = list->next) {
    	button_data = BUTTON_DATA (list->data);
    	ctk_widget_get_preferred_width (button_data->button, &child_min, &child_nat);
    	ctk_widget_get_preferred_height (button_data->button, &child_height, NULL);
    	height = MAX (height, child_height);

    	if (button_data->type == NORMAL_BUTTON) {
    		/* Use 2*Height as button width because of ellipsized label.  */
    		child_min = MAX (child_min, child_height * 2);
    		child_nat = MAX (child_min, child_height * 2);
    	}

    	*minimum = MAX (*minimum, child_min);
    	*natural = MAX (*natural, child_nat);
    }

    /* Add space for slider, if we have more than one path */
    /* Theoretically, the slider could be bigger than the other button.  But we're
     * not going to worry about that now.
     */
    ctk_widget_get_preferred_width (path_bar->down_slider_button,
                                    &slider_width,
                                    NULL);
    ctk_widget_get_preferred_width (path_bar->up_slider_button,
                                    &slider_width,
                                    NULL);


    if (path_bar->button_list) {
        *minimum += (path_bar->spacing + slider_width) * 2;
        *natural += (path_bar->spacing + slider_width) * 2;
    }
    /*Let's keep the rest of this as it was */
    path_bar->slider_width = slider_width;
}

static void
baul_path_bar_get_preferred_height (CtkWidget *widget,
    				gint      *minimum,
    				gint      *natural)
{
    BaulPathBar *path_bar;
    GList *list;
    gint child_min, child_nat;
    ButtonData *button_data = NULL;

    path_bar = BAUL_PATH_BAR (widget);

    *minimum = *natural = 0;

    for (list = path_bar->button_list; list; list = list->next) {
    	button_data = BUTTON_DATA (list->data);
    	ctk_widget_get_preferred_height (button_data->button, &child_min, &child_nat);

    	*minimum = MAX (*minimum, child_min);
    	*natural = MAX (*natural, child_nat);
    }
}

static void
baul_path_bar_update_slider_buttons (BaulPathBar *path_bar)
{
    if (path_bar->button_list)
    {

        CtkWidget *button;

        button = BUTTON_DATA (path_bar->button_list->data)->button;
        if (ctk_widget_get_child_visible (button))
        {
            ctk_widget_set_sensitive (path_bar->down_slider_button, FALSE);
        }
        else
        {
            ctk_widget_set_sensitive (path_bar->down_slider_button, TRUE);
        }
        button = BUTTON_DATA (g_list_last (path_bar->button_list)->data)->button;
        if (ctk_widget_get_child_visible (button))
        {
            ctk_widget_set_sensitive (path_bar->up_slider_button, FALSE);
        }
        else
        {
            ctk_widget_set_sensitive (path_bar->up_slider_button, TRUE);
        }
    }
}

static void
baul_path_bar_unmap (CtkWidget *widget)
{
    baul_path_bar_stop_scrolling (BAUL_PATH_BAR (widget));

    CTK_WIDGET_CLASS (baul_path_bar_parent_class)->unmap (widget);
}

/* This is a tad complicated */
static void
baul_path_bar_size_allocate (CtkWidget     *widget,
                             CtkAllocation *allocation)
{
    CtkWidget *child;
    BaulPathBar *path_bar;
    CtkTextDirection direction;
    CtkAllocation child_allocation;
    GList *list, *first_button;
    gint width;
    gint allocation_width;
    gboolean need_sliders;
    gint up_slider_offset;
    gint down_slider_offset;
    CtkRequisition child_requisition;
    CtkAllocation widget_allocation;

    need_sliders = TRUE;
    up_slider_offset = 0;
    down_slider_offset = 0;
    path_bar = BAUL_PATH_BAR (widget);

    ctk_widget_set_allocation (widget, allocation);

    /* No path is set so we don't have to allocate anything. */
    if (path_bar->button_list == NULL)
    {
        return;
    }
    direction = ctk_widget_get_direction (widget);

    allocation_width = allocation->width;

    /* First, we check to see if we need the scrollbars. */
    if (path_bar->fake_root)
    {
        width = path_bar->spacing + path_bar->slider_width;
    }
    else
    {
        width = 0;
    }

    ctk_widget_get_preferred_size (BUTTON_DATA (path_bar->button_list->data)->button,
    				   &child_requisition, NULL);
    width += child_requisition.width;

    for (list = path_bar->button_list->next; list; list = list->next)
    {
        child = BUTTON_DATA (list->data)->button;
        ctk_widget_get_preferred_size (child, &child_requisition, NULL);
        width += child_requisition.width + path_bar->spacing;

        if (list == path_bar->fake_root)
        {
            break;
        }
    }

    if (width <= allocation_width)
    {
        if (path_bar->fake_root)
        {
            first_button = path_bar->fake_root;
        }
        else
        {
            first_button = g_list_last (path_bar->button_list);
        }
    }
    else
    {
        gboolean reached_end;
        gint slider_space;
        reached_end = FALSE;
        slider_space = 2 * (path_bar->spacing + path_bar->slider_width);

        if (path_bar->first_scrolled_button)
        {
            first_button = path_bar->first_scrolled_button;
        }
        else
        {
            first_button = path_bar->button_list;
        }

        need_sliders = TRUE;
        /* To see how much space we have, and how many buttons we can display.
        * We start at the first button, count forward until hit the new
        * button, then count backwards.
        */
        /* Count down the path chain towards the end. */
        ctk_widget_get_preferred_size (BUTTON_DATA (first_button->data)->button,
        			       &child_requisition, NULL);
        width = child_requisition.width;
        list = first_button->prev;
        while (list && !reached_end)
        {
            child = BUTTON_DATA (list->data)->button;
            ctk_widget_get_preferred_size (child, &child_requisition, NULL);

            if (width + child_requisition.width + path_bar->spacing + slider_space > allocation_width)
            {
                reached_end = TRUE;
            }
            else
            {
                if (list == path_bar->fake_root)
                {
                    break;
                }
                else
                {
                    width += child_requisition.width + path_bar->spacing;
                }
            }

            list = list->prev;
        }

        /* Finally, we walk up, seeing how many of the previous buttons we can add*/

        while (first_button->next && ! reached_end)
        {
            child = BUTTON_DATA (first_button->next->data)->button;
            ctk_widget_get_preferred_size (child, &child_requisition, NULL);

            if (width + child_requisition.width + path_bar->spacing + slider_space > allocation_width)
            {
                reached_end = TRUE;
            }
            else
            {
                width += child_requisition.width + path_bar->spacing;
                if (first_button == path_bar->fake_root)
                {
                    break;
                }
                first_button = first_button->next;
            }
        }
    }

    /* Now, we allocate space to the buttons */
    child_allocation.y = allocation->y;
    child_allocation.height = allocation->height;

    if (direction == CTK_TEXT_DIR_RTL)
    {
        child_allocation.x = allocation->x + allocation->width;

        if (need_sliders || path_bar->fake_root)
        {
            child_allocation.x -= (path_bar->spacing + path_bar->slider_width);
            up_slider_offset = allocation->width - path_bar->slider_width;

        }
    }
    else
    {
        child_allocation.x = allocation->x;

        if (need_sliders || path_bar->fake_root)
        {
            up_slider_offset = 0;
            child_allocation.x += (path_bar->spacing + path_bar->slider_width);
        }
    }

    for (list = first_button; list; list = list->prev)
    {
        child = BUTTON_DATA (list->data)->button;
        ctk_widget_get_preferred_size (child, &child_requisition, NULL);

        ctk_widget_get_allocation (widget, &widget_allocation);

        child_allocation.width = child_requisition.width;
        if (direction == CTK_TEXT_DIR_RTL)
        {
            child_allocation.x -= child_allocation.width;
        }
        /* Check to see if we've don't have any more space to allocate buttons */
        if (need_sliders && direction == CTK_TEXT_DIR_RTL)
        {
            if (child_allocation.x - path_bar->spacing - path_bar->slider_width < widget_allocation.x)
            {
                break;
            }
        }
        else
        {
            if (need_sliders && direction == CTK_TEXT_DIR_LTR)
            {
                if (child_allocation.x + child_allocation.width + path_bar->spacing + path_bar->slider_width > widget_allocation.x + allocation_width)

                {
                    break;
                }
            }
        }

        ctk_widget_set_child_visible (BUTTON_DATA (list->data)->button, TRUE);
        ctk_widget_size_allocate (child, &child_allocation);

        if (direction == CTK_TEXT_DIR_RTL)
        {
            child_allocation.x -= path_bar->spacing;
            down_slider_offset = child_allocation.x - allocation->x - path_bar->slider_width;
        }
        else
        {
            down_slider_offset = child_allocation.x - widget_allocation.x;
            down_slider_offset += child_allocation.width + path_bar->spacing;

            child_allocation.x += child_allocation.width + path_bar->spacing;
        }
    }
    /* Now we go hide all the widgets that don't fit */
    while (list)
    {
        ctk_widget_set_child_visible (BUTTON_DATA (list->data)->button, FALSE);
        list = list->prev;
    }
    for (list = first_button->next; list; list = list->next)
    {
        ctk_widget_set_child_visible (BUTTON_DATA (list->data)->button, FALSE);
    }

    if (need_sliders || path_bar->fake_root)
    {
        child_allocation.width = path_bar->slider_width;
        child_allocation.x = up_slider_offset + allocation->x;
        ctk_widget_size_allocate (path_bar->up_slider_button, &child_allocation);

        ctk_widget_set_child_visible (path_bar->up_slider_button, TRUE);
        ctk_widget_show_all (path_bar->up_slider_button);
    }
    else
    {
        ctk_widget_set_child_visible (path_bar->up_slider_button, FALSE);
    }

    if (need_sliders)
    {
        child_allocation.width = path_bar->slider_width;
        child_allocation.x = down_slider_offset + allocation->x;
        ctk_widget_size_allocate (path_bar->down_slider_button, &child_allocation);

        ctk_widget_set_child_visible (path_bar->down_slider_button, TRUE);
        ctk_widget_show_all (path_bar->down_slider_button);
        baul_path_bar_update_slider_buttons (path_bar);
    }
    else
    {
        ctk_widget_set_child_visible (path_bar->down_slider_button, FALSE);
    }
}

static void
baul_path_bar_style_updated (CtkWidget *widget)
{
    if (CTK_WIDGET_CLASS (baul_path_bar_parent_class)->style_updated)
    {
        CTK_WIDGET_CLASS (baul_path_bar_parent_class)->style_updated (widget);
    }

    baul_path_bar_check_icon_theme (BAUL_PATH_BAR (widget));
}

static void
baul_path_bar_screen_changed (CtkWidget *widget,
                              CdkScreen *previous_screen)
{
    if (CTK_WIDGET_CLASS (baul_path_bar_parent_class)->screen_changed)
    {
        CTK_WIDGET_CLASS (baul_path_bar_parent_class)->screen_changed (widget, previous_screen);
    }
    /* We might nave a new settings, so we remove the old one */
    if (previous_screen)
    {
        remove_settings_signal (BAUL_PATH_BAR (widget), previous_screen);
    }
    baul_path_bar_check_icon_theme (BAUL_PATH_BAR (widget));
}

static gboolean
baul_path_bar_scroll (CtkWidget      *widget,
                      CdkEventScroll *event)
{
    BaulPathBar *path_bar;

    path_bar = BAUL_PATH_BAR (widget);

    switch (event->direction)
    {
    case CDK_SCROLL_RIGHT:
    case CDK_SCROLL_DOWN:
        baul_path_bar_scroll_down (path_bar);
        return TRUE;

    case CDK_SCROLL_LEFT:
    case CDK_SCROLL_UP:
        baul_path_bar_scroll_up (path_bar);
        return TRUE;

    case CDK_SCROLL_SMOOTH:
        break;
    }

    return FALSE;
}


static void
baul_path_bar_add (CtkContainer *container,
                   CtkWidget    *widget)
{
    ctk_widget_set_parent (widget, CTK_WIDGET (container));
}

static void
baul_path_bar_remove_1 (CtkContainer *container,
                        CtkWidget    *widget)
{
    gboolean was_visible = ctk_widget_get_visible (widget);
    ctk_widget_unparent (widget);
    if (was_visible)
    {
        ctk_widget_queue_resize (CTK_WIDGET (container));
    }
}

static void
baul_path_bar_remove (CtkContainer *container,
                      CtkWidget    *widget)
{
    BaulPathBar *path_bar;
    GList *children;

    path_bar = BAUL_PATH_BAR (container);

    if (widget == path_bar->up_slider_button)
    {
        baul_path_bar_remove_1 (container, widget);
        path_bar->up_slider_button = NULL;
        return;
    }

    if (widget == path_bar->down_slider_button)
    {
        baul_path_bar_remove_1 (container, widget);
        path_bar->down_slider_button = NULL;
        return;
    }

    children = path_bar->button_list;
    while (children)
    {
        if (widget == BUTTON_DATA (children->data)->button)
        {
            baul_path_bar_remove_1 (container, widget);
            path_bar->button_list = g_list_remove_link (path_bar->button_list, children);
            g_list_free_1 (children);
            return;
        }
        children = children->next;
    }
}

static void
baul_path_bar_forall (CtkContainer *container,
		      gboolean      include_internals G_GNUC_UNUSED,
		      CtkCallback   callback,
		      gpointer      callback_data)
{
    BaulPathBar *path_bar;
    GList *children;

    g_return_if_fail (callback != NULL);
    path_bar = BAUL_PATH_BAR (container);

    children = path_bar->button_list;
    while (children)
    {
        CtkWidget *child;
        child = BUTTON_DATA (children->data)->button;
        children = children->next;
        (* callback) (child, callback_data);
    }

    if (path_bar->up_slider_button)
    {
        (* callback) (path_bar->up_slider_button, callback_data);
    }

    if (path_bar->down_slider_button)
    {
        (* callback) (path_bar->down_slider_button, callback_data);
    }
}

static void
baul_path_bar_scroll_down (BaulPathBar *path_bar)
{
    GList *list;
    GList *down_button;
    GList *up_button;
    gint space_available;
    gint space_needed;
    CtkTextDirection direction;
    CtkAllocation allocation, button_allocation, slider_allocation;

    down_button = NULL;
    up_button = NULL;

    if (path_bar->ignore_click)
    {
        path_bar->ignore_click = FALSE;
        return;
    }

    ctk_widget_queue_resize (CTK_WIDGET (path_bar));

    direction = ctk_widget_get_direction (CTK_WIDGET (path_bar));

    /* We find the button at the 'down' end that we have to make */
    /* visible */
    for (list = path_bar->button_list; list; list = list->next)
    {
        if (list->next && ctk_widget_get_child_visible (BUTTON_DATA (list->next->data)->button))
        {
            down_button = list;
            break;
        }
    }

    if (down_button == NULL)
    {
        return;
    }

    /* Find the last visible button on the 'up' end */
    for (list = g_list_last (path_bar->button_list); list; list = list->prev)
    {
        if (ctk_widget_get_child_visible (BUTTON_DATA (list->data)->button))
        {
            up_button = list;
            break;
        }
    }

    ctk_widget_get_allocation (BUTTON_DATA (down_button->data)->button, &button_allocation);
    ctk_widget_get_allocation (CTK_WIDGET (path_bar), &allocation);
    ctk_widget_get_allocation (path_bar->down_slider_button, &slider_allocation);

    space_needed = button_allocation.width + path_bar->spacing;
    if (direction == CTK_TEXT_DIR_RTL)
    {
        space_available = slider_allocation.x - allocation.x;
    }
    else
    {
        space_available = (allocation.x + allocation.width) -
                          (slider_allocation.x + slider_allocation.width);
    }

    /* We have space_available extra space that's not being used.  We
    * need space_needed space to make the button fit.  So we walk down
    * from the end, removing buttons until we get all the space we
    * need. */
    ctk_widget_get_allocation (BUTTON_DATA (up_button->data)->button, &button_allocation);
    while (space_available < space_needed && up_button)
    {
        space_available += button_allocation.width + path_bar->spacing;
        up_button = up_button->prev;
        path_bar->first_scrolled_button = up_button;
    }
}

static void
baul_path_bar_scroll_up (BaulPathBar *path_bar)
{
    GList *list;

    if (path_bar->ignore_click)
    {
        path_bar->ignore_click = FALSE;
        return;
    }

    ctk_widget_queue_resize (CTK_WIDGET (path_bar));

    for (list = g_list_last (path_bar->button_list); list; list = list->prev)
    {
        if (list->prev && ctk_widget_get_child_visible (BUTTON_DATA (list->prev->data)->button))
        {
            if (list->prev == path_bar->fake_root)
            {
                path_bar->fake_root = NULL;
            }
            path_bar->first_scrolled_button = list;
            return;
        }
    }
}

static gboolean
baul_path_bar_scroll_timeout (BaulPathBar *path_bar)
{
    gboolean retval = FALSE;

    if (path_bar->timer)
    {
        if (ctk_widget_has_focus (path_bar->up_slider_button))
        {
            baul_path_bar_scroll_up (path_bar);
        }
        else
        {
            if (ctk_widget_has_focus (path_bar->down_slider_button))
            {
                baul_path_bar_scroll_down (path_bar);
            }
        }
        if (path_bar->need_timer)
        {
            path_bar->need_timer = FALSE;

            path_bar->timer = g_timeout_add (SCROLL_TIMEOUT,
                                             (GSourceFunc)baul_path_bar_scroll_timeout,
                                             path_bar);

        }
        else
        {
            retval = TRUE;
        }
    }

    return retval;
}

static void
baul_path_bar_stop_scrolling (BaulPathBar *path_bar)
{
    if (path_bar->timer)
    {
        g_source_remove (path_bar->timer);
        path_bar->timer = 0;
        path_bar->need_timer = FALSE;
    }
}

static gboolean
baul_path_bar_slider_button_press (CtkWidget       *widget,
                                   CdkEventButton  *event,
                                   BaulPathBar *path_bar)
{
    if (!ctk_widget_has_focus (widget))
    {
        ctk_widget_grab_focus (widget);
    }

    if (event->type != CDK_BUTTON_PRESS || event->button != 1)
    {
        return FALSE;
    }

    path_bar->ignore_click = FALSE;

    if (widget == path_bar->up_slider_button)
    {
        baul_path_bar_scroll_up (path_bar);
    }
    else
    {
        if (widget == path_bar->down_slider_button)
        {
            baul_path_bar_scroll_down (path_bar);
        }
    }

    if (!path_bar->timer)
    {
        path_bar->need_timer = TRUE;
        path_bar->timer = g_timeout_add (INITIAL_SCROLL_TIMEOUT,
                                         (GSourceFunc)baul_path_bar_scroll_timeout,
                                         path_bar);
    }

    return FALSE;
}

static gboolean
baul_path_bar_slider_button_release (CtkWidget      *widget G_GNUC_UNUSED,
				     CdkEventButton *event,
				     BaulPathBar    *path_bar)
{
    if (event->type != CDK_BUTTON_RELEASE)
    {
        return FALSE;
    }

    path_bar->ignore_click = TRUE;
    baul_path_bar_stop_scrolling (path_bar);

    return FALSE;
}

static void
baul_path_bar_grab_notify (CtkWidget *widget,
                           gboolean   was_grabbed)
{
    if (!was_grabbed)
    {
        baul_path_bar_stop_scrolling (BAUL_PATH_BAR (widget));
    }
}

static void
baul_path_bar_state_changed (CtkWidget    *widget,
			     CtkStateType  previous_state G_GNUC_UNUSED)
{
    if (!ctk_widget_get_sensitive (widget))
    {
        baul_path_bar_stop_scrolling (BAUL_PATH_BAR (widget));
    }
}



/* Changes the icons wherever it is needed */
static void
reload_icons (BaulPathBar *path_bar)
{
    GList *list;

    for (list = path_bar->button_list; list; list = list->next)
    {
        ButtonData *button_data;

        button_data = BUTTON_DATA (list->data);
        if (button_data->type != NORMAL_BUTTON || button_data->is_base_dir)
        {
            baul_path_bar_update_button_appearance (button_data);
        }

    }
}

static void
change_icon_theme (BaulPathBar *path_bar)
{
    path_bar->icon_size = BAUL_PATH_BAR_ICON_SIZE;
    reload_icons (path_bar);
}

/* Callback used when a CtkSettings value changes */
static void
settings_notify_cb (GObject     *object G_GNUC_UNUSED,
		    GParamSpec  *pspec,
		    BaulPathBar *path_bar)
{
    const char *name;

    name = g_param_spec_get_name (pspec);

    if (! strcmp (name, "ctk-icon-theme-name") || ! strcmp (name, "ctk-icon-sizes"))
    {
        change_icon_theme (path_bar);
    }
}

static void
baul_path_bar_check_icon_theme (BaulPathBar *path_bar)
{
    CtkSettings *settings;

    if (path_bar->settings_signal_id)
    {
        return;
    }

    settings = ctk_settings_get_for_screen (ctk_widget_get_screen (CTK_WIDGET (path_bar)));
    path_bar->settings_signal_id = g_signal_connect (settings, "notify", G_CALLBACK (settings_notify_cb), path_bar);

    change_icon_theme (path_bar);
}

/* Public functions and their helpers */
void
baul_path_bar_clear_buttons (BaulPathBar *path_bar)
{
    while (path_bar->button_list != NULL)
    {
        ctk_container_remove (CTK_CONTAINER (path_bar), BUTTON_DATA (path_bar->button_list->data)->button);
    }
    path_bar->first_scrolled_button = NULL;
    path_bar->fake_root = NULL;
}

static void
button_clicked_cb (CtkWidget *button,
                   gpointer   data)
{
    ButtonData *button_data;
    BaulPathBar *path_bar;
    GList *button_list;

    button_data = BUTTON_DATA (data);
    if (button_data->ignore_changes)
    {
        return;
    }

    path_bar = BAUL_PATH_BAR (ctk_widget_get_parent (button));

    button_list = g_list_find (path_bar->button_list, button_data);
    g_assert (button_list != NULL);

    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);

    g_signal_emit (path_bar, path_bar_signals [PATH_CLICKED], 0, button_data->path);
}

static gboolean
button_event_cb (CtkWidget *button,
		 CdkEventButton *event,
		 gpointer   data)
{
        ButtonData *button_data;
        BaulPathBar *path_bar;
        GList *button_list;
        gboolean retval;

        button_data = BUTTON_DATA (data);
        path_bar = BAUL_PATH_BAR (ctk_widget_get_parent (button));

	if (event->type == CDK_BUTTON_PRESS) {
		g_object_set_data (G_OBJECT (button), "handle-button-release",
				   GINT_TO_POINTER (TRUE));
	}

	if (event->type == CDK_BUTTON_RELEASE &&
	    !GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (button),
						  "handle-button-release"))) {
		return FALSE;
	}

        button_list = g_list_find (path_bar->button_list, button_data);
        g_assert (button_list != NULL);

        g_signal_emit (path_bar, path_bar_signals [PATH_EVENT], 0, button_data->path, event, &retval);

	return retval;
}

static void
button_drag_begin_cb (CtkWidget      *widget,
		      CdkDragContext *drag_context G_GNUC_UNUSED,
		      gpointer        user_data G_GNUC_UNUSED)
{
	g_object_set_data (G_OBJECT (widget), "handle-button-release",
			   GINT_TO_POINTER (FALSE));
}


static BaulIconInfo *
get_type_icon_info (ButtonData *button_data)
{
    gint icon_scale = ctk_widget_get_scale_factor (CTK_WIDGET (button_data->button));

    switch (button_data->type)
    {
    case ROOT_BUTTON:
        return baul_icon_info_lookup_from_name (BAUL_ICON_FILESYSTEM,
                                                BAUL_PATH_BAR_ICON_SIZE,
                                                icon_scale);

    case HOME_BUTTON:
        return baul_icon_info_lookup_from_name (BAUL_ICON_HOME,
                                                BAUL_PATH_BAR_ICON_SIZE,
                                                icon_scale);

    case DESKTOP_BUTTON:
        return baul_icon_info_lookup_from_name (BAUL_ICON_DESKTOP,
                                                BAUL_PATH_BAR_ICON_SIZE,
                                                icon_scale);

    case NORMAL_BUTTON:
        if (button_data->is_base_dir)
        {
            return baul_file_get_icon (button_data->file,
                                       BAUL_PATH_BAR_ICON_SIZE, icon_scale,
                                       BAUL_FILE_ICON_FLAGS_NONE);
        }

    default:
        return NULL;
    }

    return NULL;
}

static void
button_data_free (ButtonData *button_data)
{
    g_object_unref (button_data->path);
    g_free (button_data->dir_name);
    if (button_data->custom_icon)
    {
        cairo_surface_destroy (button_data->custom_icon);
    }
    if (button_data->file != NULL)
    {
        g_signal_handler_disconnect (button_data->file,
                                     button_data->file_changed_signal_id);
        baul_file_monitor_remove (button_data->file, button_data);
        baul_file_unref (button_data->file);
    }

    g_object_unref (button_data->drag_info.target_location);
    button_data->drag_info.target_location = NULL;

    g_free (button_data);
}

static const char *
get_dir_name (ButtonData *button_data)
{
    if (button_data->type == DESKTOP_BUTTON)
    {
        return _("Desktop");
    }
    else
    {
        return button_data->dir_name;
    }
}

/* We always want to request the same size for the label, whether
 * or not the contents are bold
 */
static void
set_label_padding_size (ButtonData *button_data)
{
    const gchar *dir_name = get_dir_name (button_data);
    PangoLayout *layout;
    gint width, height, bold_width, bold_height;
    gint pad_left, pad_right;
    gchar *markup;

    layout = ctk_widget_create_pango_layout (button_data->label, dir_name);
    pango_layout_get_pixel_size (layout, &width, &height);

    markup = g_markup_printf_escaped ("<b>%s</b>", dir_name);
    pango_layout_set_markup (layout, markup, -1);
    g_free (markup);

    pango_layout_get_pixel_size (layout, &bold_width, &bold_height);

    pad_left = (bold_width - width) / 2;
    pad_right = (bold_width + 1 - width) / 2; /* this ensures rounding up - the
    pixel size difference between bold and normal fonts is not always even and
    will give an off-by-one error when dividing by 2 */

    ctk_widget_set_margin_start (CTK_WIDGET (button_data->label), pad_left);
    ctk_widget_set_margin_end (CTK_WIDGET (button_data->label), pad_right);

    g_object_unref (layout);
}

static void
baul_path_bar_update_button_appearance (ButtonData *button_data)
{
    const gchar *dir_name = get_dir_name (button_data);

    if (button_data->label != NULL)
    {
        if (ctk_label_get_use_markup (CTK_LABEL (button_data->label)))
        {
            char *markup;

            markup = g_markup_printf_escaped ("<b>%s</b>", dir_name);
            ctk_label_set_markup (CTK_LABEL (button_data->label), markup);

            ctk_widget_set_margin_end (CTK_WIDGET (button_data->label), 0);
            ctk_widget_set_margin_start (CTK_WIDGET (button_data->label), 0);
            g_free(markup);
        }
        else
        {
            ctk_label_set_text (CTK_LABEL (button_data->label), dir_name);
            set_label_padding_size (button_data);
        }
    }

    if (button_data->image != NULL)
    {
        if (button_data->custom_icon)
        {
            ctk_image_set_from_surface (CTK_IMAGE (button_data->image), button_data->custom_icon);
            ctk_widget_show (CTK_WIDGET (button_data->image));
        }
        else
        {
            BaulIconInfo *icon_info;
            cairo_surface_t *surface;

            icon_info = get_type_icon_info (button_data);
            surface = NULL;

            if (icon_info != NULL)
            {
                surface = baul_icon_info_get_surface_at_size (icon_info, BAUL_PATH_BAR_ICON_SIZE);
                g_object_unref (icon_info);
            }

            if (surface != NULL)
            {
                ctk_image_set_from_surface (CTK_IMAGE (button_data->image), surface);
                ctk_style_context_add_class (ctk_widget_get_style_context (button_data->button),
                                             "image-button");
                ctk_widget_show (CTK_WIDGET (button_data->image));
                cairo_surface_destroy (surface);
            }
            else
            {
                ctk_widget_hide (CTK_WIDGET (button_data->image));
                ctk_style_context_remove_class (ctk_widget_get_style_context (button_data->button),
                                                "image-button");
            }
        }
    }

}

static void
baul_path_bar_update_button_state (ButtonData *button_data,
                                   gboolean    current_dir)
{
    if (button_data->label != NULL)
    {
        g_object_set (button_data->label, "use-markup", current_dir, NULL);
    }

    baul_path_bar_update_button_appearance (button_data);

    if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button_data->button)) != current_dir)
    {
        button_data->ignore_changes = TRUE;
        ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button_data->button), current_dir);
        button_data->ignore_changes = FALSE;
    }
}

static gboolean
setup_file_path_mounted_mount (GFile *location, ButtonData *button_data)
{
    GVolumeMonitor *volume_monitor;
    GList *mounts, *l;
    gboolean result;
    GIcon *icon;
    BaulIconInfo *info;
    GFile *root, *default_location;
    gint scale;
    GMount *mount = NULL;

    /* Return false if button has not been set up yet or has been destroyed*/
    if (!button_data->button)
        return FALSE;
    result = FALSE;
    volume_monitor = g_volume_monitor_get ();
    mounts = g_volume_monitor_get_mounts (volume_monitor);

    scale = ctk_widget_get_scale_factor (CTK_WIDGET (button_data->button));
    for (l = mounts; l != NULL; l = l->next)
    {
        mount = l->data;
        if (g_mount_is_shadowed (mount))
        {
            continue;
        }
        if (result)
        {
            continue;
        }
        root = g_mount_get_root (mount);
        if (g_file_equal (location, root))
        {
            result = TRUE;
            /* set mount specific details in button_data */
            if (button_data)
            {
                icon = g_mount_get_icon (mount);
                if (icon == NULL)
                {
                    icon = g_themed_icon_new (BAUL_ICON_FOLDER);
                }
                info = baul_icon_info_lookup (icon, BAUL_PATH_BAR_ICON_SIZE, scale);
                g_object_unref (icon);
                button_data->custom_icon = baul_icon_info_get_surface_at_size (info, BAUL_PATH_BAR_ICON_SIZE);
                g_object_unref (info);
                button_data->dir_name = g_mount_get_name (mount);
                button_data->type = MOUNT_BUTTON;
                button_data->fake_root = TRUE;
            }
            g_object_unref (root);
            break;
        }
        default_location = g_mount_get_default_location (mount);
        if (!g_file_equal (default_location, root) &&
                g_file_equal (location, default_location))
        {
            result = TRUE;
            /* set mount specific details in button_data */
            if (button_data)
            {
                icon = g_mount_get_icon (mount);
                if (icon == NULL)
                {
                    icon = g_themed_icon_new (BAUL_ICON_FOLDER);
                }
                info = baul_icon_info_lookup (icon, BAUL_PATH_BAR_ICON_SIZE, scale);
                g_object_unref (icon);
                button_data->custom_icon = baul_icon_info_get_surface_at_size (info, BAUL_PATH_BAR_ICON_SIZE);
                g_object_unref (info);
                button_data->type = DEFAULT_LOCATION_BUTTON;
                button_data->fake_root = TRUE;
            }
            g_object_unref (default_location);
            g_object_unref (root);
            break;
        }
        g_object_unref (default_location);
        g_object_unref (root);
    }
    g_list_free_full (mounts, g_object_unref);
    return result;
}

static void
setup_button_type (ButtonData       *button_data,
                   BaulPathBar  *path_bar,
                   GFile *location)
{
    if (path_bar->root_path != NULL && g_file_equal (location, path_bar->root_path))
    {
        button_data->type = ROOT_BUTTON;
    }
    else if (path_bar->home_path != NULL && g_file_equal (location, path_bar->home_path))
    {
        button_data->type = HOME_BUTTON;
        button_data->fake_root = TRUE;
    }
    else if (path_bar->desktop_path != NULL && g_file_equal (location, path_bar->desktop_path))
    {
        if (!desktop_is_home)
        {
            button_data->type = DESKTOP_BUTTON;
        }
        else
        {
            button_data->type = NORMAL_BUTTON;
        }
    }
    else if (setup_file_path_mounted_mount (location, button_data))
    {
        /* already setup */
    }
    else
    {
        button_data->type = NORMAL_BUTTON;
    }
}

static void
button_drag_data_get_cb (CtkWidget        *widget G_GNUC_UNUSED,
			 CdkDragContext   *context G_GNUC_UNUSED,
			 CtkSelectionData *selection_data,
			 guint             info,
			 guint             time_ G_GNUC_UNUSED,
			 gpointer          user_data)
{
    ButtonData *button_data;
    char *uri_list[2];

    button_data = user_data;

    uri_list[0] = g_file_get_uri (button_data->path);
    uri_list[1] = NULL;

    if (info == BAUL_ICON_DND_CAFE_ICON_LIST)
    {
        char *tmp;

        tmp = g_strdup_printf ("%s\r\n", uri_list[0]);
        ctk_selection_data_set (selection_data, ctk_selection_data_get_target (selection_data),
                                8, tmp, strlen (tmp));
        g_free (tmp);
    }
    else if (info == BAUL_ICON_DND_URI_LIST)
    {
        ctk_selection_data_set_uris (selection_data, uri_list);
    }

    g_free (uri_list[0]);
}

static void
setup_button_drag_source (ButtonData *button_data)
{
    CtkTargetList *target_list;
    const CtkTargetEntry targets[] =
    {
        { BAUL_ICON_DND_CAFE_ICON_LIST_TYPE, 0, BAUL_ICON_DND_CAFE_ICON_LIST }
    };

    ctk_drag_source_set (button_data->button,
                         CDK_BUTTON1_MASK |
                         CDK_BUTTON2_MASK,
                         NULL, 0,
                         CDK_ACTION_MOVE |
                         CDK_ACTION_COPY |
                         CDK_ACTION_LINK |
                         CDK_ACTION_ASK);

    target_list = ctk_target_list_new (targets, G_N_ELEMENTS (targets));
    ctk_target_list_add_uri_targets (target_list, BAUL_ICON_DND_URI_LIST);
    ctk_drag_source_set_target_list (button_data->button, target_list);
    ctk_target_list_unref (target_list);

    g_signal_connect (button_data->button, "drag-data-get",
                      G_CALLBACK (button_drag_data_get_cb),
                      button_data);
}

static void
button_data_file_changed (BaulFile *file,
                          ButtonData *button_data)
{
    GFile *location, *current_location;
    ButtonData *current_button_data;
    BaulPathBar *path_bar;
    gboolean renamed, child;

    path_bar = (BaulPathBar *) ctk_widget_get_ancestor (button_data->button,
               BAUL_TYPE_PATH_BAR);
    if (path_bar == NULL)
    {
        return;
    }

    g_return_if_fail (path_bar->current_path != NULL);
    g_return_if_fail (path_bar->current_button_data != NULL);

    current_button_data = path_bar->current_button_data;

    location = baul_file_get_location (file);
    if (!g_file_equal (button_data->path, location))
    {
        GFile *parent, *button_parent;

        parent = g_file_get_parent (location);
        button_parent = g_file_get_parent (button_data->path);

        renamed = (parent != NULL && button_parent != NULL) &&
                  g_file_equal (parent, button_parent);

        if (parent != NULL)
        {
            g_object_unref (parent);
        }
        if (button_parent != NULL)
        {
            g_object_unref (button_parent);
        }

        if (renamed)
        {
            button_data->path = g_object_ref (location);
        }
        else
        {
            /* the file has been moved.
             * If it was below the currently displayed location, remove it.
             * If it was not below the currently displayed location, update the path bar
             */
            child = g_file_has_prefix (button_data->path,
                                       path_bar->current_path);

            if (child)
            {
                /* moved file inside current path hierarchy */
                g_object_unref (location);
                location = g_file_get_parent (button_data->path);
                current_location = g_object_ref (path_bar->current_path);
            }
            else
            {
                /* moved current path, or file outside current path hierarchy.
                 * Update path bar to new locations.
                 */
                current_location = baul_file_get_location (current_button_data->file);
            }

            baul_path_bar_update_path (path_bar, location, FALSE);
            baul_path_bar_set_path (path_bar, current_location);
            g_object_unref (location);
            g_object_unref (current_location);
            return;
        }
    }
    else if (baul_file_is_gone (file))
    {
        gint idx, position;

        /* if the current or a parent location are gone, don't do anything, as the view
         * will get the event too and call us back.
         */
        current_location = baul_file_get_location (current_button_data->file);

        if (g_file_has_prefix (location, current_location))
        {
            /* remove this and the following buttons */
            position = g_list_position (path_bar->button_list,
                                        g_list_find (path_bar->button_list, button_data));

            if (position != -1)
            {
                for (idx = 0; idx <= position; idx++)
                {
                    ctk_container_remove (CTK_CONTAINER (path_bar),
                                          BUTTON_DATA (path_bar->button_list->data)->button);
                }
            }
        }

        g_object_unref (current_location);
        g_object_unref (location);
        return;
    }
    g_object_unref (location);

    /* MOUNTs use the GMount as the name, so don't update for those */
    if (button_data->type != MOUNT_BUTTON)
    {
        char *display_name;

        display_name = baul_file_get_display_name (file);
        if (g_strcmp0 (display_name, button_data->dir_name) != 0)
        {
            g_free (button_data->dir_name);
            button_data->dir_name = g_strdup (display_name);
        }

        g_free (display_name);
    }
    baul_path_bar_update_button_appearance (button_data);
}

static ButtonData *
make_directory_button (BaulPathBar  *path_bar,
                       BaulFile     *file,
                       gboolean          current_dir,
                       gboolean          base_dir,
                       gboolean          file_is_hidden)
{
    GFile *path;
    CtkWidget *child;
    ButtonData *button_data;

    path = baul_file_get_location (file);

    child = NULL;

    file_is_hidden = !! file_is_hidden;
    /* Is it a special button? */
    button_data = g_new0 (ButtonData, 1);

    setup_button_type (button_data, path_bar, path);
    button_data->button = ctk_toggle_button_new ();
    ctk_style_context_add_class (ctk_widget_get_style_context (button_data->button),
                                 "text-button");
    ctk_widget_set_focus_on_click (button_data->button, FALSE);
    ctk_widget_add_events (button_data->button, CDK_SCROLL_MASK);
    /* TODO update button type when xdg directories change */

    button_data->drag_info.target_location = g_object_ref (path);

    button_data->image = ctk_image_new ();

    switch (button_data->type)
    {
    case ROOT_BUTTON:
    /* Fall through */
    case HOME_BUTTON:
    /* Fall through */
    case DESKTOP_BUTTON:
    /* Fall through */
    case MOUNT_BUTTON:
    /* Fall through */
    case DEFAULT_LOCATION_BUTTON:
        button_data->label = ctk_label_new (NULL);
        child = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
        ctk_box_pack_start (CTK_BOX (child), button_data->image, FALSE, FALSE, 0);
        ctk_box_pack_start (CTK_BOX (child), button_data->label, FALSE, FALSE, 0);

        break;
    case NORMAL_BUTTON:
    default:
        button_data->label = ctk_label_new (NULL);
        child = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
        ctk_box_pack_start (CTK_BOX (child), button_data->image, FALSE, FALSE, 0);
        ctk_box_pack_start (CTK_BOX (child), button_data->label, FALSE, FALSE, 0);
        button_data->is_base_dir = base_dir;
    }

    if (button_data->path == NULL)
    {
        button_data->path = g_object_ref (path);
    }
    if (button_data->dir_name == NULL)
    {
        button_data->dir_name = baul_file_get_display_name (file);
    }
    if (button_data->file == NULL)
    {
        button_data->file = baul_file_ref (file);
        baul_file_monitor_add (button_data->file, button_data,
                               BAUL_FILE_ATTRIBUTES_FOR_ICON);
        button_data->file_changed_signal_id =
            g_signal_connect (button_data->file, "changed",
                              G_CALLBACK (button_data_file_changed),
                              button_data);
    }

    button_data->file_is_hidden = file_is_hidden;

    ctk_container_add (CTK_CONTAINER (button_data->button), child);
    ctk_widget_show_all (button_data->button);

    baul_path_bar_update_button_state (button_data, current_dir);

    g_signal_connect (button_data->button, "clicked", G_CALLBACK (button_clicked_cb), button_data);
    g_signal_connect (button_data->button, "button-press-event", G_CALLBACK (button_event_cb), button_data);
    g_signal_connect (button_data->button, "button-release-event", G_CALLBACK (button_event_cb), button_data);
    g_signal_connect (button_data->button, "drag-begin", G_CALLBACK (button_drag_begin_cb), button_data);
    g_object_weak_ref (G_OBJECT (button_data->button), (GWeakNotify) button_data_free, button_data);

    setup_button_drag_source (button_data);

    baul_drag_slot_proxy_init (button_data->button,
                               &(button_data->drag_info));

    g_object_unref (path);

    return button_data;
}

static gboolean
baul_path_bar_check_parent_path (BaulPathBar *path_bar,
                                 GFile *location,
                                 ButtonData **current_button_data)
{
    GList *list;
    GList *current_path;
    gboolean need_new_fake_root;

    current_path = NULL;
    need_new_fake_root = FALSE;

    if (current_button_data)
    {
        *current_button_data = NULL;
    }

    for (list = path_bar->button_list; list; list = list->next)
    {
        ButtonData *button_data;

        button_data = list->data;
        if (g_file_equal (location, button_data->path))
        {
            current_path = list;

            if (current_button_data)
            {
                *current_button_data = button_data;
            }
            break;
        }
        if (list == path_bar->fake_root)
        {
            need_new_fake_root = TRUE;
        }
    }

    if (current_path)
    {

        if (need_new_fake_root)
        {
            path_bar->fake_root = NULL;
            for (list = current_path; list; list = list->next)
            {
                ButtonData *button_data;

                button_data = list->data;
                if (list->prev != NULL &&
                        button_data->fake_root)
                {
                    path_bar->fake_root = list;
                    break;
                }
            }
        }

        for (list = path_bar->button_list; list; list = list->next)
        {

            baul_path_bar_update_button_state (BUTTON_DATA (list->data),
                                               (list == current_path) ? TRUE : FALSE);
        }

        if (!ctk_widget_get_child_visible (BUTTON_DATA (current_path->data)->button))
        {
            path_bar->first_scrolled_button = current_path;
            ctk_widget_queue_resize (CTK_WIDGET (path_bar));
        }
        return TRUE;
    }
    return FALSE;
}

static gboolean
baul_path_bar_update_path (BaulPathBar *path_bar,
			   GFile       *file_path,
			   gboolean     emit_signal G_GNUC_UNUSED)
{
    BaulFile *file, *parent_file;
    gboolean first_directory, last_directory;
    gboolean result;
    GList *new_buttons, *l, *fake_root;
    ButtonData *button_data, *current_button_data;

    g_return_val_if_fail (BAUL_IS_PATH_BAR (path_bar), FALSE);
    g_return_val_if_fail (file_path != NULL, FALSE);

    fake_root = NULL;
    result = TRUE;
    first_directory = TRUE;
    new_buttons = NULL;
    current_button_data = NULL;

    file = baul_file_get (file_path);

    while (file != NULL)
    {
        parent_file = baul_file_get_parent (file);
        last_directory = !parent_file;
        button_data = make_directory_button (path_bar, file, first_directory, last_directory, FALSE);
        baul_file_unref (file);

        if (first_directory)
        {
            current_button_data = button_data;
        }

        new_buttons = g_list_prepend (new_buttons, button_data);

        if (parent_file != NULL &&
                button_data->fake_root)
        {
            fake_root = new_buttons;
        }

        file = parent_file;
        first_directory = FALSE;
    }

    baul_path_bar_clear_buttons (path_bar);
    path_bar->button_list = g_list_reverse (new_buttons);
    path_bar->fake_root = fake_root;

    for (l = path_bar->button_list; l; l = l->next)
    {
        CtkWidget *button;
        button = BUTTON_DATA (l->data)->button;
        ctk_container_add (CTK_CONTAINER (path_bar), button);
    }

    if (path_bar->current_path != NULL)
    {
        g_object_unref (path_bar->current_path);
    }

    path_bar->current_path = g_object_ref (file_path);

    path_bar->current_button_data = current_button_data;

    return result;
}

gboolean
baul_path_bar_set_path (BaulPathBar *path_bar, GFile *file_path)
{
    ButtonData *button_data;

    g_return_val_if_fail (BAUL_IS_PATH_BAR (path_bar), FALSE);
    g_return_val_if_fail (file_path != NULL, FALSE);

    /* Check whether the new path is already present in the pathbar as buttons.
     * This could be a parent directory or a previous selected subdirectory. */
    if (baul_path_bar_check_parent_path (path_bar, file_path, &button_data))
    {
        if (path_bar->current_path != NULL)
        {
            g_object_unref (path_bar->current_path);
        }

        path_bar->current_path = g_object_ref (file_path);
        path_bar->current_button_data = button_data;

        return TRUE;
    }

    return baul_path_bar_update_path (path_bar, file_path, TRUE);
}

GFile *
baul_path_bar_get_path_for_button (BaulPathBar *path_bar,
                                   CtkWidget       *button)
{
    GList *list;

    g_return_val_if_fail (BAUL_IS_PATH_BAR (path_bar), NULL);
    g_return_val_if_fail (CTK_IS_BUTTON (button), NULL);

    for (list = path_bar->button_list; list; list = list->next)
    {
        ButtonData *button_data;
        button_data = BUTTON_DATA (list->data);
        if (button_data->button == button)
        {
            return g_object_ref (button_data->path);
        }
    }

    return NULL;
}

CtkWidget *
baul_path_bar_get_button_from_button_list_entry (gpointer entry)
{
    return BUTTON_DATA(entry)->button;
}
