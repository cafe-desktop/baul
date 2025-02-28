/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-icon-container.c - Icon container widget.

   Copyright (C) 1999, 2000 Free Software Foundation
   Copyright (C) 2000, 2001 Eazel, Inc.
   Copyright (C) 2002, 2003 Red Hat, Inc.

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

   Authors: Ettore Perazzoli <ettore@gnu.org>,
   Darin Adler <darin@bentspoon.com>
*/

#include <config.h>
#include <math.h>
#include <atk/atkaction.h>
#include <cdk/cdkkeysyms.h>
#include <ctk/ctk.h>
#include <cdk/cdkx.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <string.h>

#include <eel/eel-accessibility.h>
#include <eel/eel-background.h>
#include <eel/eel-vfs-extensions.h>
#include <eel/eel-gdk-pixbuf-extensions.h>
#include <eel/eel-cafe-extensions.h>
#include <eel/eel-ctk-extensions.h>
#include <eel/eel-art-extensions.h>
#include <eel/eel-editable-label.h>
#include <eel/eel-string.h>
#include <eel/eel-canvas.h>
#include <eel/eel-canvas-rect-ellipse.h>

#include "baul-icon-container.h"
#include "baul-debug-log.h"
#include "baul-global-preferences.h"
#include "baul-icon-private.h"
#include "baul-lib-self-check-functions.h"
#include "baul-marshal.h"

#define TAB_NAVIGATION_DISABLED

/* Interval for updating the rubberband selection, in milliseconds.  */
#define RUBBERBAND_TIMEOUT_INTERVAL 10

/* Initial unpositioned icon value */
#define ICON_UNPOSITIONED_VALUE -1

/* Timeout for making the icon currently selected for keyboard operation visible.
 * If this is 0, you can get into trouble with extra scrolling after holding
 * down the arrow key for awhile when there are many items.
 */
#define KEYBOARD_ICON_REVEAL_TIMEOUT 10

#define CONTEXT_MENU_TIMEOUT_INTERVAL 500

/* Maximum amount of milliseconds the mouse button is allowed to stay down
 * and still be considered a click.
 */
#define MAX_CLICK_TIME 1500

/* Button assignments. */
#define DRAG_BUTTON 1
#define RUBBERBAND_BUTTON 1
#define MIDDLE_BUTTON 2
#define CONTEXTUAL_MENU_BUTTON 3
#define DRAG_MENU_BUTTON 2

/* Maximum size (pixels) allowed for icons at the standard zoom level. */
#define MINIMUM_IMAGE_SIZE 24
#define MAXIMUM_IMAGE_SIZE 96

#define ICON_PAD_LEFT 4
#define ICON_PAD_RIGHT 4
#define ICON_BASE_WIDTH 96

#define ICON_PAD_TOP 4
#define ICON_PAD_BOTTOM 4

#define CONTAINER_PAD_LEFT 4
#define CONTAINER_PAD_RIGHT 4
#define CONTAINER_PAD_TOP 4
#define CONTAINER_PAD_BOTTOM 4

#define STANDARD_ICON_GRID_WIDTH 155

#define TEXT_BESIDE_ICON_GRID_WIDTH 205

/* Desktop layout mode defines */
#define DESKTOP_PAD_HORIZONTAL 	10
#define DESKTOP_PAD_VERTICAL 	10
#define SNAP_SIZE_X 		78
#define SNAP_SIZE_Y 		20

#define DEFAULT_SELECTION_BOX_ALPHA 0x40
#define DEFAULT_HIGHLIGHT_ALPHA 0xff
#define DEFAULT_NORMAL_ALPHA 0xff
#define DEFAULT_PRELIGHT_ALPHA 0xff
#define DEFAULT_LIGHT_INFO_COLOR "#AAAAFD"
#define DEFAULT_DARK_INFO_COLOR  "#33337F"

#define MINIMUM_EMBEDDED_TEXT_RECT_WIDTH       20
#define MINIMUM_EMBEDDED_TEXT_RECT_HEIGHT      20

/* If icon size is bigger than this, request large embedded text.
 * Its selected so that the non-large text should fit in "normal" icon sizes
 */
#define ICON_SIZE_FOR_LARGE_EMBEDDED_TEXT 55

/* From baul-icon-canvas-item.c */
#define MAX_TEXT_WIDTH_BESIDE 90

#define SNAP_HORIZONTAL(func,x) ((func ((double)((x) - DESKTOP_PAD_HORIZONTAL) / SNAP_SIZE_X) * SNAP_SIZE_X) + DESKTOP_PAD_HORIZONTAL)
#define SNAP_VERTICAL(func, y) ((func ((double)((y) - DESKTOP_PAD_VERTICAL) / SNAP_SIZE_Y) * SNAP_SIZE_Y) + DESKTOP_PAD_VERTICAL)

#define SNAP_NEAREST_HORIZONTAL(x) SNAP_HORIZONTAL (eel_round, x)
#define SNAP_NEAREST_VERTICAL(y) SNAP_VERTICAL (eel_round, y)

#define SNAP_CEIL_HORIZONTAL(x) SNAP_HORIZONTAL (ceil, x)
#define SNAP_CEIL_VERTICAL(y) SNAP_VERTICAL (ceil, y)

/* Copied from BaulIconContainer */
#define BAUL_ICON_CONTAINER_SEARCH_DIALOG_TIMEOUT 5

/* Copied from BaulFile */
#define UNDEFINED_TIME ((time_t) (-1))

enum
{
    ACTION_ACTIVATE,
    ACTION_MENU,
    LAST_ACTION
};

typedef struct
{
    GList *selection;
    char *action_descriptions[LAST_ACTION];
} BaulIconContainerAccessiblePrivate;

static GType         baul_icon_container_accessible_get_type (void);

typedef struct _BaulIconContainerAccessible BaulIconContainerAccessible;
typedef struct _BaulIconContainerAccessibleClass BaulIconContainerAccessibleClass;

struct _BaulIconContainerAccessible
{
    EelCanvasAccessible parent;
};

struct _BaulIconContainerAccessibleClass
{
    EelCanvasAccessibleClass parent_class;
};

static void          activate_selected_items                        (BaulIconContainer *container);
static void          activate_selected_items_alternate              (BaulIconContainer *container,
        BaulIcon          *icon);
static void          compute_stretch                                (StretchState          *start,
        StretchState          *current);
static BaulIcon *get_first_selected_icon                        (BaulIconContainer *container);
static BaulIcon *get_nth_selected_icon                          (BaulIconContainer *container,
        int                    index);
static gboolean      has_multiple_selection                         (BaulIconContainer *container);
static gboolean      all_selected                                   (BaulIconContainer *container);
static gboolean      has_selection                                  (BaulIconContainer *container);
static void          icon_destroy                                   (BaulIconContainer *container,
        BaulIcon          *icon);
static void          end_renaming_mode                              (BaulIconContainer *container,
        gboolean               commit);
static BaulIcon *get_icon_being_renamed                         (BaulIconContainer *container);
static void          finish_adding_new_icons                        (BaulIconContainer *container);
static inline void   icon_get_bounding_box                          (BaulIcon          *icon,
        int                   *x1_return,
        int                   *y1_return,
        int                   *x2_return,
        int                   *y2_return,
        BaulIconCanvasItemBoundsUsage usage);
static gboolean      is_renaming                                    (BaulIconContainer *container);
static gboolean      is_renaming_pending                            (BaulIconContainer *container);
static void          process_pending_icon_to_rename                 (BaulIconContainer *container);
static void          handle_hadjustment_changed                     (CtkAdjustment         *adjustment,
        BaulIconContainer *container);
static void          handle_vadjustment_changed                     (CtkAdjustment         *adjustment,
        BaulIconContainer *container);
static GList *       baul_icon_container_get_selected_icons (BaulIconContainer *container);
static void          baul_icon_container_update_visible_icons   (BaulIconContainer *container);
static void          reveal_icon                                    (BaulIconContainer *container,
        BaulIcon *icon);

static void	     baul_icon_container_set_rtl_positions (BaulIconContainer *container);
static double	     get_mirror_x_position                     (BaulIconContainer *container,
        BaulIcon *icon,
        double x);
static void         text_ellipsis_limit_changed_container_callback  (gpointer callback_data);

static int compare_icons_horizontal (BaulIconContainer *container,
                                     BaulIcon *icon_a,
                                     BaulIcon *icon_b);

static int compare_icons_vertical (BaulIconContainer *container,
                                   BaulIcon *icon_a,
                                   BaulIcon *icon_b);

static void store_layout_timestamps_now (BaulIconContainer *container);

static gpointer accessible_parent_class;

static GQuark accessible_private_data_quark = 0;

static const char *baul_icon_container_accessible_action_names[] =
{
    "activate",
    "menu",
    NULL
};

static const char *baul_icon_container_accessible_action_descriptions[] =
{
    "Activate selected items",
    "Popup context menu",
    NULL
};

G_DEFINE_TYPE (BaulIconContainer, baul_icon_container, EEL_TYPE_CANVAS);

/* The BaulIconContainer signals.  */
enum
{
    ACTIVATE,
    ACTIVATE_ALTERNATE,
    BAND_SELECT_STARTED,
    BAND_SELECT_ENDED,
    BUTTON_PRESS,
    CAN_ACCEPT_ITEM,
    CONTEXT_CLICK_BACKGROUND,
    CONTEXT_CLICK_SELECTION,
    MIDDLE_CLICK,
    GET_CONTAINER_URI,
    GET_ICON_URI,
    GET_ICON_DROP_TARGET_URI,
    GET_STORED_ICON_POSITION,
    ICON_POSITION_CHANGED,
    GET_STORED_LAYOUT_TIMESTAMP,
    STORE_LAYOUT_TIMESTAMP,
    ICON_TEXT_CHANGED,
    ICON_STRETCH_STARTED,
    ICON_STRETCH_ENDED,
    RENAMING_ICON,
    LAYOUT_CHANGED,
    MOVE_COPY_ITEMS,
    HANDLE_NETSCAPE_URL,
    HANDLE_URI_LIST,
    HANDLE_TEXT,
    HANDLE_RAW,
    PREVIEW,
    SELECTION_CHANGED,
    ICON_ADDED,
    ICON_REMOVED,
    CLEARED,
    START_INTERACTIVE_SEARCH,
    LAST_SIGNAL
};

typedef struct
{
    int **icon_grid;
    int *grid_memory;
    int num_rows;
    int num_columns;
    gboolean tight;
} PlacementGrid;

static guint signals[LAST_SIGNAL] = { 0 };

/* Functions dealing with BaulIcons.  */

static void
icon_free (BaulIcon *icon)
{
    /* Destroy this canvas item; the parent will unref it. */
    eel_canvas_item_destroy (EEL_CANVAS_ITEM (icon->item));
    g_free (icon);
}

static gboolean
icon_is_positioned (const BaulIcon *icon)
{
    return icon->x != ICON_UNPOSITIONED_VALUE && icon->y != ICON_UNPOSITIONED_VALUE;
}


/* x, y are the top-left coordinates of the icon. */
static void
icon_set_position (BaulIcon *icon,
                   double x, double y)
{
    BaulIconContainer *container;
    int x1, x2, y1, y2;
    EelDRect icon_bounds;

    if (icon->x == x && icon->y == y)
    {
        return;
    }

    container = BAUL_ICON_CONTAINER (EEL_CANVAS_ITEM (icon->item)->canvas);

    if (icon == get_icon_being_renamed (container))
    {
        end_renaming_mode (container, TRUE);
    }

    if (baul_icon_container_get_is_fixed_size (container))
    {
        double pixels_per_unit;
        int container_left, container_top, container_right, container_bottom;
        int container_x, container_y, container_width, container_height;
        int item_width, item_height;
        int height_above, width_left;
        int min_x, max_x, min_y, max_y;
        int scale;

        /*  FIXME: This should be:

        container_x = CTK_WIDGET (container)->allocation.x;
        container_y = CTK_WIDGET (container)->allocation.y;
        container_width = CTK_WIDGET (container)->allocation.width;
        container_height = CTK_WIDGET (container)->allocation.height;

        But for some reason the widget allocation is sometimes not done
        at startup, and the allocation is then only 45x60. which is
        really bad.

        For now, we have a cheesy workaround:
        */
        scale = ctk_widget_get_scale_factor (CTK_WIDGET (container));
        container_x = 0;
        container_y = 0;
        container_width = WidthOfScreen (cdk_x11_screen_get_xscreen (cdk_screen_get_default ())) / scale - container_x
                          - container->details->left_margin
                          - container->details->right_margin;
        container_height = HeightOfScreen (cdk_x11_screen_get_xscreen (cdk_screen_get_default ())) / scale - container_y
                           - container->details->top_margin
                           - container->details->bottom_margin;
        pixels_per_unit = EEL_CANVAS (container)->pixels_per_unit;
        /* Clip the position of the icon within our desktop bounds */
        container_left = container_x / pixels_per_unit;
        container_top =  container_y / pixels_per_unit;
        container_right = container_left + container_width / pixels_per_unit;
        container_bottom = container_top + container_height / pixels_per_unit;

        icon_get_bounding_box (icon, &x1, &y1, &x2, &y2,
                               BOUNDS_USAGE_FOR_ENTIRE_ITEM);
        item_width = x2 - x1;
        item_height = y2 - y1;

        icon_bounds = baul_icon_canvas_item_get_icon_rectangle (icon->item);

        /* determine icon rectangle relative to item rectangle */
        height_above = icon_bounds.y0 - y1;
        width_left = icon_bounds.x0 - x1;

        min_x = container_left + DESKTOP_PAD_HORIZONTAL + width_left;
        max_x = container_right - DESKTOP_PAD_HORIZONTAL - item_width + width_left;
        x = CLAMP (x, min_x, max_x);

        min_y = container_top + height_above + DESKTOP_PAD_VERTICAL;
        max_y = container_bottom - DESKTOP_PAD_VERTICAL - item_height + height_above;
        y = CLAMP (y, min_y, max_y);
    }

    if (icon->x == ICON_UNPOSITIONED_VALUE)
    {
        icon->x = 0;
    }
    if (icon->y == ICON_UNPOSITIONED_VALUE)
    {
        icon->y = 0;
    }

    eel_canvas_item_move (EEL_CANVAS_ITEM (icon->item),
                          x - icon->x,
                          y - icon->y);

    icon->x = x;
    icon->y = y;
}

static void
icon_get_size (BaulIconContainer *container,
               BaulIcon *icon,
               guint *size)
{
    if (size != NULL)
    {
        *size = MAX (baul_get_icon_size_for_zoom_level (container->details->zoom_level)
                     * icon->scale, BAUL_ICON_SIZE_SMALLEST);
    }
}

/* The icon_set_size function is used by the stretching user
 * interface, which currently stretches in a way that keeps the aspect
 * ratio. Later we might have a stretching interface that stretches Y
 * separate from X and we will change this around.
 */
static void
icon_set_size (BaulIconContainer *container,
               BaulIcon *icon,
               guint icon_size,
               gboolean snap,
               gboolean update_position)
{
    guint old_size;
    double scale;

    icon_get_size (container, icon, &old_size);
    if (icon_size == old_size)
    {
        return;
    }

    scale = (double) icon_size /
            baul_get_icon_size_for_zoom_level
            (container->details->zoom_level);
    baul_icon_container_move_icon (container, icon,
                                   icon->x, icon->y,
                                   scale, FALSE,
                                   snap, update_position);
}

static void
icon_raise (BaulIcon *icon)
{
    EelCanvasItem *item, *band;

    item = EEL_CANVAS_ITEM (icon->item);
    band = BAUL_ICON_CONTAINER (item->canvas)->details->rubberband_info.selection_rectangle;

    eel_canvas_item_send_behind (item, band);
}

static void
emit_stretch_started (BaulIconContainer *container, BaulIcon *icon)
{
    g_signal_emit (container,
                   signals[ICON_STRETCH_STARTED], 0,
                   icon->data);
}

static void
emit_stretch_ended (BaulIconContainer *container, BaulIcon *icon)
{
    g_signal_emit (container,
                   signals[ICON_STRETCH_ENDED], 0,
                   icon->data);
}

static void
icon_toggle_selected (BaulIconContainer *container,
                      BaulIcon *icon)
{
    end_renaming_mode (container, TRUE);

    icon->is_selected = !icon->is_selected;
    eel_canvas_item_set (EEL_CANVAS_ITEM (icon->item),
                         "highlighted_for_selection", (gboolean) icon->is_selected,
                         NULL);

    /* If the icon is deselected, then get rid of the stretch handles.
     * No harm in doing the same if the item is newly selected.
     */
    if (icon == container->details->stretch_icon)
    {
        container->details->stretch_icon = NULL;
        baul_icon_canvas_item_set_show_stretch_handles (icon->item, FALSE);
        /* snap the icon if necessary */
        if (container->details->keep_aligned)
        {
            baul_icon_container_move_icon (container,
                                           icon,
                                           icon->x, icon->y,
                                           icon->scale,
                                           FALSE, TRUE, TRUE);
        }

        emit_stretch_ended (container, icon);
    }

    /* Raise each newly-selected icon to the front as it is selected. */
    if (icon->is_selected)
    {
        icon_raise (icon);
    }
}

/* Select an icon. Return TRUE if selection has changed. */
static gboolean
icon_set_selected (BaulIconContainer *container,
                   BaulIcon *icon,
                   gboolean select)
{
    g_assert (select == FALSE || select == TRUE);
    g_assert (icon->is_selected == FALSE || icon->is_selected == TRUE);

    if (select == icon->is_selected)
    {
        return FALSE;
    }

    icon_toggle_selected (container, icon);
    g_assert (select == icon->is_selected);
    return TRUE;
}

static inline void
icon_get_bounding_box (BaulIcon *icon,
                       int *x1_return, int *y1_return,
                       int *x2_return, int *y2_return,
                       BaulIconCanvasItemBoundsUsage usage)
{
    double x1, y1, x2, y2;

    if (usage == BOUNDS_USAGE_FOR_DISPLAY)
    {
        eel_canvas_item_get_bounds (EEL_CANVAS_ITEM (icon->item),
                                    &x1, &y1, &x2, &y2);
    }
    else if (usage == BOUNDS_USAGE_FOR_LAYOUT)
    {
        baul_icon_canvas_item_get_bounds_for_layout (icon->item,
                &x1, &y1, &x2, &y2);
    }
    else if (usage == BOUNDS_USAGE_FOR_ENTIRE_ITEM)
    {
        baul_icon_canvas_item_get_bounds_for_entire_item (icon->item,
                &x1, &y1, &x2, &y2);
    }
    else
    {
        g_assert_not_reached ();
    }

    if (x1_return != NULL)
    {
        *x1_return = x1;
    }

    if (y1_return != NULL)
    {
        *y1_return = y1;
    }

    if (x2_return != NULL)
    {
        *x2_return = x2;
    }

    if (y2_return != NULL)
    {
        *y2_return = y2;
    }
}

/* Utility functions for BaulIconContainer.  */

gboolean
baul_icon_container_scroll (BaulIconContainer *container,
                            int delta_x, int delta_y)
{
    CtkAdjustment *hadj, *vadj;
    int old_h_value, old_v_value;

    hadj = ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (container));
    vadj = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (container));

    /* Store the old ajustment values so we can tell if we
     * ended up actually scrolling. We may not have in a case
     * where the resulting value got pinned to the adjustment
     * min or max.
     */
    old_h_value = ctk_adjustment_get_value (hadj);
    old_v_value = ctk_adjustment_get_value (vadj);

    ctk_adjustment_set_value (hadj, ctk_adjustment_get_value (hadj) + delta_x);
    ctk_adjustment_set_value (vadj, ctk_adjustment_get_value (vadj) + delta_y);

    /* return TRUE if we did scroll */
    return ctk_adjustment_get_value (hadj) != old_h_value || ctk_adjustment_get_value (vadj) != old_v_value;
}

static void
pending_icon_to_reveal_destroy_callback (BaulIconCanvasItem *item,
        BaulIconContainer *container)
{
    g_assert (BAUL_IS_ICON_CONTAINER (container));
    g_assert (container->details->pending_icon_to_reveal != NULL);
    g_assert (container->details->pending_icon_to_reveal->item == item);

    container->details->pending_icon_to_reveal = NULL;
}

static BaulIcon*
get_pending_icon_to_reveal (BaulIconContainer *container)
{
    return container->details->pending_icon_to_reveal;
}

static void
set_pending_icon_to_reveal (BaulIconContainer *container, BaulIcon *icon)
{
    BaulIcon *old_icon;

    old_icon = container->details->pending_icon_to_reveal;

    if (icon == old_icon)
    {
        return;
    }

    if (old_icon != NULL)
    {
        g_signal_handlers_disconnect_by_func
        (old_icon->item,
         G_CALLBACK (pending_icon_to_reveal_destroy_callback),
         container);
    }

    if (icon != NULL)
    {
        g_signal_connect (icon->item, "destroy",
                          G_CALLBACK (pending_icon_to_reveal_destroy_callback),
                          container);
    }

    container->details->pending_icon_to_reveal = icon;
}

static void
item_get_canvas_bounds (EelCanvasItem *item,
                        EelIRect *bounds,
                        gboolean safety_pad)
{
    EelDRect world_rect;

    eel_canvas_item_get_bounds (item,
                                &world_rect.x0,
                                &world_rect.y0,
                                &world_rect.x1,
                                &world_rect.y1);
    eel_canvas_item_i2w (item->parent,
                         &world_rect.x0,
                         &world_rect.y0);
    eel_canvas_item_i2w (item->parent,
                         &world_rect.x1,
                         &world_rect.y1);
    if (safety_pad)
    {
        world_rect.x0 -= ICON_PAD_LEFT + ICON_PAD_RIGHT;
        world_rect.x1 += ICON_PAD_LEFT + ICON_PAD_RIGHT;

        world_rect.y0 -= ICON_PAD_TOP + ICON_PAD_BOTTOM;
        world_rect.y1 += ICON_PAD_TOP + ICON_PAD_BOTTOM;
    }

    eel_canvas_w2c (item->canvas,
                    world_rect.x0,
                    world_rect.y0,
                    &bounds->x0,
                    &bounds->y0);
    eel_canvas_w2c (item->canvas,
                    world_rect.x1,
                    world_rect.y1,
                    &bounds->x1,
                    &bounds->y1);
}

static void
icon_get_row_and_column_bounds (BaulIconContainer *container,
                                BaulIcon *icon,
                                EelIRect *bounds,
                                gboolean safety_pad)
{
    GList *p;
    EelIRect one_bounds;
    BaulIcon *one_icon = NULL;

    item_get_canvas_bounds (EEL_CANVAS_ITEM (icon->item), bounds, safety_pad);

    for (p = container->details->icons; p != NULL; p = p->next) {
        one_icon = p->data;

        if (icon == one_icon) {
            continue;
        }

        if (compare_icons_horizontal (container, icon, one_icon) == 0) {
            item_get_canvas_bounds (EEL_CANVAS_ITEM (one_icon->item), &one_bounds, safety_pad);
            bounds->x0 = MIN (bounds->x0, one_bounds.x0);
            bounds->x1 = MAX (bounds->x1, one_bounds.x1);
        }

        if (compare_icons_vertical (container, icon, one_icon) == 0) {
            item_get_canvas_bounds (EEL_CANVAS_ITEM (one_icon->item), &one_bounds, safety_pad);
            bounds->y0 = MIN (bounds->y0, one_bounds.y0);
            bounds->y1 = MAX (bounds->y1, one_bounds.y1);
        }
    }


}

static void
reveal_icon (BaulIconContainer *container,
             BaulIcon *icon)
{
    CtkAllocation allocation;
    CtkAdjustment *hadj, *vadj;
    EelIRect bounds;

    if (!icon_is_positioned (icon)) {
        set_pending_icon_to_reveal (container, icon);
        return;
    }

    set_pending_icon_to_reveal (container, NULL);

    ctk_widget_get_allocation (CTK_WIDGET (container), &allocation);

    hadj = ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (container));
    vadj = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (container));

    if (baul_icon_container_is_auto_layout (container)) {
        /* ensure that we reveal the entire row/column */
        icon_get_row_and_column_bounds (container, icon, &bounds, TRUE);
    } else {
        item_get_canvas_bounds (EEL_CANVAS_ITEM (icon->item), &bounds, TRUE);
    }
    if (bounds.y0 < ctk_adjustment_get_value (vadj)) {
        ctk_adjustment_set_value (vadj, bounds.y0);
    } else if (bounds.y1 > ctk_adjustment_get_value (vadj) + allocation.height) {
        ctk_adjustment_set_value (vadj, bounds.y1 - allocation.height);
    }

    if (bounds.x0 < ctk_adjustment_get_value (hadj)) {
        ctk_adjustment_set_value (hadj, bounds.x0);
    } else if (bounds.x1 > ctk_adjustment_get_value (hadj) + allocation.width) {
        if (bounds.x1 - allocation.width > bounds.x0) {
            ctk_adjustment_set_value (hadj, bounds.x0);
        } else {
            ctk_adjustment_set_value (hadj, bounds.x1 - allocation.width);
        }
    }
}

static void
process_pending_icon_to_reveal (BaulIconContainer *container)
{
    BaulIcon *pending_icon_to_reveal;

    pending_icon_to_reveal = get_pending_icon_to_reveal (container);

    if (pending_icon_to_reveal != NULL) {
        reveal_icon (container, pending_icon_to_reveal);
    }
}

static gboolean
keyboard_icon_reveal_timeout_callback (gpointer data)
{
    BaulIconContainer *container;
    BaulIcon *icon;

    container = BAUL_ICON_CONTAINER (data);
    icon = container->details->keyboard_icon_to_reveal;

    g_assert (icon != NULL);

    /* Only reveal the icon if it's still the keyboard focus or if
     * it's still selected. Someone originally thought we should
     * cancel this reveal if the user manages to sneak a direct
     * scroll in before the timeout fires, but we later realized
     * this wouldn't actually be an improvement
     * (see bugzilla.gnome.org 40612).
     */
    if (icon == container->details->keyboard_focus
            || icon->is_selected) {
        reveal_icon (container, icon);
    }
    container->details->keyboard_icon_reveal_timer_id = 0;

    return FALSE;
}

static void
unschedule_keyboard_icon_reveal (BaulIconContainer *container)
{
    BaulIconContainerDetails *details;

    details = container->details;

    if (details->keyboard_icon_reveal_timer_id != 0) {
        g_source_remove (details->keyboard_icon_reveal_timer_id);
    }
}

static void
schedule_keyboard_icon_reveal (BaulIconContainer *container,
                               BaulIcon *icon)
{
    BaulIconContainerDetails *details;

    details = container->details;

    unschedule_keyboard_icon_reveal (container);

    details->keyboard_icon_to_reveal = icon;
    details->keyboard_icon_reveal_timer_id
        = g_timeout_add (KEYBOARD_ICON_REVEAL_TIMEOUT,
                         keyboard_icon_reveal_timeout_callback,
                         container);
}

static void
clear_keyboard_focus (BaulIconContainer *container)
{
    if (container->details->keyboard_focus != NULL)
    {
        eel_canvas_item_set (EEL_CANVAS_ITEM (container->details->keyboard_focus->item),
                             "highlighted_as_keyboard_focus", 0,
                             NULL);
    }

    container->details->keyboard_focus = NULL;
}

static void inline
emit_atk_focus_state_change (BaulIcon *icon, gboolean focused)
{
    AtkObject *atk_object = atk_gobject_accessible_for_object (G_OBJECT (icon->item));
    atk_object_notify_state_change (atk_object, ATK_STATE_FOCUSED, focused);
}

/* Set @icon as the icon currently selected for keyboard operations. */
static void
set_keyboard_focus (BaulIconContainer *container,
                    BaulIcon *icon)
{
    g_assert (icon != NULL);

    if (icon == container->details->keyboard_focus)
    {
        return;
    }

    clear_keyboard_focus (container);

    container->details->keyboard_focus = icon;

    eel_canvas_item_set (EEL_CANVAS_ITEM (container->details->keyboard_focus->item),
                         "highlighted_as_keyboard_focus", 1,
                         NULL);

    emit_atk_focus_state_change (icon, TRUE);
}

static void
set_keyboard_rubberband_start (BaulIconContainer *container,
                               BaulIcon *icon)
{
    container->details->keyboard_rubberband_start = icon;
}

static void
clear_keyboard_rubberband_start (BaulIconContainer *container)
{
    container->details->keyboard_rubberband_start = NULL;
}

/* carbon-copy of eel_canvas_group_bounds(), but
 * for BaulIconContainerItems it returns the
 * bounds for the “entire item”.
 */
static void
get_icon_bounds_for_canvas_bounds (EelCanvasGroup *group,
                                   double *x1, double *y1,
                                   double *x2, double *y2,
                                   BaulIconCanvasItemBoundsUsage usage)
{
    EelCanvasItem *child;
    GList *list;
    double tx1, ty1, tx2, ty2;
    double minx, miny, maxx, maxy;
    int set;

    /* Get the bounds of the first visible item */

    child = NULL; /* Unnecessary but eliminates a warning. */

    set = FALSE;

    for (list = group->item_list; list; list = list->next)
    {
        child = list->data;

        if (child->flags & EEL_CANVAS_ITEM_VISIBLE)
        {
            set = TRUE;
            if (!BAUL_IS_ICON_CANVAS_ITEM (child) ||
                    usage == BOUNDS_USAGE_FOR_DISPLAY)
            {
                eel_canvas_item_get_bounds (child, &minx, &miny, &maxx, &maxy);
            }
            else if (usage == BOUNDS_USAGE_FOR_LAYOUT)
            {
                baul_icon_canvas_item_get_bounds_for_layout (BAUL_ICON_CANVAS_ITEM (child),
                        &minx, &miny, &maxx, &maxy);
            }
            else if (usage == BOUNDS_USAGE_FOR_ENTIRE_ITEM)
            {
                baul_icon_canvas_item_get_bounds_for_entire_item (BAUL_ICON_CANVAS_ITEM (child),
                        &minx, &miny, &maxx, &maxy);
            }
            else
            {
                g_assert_not_reached ();
            }
            break;
        }
    }

    /* If there were no visible items, return an empty bounding box */

    if (!set)
    {
        *x1 = *y1 = *x2 = *y2 = 0.0;
        return;
    }

    /* Now we can grow the bounds using the rest of the items */

    list = list->next;

    for (; list; list = list->next)
    {
        child = list->data;

        if (!(child->flags & EEL_CANVAS_ITEM_VISIBLE))
            continue;

        if (!BAUL_IS_ICON_CANVAS_ITEM (child) ||
                usage == BOUNDS_USAGE_FOR_DISPLAY)
        {
            eel_canvas_item_get_bounds (child, &tx1, &ty1, &tx2, &ty2);
        }
        else if (usage == BOUNDS_USAGE_FOR_LAYOUT)
        {
            baul_icon_canvas_item_get_bounds_for_layout (BAUL_ICON_CANVAS_ITEM (child),
                    &tx1, &ty1, &tx2, &ty2);
        }
        else if (usage == BOUNDS_USAGE_FOR_ENTIRE_ITEM)
        {
            baul_icon_canvas_item_get_bounds_for_entire_item (BAUL_ICON_CANVAS_ITEM (child),
                    &tx1, &ty1, &tx2, &ty2);
        }
        else
        {
            g_assert_not_reached ();
        }

        if (tx1 < minx)
            minx = tx1;

        if (ty1 < miny)
            miny = ty1;

        if (tx2 > maxx)
            maxx = tx2;

        if (ty2 > maxy)
            maxy = ty2;
    }

    /* Make the bounds be relative to our parent's coordinate system */

    if (EEL_CANVAS_ITEM (group)->parent)
    {
        minx += group->xpos;
        miny += group->ypos;
        maxx += group->xpos;
        maxy += group->ypos;
    }

    if (x1 != NULL)
    {
        *x1 = minx;
    }

    if (y1 != NULL)
    {
        *y1 = miny;
    }

    if (x2 != NULL)
    {
        *x2 = maxx;
    }

    if (y2 != NULL)
    {
        *y2 = maxy;
    }
}

static void
get_all_icon_bounds (BaulIconContainer *container,
                     double *x1, double *y1,
                     double *x2, double *y2,
                     BaulIconCanvasItemBoundsUsage usage)
{
    /* FIXME bugzilla.gnome.org 42477: Do we have to do something about the rubberband
     * here? Any other non-icon items?
     */
    get_icon_bounds_for_canvas_bounds (EEL_CANVAS_GROUP (EEL_CANVAS (container)->root),
                                       x1, y1, x2, y2, usage);
}

/* Don't preserve visible white space the next time the scroll region
 * is recomputed when the container is not empty. */
void
baul_icon_container_reset_scroll_region (BaulIconContainer *container)
{
    container->details->reset_scroll_region_trigger = TRUE;
}

/* Set a new scroll region without eliminating any of the currently-visible area. */
static void
canvas_set_scroll_region_include_visible_area (EelCanvas *canvas,
        double x1, double y1,
        double x2, double y2)
{
    double old_x1, old_y1, old_x2, old_y2;
    double old_scroll_x, old_scroll_y;
    double height, width;
    CtkAllocation allocation;

    eel_canvas_get_scroll_region (canvas, &old_x1, &old_y1, &old_x2, &old_y2);
    ctk_widget_get_allocation (CTK_WIDGET (canvas), &allocation);

    width = (allocation.width) / canvas->pixels_per_unit;
    height = (allocation.height) / canvas->pixels_per_unit;

    old_scroll_x = ctk_adjustment_get_value (ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (canvas)));
    old_scroll_y = ctk_adjustment_get_value (ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (canvas)));

    x1 = MIN (x1, old_x1 + old_scroll_x);
    y1 = MIN (y1, old_y1 + old_scroll_y);
    x2 = MAX (x2, old_x1 + old_scroll_x + width);
    y2 = MAX (y2, old_y1 + old_scroll_y + height);

    eel_canvas_set_scroll_region
    (canvas, x1, y1, x2, y2);
}

void
baul_icon_container_update_scroll_region (BaulIconContainer *container)
{
    double x1, y1, x2, y2;
    double pixels_per_unit;
    CtkAdjustment *hadj, *vadj;
    float step_increment;
    gboolean reset_scroll_region;
    CtkAllocation allocation;

    pixels_per_unit = EEL_CANVAS (container)->pixels_per_unit;

    if (baul_icon_container_get_is_fixed_size (container))
    {
        /* Set the scroll region to the size of the container allocation */
        ctk_widget_get_allocation (CTK_WIDGET (container), &allocation);
        eel_canvas_set_scroll_region
        (EEL_CANVAS (container),
         (double) - container->details->left_margin / pixels_per_unit,
         (double) - container->details->top_margin / pixels_per_unit,
         ((double) (allocation.width - 1)
          - container->details->left_margin
          - container->details->right_margin)
         / pixels_per_unit,
         ((double) (allocation.height - 1)
          - container->details->top_margin
          - container->details->bottom_margin)
         / pixels_per_unit);
        return;
    }

    reset_scroll_region = container->details->reset_scroll_region_trigger
                          || baul_icon_container_is_empty (container)
                          || baul_icon_container_is_auto_layout (container);

    /* The trigger is only cleared when container is non-empty, so
     * callers can reliably reset the scroll region when an item
     * is added even if extraneous relayouts are called when the
     * window is still empty.
     */
    if (!baul_icon_container_is_empty (container))
    {
        container->details->reset_scroll_region_trigger = FALSE;
    }

    get_all_icon_bounds (container, &x1, &y1, &x2, &y2, BOUNDS_USAGE_FOR_ENTIRE_ITEM);

    /* Add border at the "end"of the layout (i.e. after the icons), to
     * ensure we get some space when scrolled to the end.
     * For horizontal layouts, we add a bottom border.
     * Vertical layout is used by the compact view so the end
     * depends on the RTL setting.
     */
    if (baul_icon_container_is_layout_vertical (container))
    {
        if (baul_icon_container_is_layout_rtl (container))
        {
            x1 -= ICON_PAD_LEFT + CONTAINER_PAD_LEFT;
        }
        else
        {
            x2 += ICON_PAD_RIGHT + CONTAINER_PAD_RIGHT;
        }
    }
    else
    {
        y2 += ICON_PAD_BOTTOM + CONTAINER_PAD_BOTTOM;
    }

    /* Auto-layout assumes a 0, 0 scroll origin and at least allocation->width.
     * Then we lay out to the right or to the left, so
     * x can be < 0 and > allocation */
    if (baul_icon_container_is_auto_layout (container))
    {
        ctk_widget_get_allocation (CTK_WIDGET (container), &allocation);
        x1 = MIN (x1, 0);
        x2 = MAX (x2, allocation.width / pixels_per_unit);
        y1 = 0;
    }
    else
    {
        /* Otherwise we add the padding that is at the start of the
           layout */
        if (baul_icon_container_is_layout_rtl (container))
        {
            x2 += ICON_PAD_RIGHT + CONTAINER_PAD_RIGHT;
        }
        else
        {
            x1 -= ICON_PAD_LEFT + CONTAINER_PAD_LEFT;
        }
        y1 -= ICON_PAD_TOP + CONTAINER_PAD_TOP;
    }

    x2 -= 1;
    x2 = MAX(x1, x2);

    y2 -= 1;
    y2 = MAX(y1, y2);

    if (reset_scroll_region)
    {
        eel_canvas_set_scroll_region
        (EEL_CANVAS (container),
         x1, y1, x2, y2);
    }
    else
    {
        canvas_set_scroll_region_include_visible_area
        (EEL_CANVAS (container),
         x1, y1, x2, y2);
    }

    hadj = ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (container));
    vadj = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (container));

    /* Scroll by 1/4 icon each time you click. */
    step_increment = baul_get_icon_size_for_zoom_level
                     (container->details->zoom_level) / 4;
    if (ctk_adjustment_get_step_increment (hadj) != step_increment)
    {
        ctk_adjustment_set_step_increment (hadj, step_increment);
    }
    if (ctk_adjustment_get_step_increment (vadj) != step_increment)
    {
        ctk_adjustment_set_step_increment (vadj, step_increment);
    }
}

static int
compare_icons (gconstpointer a, gconstpointer b, gpointer icon_container)
{
    BaulIconContainerClass *klass;
    const BaulIcon *icon_a, *icon_b;

    icon_a = a;
    icon_b = b;
    klass  = BAUL_ICON_CONTAINER_GET_CLASS (icon_container);

    return klass->compare_icons (icon_container, icon_a->data, icon_b->data);
}

static void
sort_icons (BaulIconContainer *container,
            GList                **icons)
{
    BaulIconContainerClass *klass;

    klass = BAUL_ICON_CONTAINER_GET_CLASS (container);
    g_assert (klass->compare_icons != NULL);

    *icons = g_list_sort_with_data (*icons, compare_icons, container);
}

static void
resort (BaulIconContainer *container)
{
    sort_icons (container, &container->details->icons);
}

typedef struct
{
    double width;
    double height;
    double x_offset;
    double y_offset;
} IconPositions;

static void
lay_down_one_line (BaulIconContainer *container,
                   GList *line_start,
                   GList *line_end,
                   double y,
                   double max_height,
                   GArray *positions,
                   gboolean whole_text)
{
    GList *p;
    double x, y_offset;
    IconPositions *position;
    int i;
    gboolean is_rtl;
    BaulIcon *icon = NULL;

    is_rtl = baul_icon_container_is_layout_rtl (container);

    /* Lay out the icons along the baseline. */
    x = ICON_PAD_LEFT;
    i = 0;
    for (p = line_start; p != line_end; p = p->next)
    {
        icon = p->data;

        position = &g_array_index (positions, IconPositions, i++);

        if (container->details->label_position == BAUL_ICON_LABEL_POSITION_BESIDE)
        {
            y_offset = (max_height - position->height) / 2;
        }
        else
        {
            y_offset = position->y_offset;
        }

        icon_set_position
        (icon,
         is_rtl ? get_mirror_x_position (container, icon, x + position->x_offset) : x + position->x_offset,
         y + y_offset);
        baul_icon_canvas_item_set_entire_text (icon->item, whole_text);

        icon->saved_ltr_x = is_rtl ? get_mirror_x_position (container, icon, icon->x) : icon->x;

        x += position->width;
    }
}

static void
lay_down_one_column (BaulIconContainer *container,
                     GList *line_start,
                     GList *line_end,
                     double x,
                     double y_start,
                     double y_iter,
                     GArray *positions)
{
    GList *p;
    double y;
    int i;
    gboolean is_rtl;
    IconPositions *position = NULL;
    BaulIcon *icon = NULL;

    is_rtl = baul_icon_container_is_layout_rtl (container);

    /* Lay out the icons along the baseline. */
    y = y_start;
    i = 0;
    for (p = line_start; p != line_end; p = p->next)
    {
        icon = p->data;

        position = &g_array_index (positions, IconPositions, i++);

        icon_set_position
        (icon,
         is_rtl ? get_mirror_x_position (container, icon, x + position->x_offset) : x + position->x_offset,
         y + position->y_offset);

        icon->saved_ltr_x = is_rtl ? get_mirror_x_position (container, icon, icon->x) : icon->x;

        y += y_iter;
    }
}

static void
lay_down_icons_horizontal (BaulIconContainer *container,
                           GList *icons,
                           double start_y)
{
    GList *p, *line_start;
    BaulIcon *icon;
    double canvas_width, y;
    EelDRect bounds;
    EelDRect icon_bounds;
    EelDRect text_bounds;
    double max_height_above, max_height_below;
    double line_width;
    gboolean gridded_layout;
    double grid_width;
    double max_text_width, max_icon_width;
    int icon_width;
    int i;
    CtkAllocation allocation;
    GArray *positions;
    IconPositions *position = NULL;

    g_assert (BAUL_IS_ICON_CONTAINER (container));

    if (icons == NULL)
    {
        return;
    }

    positions = g_array_new (FALSE, FALSE, sizeof (IconPositions));
    ctk_widget_get_allocation (CTK_WIDGET (container), &allocation);

    /* Lay out icons a line at a time. */
    canvas_width = CANVAS_WIDTH(container, allocation);
    max_icon_width = max_text_width = 0.0;

    if (container->details->label_position == BAUL_ICON_LABEL_POSITION_BESIDE)
    {
        /* Would it be worth caching these bounds for the next loop? */
        for (p = icons; p != NULL; p = p->next)
        {
            icon = p->data;

            icon_bounds = baul_icon_canvas_item_get_icon_rectangle (icon->item);
            max_icon_width = MAX (max_icon_width, ceil (icon_bounds.x1 - icon_bounds.x0));

            text_bounds = baul_icon_canvas_item_get_text_rectangle (icon->item, TRUE);
            max_text_width = MAX (max_text_width, ceil (text_bounds.x1 - text_bounds.x0));
        }

        grid_width = max_icon_width + max_text_width + ICON_PAD_LEFT + ICON_PAD_RIGHT;
    }
    else
    {
        grid_width = STANDARD_ICON_GRID_WIDTH;
    }

    gridded_layout = !baul_icon_container_is_tighter_layout (container);

    line_width = container->details->label_position == BAUL_ICON_LABEL_POSITION_BESIDE ? ICON_PAD_LEFT : 0;
    line_start = icons;
    y = start_y + CONTAINER_PAD_TOP;
    i = 0;

    max_height_above = 0;
    max_height_below = 0;
    for (p = icons; p != NULL; p = p->next)
    {
        double height_above, height_below;

        icon = p->data;

        /* Assume it's only one level hierarchy to avoid costly affine calculations */
        baul_icon_canvas_item_get_bounds_for_layout (icon->item,
                &bounds.x0, &bounds.y0,
                &bounds.x1, &bounds.y1);

        icon_bounds = baul_icon_canvas_item_get_icon_rectangle (icon->item);
        text_bounds = baul_icon_canvas_item_get_text_rectangle (icon->item, TRUE);

        if (gridded_layout)
        {
            icon_width = ceil ((bounds.x1 - bounds.x0)/grid_width) * grid_width;


        }
        else
        {
            icon_width = (bounds.x1 - bounds.x0) + ICON_PAD_RIGHT + 8; /* 8 pixels extra for fancy selection box */
        }

        /* Calculate size above/below baseline */
        height_above = icon_bounds.y1 - bounds.y0;
        height_below = bounds.y1 - icon_bounds.y1;

        /* If this icon doesn't fit, it's time to lay out the line that's queued up. */
        if (line_start != p && line_width + icon_width >= canvas_width )
        {
            if (container->details->label_position == BAUL_ICON_LABEL_POSITION_BESIDE)
            {
                y += ICON_PAD_TOP;
            }
            else
            {
                /* Advance to the baseline. */
                y += ICON_PAD_TOP + max_height_above;
            }

            lay_down_one_line (container, line_start, p, y, max_height_above, positions, FALSE);

            if (container->details->label_position == BAUL_ICON_LABEL_POSITION_BESIDE)
            {
                y += max_height_above + max_height_below + ICON_PAD_BOTTOM;
            }
            else
            {
                /* Advance to next line. */
                y += max_height_below + ICON_PAD_BOTTOM;
            }

            line_width = container->details->label_position == BAUL_ICON_LABEL_POSITION_BESIDE ? ICON_PAD_LEFT : 0;
            line_start = p;
            i = 0;

            max_height_above = height_above;
            max_height_below = height_below;
        }
        else
        {
            if (height_above > max_height_above)
            {
                max_height_above = height_above;
            }
            if (height_below > max_height_below)
            {
                max_height_below = height_below;
            }
        }

        g_array_set_size (positions, i + 1);
        position = &g_array_index (positions, IconPositions, i++);
        position->width = icon_width;
        position->height = icon_bounds.y1 - icon_bounds.y0;

        if (container->details->label_position == BAUL_ICON_LABEL_POSITION_BESIDE)
        {
            if (gridded_layout)
            {
                position->x_offset = max_icon_width + ICON_PAD_LEFT + ICON_PAD_RIGHT - (icon_bounds.x1 - icon_bounds.x0);
            }
            else
            {
                position->x_offset = icon_width - ((icon_bounds.x1 - icon_bounds.x0) + (text_bounds.x1 - text_bounds.x0));
            }
            position->y_offset = 0;
        }
        else
        {
            position->x_offset = (icon_width - (icon_bounds.x1 - icon_bounds.x0)) / 2;
            position->y_offset = icon_bounds.y0 - icon_bounds.y1;
        }

        /* Add this icon. */
        line_width += icon_width;
    }

    /* Lay down that last line of icons. */
    if (line_start != NULL)
    {
        if (container->details->label_position == BAUL_ICON_LABEL_POSITION_BESIDE)
        {
            y += ICON_PAD_TOP;
        }
        else
        {
            /* Advance to the baseline. */
            y += ICON_PAD_TOP + max_height_above;
        }

        lay_down_one_line (container, line_start, NULL, y, max_height_above, positions, TRUE);

        /* Advance to next line. */
        y += max_height_below + ICON_PAD_BOTTOM;
    }

    g_array_free (positions, TRUE);
}

static void
get_max_icon_dimensions (GList *icon_start,
                         GList *icon_end,
                         double *max_icon_width,
                         double *max_icon_height,
                         double *max_text_width,
                         double *max_text_height,
                         double *max_bounds_height)
{
    EelDRect icon_bounds;
    EelDRect text_bounds;
    GList *p;
    double y1, y2;
    BaulIcon *icon = NULL;

    *max_icon_width = *max_text_width = 0.0;
    *max_icon_height = *max_text_height = 0.0;
    *max_bounds_height = 0.0;

    /* Would it be worth caching these bounds for the next loop? */
    for (p = icon_start; p != icon_end; p = p->next)
    {
        icon = p->data;

        icon_bounds = baul_icon_canvas_item_get_icon_rectangle (icon->item);
        *max_icon_width = MAX (*max_icon_width, ceil (icon_bounds.x1 - icon_bounds.x0));
        *max_icon_height = MAX (*max_icon_height, ceil (icon_bounds.y1 - icon_bounds.y0));

        text_bounds = baul_icon_canvas_item_get_text_rectangle (icon->item, TRUE);
        *max_text_width = MAX (*max_text_width, ceil (text_bounds.x1 - text_bounds.x0));
        *max_text_height = MAX (*max_text_height, ceil (text_bounds.y1 - text_bounds.y0));

        baul_icon_canvas_item_get_bounds_for_layout (icon->item,
                NULL, &y1,
                NULL, &y2);
        *max_bounds_height = MAX (*max_bounds_height, y2 - y1);
    }
}

/* column-wise layout. At the moment, this only works with label-beside-icon (used by "Compact View"). */
static void
lay_down_icons_vertical (BaulIconContainer *container,
			 GList             *icons,
			 double             start_y G_GNUC_UNUSED)
{
    GList *p, *line_start;
    double x, canvas_height;
    GArray *positions;
    IconPositions *position;
    EelDRect icon_bounds;
    EelDRect text_bounds;
    CtkAllocation allocation;

    double line_height;

    double max_height;
    double max_height_with_borders;
    double max_width;
    double max_width_in_column;

    double max_bounds_height;
    double max_bounds_height_with_borders;

    double max_text_width, max_icon_width;
    double max_text_height, max_icon_height;
    int i;

    BaulIcon *icon = NULL;

    g_assert (BAUL_IS_ICON_CONTAINER (container));
    g_assert (container->details->label_position == BAUL_ICON_LABEL_POSITION_BESIDE);

    if (icons == NULL)
    {
        return;
    }

    positions = g_array_new (FALSE, FALSE, sizeof (IconPositions));
    ctk_widget_get_allocation (CTK_WIDGET (container), &allocation);

    /* Lay out icons a column at a time. */
    canvas_height = CANVAS_HEIGHT(container, allocation);

    max_icon_width = max_text_width = 0.0;
    max_icon_height = max_text_height = 0.0;
    max_bounds_height = 0.0;

    get_max_icon_dimensions (icons, NULL,
                             &max_icon_width, &max_icon_height,
                             &max_text_width, &max_text_height,
                             &max_bounds_height);

    max_width = max_icon_width + max_text_width;
    max_height = MAX (max_icon_height, max_text_height);
    max_height_with_borders = ICON_PAD_TOP + max_height;

    max_bounds_height_with_borders = ICON_PAD_TOP + max_bounds_height;

    line_height = ICON_PAD_TOP;
    line_start = icons;
    x = 0;
    i = 0;

    max_width_in_column = 0.0;

    for (p = icons; p != NULL; p = p->next)
    {
        int height;

        icon = p->data;

        /* If this icon doesn't fit, it's time to lay out the column that's queued up. */

        /* We use the bounds height here, since for wrapping we also want to consider
         * overlapping emblems at the bottom. We may wrap a little bit too early since
         * the icon with the max. bounds height may actually not be in the last row, but
         * it is better than visual glitches
         */
        if (line_start != p && line_height + (max_bounds_height_with_borders-1) >= canvas_height )
        {
            x += ICON_PAD_LEFT;

            /* correctly set (per-column) width */
            if (!container->details->all_columns_same_width)
            {
                for (i = 0; i < (int) positions->len; i++)
                {
                    position = &g_array_index (positions, IconPositions, i);
                    position->width = max_width_in_column;
                }
            }

            lay_down_one_column (container, line_start, p, x, CONTAINER_PAD_TOP, max_height_with_borders, positions);

            /* Advance to next column. */
            if (container->details->all_columns_same_width)
            {
                x += max_width + ICON_PAD_RIGHT;
            }
            else
            {
                x += max_width_in_column + ICON_PAD_RIGHT;
            }

            line_height = ICON_PAD_TOP;
            line_start = p;
            i = 0;

            max_width_in_column = 0;
        }

        icon_bounds = baul_icon_canvas_item_get_icon_rectangle (icon->item);
        text_bounds = baul_icon_canvas_item_get_text_rectangle (icon->item, TRUE);

        max_width_in_column = MAX (max_width_in_column,
                                   ceil (icon_bounds.x1 - icon_bounds.x0) +
                                   ceil (text_bounds.x1 - text_bounds.x0));

        g_array_set_size (positions, i + 1);
        position = &g_array_index (positions, IconPositions, i++);
        if (container->details->all_columns_same_width)
        {
            position->width = max_width;
        }
        position->height = max_height;
        position->y_offset = ICON_PAD_TOP;
        position->x_offset = ICON_PAD_LEFT;

        position->x_offset += max_icon_width - ceil (icon_bounds.x1 - icon_bounds.x0);

        height = MAX (ceil (icon_bounds.y1 - icon_bounds.y0), ceil(text_bounds.y1 - text_bounds.y0));
        position->y_offset += (max_height - height) / 2;

        /* Add this icon. */
        line_height += max_height_with_borders;
    }

    /* Lay down that last column of icons. */
    if (line_start != NULL)
    {
        x += ICON_PAD_LEFT;
        lay_down_one_column (container, line_start, NULL, x, CONTAINER_PAD_TOP, max_height_with_borders, positions);
    }

    g_array_free (positions, TRUE);
}

static void
snap_position (BaulIconContainer *container,
               BaulIcon *icon,
               int *x, int *y)
{
    int center_x;
    int baseline_y;
    int icon_width;
    int icon_height;
    int total_width;
    int total_height;
    EelDRect icon_position;
    CtkAllocation allocation;

    icon_position = baul_icon_canvas_item_get_icon_rectangle (icon->item);
    icon_width = icon_position.x1 - icon_position.x0;
    icon_height = icon_position.y1 - icon_position.y0;

    ctk_widget_get_allocation (CTK_WIDGET (container), &allocation);
    total_width = CANVAS_WIDTH (container, allocation);
    total_height = CANVAS_HEIGHT (container, allocation);

    if (baul_icon_container_is_layout_rtl (container))
        *x = get_mirror_x_position (container, icon, *x);

    if (*x + icon_width / 2 < DESKTOP_PAD_HORIZONTAL + SNAP_SIZE_X)
    {
        *x = DESKTOP_PAD_HORIZONTAL + SNAP_SIZE_X - icon_width / 2;
    }

    if (*x + icon_width / 2 > total_width - (DESKTOP_PAD_HORIZONTAL + SNAP_SIZE_X))
    {
        *x = total_width - (DESKTOP_PAD_HORIZONTAL + SNAP_SIZE_X + (icon_width / 2));
    }

    if (*y + icon_height < DESKTOP_PAD_VERTICAL + SNAP_SIZE_Y)
    {
        *y = DESKTOP_PAD_VERTICAL + SNAP_SIZE_Y - icon_height;
    }

    if (*y + icon_height > total_height - (DESKTOP_PAD_VERTICAL + SNAP_SIZE_Y))
    {
        *y = total_height - (DESKTOP_PAD_VERTICAL + SNAP_SIZE_Y + (icon_height / 2));
    }

    center_x = *x + icon_width / 2;
    *x = SNAP_NEAREST_HORIZONTAL (center_x) - (icon_width / 2);
    if (baul_icon_container_is_layout_rtl (container))
    {
        *x = get_mirror_x_position (container, icon, *x);
    }


    /* Find the grid position vertically and place on the proper baseline */
    baseline_y = *y + icon_height;
    baseline_y = SNAP_NEAREST_VERTICAL (baseline_y);
    *y = baseline_y - icon_height;
}

static int
compare_icons_by_position (gconstpointer a, gconstpointer b)
{
    BaulIcon *icon_a, *icon_b;
    int x1, y1, x2, y2;
    int center_a;
    int center_b;

    icon_a = (BaulIcon*)a;
    icon_b = (BaulIcon*)b;

    icon_get_bounding_box (icon_a, &x1, &y1, &x2, &y2,
                           BOUNDS_USAGE_FOR_DISPLAY);
    center_a = x1 + (x2 - x1) / 2;
    icon_get_bounding_box (icon_b, &x1, &y1, &x2, &y2,
                           BOUNDS_USAGE_FOR_DISPLAY);
    center_b = x1 + (x2 - x1) / 2;

    return center_a == center_b ?
           icon_a->y - icon_b->y :
           center_a - center_b;
}

static PlacementGrid *
placement_grid_new (BaulIconContainer *container, gboolean tight)
{
    PlacementGrid *grid;
    int width, height;
    int num_columns;
    int num_rows;
    int i;
    CtkAllocation allocation;

    /* Get container dimensions */
    ctk_widget_get_allocation (CTK_WIDGET (container), &allocation);
    width  = CANVAS_WIDTH(container, allocation);
    height = CANVAS_HEIGHT(container, allocation);

    num_columns = width / SNAP_SIZE_X;
    num_rows = height / SNAP_SIZE_Y;

    if (num_columns == 0 || num_rows == 0)
    {
        return NULL;
    }

    grid = g_new0 (PlacementGrid, 1);
    grid->tight = tight;
    grid->num_columns = num_columns;
    grid->num_rows = num_rows;

    grid->grid_memory = g_new0 (int, (num_rows * num_columns));
    grid->icon_grid = g_new0 (int *, num_columns);

    for (i = 0; i < num_columns; i++)
    {
        grid->icon_grid[i] = grid->grid_memory + (i * num_rows);
    }

    return grid;
}

static void
placement_grid_free (PlacementGrid *grid)
{
    g_free (grid->icon_grid);
    g_free (grid->grid_memory);
    g_free (grid);
}

static gboolean
placement_grid_position_is_free (PlacementGrid *grid, EelIRect pos)
{
    int x, y;

    g_assert (pos.x0 >= 0 && pos.x0 < grid->num_columns);
    g_assert (pos.y0 >= 0 && pos.y0 < grid->num_rows);
    g_assert (pos.x1 >= 0 && pos.x1 < grid->num_columns);
    g_assert (pos.y1 >= 0 && pos.y1 < grid->num_rows);

    for (x = pos.x0; x <= pos.x1; x++)
    {
        for (y = pos.y0; y <= pos.y1; y++)
        {
            if (grid->icon_grid[x][y] != 0)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

static void
placement_grid_mark (PlacementGrid *grid, EelIRect pos)
{
    int x, y;

    g_assert (pos.x0 >= 0 && pos.x0 < grid->num_columns);
    g_assert (pos.y0 >= 0 && pos.y0 < grid->num_rows);
    g_assert (pos.x1 >= 0 && pos.x1 < grid->num_columns);
    g_assert (pos.y1 >= 0 && pos.y1 < grid->num_rows);

    for (x = pos.x0; x <= pos.x1; x++)
    {
        for (y = pos.y0; y <= pos.y1; y++)
        {
            grid->icon_grid[x][y] = 1;
        }
    }
}

static void
canvas_position_to_grid_position (PlacementGrid *grid,
                                  EelIRect canvas_position,
                                  EelIRect *grid_position)
{
    /* The first causes minimal moving around during a snap, but
     * can end up with partially overlapping icons.  The second one won't
     * allow any overlapping, but can cause more movement to happen
     * during a snap. */
    if (grid->tight)
    {
        grid_position->x0 = ceil ((double)(canvas_position.x0 - DESKTOP_PAD_HORIZONTAL) / SNAP_SIZE_X);
        grid_position->y0 = ceil ((double)(canvas_position.y0 - DESKTOP_PAD_VERTICAL) / SNAP_SIZE_Y);
        grid_position->x1 = floor ((double)(canvas_position.x1 - DESKTOP_PAD_HORIZONTAL) / SNAP_SIZE_X);
        grid_position->y1 = floor ((double)(canvas_position.y1 - DESKTOP_PAD_VERTICAL) / SNAP_SIZE_Y);
    }
    else
    {
        grid_position->x0 = floor ((double)(canvas_position.x0 - DESKTOP_PAD_HORIZONTAL) / SNAP_SIZE_X);
        grid_position->y0 = floor ((double)(canvas_position.y0 - DESKTOP_PAD_VERTICAL) / SNAP_SIZE_Y);
        grid_position->x1 = floor ((double)(canvas_position.x1 - DESKTOP_PAD_HORIZONTAL) / SNAP_SIZE_X);
        grid_position->y1 = floor ((double)(canvas_position.y1 - DESKTOP_PAD_VERTICAL) / SNAP_SIZE_Y);
    }

    grid_position->x0 = CLAMP (grid_position->x0, 0, grid->num_columns - 1);
    grid_position->y0 = CLAMP (grid_position->y0, 0, grid->num_rows - 1);
    grid_position->x1 = CLAMP (grid_position->x1, grid_position->x0, grid->num_columns - 1);
    grid_position->y1 = CLAMP (grid_position->y1, grid_position->y0, grid->num_rows - 1);
}

static void
placement_grid_mark_icon (PlacementGrid *grid, BaulIcon *icon)
{
    EelIRect icon_pos;
    EelIRect grid_pos;

    icon_get_bounding_box (icon,
                           &icon_pos.x0, &icon_pos.y0,
                           &icon_pos.x1, &icon_pos.y1,
                           BOUNDS_USAGE_FOR_LAYOUT);
    canvas_position_to_grid_position (grid,
                                      icon_pos,
                                      &grid_pos);
    placement_grid_mark (grid, grid_pos);
}

static void
find_empty_location (BaulIconContainer *container,
                     PlacementGrid *grid,
                     BaulIcon *icon,
                     int start_x,
                     int start_y,
                     int *x,
                     int *y)
{
    double icon_width, icon_height;
    int canvas_width;
    int canvas_height;
    int height_for_bound_check;
    EelIRect icon_position;
    EelDRect pixbuf_rect;
    gboolean collision;
    CtkAllocation allocation;

    /* Get container dimensions */
    ctk_widget_get_allocation (CTK_WIDGET (container), &allocation);
    canvas_width  = CANVAS_WIDTH(container, allocation);
    canvas_height = CANVAS_HEIGHT(container, allocation);

    icon_get_bounding_box (icon,
                           &icon_position.x0, &icon_position.y0,
                           &icon_position.x1, &icon_position.y1,
                           BOUNDS_USAGE_FOR_LAYOUT);
    icon_width = icon_position.x1 - icon_position.x0;
    icon_height = icon_position.y1 - icon_position.y0;

    icon_get_bounding_box (icon,
                           NULL, &icon_position.y0,
                           NULL, &icon_position.y1,
                           BOUNDS_USAGE_FOR_ENTIRE_ITEM);
    height_for_bound_check = icon_position.y1 - icon_position.y0;

    pixbuf_rect = baul_icon_canvas_item_get_icon_rectangle (icon->item);

    /* Start the icon on a grid location */
    snap_position (container, icon, &start_x, &start_y);

    icon_position.x0 = start_x;
    icon_position.y0 = start_y;
    icon_position.x1 = icon_position.x0 + icon_width;
    icon_position.y1 = icon_position.y0 + icon_height;

    do
    {
        EelIRect grid_position;
        gboolean need_new_column;

        collision = FALSE;

        canvas_position_to_grid_position (grid,
                                          icon_position,
                                          &grid_position);

        need_new_column = icon_position.y0 + height_for_bound_check + DESKTOP_PAD_VERTICAL > canvas_height;

        if (need_new_column ||
                !placement_grid_position_is_free (grid, grid_position))
        {
            icon_position.y0 += SNAP_SIZE_Y;
            icon_position.y1 = icon_position.y0 + icon_height;

            if (need_new_column)
            {
                /* Move to the next column */
                icon_position.y0 = DESKTOP_PAD_VERTICAL + SNAP_SIZE_Y - (pixbuf_rect.y1 - pixbuf_rect.y0);
                while (icon_position.y0 < DESKTOP_PAD_VERTICAL)
                {
                    icon_position.y0 += SNAP_SIZE_Y;
                }
                icon_position.y1 = icon_position.y0 + icon_height;

                icon_position.x0 += SNAP_SIZE_X;
                icon_position.x1 = icon_position.x0 + icon_width;
            }

            collision = TRUE;
        }
    }
    while (collision && (icon_position.x1 < canvas_width));

    *x = icon_position.x0;
    *y = icon_position.y0;
}

static void
align_icons (BaulIconContainer *container)
{
    GList *unplaced_icons;
    GList *l;
    PlacementGrid *grid;

    unplaced_icons = g_list_copy (container->details->icons);

    unplaced_icons = g_list_sort (unplaced_icons,
                                  compare_icons_by_position);

    if (baul_icon_container_is_layout_rtl (container))
    {
        unplaced_icons = g_list_reverse (unplaced_icons);
    }

    grid = placement_grid_new (container, TRUE);

    if (!grid)
    {
	g_list_free (unplaced_icons);
        return;
    }

    for (l = unplaced_icons; l != NULL; l = l->next)
    {
        BaulIcon *icon;
        int x, y;

        icon = l->data;
        x = icon->saved_ltr_x;
        y = icon->y;
        find_empty_location (container, grid,
                             icon, x, y, &x, &y);

        icon_set_position (icon, x, y);
        icon->saved_ltr_x = icon->x;
        placement_grid_mark_icon (grid, icon);
    }

    g_list_free (unplaced_icons);

    placement_grid_free (grid);

    if (baul_icon_container_is_layout_rtl (container))
    {
        baul_icon_container_set_rtl_positions (container);
    }
}

static double
get_mirror_x_position (BaulIconContainer *container, BaulIcon *icon, double x)
{
    EelDRect icon_bounds;
    CtkAllocation allocation;

    ctk_widget_get_allocation (CTK_WIDGET (container), &allocation);
    icon_bounds = baul_icon_canvas_item_get_icon_rectangle (icon->item);

    return CANVAS_WIDTH(container, allocation) - x - (icon_bounds.x1 - icon_bounds.x0);
}

static void
baul_icon_container_set_rtl_positions (BaulIconContainer *container)
{
    GList *l;
    BaulIcon *icon = NULL;

    if (!container->details->icons)
    {
        return;
    }

    for (l = container->details->icons; l != NULL; l = l->next)
    {
        double x;

        icon = l->data;
        x = get_mirror_x_position (container, icon, icon->saved_ltr_x);
        icon_set_position (icon, x, icon->y);
    }
}

static void
lay_down_icons_vertical_desktop (BaulIconContainer *container, GList *icons)
{
    GList *p, *placed_icons, *unplaced_icons;
    int total, new_length, placed;
    BaulIcon *icon;
    int height;
    int x, y, x1, x2, y1, y2;
    EelDRect icon_rect;
    CtkAllocation allocation;

    /* Get container dimensions */
    ctk_widget_get_allocation (CTK_WIDGET (container), &allocation);
    height = CANVAS_HEIGHT(container, allocation);

    /* Determine which icons have and have not been placed */
    placed_icons = NULL;
    unplaced_icons = NULL;

    total = g_list_length (container->details->icons);
    new_length = g_list_length (icons);
    placed = total - new_length;
    if (placed > 0)
    {
        PlacementGrid *grid;
        /* Add only placed icons in list */
        for (p = container->details->icons; p != NULL; p = p->next)
        {
            icon = p->data;
            if (icon_is_positioned (icon))
            {
                icon_set_position(icon, icon->saved_ltr_x, icon->y);
                placed_icons = g_list_prepend (placed_icons, icon);
            }
            else
            {
                icon->x = 0;
                icon->y = 0;
                unplaced_icons = g_list_prepend (unplaced_icons, icon);
            }
        }
        placed_icons = g_list_reverse (placed_icons);
        unplaced_icons = g_list_reverse (unplaced_icons);

        grid = placement_grid_new (container, FALSE);

        if (grid)
        {
            for (p = placed_icons; p != NULL; p = p->next)
            {
                placement_grid_mark_icon
                (grid, (BaulIcon*)p->data);
            }

            /* Place unplaced icons in the best locations */
            for (p = unplaced_icons; p != NULL; p = p->next)
            {
                icon = p->data;

                icon_rect = baul_icon_canvas_item_get_icon_rectangle (icon->item);

                /* Start the icon in the first column */
                x = DESKTOP_PAD_HORIZONTAL + (SNAP_SIZE_X / 2) - ((icon_rect.x1 - icon_rect.x0) / 2);
                y = DESKTOP_PAD_VERTICAL + SNAP_SIZE_Y - (icon_rect.y1 - icon_rect.y0);

                find_empty_location (container,
                                     grid,
                                     icon,
                                     x, y,
                                     &x, &y);

                icon_set_position (icon, x, y);
                icon->saved_ltr_x = x;
                placement_grid_mark_icon (grid, icon);
            }

            placement_grid_free (grid);
        }

        g_list_free (placed_icons);
        g_list_free (unplaced_icons);
    }
    else
    {
        /* There are no placed icons.  Just lay them down using our rules */
        x = DESKTOP_PAD_HORIZONTAL;

        while (icons != NULL)
        {
            int max_width, column_width, icon_height;
            int center_x;
            int baseline;
            int icon_height_for_bound_check;
            gboolean should_snap;

            should_snap = !(container->details->tighter_layout && !container->details->keep_aligned);

            y = DESKTOP_PAD_VERTICAL;

            max_width = 0;

            /* Calculate max width for column */
            for (p = icons; p != NULL; p = p->next)
            {
                int icon_width;

                icon = p->data;

                icon_get_bounding_box (icon, &x1, &y1, &x2, &y2,
                                       BOUNDS_USAGE_FOR_LAYOUT);
                icon_width = x2 - x1;
                icon_height = y2 - y1;

                icon_get_bounding_box (icon, NULL, &y1, NULL, &y2,
                                       BOUNDS_USAGE_FOR_ENTIRE_ITEM);
                icon_height_for_bound_check = y2 - y1;

                if (should_snap)
                {
                    /* Snap the baseline to a grid position */
                    icon_rect = baul_icon_canvas_item_get_icon_rectangle (icon->item);
                    baseline = y + (icon_rect.y1 - icon_rect.y0);
                    baseline = SNAP_CEIL_VERTICAL (baseline);
                    y = baseline - (icon_rect.y1 - icon_rect.y0);
                }

                /* Check and see if we need to move to a new column */
                if (y != DESKTOP_PAD_VERTICAL && y + icon_height_for_bound_check > height)
                {
                    break;
                }

                if (max_width < icon_width)
                {
                    max_width = icon_width;
                }

                y += icon_height + DESKTOP_PAD_VERTICAL;
            }

            y = DESKTOP_PAD_VERTICAL;

            center_x = x + max_width / 2;
            column_width = max_width;
            if (should_snap)
            {
                /* Find the grid column to center on */
                center_x = SNAP_CEIL_HORIZONTAL (center_x);
                column_width = (center_x - x) + (max_width / 2);
            }

            /* Lay out column */
            for (p = icons; p != NULL; p = p->next)
            {
                icon = p->data;
                icon_get_bounding_box (icon, &x1, &y1, &x2, &y2,
                                       BOUNDS_USAGE_FOR_LAYOUT);
                icon_height = y2 - y1;

                icon_get_bounding_box (icon, NULL, &y1, NULL, &y2,
                                       BOUNDS_USAGE_FOR_ENTIRE_ITEM);
                icon_height_for_bound_check = y2 - y1;

                icon_rect = baul_icon_canvas_item_get_icon_rectangle (icon->item);

                if (should_snap)
                {
                    baseline = y + (icon_rect.y1 - icon_rect.y0);
                    baseline = SNAP_CEIL_VERTICAL (baseline);
                    y = baseline - (icon_rect.y1 - icon_rect.y0);
                }

                /* Check and see if we need to move to a new column */
                if (y != DESKTOP_PAD_VERTICAL && y > height - icon_height_for_bound_check &&
                        /* Make sure we lay out at least one icon per column, to make progress */
                        p != icons)
                {
                    x += column_width + DESKTOP_PAD_HORIZONTAL;
                    break;
                }

                icon_set_position (icon,
                                   center_x - (icon_rect.x1 - icon_rect.x0) / 2,
                                   y);

                icon->saved_ltr_x = icon->x;
                y += icon_height + DESKTOP_PAD_VERTICAL;
            }
            icons = p;
        }
    }

    /* These modes are special. We freeze all of our positions
     * after we do the layout.
     */
    /* FIXME bugzilla.gnome.org 42478:
     * This should not be tied to the direction of layout.
     * It should be a separate switch.
     */
    baul_icon_container_freeze_icon_positions (container);
}


static void
lay_down_icons (BaulIconContainer *container, GList *icons, double start_y)
{
    switch (container->details->layout_mode)
    {
    case BAUL_ICON_LAYOUT_L_R_T_B:
    case BAUL_ICON_LAYOUT_R_L_T_B:
        lay_down_icons_horizontal (container, icons, start_y);
        break;

    case BAUL_ICON_LAYOUT_T_B_L_R:
    case BAUL_ICON_LAYOUT_T_B_R_L:
        if (baul_icon_container_get_is_desktop (container))
        {
            lay_down_icons_vertical_desktop (container, icons);
        }
        else
        {
            lay_down_icons_vertical (container, icons, start_y);
        }
        break;

    default:
        g_assert_not_reached ();
    }
}

static void
redo_layout_internal (BaulIconContainer *container)
{
    finish_adding_new_icons (container);

    /* Don't do any re-laying-out during stretching. Later we
     * might add smart logic that does this and leaves room for
     * the stretched icon, but if we do it we want it to be fast
     * and only re-lay-out when it's really needed.
     */
    if (container->details->auto_layout
            && container->details->drag_state != DRAG_STATE_STRETCH)
    {
        resort (container);
        lay_down_icons (container, container->details->icons, 0);
    }

    if (baul_icon_container_is_layout_rtl (container))
    {
        baul_icon_container_set_rtl_positions (container);
    }

    baul_icon_container_update_scroll_region (container);

    process_pending_icon_to_reveal (container);
    process_pending_icon_to_rename (container);
    baul_icon_container_update_visible_icons (container);
}

static gboolean
redo_layout_callback (gpointer callback_data)
{
    BaulIconContainer *container;

    container = BAUL_ICON_CONTAINER (callback_data);
    redo_layout_internal (container);
    container->details->idle_id = 0;

    return FALSE;
}

static void
unschedule_redo_layout (BaulIconContainer *container)
{
    if (container->details->idle_id != 0)
    {
        g_source_remove (container->details->idle_id);
        container->details->idle_id = 0;
    }
}

static void
schedule_redo_layout (BaulIconContainer *container)
{
    if (container->details->idle_id == 0
            && container->details->has_been_allocated)
    {
        container->details->idle_id = g_idle_add
                                      (redo_layout_callback, container);
    }
}

static void
redo_layout (BaulIconContainer *container)
{
    unschedule_redo_layout (container);
    redo_layout_internal (container);
}

static void
reload_icon_positions (BaulIconContainer *container)
{
    GList *p, *no_position_icons;
    gboolean have_stored_position;
    BaulIconPosition position;
    EelDRect bounds;
    double bottom;
    EelCanvasItem *item;
    BaulIcon *icon = NULL;

    g_assert (!container->details->auto_layout);

    resort (container);

    no_position_icons = NULL;

    /* Place all the icons with positions. */
    bottom = 0;
    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;

        have_stored_position = FALSE;
        g_signal_emit (container,
                       signals[GET_STORED_ICON_POSITION], 0,
                       icon->data,
                       &position,
                       &have_stored_position);
        if (have_stored_position)
        {
            icon_set_position (icon, position.x, position.y);
            item = EEL_CANVAS_ITEM (icon->item);
            baul_icon_canvas_item_get_bounds_for_layout (icon->item,
                    &bounds.x0,
                    &bounds.y0,
                    &bounds.x1,
                    &bounds.y1);
            eel_canvas_item_i2w (item->parent,
                                 &bounds.x0,
                                 &bounds.y0);
            eel_canvas_item_i2w (item->parent,
                                 &bounds.x1,
                                 &bounds.y1);
            if (bounds.y1 > bottom)
            {
                bottom = bounds.y1;
            }
        }
        else
        {
            no_position_icons = g_list_prepend (no_position_icons, icon);
        }
    }
    no_position_icons = g_list_reverse (no_position_icons);

    /* Place all the other icons. */
    lay_down_icons (container, no_position_icons, bottom + ICON_PAD_BOTTOM);
    g_list_free (no_position_icons);
}

/* Container-level icon handling functions.  */

static gboolean
button_event_modifies_selection (CdkEventButton *event)
{
    return (event->state & (CDK_CONTROL_MASK | CDK_SHIFT_MASK)) != 0;
}

/* invalidate the cached label sizes for all the icons */
static void
invalidate_label_sizes (BaulIconContainer *container)
{
    GList *p;
    BaulIcon *icon = NULL;

    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;

        baul_icon_canvas_item_invalidate_label_size (icon->item);
    }
}

/* invalidate the entire labels (i.e. their attributes) for all the icons */
static void
invalidate_labels (BaulIconContainer *container)
{
    GList *p;
    BaulIcon *icon = NULL;

    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;

        baul_icon_canvas_item_invalidate_label (icon->item);
    }
}

static gboolean
select_range (BaulIconContainer *container,
              BaulIcon *icon1,
              BaulIcon *icon2,
              gboolean unselect_outside_range)
{
    gboolean selection_changed;
    GList *p;
    BaulIcon *icon = NULL;
    BaulIcon *unmatched_icon;
    gboolean select;

    selection_changed = FALSE;

    unmatched_icon = NULL;
    select = FALSE;
    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;

        if (unmatched_icon == NULL)
        {
            if (icon == icon1)
            {
                unmatched_icon = icon2;
                select = TRUE;
            }
            else if (icon == icon2)
            {
                unmatched_icon = icon1;
                select = TRUE;
            }
        }

        if (select || unselect_outside_range)
        {
            selection_changed |= icon_set_selected
                                 (container, icon, select);
        }

        if (unmatched_icon != NULL && icon == unmatched_icon)
        {
            select = FALSE;
        }

    }

    if (selection_changed && icon2 != NULL)
    {
        emit_atk_focus_state_change (icon2, TRUE);
    }
    return selection_changed;
}


static gboolean
select_one_unselect_others (BaulIconContainer *container,
                            BaulIcon *icon_to_select)
{
    gboolean selection_changed;
    GList *p;
    BaulIcon *icon = NULL;

    selection_changed = FALSE;

    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;

        selection_changed |= icon_set_selected
                             (container, icon, icon == icon_to_select);
    }

    if (selection_changed && icon_to_select != NULL)
    {
        emit_atk_focus_state_change (icon_to_select, TRUE);
        reveal_icon (container, icon_to_select);
    }
    return selection_changed;
}

static gboolean
unselect_all (BaulIconContainer *container)
{
    return select_one_unselect_others (container, NULL);
}

void
baul_icon_container_move_icon (BaulIconContainer *container,
                               BaulIcon *icon,
                               int x, int y,
                               double scale,
                               gboolean raise,
                               gboolean snap,
                               gboolean update_position)
{
    BaulIconContainerDetails *details;
    gboolean emit_signal;
    BaulIconPosition position;

    details = container->details;

    emit_signal = FALSE;

    if (icon == get_icon_being_renamed (container))
    {
        end_renaming_mode (container, TRUE);
    }

    if (scale != icon->scale)
    {
        icon->scale = scale;
        baul_icon_container_update_icon (container, icon);
        if (update_position)
        {
            redo_layout (container);
            emit_signal = TRUE;
        }
    }

    if (!details->auto_layout)
    {
        if (details->keep_aligned && snap)
        {
            snap_position (container, icon, &x, &y);
        }

        if (x != icon->x || y != icon->y)
        {
            icon_set_position (icon, x, y);
            emit_signal = update_position;
        }

        icon->saved_ltr_x = baul_icon_container_is_layout_rtl (container) ? get_mirror_x_position (container, icon, icon->x) : icon->x;
    }

    if (emit_signal)
    {
        position.x = icon->saved_ltr_x;
        position.y = icon->y;
        position.scale = scale;
        g_signal_emit (container,
                       signals[ICON_POSITION_CHANGED], 0,
                       icon->data, &position);
    }

    if (raise)
    {
        icon_raise (icon);
    }

    /* FIXME bugzilla.gnome.org 42474:
     * Handling of the scroll region is inconsistent here. In
     * the scale-changing case, redo_layout is called, which updates the
     * scroll region appropriately. In other cases, it's up to the
     * caller to make sure the scroll region is updated. This could
     * lead to hard-to-track-down bugs.
     */
}

/* Implementation of rubberband selection.  */
static void
rubberband_select (BaulIconContainer *container,
		   const EelDRect    *previous_rect G_GNUC_UNUSED,
		   const EelDRect    *current_rect)
{
    GList *p;
    gboolean selection_changed, is_in, canvas_rect_calculated;
    EelIRect canvas_rect;
    EelCanvas *canvas;
    BaulIcon *icon = NULL;

    selection_changed = FALSE;
    canvas_rect_calculated = FALSE;

    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;

        if (!canvas_rect_calculated)
        {
            /* Only do this calculation once, since all the canvas items
             * we are interating are in the same coordinate space
             */
            canvas = EEL_CANVAS_ITEM (icon->item)->canvas;
            eel_canvas_w2c (canvas,
                            current_rect->x0,
                            current_rect->y0,
                            &canvas_rect.x0,
                            &canvas_rect.y0);
            eel_canvas_w2c (canvas,
                            current_rect->x1,
                            current_rect->y1,
                            &canvas_rect.x1,
                            &canvas_rect.y1);
            canvas_rect_calculated = TRUE;
        }

        is_in = baul_icon_canvas_item_hit_test_rectangle (icon->item, canvas_rect);

        selection_changed |= icon_set_selected
                             (container, icon,
                              is_in ^ icon->was_selected_before_rubberband);
    }

    if (selection_changed)
    {
        g_signal_emit (container,
                       signals[SELECTION_CHANGED], 0);
    }
}

static int
rubberband_timeout_callback (gpointer data)
{
    BaulIconContainer *container;
    CtkWidget *widget;
    BaulIconRubberbandInfo *band_info;
    int x, y;
    double x1, y1, x2, y2;
    double world_x, world_y;
    int x_scroll, y_scroll;
    int adj_x, adj_y;
    CdkDisplay *display;
    CdkSeat *seat;
    gboolean adj_changed;
    CtkAllocation allocation;

    EelDRect selection_rect;

    widget = CTK_WIDGET (data);
    container = BAUL_ICON_CONTAINER (data);
    band_info = &container->details->rubberband_info;

    g_assert (band_info->timer_id != 0);
    g_assert (EEL_IS_CANVAS_RECT (band_info->selection_rectangle) ||
              EEL_IS_CANVAS_RECT (band_info->selection_rectangle));

    adj_changed = FALSE;
    ctk_widget_get_allocation (widget, &allocation);

    adj_x = ctk_adjustment_get_value (ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (container)));
    if (adj_x != band_info->last_adj_x)
    {
        band_info->last_adj_x = adj_x;
        adj_changed = TRUE;
    }

    adj_y = ctk_adjustment_get_value (ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (container)));
    if (adj_y != band_info->last_adj_y)
    {
        band_info->last_adj_y = adj_y;
        adj_changed = TRUE;
    }
    display = ctk_widget_get_display (widget);
    seat = cdk_display_get_default_seat (display);

    cdk_window_get_device_position (ctk_widget_get_window (widget),
                                    cdk_seat_get_pointer (seat),
                                    &x, &y, NULL);

    if (x < 0)
    {
        x_scroll = x;
        x = 0;
    }
    else if (x >= allocation.width)
    {
        x_scroll = x - allocation.width + 1;
        x = allocation.width - 1;
    }
    else
    {
        x_scroll = 0;
    }

    if (y < 0)
    {
        y_scroll = y;
        y = 0;
    }
    else if (y >= allocation.height)
    {
        y_scroll = y - allocation.height + 1;
        y = allocation.height - 1;
    }
    else
    {
        y_scroll = 0;
    }

    if (y_scroll == 0 && x_scroll == 0
            && (int) band_info->prev_x == x && (int) band_info->prev_y == y && !adj_changed)
    {
        return TRUE;
    }

    baul_icon_container_scroll (container, x_scroll, y_scroll);

    /* Remember to convert from widget to scrolled window coords */
    eel_canvas_window_to_world (EEL_CANVAS (container),
    			    x + ctk_adjustment_get_value (ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (container))),
    			    y + ctk_adjustment_get_value (ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (container))),
                                &world_x, &world_y);

    if (world_x < band_info->start_x)
    {
        x1 = world_x;
        x2 = band_info->start_x;
    }
    else
    {
        x1 = band_info->start_x;
        x2 = world_x;
    }

    if (world_y < band_info->start_y)
    {
        y1 = world_y;
        y2 = band_info->start_y;
    }
    else
    {
        y1 = band_info->start_y;
        y2 = world_y;
    }

    /* Don't let the area of the selection rectangle be empty.
     * Aside from the fact that it would be funny when the rectangle disappears,
     * this also works around a crash in libart that happens sometimes when a
     * zero height rectangle is passed.
     */
    x2 = MAX (x1 + 1, x2);
    y2 = MAX (y1 + 1, y2);

    eel_canvas_item_set
    (band_info->selection_rectangle,
     "x1", x1, "y1", y1,
     "x2", x2, "y2", y2,
     NULL);

    selection_rect.x0 = x1;
    selection_rect.y0 = y1;
    selection_rect.x1 = x2;
    selection_rect.y1 = y2;

    rubberband_select (container,
                       &band_info->prev_rect,
                       &selection_rect);

    band_info->prev_x = x;
    band_info->prev_y = y;

    band_info->prev_rect = selection_rect;

    return TRUE;
}

/*borrowed from Nemo, makes Baul rubberbanding follow same selectors as Nemo and presumably Nautilus */
static void
start_rubberbanding (BaulIconContainer *container,
		     CdkEventButton *event)
{
	AtkObject *accessible;
	BaulIconContainerDetails *details;
	BaulIconRubberbandInfo *band_info;
	CdkRGBA bg_color, border_color;
	CdkRGBA *c;
	GList *p;
	BaulIcon *icon;
	CtkStyleContext *context;

	details = container->details;
	band_info = &details->rubberband_info;

	g_signal_emit (container,
		       signals[BAND_SELECT_STARTED], 0);

	for (p = details->icons; p != NULL; p = p->next) {
		icon = p->data;
		icon->was_selected_before_rubberband = icon->is_selected;
	}

	eel_canvas_window_to_world
		(EEL_CANVAS (container), event->x, event->y,
		 &band_info->start_x, &band_info->start_y);

	context = ctk_widget_get_style_context (CTK_WIDGET (container));
	ctk_style_context_save (context);
	ctk_style_context_add_class (context, CTK_STYLE_CLASS_RUBBERBAND);

	ctk_style_context_get (context, CTK_STATE_FLAG_NORMAL,
			       CTK_STYLE_PROPERTY_BACKGROUND_COLOR,
			       &c, NULL);

	bg_color = *c;

	ctk_style_context_get (context, CTK_STATE_FLAG_NORMAL,
			       CTK_STYLE_PROPERTY_BORDER_COLOR,
			       &c, NULL);

	border_color = *c;
	cdk_rgba_free (c);

	ctk_style_context_restore (context);

	band_info->selection_rectangle = eel_canvas_item_new
		(eel_canvas_root
		 (EEL_CANVAS (container)),
		  EEL_TYPE_CANVAS_RECT,
		 "x1", band_info->start_x,
		 "y1", band_info->start_y,
		 "x2", band_info->start_x,
		 "y2", band_info->start_y,
		 "fill_color_rgba", &bg_color,
		 "outline_color_rgba", &border_color,
		 "width_pixels", 1,
		 NULL);

	accessible = atk_gobject_accessible_for_object
		(G_OBJECT (band_info->selection_rectangle));
	atk_object_set_name (accessible, "selection");
	atk_object_set_description (accessible, _("The selection rectangle"));

	band_info->prev_x = event->x - ctk_adjustment_get_value (ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (container)));
	band_info->prev_y = event->y - ctk_adjustment_get_value (ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (container)));

	band_info->active = TRUE;

	if (band_info->timer_id == 0) {
		band_info->timer_id = g_timeout_add
			(RUBBERBAND_TIMEOUT_INTERVAL,
			 rubberband_timeout_callback,
			 container);
	}

	eel_canvas_item_grab (band_info->selection_rectangle,
				(CDK_POINTER_MOTION_MASK
				 | CDK_BUTTON_RELEASE_MASK
				 | CDK_SCROLL_MASK),
				NULL,
				(CdkEvent *)event);
}

static void
stop_rubberbanding (BaulIconContainer *container)
{
    BaulIconRubberbandInfo *band_info;
    GList *icons;

    band_info = &container->details->rubberband_info;

    g_assert (band_info->timer_id != 0);
    g_source_remove (band_info->timer_id);
    band_info->timer_id = 0;

    band_info->active = FALSE;

    /* Destroy this canvas item; the parent will unref it. */
    eel_canvas_item_ungrab (band_info->selection_rectangle);
    eel_canvas_item_destroy (band_info->selection_rectangle);
    band_info->selection_rectangle = NULL;

    /* if only one item has been selected, use it as range
     * selection base (cf. handle_icon_button_press) */
    icons = baul_icon_container_get_selected_icons (container);
    if (g_list_length (icons) == 1)
    {
        container->details->range_selection_base_icon = icons->data;
    }
    g_list_free (icons);

    g_signal_emit (container,
                   signals[BAND_SELECT_ENDED], 0);
}

/* Keyboard navigation.  */

typedef gboolean (* IsBetterIconFunction) (BaulIconContainer *container,
        BaulIcon *start_icon,
        BaulIcon *best_so_far,
        BaulIcon *candidate,
        void *data);

static BaulIcon *
find_best_icon (BaulIconContainer *container,
                BaulIcon *start_icon,
                IsBetterIconFunction function,
                void *data)
{
    GList *p;
    BaulIcon *best, *candidate;

    best = NULL;
    for (p = container->details->icons; p != NULL; p = p->next)
    {
        candidate = p->data;

        if (candidate != start_icon)
        {
            if ((* function) (container, start_icon, best, candidate, data))
            {
                best = candidate;
            }
        }
    }
    return best;
}

static BaulIcon *
find_best_selected_icon (BaulIconContainer *container,
                         BaulIcon *start_icon,
                         IsBetterIconFunction function,
                         void *data)
{
    GList *p;
    BaulIcon *best, *candidate;

    best = NULL;
    for (p = container->details->icons; p != NULL; p = p->next)
    {
        candidate = p->data;

        if (candidate != start_icon && candidate->is_selected)
        {
            if ((* function) (container, start_icon, best, candidate, data))
            {
                best = candidate;
            }
        }
    }
    return best;
}

static int
compare_icons_by_uri (BaulIconContainer *container,
                      BaulIcon *icon_a,
                      BaulIcon *icon_b)
{
    char *uri_a, *uri_b;
    int result;

    g_assert (BAUL_IS_ICON_CONTAINER (container));
    g_assert (icon_a != NULL);
    g_assert (icon_b != NULL);
    g_assert (icon_a != icon_b);

    uri_a = baul_icon_container_get_icon_uri (container, icon_a);
    uri_b = baul_icon_container_get_icon_uri (container, icon_b);
    result = strcmp (uri_a, uri_b);
    g_assert (result != 0);
    g_free (uri_a);
    g_free (uri_b);

    return result;
}

static int
get_cmp_point_x (BaulIconContainer *container,
                 EelDRect icon_rect)
{
    if (container->details->label_position == BAUL_ICON_LABEL_POSITION_BESIDE)
    {
        if (ctk_widget_get_direction (CTK_WIDGET (container)) == CTK_TEXT_DIR_RTL)
        {
            return icon_rect.x0;
        }
        else
        {
            return icon_rect.x1;
        }
    }
    else
    {
        return (icon_rect.x0 + icon_rect.x1) / 2;
    }
}

static int
get_cmp_point_y (BaulIconContainer *container,
                 EelDRect icon_rect)
{
    if (container->details->label_position == BAUL_ICON_LABEL_POSITION_BESIDE)
    {
        return (icon_rect.y0 + icon_rect.y1)/2;
    }
    else
    {
        return icon_rect.y1;
    }
}


static int
compare_icons_horizontal (BaulIconContainer *container,
                          BaulIcon *icon_a,
                          BaulIcon *icon_b)
{
    EelDRect world_rect;
    int ax, bx;

    world_rect = baul_icon_canvas_item_get_icon_rectangle (icon_a->item);
    eel_canvas_w2c
    (EEL_CANVAS (container),
     get_cmp_point_x (container, world_rect),
     get_cmp_point_y (container, world_rect),
     &ax,
     NULL);
    world_rect = baul_icon_canvas_item_get_icon_rectangle (icon_b->item);
    eel_canvas_w2c
    (EEL_CANVAS (container),
     get_cmp_point_x (container, world_rect),
     get_cmp_point_y (container, world_rect),
     &bx,
     NULL);

    if (ax < bx)
    {
        return -1;
    }
    if (ax > bx)
    {
        return +1;
    }
    return 0;
}

static int
compare_icons_vertical (BaulIconContainer *container,
                        BaulIcon *icon_a,
                        BaulIcon *icon_b)
{
    EelDRect world_rect;
    int ay, by;

    world_rect = baul_icon_canvas_item_get_icon_rectangle (icon_a->item);
    eel_canvas_w2c
    (EEL_CANVAS (container),
     get_cmp_point_x (container, world_rect),
     get_cmp_point_y (container, world_rect),
     NULL,
     &ay);
    world_rect = baul_icon_canvas_item_get_icon_rectangle (icon_b->item);
    eel_canvas_w2c
    (EEL_CANVAS (container),
     get_cmp_point_x (container, world_rect),
     get_cmp_point_y (container, world_rect),
     NULL,
     &by);

    if (ay < by)
    {
        return -1;
    }
    if (ay > by)
    {
        return +1;
    }
    return 0;
}

static int
compare_icons_horizontal_first (BaulIconContainer *container,
                                BaulIcon *icon_a,
                                BaulIcon *icon_b)
{
    EelDRect world_rect;
    int ax, ay, bx, by;

    world_rect = baul_icon_canvas_item_get_icon_rectangle (icon_a->item);
    eel_canvas_w2c
    (EEL_CANVAS (container),
     get_cmp_point_x (container, world_rect),
     get_cmp_point_y (container, world_rect),
     &ax,
     &ay);
    world_rect = baul_icon_canvas_item_get_icon_rectangle (icon_b->item);
    eel_canvas_w2c
    (EEL_CANVAS (container),
     get_cmp_point_x (container, world_rect),
     get_cmp_point_y (container, world_rect),
     &bx,
     &by);

    if (ax < bx)
    {
        return -1;
    }
    if (ax > bx)
    {
        return +1;
    }
    if (ay < by)
    {
        return -1;
    }
    if (ay > by)
    {
        return +1;
    }
    return compare_icons_by_uri (container, icon_a, icon_b);
}

static int
compare_icons_vertical_first (BaulIconContainer *container,
                              BaulIcon *icon_a,
                              BaulIcon *icon_b)
{
    EelDRect world_rect;
    int ax, ay, bx, by;

    world_rect = baul_icon_canvas_item_get_icon_rectangle (icon_a->item);
    eel_canvas_w2c
    (EEL_CANVAS (container),
     get_cmp_point_x (container, world_rect),
     get_cmp_point_y (container, world_rect),
     &ax,
     &ay);
    world_rect = baul_icon_canvas_item_get_icon_rectangle (icon_b->item);
    eel_canvas_w2c
    (EEL_CANVAS (container),
     get_cmp_point_x (container, world_rect),
     get_cmp_point_y (container, world_rect),
     &bx,
     &by);

    if (ay < by)
    {
        return -1;
    }
    if (ay > by)
    {
        return +1;
    }
    if (ax < bx)
    {
        return -1;
    }
    if (ax > bx)
    {
        return +1;
    }
    return compare_icons_by_uri (container, icon_a, icon_b);
}

static gboolean
leftmost_in_top_row (BaulIconContainer *container,
		     BaulIcon          *start_icon G_GNUC_UNUSED,
		     BaulIcon          *best_so_far,
		     BaulIcon          *candidate,
		     void              *data G_GNUC_UNUSED)
{
    if (best_so_far == NULL)
    {
        return TRUE;
    }
    return compare_icons_vertical_first (container, best_so_far, candidate) > 0;
}

static gboolean
rightmost_in_top_row (BaulIconContainer *container,
		      BaulIcon          *start_icon G_GNUC_UNUSED,
		      BaulIcon          *best_so_far,
		      BaulIcon          *candidate,
		      void              *data G_GNUC_UNUSED)
{
    if (best_so_far == NULL)
    {
        return TRUE;
    }
    return compare_icons_vertical (container, best_so_far, candidate) > 0;
    return compare_icons_horizontal (container, best_so_far, candidate) < 0;
}

static gboolean
rightmost_in_bottom_row (BaulIconContainer *container,
			 BaulIcon          *start_icon G_GNUC_UNUSED,
			 BaulIcon          *best_so_far,
			 BaulIcon          *candidate,
			 void              *data G_GNUC_UNUSED)
{
    if (best_so_far == NULL)
    {
        return TRUE;
    }
    return compare_icons_vertical_first (container, best_so_far, candidate) < 0;
}

static int
compare_with_start_row (BaulIconContainer *container,
                        BaulIcon *icon)
{
    EelCanvasItem *item;

    item = EEL_CANVAS_ITEM (icon->item);

    if (container->details->arrow_key_start_y < item->y1)
    {
        return -1;
    }
    if (container->details->arrow_key_start_y > item->y2)
    {
        return +1;
    }
    return 0;
}

static int
compare_with_start_column (BaulIconContainer *container,
                           BaulIcon *icon)
{
    EelCanvasItem *item;

    item = EEL_CANVAS_ITEM (icon->item);

    if (container->details->arrow_key_start_x < item->x1)
    {
        return -1;
    }
    if (container->details->arrow_key_start_x > item->x2)
    {
        return +1;
    }
    return 0;
}

static gboolean
same_row_right_side_leftmost (BaulIconContainer *container,
			      BaulIcon          *start_icon,
			      BaulIcon          *best_so_far,
			      BaulIcon          *candidate,
			      void              *data G_GNUC_UNUSED)
{
    /* Candidates not on the start row do not qualify. */
    if (compare_with_start_row (container, candidate) != 0)
    {
        return FALSE;
    }

    /* Candidates that are farther right lose out. */
    if (best_so_far != NULL)
    {
        if (compare_icons_horizontal_first (container,
                                            best_so_far,
                                            candidate) < 0)
        {
            return FALSE;
        }
    }

    /* Candidate to the left of the start do not qualify. */
    if (compare_icons_horizontal_first (container,
                                        candidate,
                                        start_icon) <= 0)
    {
        return FALSE;
    }

    return TRUE;
}

static gboolean
same_row_left_side_rightmost (BaulIconContainer *container,
			      BaulIcon          *start_icon,
			      BaulIcon          *best_so_far,
			      BaulIcon          *candidate,
			      void              *data G_GNUC_UNUSED)
{
    /* Candidates not on the start row do not qualify. */
    if (compare_with_start_row (container, candidate) != 0)
    {
        return FALSE;
    }

    /* Candidates that are farther left lose out. */
    if (best_so_far != NULL)
    {
        if (compare_icons_horizontal_first (container,
                                            best_so_far,
                                            candidate) > 0)
        {
            return FALSE;
        }
    }

    /* Candidate to the right of the start do not qualify. */
    if (compare_icons_horizontal_first (container,
                                        candidate,
                                        start_icon) >= 0)
    {
        return FALSE;
    }

    return TRUE;
}

static gboolean
next_row_leftmost (BaulIconContainer *container,
		   BaulIcon          *start_icon G_GNUC_UNUSED,
		   BaulIcon          *best_so_far,
		   BaulIcon          *candidate,
		   void              *data G_GNUC_UNUSED)
{
    /* sort out icons that are not below the current row */
    if (compare_with_start_row (container, candidate) >= 0)
    {
        return FALSE;
    }

    if (best_so_far != NULL)
    {
        if (compare_icons_vertical_first (container,
                                          best_so_far,
                                          candidate) > 0)
        {
            /* candidate is above best choice, but below the current row */
            return TRUE;
        }

        if (compare_icons_horizontal_first (container,
                                            best_so_far,
                                            candidate) > 0)
        {
            return TRUE;
        }
    }

    return best_so_far == NULL;
}

static gboolean
next_row_rightmost (BaulIconContainer *container,
		    BaulIcon          *start_icon G_GNUC_UNUSED,
		    BaulIcon          *best_so_far,
		    BaulIcon          *candidate,
		    void              *data G_GNUC_UNUSED)
{
    /* sort out icons that are not below the current row */
    if (compare_with_start_row (container, candidate) >= 0)
    {
        return FALSE;
    }

    if (best_so_far != NULL)
    {
        if (compare_icons_vertical_first (container,
                                          best_so_far,
                                          candidate) > 0)
        {
            /* candidate is above best choice, but below the current row */
            return TRUE;
        }

        if (compare_icons_horizontal_first (container,
                                            best_so_far,
                                            candidate) < 0)
        {
            return TRUE;
        }
    }

    return best_so_far == NULL;
}

static gboolean
next_column_bottommost (BaulIconContainer *container,
			BaulIcon          *start_icon G_GNUC_UNUSED,
			BaulIcon          *best_so_far,
			BaulIcon          *candidate,
			void              *data G_GNUC_UNUSED)
{
    /* sort out icons that are not on the right of the current column */
    if (compare_with_start_column (container, candidate) >= 0)
    {
        return FALSE;
    }

    if (best_so_far != NULL)
    {
        if (compare_icons_horizontal_first (container,
                                            best_so_far,
                                            candidate) > 0)
        {
            /* candidate is above best choice, but below the current row */
            return TRUE;
        }

        if (compare_icons_vertical_first (container,
                                          best_so_far,
                                          candidate) < 0)
        {
            return TRUE;
        }
    }

    return best_so_far == NULL;
}

static gboolean
previous_row_rightmost (BaulIconContainer *container,
			BaulIcon          *start_icon G_GNUC_UNUSED,
			BaulIcon          *best_so_far,
			BaulIcon          *candidate,
			void              *data G_GNUC_UNUSED)
{
    /* sort out icons that are not above the current row */
    if (compare_with_start_row (container, candidate) <= 0)
    {
        return FALSE;
    }

    if (best_so_far != NULL)
    {
        if (compare_icons_vertical_first (container,
                                          best_so_far,
                                          candidate) < 0)
        {
            /* candidate is below the best choice, but above the current row */
            return TRUE;
        }

        if (compare_icons_horizontal_first (container,
                                            best_so_far,
                                            candidate) < 0)
        {
            return TRUE;
        }
    }

    return best_so_far == NULL;
}

static gboolean
same_column_above_lowest (BaulIconContainer *container,
			  BaulIcon          *start_icon,
			  BaulIcon          *best_so_far,
			  BaulIcon          *candidate,
			  void              *data G_GNUC_UNUSED)
{
    /* Candidates not on the start column do not qualify. */
    if (compare_with_start_column (container, candidate) != 0)
    {
        return FALSE;
    }

    /* Candidates that are higher lose out. */
    if (best_so_far != NULL)
    {
        if (compare_icons_vertical_first (container,
                                          best_so_far,
                                          candidate) > 0)
        {
            return FALSE;
        }
    }

    /* Candidates below the start do not qualify. */
    if (compare_icons_vertical_first (container,
                                      candidate,
                                      start_icon) >= 0)
    {
        return FALSE;
    }

    return TRUE;
}

static gboolean
same_column_below_highest (BaulIconContainer *container,
			   BaulIcon *start_icon,
			   BaulIcon *best_so_far,
			   BaulIcon *candidate,
			   void *data G_GNUC_UNUSED)
{
    /* Candidates not on the start column do not qualify. */
    if (compare_with_start_column (container, candidate) != 0)
    {
        return FALSE;
    }

    /* Candidates that are lower lose out. */
    if (best_so_far != NULL)
    {
        if (compare_icons_vertical_first (container,
                                          best_so_far,
                                          candidate) < 0)
        {
            return FALSE;
        }
    }

    /* Candidates above the start do not qualify. */
    if (compare_icons_vertical_first (container,
                                      candidate,
                                      start_icon) <= 0)
    {
        return FALSE;
    }

    return TRUE;
}

static gboolean
previous_column_highest (BaulIconContainer *container,
			 BaulIcon          *start_icon G_GNUC_UNUSED,
			 BaulIcon          *best_so_far,
			 BaulIcon          *candidate,
			 void              *data G_GNUC_UNUSED)
{
    /* sort out icons that are not before the current column */
    if (compare_with_start_column (container, candidate) <= 0)
    {
        return FALSE;
    }

    if (best_so_far != NULL)
    {
        if (compare_icons_horizontal (container,
                                      best_so_far,
                                      candidate) < 0)
        {
            /* candidate is right of the best choice, but left of the current column */
            return TRUE;
        }

        if (compare_icons_vertical (container,
                                    best_so_far,
                                    candidate) > 0)
        {
            return TRUE;
        }
    }

    return best_so_far == NULL;
}


static gboolean
next_column_highest (BaulIconContainer *container,
		     BaulIcon          *start_icon G_GNUC_UNUSED,
		     BaulIcon          *best_so_far,
		     BaulIcon          *candidate,
		     void              *data G_GNUC_UNUSED)
{
    /* sort out icons that are not after the current column */
    if (compare_with_start_column (container, candidate) >= 0)
    {
        return FALSE;
    }

    if (best_so_far != NULL)
    {
        if (compare_icons_horizontal_first (container,
                                            best_so_far,
                                            candidate) > 0)
        {
            /* candidate is left of the best choice, but right of the current column */
            return TRUE;
        }

        if (compare_icons_vertical_first (container,
                                          best_so_far,
                                          candidate) > 0)
        {
            return TRUE;
        }
    }

    return best_so_far == NULL;
}

static gboolean
previous_column_lowest (BaulIconContainer *container,
			BaulIcon          *start_icon G_GNUC_UNUSED,
			BaulIcon          *best_so_far,
			BaulIcon          *candidate,
			void              *data G_GNUC_UNUSED)
{
    /* sort out icons that are not before the current column */
    if (compare_with_start_column (container, candidate) <= 0)
    {
        return FALSE;
    }

    if (best_so_far != NULL)
    {
        if (compare_icons_horizontal_first (container,
                                            best_so_far,
                                            candidate) < 0)
        {
            /* candidate is right of the best choice, but left of the current column */
            return TRUE;
        }

        if (compare_icons_vertical_first (container,
                                          best_so_far,
                                          candidate) < 0)
        {
            return TRUE;
        }
    }

    return best_so_far == NULL;
}

static gboolean
last_column_lowest (BaulIconContainer *container,
		    BaulIcon          *start_icon G_GNUC_UNUSED,
		    BaulIcon          *best_so_far,
		    BaulIcon          *candidate,
		    void              *data G_GNUC_UNUSED)
{
    if (best_so_far == NULL)
    {
        return TRUE;
    }
    return compare_icons_horizontal_first (container, best_so_far, candidate) < 0;
}

static gboolean
closest_in_90_degrees (BaulIconContainer *container,
		       BaulIcon          *start_icon G_GNUC_UNUSED,
		       BaulIcon          *best_so_far,
		       BaulIcon          *candidate,
		       void              *data)
{
    EelDRect world_rect;
    int x, y;
    int dx, dy;
    int dist;
    int *best_dist;


    world_rect = baul_icon_canvas_item_get_icon_rectangle (candidate->item);
    eel_canvas_w2c
    (EEL_CANVAS (container),
     get_cmp_point_x (container, world_rect),
     get_cmp_point_y (container, world_rect),
     &x,
     &y);

    dx = x - container->details->arrow_key_start_x;
    dy = y - container->details->arrow_key_start_y;

    switch (container->details->arrow_key_direction)
    {
    case CTK_DIR_UP:
        if (dy > 0 ||
                ABS(dx) > ABS(dy))
        {
            return FALSE;
        }
        break;
    case CTK_DIR_DOWN:
        if (dy < 0 ||
                ABS(dx) > ABS(dy))
        {
            return FALSE;
        }
        break;
    case CTK_DIR_LEFT:
        if (dx > 0 ||
                ABS(dy) > ABS(dx))
        {
            return FALSE;
        }
        break;
    case CTK_DIR_RIGHT:
        if (dx < 0 ||
                ABS(dy) > ABS(dx))
        {
            return FALSE;
        }
        break;
    default:
        g_assert_not_reached();
    }

    dist = dx*dx + dy*dy;
    best_dist = data;

    if (best_so_far == NULL)
    {
        *best_dist = dist;
        return TRUE;
    }

    if (dist < *best_dist)
    {
        *best_dist = dist;
        return TRUE;
    }

    return FALSE;
}

static EelDRect
get_rubberband (BaulIcon *icon1,
                BaulIcon *icon2)
{
    EelDRect rect1;
    EelDRect rect2;
    EelDRect ret;

    eel_canvas_item_get_bounds (EEL_CANVAS_ITEM (icon1->item),
                                &rect1.x0, &rect1.y0,
                                &rect1.x1, &rect1.y1);
    eel_canvas_item_get_bounds (EEL_CANVAS_ITEM (icon2->item),
                                &rect2.x0, &rect2.y0,
                                &rect2.x1, &rect2.y1);

    eel_drect_union (&ret, &rect1, &rect2);

    return ret;
}

static void
keyboard_move_to (BaulIconContainer *container,
                  BaulIcon *icon,
                  BaulIcon *from,
                  CdkEventKey *event)
{
    if (icon == NULL)
    {
        return;
    }

    if (event != NULL &&
            (event->state & CDK_CONTROL_MASK) != 0 &&
            (event->state & CDK_SHIFT_MASK) == 0)
    {
        /* Move the keyboard focus. Use Control modifier
         * rather than Alt to avoid Sawfish conflict.
         */
        set_keyboard_focus (container, icon);
        container->details->keyboard_rubberband_start = NULL;
    }
    else if (event != NULL &&
             ((event->state & CDK_CONTROL_MASK) != 0 ||
              !container->details->auto_layout) &&
             (event->state & CDK_SHIFT_MASK) != 0)
    {
        /* Do rubberband selection */
        EelDRect rect;

        if (from && !container->details->keyboard_rubberband_start)
        {
            set_keyboard_rubberband_start (container, from);
        }

        set_keyboard_focus (container, icon);

        if (icon && container->details->keyboard_rubberband_start)
        {
            rect = get_rubberband (container->details->keyboard_rubberband_start,
                                   icon);
            rubberband_select (container, NULL, &rect);
        }
    }
    else if (event != NULL &&
             (event->state & CDK_CONTROL_MASK) == 0 &&
             (event->state & CDK_SHIFT_MASK) != 0)
    {
        /* Select range */
        BaulIcon *start_icon;

        start_icon = container->details->range_selection_base_icon;
        if (start_icon == NULL || !start_icon->is_selected)
        {
            start_icon = icon;
            container->details->range_selection_base_icon = icon;
        }

        set_keyboard_focus (container, icon);

        if (select_range (container, start_icon, icon, TRUE))
        {
            g_signal_emit (container,
                           signals[SELECTION_CHANGED], 0);
        }
    }
    else
    {
        /* Select icons and get rid of the special keyboard focus. */
        clear_keyboard_focus (container);
        clear_keyboard_rubberband_start (container);

        container->details->range_selection_base_icon = icon;
        if (select_one_unselect_others (container, icon))
        {
            g_signal_emit (container,
                           signals[SELECTION_CHANGED], 0);
        }
    }
    schedule_keyboard_icon_reveal (container, icon);
}

static void
keyboard_home (BaulIconContainer *container,
               CdkEventKey *event)
{
    BaulIcon *from;
    BaulIcon *to;

    /* Home selects the first icon.
     * Control-Home sets the keyboard focus to the first icon.
     */

    from = find_best_selected_icon (container, NULL,
                                    rightmost_in_bottom_row,
                                    NULL);
    to = find_best_icon (container, NULL, leftmost_in_top_row, NULL);

    keyboard_move_to (container, to, from, event);
}

static void
keyboard_end (BaulIconContainer *container,
              CdkEventKey *event)
{
    BaulIcon *to;
    BaulIcon *from;

    /* End selects the last icon.
     * Control-End sets the keyboard focus to the last icon.
     */
    from = find_best_selected_icon (container, NULL,
                                    leftmost_in_top_row,
                                    NULL);
    to = find_best_icon (container, NULL,
                         baul_icon_container_is_layout_vertical (container) ?
                         last_column_lowest :
                         rightmost_in_bottom_row,
                         NULL);

    keyboard_move_to (container, to, from, event);
}

static void
record_arrow_key_start (BaulIconContainer *container,
                        BaulIcon *icon,
                        CtkDirectionType direction)
{
    EelDRect world_rect;

    world_rect = baul_icon_canvas_item_get_icon_rectangle (icon->item);
    eel_canvas_w2c
    (EEL_CANVAS (container),
     get_cmp_point_x (container, world_rect),
     get_cmp_point_y (container, world_rect),
     &container->details->arrow_key_start_x,
     &container->details->arrow_key_start_y);
    container->details->arrow_key_direction = direction;
}

static void
keyboard_arrow_key (BaulIconContainer *container,
                    CdkEventKey *event,
                    CtkDirectionType direction,
                    IsBetterIconFunction better_start,
                    IsBetterIconFunction empty_start,
                    IsBetterIconFunction better_destination,
                    IsBetterIconFunction better_destination_fallback,
                    IsBetterIconFunction better_destination_fallback_fallback,
                    IsBetterIconFunction better_destination_manual)
{
    BaulIcon *from;
    BaulIcon *to;
    int data;

    /* Chose the icon to start with.
     * If we have a keyboard focus, start with it.
     * Otherwise, use the single selected icon.
     * If there's multiple selection, use the icon farthest toward the end.
     */

    from = container->details->keyboard_focus;

    if (from == NULL)
    {
        if (has_multiple_selection (container))
        {
            if (all_selected (container))
            {
                from = find_best_selected_icon
                       (container, NULL,
                        empty_start, NULL);
            }
            else
            {
                from = find_best_selected_icon
                       (container, NULL,
                        better_start, NULL);
            }
        }
        else
        {
            from = get_first_selected_icon (container);
        }
    }

    /* If there's no icon, select the icon farthest toward the end.
     * If there is an icon, select the next icon based on the arrow direction.
     */
    if (from == NULL)
    {
        to = from = find_best_icon
                    (container, NULL,
                     empty_start, NULL);
    }
    else
    {
        record_arrow_key_start (container, from, direction);

        to = find_best_icon
             (container, from,
              container->details->auto_layout ? better_destination : better_destination_manual,
              &data);

        /* Wrap around to next/previous row/column */
        if (to == NULL &&
            better_destination_fallback != NULL) {
            to = find_best_icon
                 (container, from,
                  better_destination_fallback,
                  &data);
        }

        /* With a layout like
         * 1 2 3
         * 4
         * (horizontal layout)
         *
         * or
         *
         * 1 4
         * 2
         * 3
         * (vertical layout)
         *
         * * pressing down for any of 1,2,3 (horizontal layout)
         * * pressing right for any of 1,2,3 (vertical layout)
         *
         * Should select 4.
         */
        if (to == NULL &&
                container->details->auto_layout &&
                better_destination_fallback_fallback != NULL)
        {
            to = find_best_icon
                 (container, from,
                  better_destination_fallback_fallback,
                  &data);
        }

        if (to == NULL)
        {
            to = from;
        }

    }

    keyboard_move_to (container, to, from, event);
}

static gboolean
is_rectangle_selection_event (CdkEventKey *event)
{
    return (event->state & CDK_CONTROL_MASK) != 0 &&
           (event->state & CDK_SHIFT_MASK) != 0;
}

static void
keyboard_right (BaulIconContainer *container,
                CdkEventKey *event)
{
    IsBetterIconFunction fallback;
    IsBetterIconFunction next_column_fallback;

    fallback = NULL;
    if (container->details->auto_layout &&
            !baul_icon_container_is_layout_vertical (container) &&
            !is_rectangle_selection_event (event))
    {
        fallback = next_row_leftmost;
    }

    next_column_fallback = NULL;
    if (baul_icon_container_is_layout_vertical (container) &&
            ctk_widget_get_direction (CTK_WIDGET (container)) != CTK_TEXT_DIR_RTL)
    {
        next_column_fallback = next_column_bottommost;
    }

    /* Right selects the next icon in the same row.
     * Control-Right sets the keyboard focus to the next icon in the same row.
     */
    keyboard_arrow_key (container,
                        event,
                        CTK_DIR_RIGHT,
                        rightmost_in_bottom_row,
                        baul_icon_container_is_layout_rtl (container) ?
                        rightmost_in_top_row : leftmost_in_top_row,
                        same_row_right_side_leftmost,
                        fallback,
                        next_column_fallback,
                        closest_in_90_degrees);
}

static void
keyboard_left (BaulIconContainer *container,
               CdkEventKey *event)
{
    IsBetterIconFunction fallback;
    IsBetterIconFunction previous_column_fallback;

    fallback = NULL;
    if (container->details->auto_layout &&
            !baul_icon_container_is_layout_vertical (container) &&
            !is_rectangle_selection_event (event))
    {
        fallback = previous_row_rightmost;
    }

    previous_column_fallback = NULL;
    if (baul_icon_container_is_layout_vertical (container) &&
            ctk_widget_get_direction (CTK_WIDGET (container)) == CTK_TEXT_DIR_RTL)
    {
        previous_column_fallback = previous_column_lowest;
    }

    /* Left selects the next icon in the same row.
     * Control-Left sets the keyboard focus to the next icon in the same row.
     */
    keyboard_arrow_key (container,
                        event,
                        CTK_DIR_LEFT,
                        rightmost_in_bottom_row,
                        baul_icon_container_is_layout_rtl (container) ?
                        rightmost_in_top_row : leftmost_in_top_row,
                        same_row_left_side_rightmost,
                        fallback,
                        previous_column_fallback,
                        closest_in_90_degrees);
}

static void
keyboard_down (BaulIconContainer *container,
               CdkEventKey *event)
{
    IsBetterIconFunction fallback;
    IsBetterIconFunction next_row_fallback;

    fallback = NULL;
    if (container->details->auto_layout &&
            baul_icon_container_is_layout_vertical (container) &&
            !is_rectangle_selection_event (event))
    {
        if (ctk_widget_get_direction (CTK_WIDGET (container)) == CTK_TEXT_DIR_RTL)
        {
            fallback = previous_column_highest;
        }
        else
        {
            fallback = next_column_highest;
        }
    }

    next_row_fallback = NULL;
    if (!baul_icon_container_is_layout_vertical (container))
    {
        if (ctk_widget_get_direction (CTK_WIDGET (container)) == CTK_TEXT_DIR_RTL)
        {
            next_row_fallback = next_row_leftmost;
        }
        else
        {
            next_row_fallback = next_row_rightmost;
        }
    }

    /* Down selects the next icon in the same column.
     * Control-Down sets the keyboard focus to the next icon in the same column.
     */
    keyboard_arrow_key (container,
                        event,
                        CTK_DIR_DOWN,
                        rightmost_in_bottom_row,
                        baul_icon_container_is_layout_rtl (container) ?
                        rightmost_in_top_row : leftmost_in_top_row,
                        same_column_below_highest,
                        fallback,
                        next_row_fallback,
                        closest_in_90_degrees);
}

static void
keyboard_up (BaulIconContainer *container,
             CdkEventKey *event)
{
    IsBetterIconFunction fallback;

    fallback = NULL;
    if (container->details->auto_layout &&
            baul_icon_container_is_layout_vertical (container) &&
            !is_rectangle_selection_event (event))
    {
        if (ctk_widget_get_direction (CTK_WIDGET (container)) == CTK_TEXT_DIR_RTL)
        {
            fallback = next_column_bottommost;
        }
        else
        {
            fallback = previous_column_lowest;
        }
    }

    /* Up selects the next icon in the same column.
     * Control-Up sets the keyboard focus to the next icon in the same column.
     */
    keyboard_arrow_key (container,
                        event,
                        CTK_DIR_UP,
                        rightmost_in_bottom_row,
                        baul_icon_container_is_layout_rtl (container) ?
                        rightmost_in_top_row : leftmost_in_top_row,
                        same_column_above_lowest,
                        fallback,
                        NULL,
                        closest_in_90_degrees);
}

static void
keyboard_space (BaulIconContainer *container,
                CdkEventKey *event)
{
    BaulIcon *icon;

    if (!has_selection (container) &&
            container->details->keyboard_focus != NULL)
    {
        keyboard_move_to (container,
                          container->details->keyboard_focus,
                          NULL, NULL);
    }
    else if ((event->state & CDK_CONTROL_MASK) != 0 &&
             (event->state & CDK_SHIFT_MASK) == 0)
    {
        /* Control-space toggles the selection state of the current icon. */
        if (container->details->keyboard_focus != NULL)
        {
            icon_toggle_selected (container, container->details->keyboard_focus);
            g_signal_emit (container, signals[SELECTION_CHANGED], 0);
            if  (container->details->keyboard_focus->is_selected)
            {
                container->details->range_selection_base_icon = container->details->keyboard_focus;
            }
        }
        else
        {
            icon = find_best_selected_icon (container,
                                            NULL,
                                            leftmost_in_top_row,
                                            NULL);
            if (icon == NULL)
            {
                icon = find_best_icon (container,
                                       NULL,
                                       leftmost_in_top_row,
                                       NULL);
            }
            if (icon != NULL)
            {
                set_keyboard_focus (container, icon);
            }
        }
    }
    else if ((event->state & CDK_SHIFT_MASK) != 0)
    {
        activate_selected_items_alternate (container, NULL);
    }
    else
    {
        activate_selected_items (container);
    }
}

/* look for the first icon that matches the longest part of a given
 * search pattern
 */
typedef struct
{
    gunichar *name;
    int last_match_length;
} BestNameMatch;

#ifndef TAB_NAVIGATION_DISABLED
static void
select_previous_or_next_icon (BaulIconContainer *container,
                              gboolean next,
                              CdkEventKey *event)
{
    BaulIcon *icon;
    const GList *item;

    item = NULL;
    /* Chose the icon to start with.
     * If we have a keyboard focus, start with it.
     * Otherwise, use the single selected icon.
     */
    icon = container->details->keyboard_focus;
    if (icon == NULL)
    {
        icon = get_first_selected_icon (container);
    }

    if (icon != NULL)
    {
        /* must have at least @icon in the list */
        g_assert (container->details->icons != NULL);
        item = g_list_find (container->details->icons, icon);
        g_assert (item != NULL);

        item = next ? item->next : item->prev;
        if (item == NULL)
        {
            item = next ? g_list_first (container->details->icons) : g_list_last (container->details->icons);
        }

    }
    else if (container->details->icons != NULL)
    {
        /* no selection yet, pick the first or last item to select */
        item = next ? g_list_first (container->details->icons) : g_list_last (container->details->icons);
    }

    icon = (item != NULL) ? item->data : NULL;

    if (icon != NULL)
    {
        keyboard_move_to (container, icon, NULL, event);
    }
}
#endif

static void
destroy (CtkWidget *object)
{
    BaulIconContainer *container;

    container = BAUL_ICON_CONTAINER (object);

    baul_icon_container_clear (container);

    if (container->details->rubberband_info.timer_id != 0)
    {
        g_source_remove (container->details->rubberband_info.timer_id);
        container->details->rubberband_info.timer_id = 0;
    }

    if (container->details->idle_id != 0)
    {
        g_source_remove (container->details->idle_id);
        container->details->idle_id = 0;
    }

    if (container->details->stretch_idle_id != 0)
    {
        g_source_remove (container->details->stretch_idle_id);
        container->details->stretch_idle_id = 0;
    }

    if (container->details->align_idle_id != 0)
    {
        g_source_remove (container->details->align_idle_id);
        container->details->align_idle_id = 0;
    }

    if (container->details->selection_changed_id != 0)
    {
        g_source_remove (container->details->selection_changed_id);
        container->details->selection_changed_id = 0;
    }

    if (container->details->size_allocation_count_id != 0)
    {
        g_source_remove (container->details->size_allocation_count_id);
        container->details->size_allocation_count_id = 0;
    }

    /* destroy interactive search dialog */
    if (container->details->search_window)
    {
        ctk_widget_destroy (container->details->search_window);
        container->details->search_window = NULL;
        container->details->search_entry = NULL;
        if (container->details->typeselect_flush_timeout)
        {
            g_source_remove (container->details->typeselect_flush_timeout);
            container->details->typeselect_flush_timeout = 0;
        }
    }

    CTK_WIDGET_CLASS (baul_icon_container_parent_class)->destroy (object);
}

static void
finalize (GObject *object)
{
    BaulIconContainerDetails *details;

    details = BAUL_ICON_CONTAINER (object)->details;

    g_signal_handlers_disconnect_by_func (baul_icon_view_preferences,
                                          text_ellipsis_limit_changed_container_callback,
                                          object);
    g_signal_handlers_disconnect_by_func (baul_desktop_preferences,
                                          text_ellipsis_limit_changed_container_callback,
                                          object);

    g_hash_table_destroy (details->icon_set);
    details->icon_set = NULL;

    g_free (details->font);

    if (details->a11y_item_action_queue != NULL)
    {
        while (!g_queue_is_empty (details->a11y_item_action_queue))
        {
            g_free (g_queue_pop_head (details->a11y_item_action_queue));
        }
        g_queue_free (details->a11y_item_action_queue);
    }
    if (details->a11y_item_action_idle_handler != 0)
    {
        g_source_remove (details->a11y_item_action_idle_handler);
    }

    g_free (details);

    G_OBJECT_CLASS (baul_icon_container_parent_class)->finalize (object);
}

/* CtkWidget methods.  */

static gboolean
clear_size_allocation_count (gpointer data)
{
    BaulIconContainer *container;

    container = BAUL_ICON_CONTAINER (data);

    container->details->size_allocation_count_id = 0;
    container->details->size_allocation_count = 0;

    return FALSE;
}

static void
size_allocate (CtkWidget *widget,
               CtkAllocation *allocation)
{
    BaulIconContainer *container;
    gboolean need_layout_redone;
    CtkAllocation wid_allocation;

    container = BAUL_ICON_CONTAINER (widget);

    need_layout_redone = !container->details->has_been_allocated;
    ctk_widget_get_allocation (widget, &wid_allocation);

    if (allocation->width != wid_allocation.width)
    {
        need_layout_redone = TRUE;
    }

    if (allocation->height != wid_allocation.height)
    {
        need_layout_redone = TRUE;
    }

    /* Under some conditions we can end up in a loop when size allocating.
     * This happens when the icons don't fit without a scrollbar, but fits
     * when a scrollbar is added (bug #129963 for details).
     * We keep track of this looping by increasing a counter in size_allocate
     * and clearing it in a high-prio idle (the only way to detect the loop is
     * done).
     * When we've done at more than two iterations (with/without scrollbar)
     * we terminate this looping by not redoing the layout when the width
     * is wider than the current one (i.e when removing the scrollbar).
     */
    if (container->details->size_allocation_count_id == 0)
    {
        container->details->size_allocation_count_id =
            g_idle_add_full  (G_PRIORITY_HIGH,
                              clear_size_allocation_count,
                              container, NULL);
    }
    container->details->size_allocation_count++;
    if (container->details->size_allocation_count > 2 &&
            allocation->width >= wid_allocation.width)
    {
        need_layout_redone = FALSE;
    }

    CTK_WIDGET_CLASS (baul_icon_container_parent_class)->size_allocate (widget, allocation);

    container->details->has_been_allocated = TRUE;

    if (need_layout_redone)
    {
        redo_layout (container);
    }
}

static CtkSizeRequestMode
get_request_mode (CtkWidget *widget G_GNUC_UNUSED)
{
    /* Don't trade size at all, since we get whatever we get anyway. */
    return CTK_SIZE_REQUEST_CONSTANT_SIZE;
}

    /* We need to implement these since the CtkScrolledWindow uses them
    to guess whether to show scrollbars or not, and if we don't report
    anything it'll tend to get it wrong causing double calls
    to size_allocate (at different sizes) during its size allocation. */
static void
get_prefered_width (CtkWidget *widget,
                    gint      *minimum_size,
                    gint      *natural_size)
{
    EelCanvasGroup *root;
    double x1, x2;
    int cx1, cx2;
    int width;

    root = eel_canvas_root (EEL_CANVAS (widget));
    eel_canvas_item_get_bounds (EEL_CANVAS_ITEM (root),
                    &x1, NULL, &x2, NULL);
    eel_canvas_w2c (EEL_CANVAS (widget), x1, 0, &cx1, NULL);
    eel_canvas_w2c (EEL_CANVAS (widget), x2, 0, &cx2, NULL);

    width = cx2 - cx1;
    if (natural_size) {
        *natural_size = width;
    }
    if (minimum_size) {
        *minimum_size = width;
    }
}

static void
get_prefered_height (CtkWidget *widget,
                     gint      *minimum_size,
                     gint      *natural_size)
{
    EelCanvasGroup *root;
    double y1, y2;
    int cy1, cy2;
    int height;

    root = eel_canvas_root (EEL_CANVAS (widget));
    eel_canvas_item_get_bounds (EEL_CANVAS_ITEM (root),
                    NULL, &y1, NULL, &y2);
    eel_canvas_w2c (EEL_CANVAS (widget), 0, y1, NULL, &cy1);
    eel_canvas_w2c (EEL_CANVAS (widget), 0, y2, NULL, &cy2);

    height = cy2 - cy1;
    if (natural_size) {
        *natural_size = height;
    }
    if (minimum_size) {
        *minimum_size = height;
    }
}

static gboolean
draw (CtkWidget *widget, cairo_t *cr)
{
    if (!BAUL_ICON_CONTAINER (widget)->details->is_desktop)
    {
        eel_background_draw (widget, cr);
    }

    return CTK_WIDGET_CLASS (baul_icon_container_parent_class)->draw (widget,
                                                                      cr);
}

static void
realize (CtkWidget *widget)
{
    CtkAdjustment *vadj, *hadj;
    BaulIconContainer *container;

    CTK_WIDGET_CLASS (baul_icon_container_parent_class)->realize (widget);

    container = BAUL_ICON_CONTAINER (widget);

    /* Set up DnD.  */
    baul_icon_dnd_init (container);

    hadj = ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (widget));
    g_signal_connect (hadj, "value_changed",
                      G_CALLBACK (handle_hadjustment_changed), widget);

    vadj = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (widget));
    g_signal_connect (vadj, "value_changed",
                      G_CALLBACK (handle_vadjustment_changed), widget);

}

static void
unrealize (CtkWidget *widget)
{
    BaulIconContainer *container;

    container = BAUL_ICON_CONTAINER (widget);

    baul_icon_dnd_fini (container);

    if (container->details->typeselect_flush_timeout)
    {
        g_source_remove (container->details->typeselect_flush_timeout);
        container->details->typeselect_flush_timeout = 0;
    }

    CTK_WIDGET_CLASS (baul_icon_container_parent_class)->unrealize (widget);
}

static void
style_updated (CtkWidget *widget)
{
    BaulIconContainer *container;

    container = BAUL_ICON_CONTAINER (widget);
    container->details->use_drop_shadows = container->details->drop_shadows_requested;

    /* Don't chain up to parent, if this is a desktop container,
    * because that resets the background of the window.
    */
    if (!baul_icon_container_get_is_desktop (container)) {
           CTK_WIDGET_CLASS (baul_icon_container_parent_class)->style_updated (widget);
    }

    if (ctk_widget_get_realized (widget))
    {
        invalidate_labels (container);
        baul_icon_container_request_update_all (container);
    }
}

static gboolean
button_press_event (CtkWidget *widget,
                    CdkEventButton *event)
{
    BaulIconContainer *container;
    gboolean selection_changed;
    gboolean return_value;
    gboolean clicked_on_icon;

    container = BAUL_ICON_CONTAINER (widget);
    container->details->button_down_time = event->time;

    /* Forget about the old keyboard selection now that we've started mousing. */
    clear_keyboard_focus (container);
    clear_keyboard_rubberband_start (container);

    if (event->type == CDK_2BUTTON_PRESS || event->type == CDK_3BUTTON_PRESS)
    {
        /* We use our own double-click detection. */
        return TRUE;
    }

    /* Invoke the canvas event handler and see if an item picks up the event. */
    clicked_on_icon = CTK_WIDGET_CLASS (baul_icon_container_parent_class)->button_press_event (widget, event);

    /* Move focus to icon container, unless we're still renaming (to avoid exiting
     * renaming mode)
     */
    if (!ctk_widget_has_focus (widget) && !(is_renaming (container) || is_renaming_pending (container)))
    {
        ctk_widget_grab_focus (widget);
    }

    if (clicked_on_icon)
    {
        return TRUE;
    }

    if (event->button == DRAG_BUTTON &&
            event->type == CDK_BUTTON_PRESS)
    {
        /* Clear the last click icon for double click */
        container->details->double_click_icon[1] = container->details->double_click_icon[0];
        container->details->double_click_icon[0] = NULL;
    }

    /* Button 1 does rubber banding. */
    if (event->button == RUBBERBAND_BUTTON)
    {
        if (! button_event_modifies_selection (event))
        {
            selection_changed = unselect_all (container);
            if (selection_changed)
            {
                g_signal_emit (container,
                               signals[SELECTION_CHANGED], 0);
            }
        }

        start_rubberbanding (container, event);
        return TRUE;
    }

    /* Prevent multi-button weirdness such as bug 6181 */
    if (container->details->rubberband_info.active)
    {
        return TRUE;
    }

    /* Button 2 may be passed to the window manager. */
    if (event->button == MIDDLE_BUTTON)
    {
        selection_changed = unselect_all (container);
        if (selection_changed)
        {
            g_signal_emit (container, signals[SELECTION_CHANGED], 0);
        }
        g_signal_emit (widget, signals[MIDDLE_CLICK], 0, event);
        return TRUE;
    }

    /* Button 3 does a contextual menu. */
    if (event->button == CONTEXTUAL_MENU_BUTTON)
    {
        end_renaming_mode (container, TRUE);
        selection_changed = unselect_all (container);
        if (selection_changed)
        {
            g_signal_emit (container, signals[SELECTION_CHANGED], 0);
        }
        g_signal_emit (widget, signals[CONTEXT_CLICK_BACKGROUND], 0, event);
        return TRUE;
    }

    /* Otherwise, we emit a button_press message. */
    g_signal_emit (widget,
                   signals[BUTTON_PRESS], 0, event,
                   &return_value);
    return return_value;
}

static void
baul_icon_container_did_not_drag (BaulIconContainer *container,
                                  CdkEventButton *event)
{
    BaulIconContainerDetails *details;
    gboolean selection_changed;
    static gint64 last_click_time = 0;
    static gint click_count = 0;
    gint double_click_time;
    gint64 current_time;

    details = container->details;

    if (details->icon_selected_on_button_down &&
            ((event->state & CDK_CONTROL_MASK) != 0 ||
             (event->state & CDK_SHIFT_MASK) == 0))
    {
        if (button_event_modifies_selection (event))
        {
            details->range_selection_base_icon = NULL;
            icon_toggle_selected (container, details->drag_icon);
            g_signal_emit (container,
                           signals[SELECTION_CHANGED], 0);
        }
        else
        {
            details->range_selection_base_icon = details->drag_icon;
            selection_changed = select_one_unselect_others
                                (container, details->drag_icon);

            if (selection_changed)
            {
                g_signal_emit (container,
                               signals[SELECTION_CHANGED], 0);
            }
        }
    }

    if (details->drag_icon != NULL &&
            (details->single_click_mode ||
             event->button == MIDDLE_BUTTON))
    {
        /* Determine click count */
        g_object_get (G_OBJECT (ctk_widget_get_settings (CTK_WIDGET (container))),
                      "ctk-double-click-time", &double_click_time,
                      NULL);
        current_time = g_get_monotonic_time ();
        if (current_time - last_click_time < double_click_time * 1000)
        {
            click_count++;
        }
        else
        {
            click_count = 0;
        }

        /* Stash time for next compare */
        last_click_time = current_time;

        /* If single-click mode, activate the selected icons, unless modifying
         * the selection or pressing for a very long time, or double clicking.
         */


        if (click_count == 0 &&
                event->time - details->button_down_time < MAX_CLICK_TIME &&
                ! button_event_modifies_selection (event))
        {

            /* It's a tricky UI issue whether this should activate
             * just the clicked item (as if it were a link), or all
             * the selected items (as if you were issuing an "activate
             * selection" command). For now, we're trying the activate
             * entire selection version to see how it feels. Note that
             * BaulList goes the other way because its "links" seem
             * much more link-like.
             */
            if (event->button == MIDDLE_BUTTON)
            {
                activate_selected_items_alternate (container, NULL);
            }
            else
            {
                activate_selected_items (container);
            }
        }
    }
}

static gboolean
clicked_within_double_click_interval (BaulIconContainer *container)
{
    static gint64 last_click_time = 0;
    static gint click_count = 0;
    gint double_click_time;
    gint64 current_time;

    /* Determine click count */
    g_object_get (G_OBJECT (ctk_widget_get_settings (CTK_WIDGET (container))),
                  "ctk-double-click-time", &double_click_time,
                  NULL);
    current_time = g_get_monotonic_time ();
    if (current_time - last_click_time < double_click_time * 1000)
    {
        click_count++;
    }
    else
    {
        click_count = 0;
    }

    /* Stash time for next compare */
    last_click_time = current_time;

    /* Only allow double click */
    if (click_count == 1) {
            click_count = 0;
            return TRUE;
    } else {
            return FALSE;
    }
}

static void
clear_drag_state (BaulIconContainer *container)
{
    container->details->drag_icon = NULL;
    container->details->drag_state = DRAG_STATE_INITIAL;
}

static gboolean
start_stretching (BaulIconContainer *container,
		  CdkEvent *event)
{
    BaulIconContainerDetails *details;
    BaulIcon *icon;
    EelDPoint world_point;
    CtkWidget *toplevel;
    CdkDisplay *display;
    CtkCornerType corner;
    CdkCursor *cursor;

    details = container->details;
    icon = details->stretch_icon;
    display = ctk_widget_get_display (CTK_WIDGET (container));

    /* Check if we hit the stretch handles. */
    world_point.x = details->drag_x;
    world_point.y = details->drag_y;
    if (!baul_icon_canvas_item_hit_test_stretch_handles (icon->item, world_point, &corner))
    {
        return FALSE;
    }

    switch (corner)
    {
    case CTK_CORNER_TOP_LEFT:
        cursor = cdk_cursor_new_for_display (display, CDK_TOP_LEFT_CORNER);
        break;
    case CTK_CORNER_BOTTOM_LEFT:
        cursor = cdk_cursor_new_for_display (display, CDK_BOTTOM_LEFT_CORNER);
        break;
    case CTK_CORNER_TOP_RIGHT:
        cursor = cdk_cursor_new_for_display (display, CDK_TOP_RIGHT_CORNER);
        break;
    case CTK_CORNER_BOTTOM_RIGHT:
        cursor = cdk_cursor_new_for_display (display, CDK_BOTTOM_RIGHT_CORNER);
        break;
    default:
        cursor = NULL;
        break;
    }
    /* Set up the dragging. */
    details->drag_state = DRAG_STATE_STRETCH;
    eel_canvas_w2c (EEL_CANVAS (container),
                    details->drag_x,
                    details->drag_y,
                    &details->stretch_start.pointer_x,
                    &details->stretch_start.pointer_y);
    eel_canvas_w2c (EEL_CANVAS (container),
                    icon->x, icon->y,
                    &details->stretch_start.icon_x,
                    &details->stretch_start.icon_y);
    icon_get_size (container, icon,
                   &details->stretch_start.icon_size);

    eel_canvas_item_grab (EEL_CANVAS_ITEM (icon->item),
                          (CDK_POINTER_MOTION_MASK
                           | CDK_BUTTON_RELEASE_MASK),
                          cursor,
                          event);

    if (cursor)
        g_object_unref (cursor);

    /* Ensure the window itself is focused.. */
    toplevel = ctk_widget_get_toplevel (CTK_WIDGET (container));
    if (toplevel != NULL && ctk_widget_get_realized (toplevel))
    {
        cdk_window_focus (ctk_widget_get_window (toplevel), CDK_CURRENT_TIME);
    }

    return TRUE;
}

static gboolean
update_stretch_at_idle (BaulIconContainer *container)
{
    BaulIconContainerDetails *details;
    BaulIcon *icon;
    double world_x, world_y;
    StretchState stretch_state;

    details = container->details;
    icon = details->stretch_icon;

    if (icon == NULL)
    {
        container->details->stretch_idle_id = 0;
        return FALSE;
    }

    eel_canvas_w2c (EEL_CANVAS (container),
                    details->world_x, details->world_y,
                    &stretch_state.pointer_x, &stretch_state.pointer_y);

    compute_stretch (&details->stretch_start,
                     &stretch_state);

    eel_canvas_c2w (EEL_CANVAS (container),
                    stretch_state.icon_x, stretch_state.icon_y,
                    &world_x, &world_y);

    icon_set_position (icon, world_x, world_y);
    icon_set_size (container, icon, stretch_state.icon_size, FALSE, FALSE);

    container->details->stretch_idle_id = 0;

    return FALSE;
}

static void
continue_stretching (BaulIconContainer *container,
                     double world_x, double world_y)
{

    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    container->details->world_x = world_x;
    container->details->world_y = world_y;

    if (container->details->stretch_idle_id == 0)
    {
        container->details->stretch_idle_id = g_idle_add ((GSourceFunc) update_stretch_at_idle, container);
    }
}

static gboolean
keyboard_stretching (BaulIconContainer *container,
                     CdkEventKey           *event)
{
    BaulIcon *icon;
    guint size;

    icon = container->details->stretch_icon;

    if (icon == NULL || !icon->is_selected)
    {
        return FALSE;
    }

    icon_get_size (container, icon, &size);

    switch (event->keyval)
    {
    case CDK_KEY_equal:
    case CDK_KEY_plus:
    case CDK_KEY_KP_Add:
        icon_set_size (container, icon, size + 5, FALSE, FALSE);
        break;
    case CDK_KEY_minus:
    case CDK_KEY_KP_Subtract:
        icon_set_size (container, icon, size - 5, FALSE, FALSE);
        break;
    case CDK_KEY_0:
    case CDK_KEY_KP_0:
        baul_icon_container_move_icon (container, icon,
                                       icon->x, icon->y,
                                       1.0,
                                       FALSE, TRUE, TRUE);
        break;
    }

    return TRUE;
}

static void
ungrab_stretch_icon (BaulIconContainer *container)
{
    eel_canvas_item_ungrab (EEL_CANVAS_ITEM (container->details->stretch_icon->item));
}

static void
end_stretching (BaulIconContainer *container,
                double world_x, double world_y)
{
    BaulIconPosition position;
    BaulIcon *icon;

    continue_stretching (container, world_x, world_y);
    ungrab_stretch_icon (container);

    /* now that we're done stretching, update the icon's position */

    icon = container->details->drag_icon;
    if (baul_icon_container_is_layout_rtl (container))
    {
        position.x = icon->saved_ltr_x = get_mirror_x_position (container, icon, icon->x);
    }
    else
    {
        position.x = icon->x;
    }
    position.y = icon->y;
    position.scale = icon->scale;
    g_signal_emit (container,
                   signals[ICON_POSITION_CHANGED], 0,
                   icon->data, &position);

    clear_drag_state (container);
    redo_layout (container);
}

static gboolean
undo_stretching (BaulIconContainer *container)
{
    BaulIcon *stretched_icon;

    stretched_icon = container->details->stretch_icon;

    if (stretched_icon == NULL)
    {
        return FALSE;
    }

    if (container->details->drag_state == DRAG_STATE_STRETCH)
    {
        ungrab_stretch_icon (container);
        clear_drag_state (container);
    }
    baul_icon_canvas_item_set_show_stretch_handles
    (stretched_icon->item, FALSE);

    icon_set_position (stretched_icon,
                       container->details->stretch_initial_x,
                       container->details->stretch_initial_y);
    icon_set_size (container,
                   stretched_icon,
                   container->details->stretch_initial_size,
                   TRUE,
                   TRUE);

    container->details->stretch_icon = NULL;
    emit_stretch_ended (container, stretched_icon);
    redo_layout (container);

    return TRUE;
}

static gboolean
button_release_event (CtkWidget *widget,
                      CdkEventButton *event)
{
    BaulIconContainer *container;
    BaulIconContainerDetails *details;
    double world_x, world_y;

    container = BAUL_ICON_CONTAINER (widget);
    details = container->details;

    if (event->button == RUBBERBAND_BUTTON && details->rubberband_info.active)
    {
        stop_rubberbanding (container);
        return TRUE;
    }

    if (event->button == details->drag_button)
    {
        details->drag_button = 0;

        switch (details->drag_state)
        {
        case DRAG_STATE_MOVE_OR_COPY:
            if (!details->drag_started)
            {
                baul_icon_container_did_not_drag (container, event);
            }
            else
            {
                baul_icon_dnd_end_drag (container);
                baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                                "end drag from icon container");
            }
            break;
        case DRAG_STATE_STRETCH:
            eel_canvas_window_to_world
            (EEL_CANVAS (container), event->x, event->y, &world_x, &world_y);
            end_stretching (container, world_x, world_y);
            break;
        default:
            break;
        }

        clear_drag_state (container);
        return TRUE;
    }

    return CTK_WIDGET_CLASS (baul_icon_container_parent_class)->button_release_event (widget, event);
}

static int
motion_notify_event (CtkWidget *widget,
                     CdkEventMotion *event)
{
    BaulIconContainer *container;
    BaulIconContainerDetails *details;
    double world_x, world_y;
    int canvas_x, canvas_y;
    CdkDragAction actions;

    container = BAUL_ICON_CONTAINER (widget);
    details = container->details;

    if (details->drag_button != 0)
    {
        switch (details->drag_state)
        {
        case DRAG_STATE_MOVE_OR_COPY:
            if (details->drag_started)
            {
                break;
            }

            eel_canvas_window_to_world
            (EEL_CANVAS (container), event->x, event->y, &world_x, &world_y);

            if (ctk_drag_check_threshold (widget,
                                          details->drag_x,
                                          details->drag_y,
                                          world_x,
                                          world_y))
            {
                details->drag_started = TRUE;
                details->drag_state = DRAG_STATE_MOVE_OR_COPY;

                end_renaming_mode (container, TRUE);

                eel_canvas_w2c (EEL_CANVAS (container),
                                details->drag_x,
                                details->drag_y,
                                &canvas_x,
                                &canvas_y);

                actions = CDK_ACTION_COPY
                          | CDK_ACTION_LINK
                          | CDK_ACTION_ASK;

                if (container->details->drag_allow_moves)
                {
                    actions |= CDK_ACTION_MOVE;
                }

                baul_icon_dnd_begin_drag (container,
                                          actions,
                                          details->drag_button,
                                          event,
                                          canvas_x,
                                          canvas_y);
                baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                                "begin drag from icon container");
            }
            break;
        case DRAG_STATE_STRETCH:
            eel_canvas_window_to_world
            (EEL_CANVAS (container), event->x, event->y, &world_x, &world_y);
            continue_stretching (container, world_x, world_y);
            break;
        default:
            break;
        }
    }

    return CTK_WIDGET_CLASS (baul_icon_container_parent_class)->motion_notify_event (widget, event);
}

static void
baul_icon_container_search_position_func (BaulIconContainer *container,
        CtkWidget *search_dialog)
{
    gint x, y;
    gint cont_x, cont_y;
    gint cont_width, cont_height;
    gint scale;
    CdkWindow *cont_window;
    CdkScreen *screen;
    CtkRequisition requisition;
    CdkMonitor *monitor_num;
    CdkRectangle monitor;


    cont_window = ctk_widget_get_window (CTK_WIDGET (container));
    scale = ctk_widget_get_scale_factor (CTK_WIDGET (container));
    screen = cdk_window_get_screen (cont_window);

    monitor_num = cdk_display_get_monitor_at_window (cdk_screen_get_display (screen),
                                                     cont_window);
    cdk_monitor_get_geometry (monitor_num, &monitor);

    ctk_widget_realize (search_dialog);

    cdk_window_get_origin (cont_window, &cont_x, &cont_y);

    cont_width = cdk_window_get_width (cont_window);
    cont_height = cdk_window_get_height (cont_window);

    ctk_widget_get_preferred_size (search_dialog, &requisition, NULL);

    if (cont_x + cont_width - requisition.width > WidthOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale)
    {
        x = WidthOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale - requisition.width;
    }
    else if (cont_x + cont_width - requisition.width < 0)
    {
        x = 0;
    }
    else
    {
        x = cont_x + cont_width - requisition.width;
    }

    if (cont_y + cont_height > HeightOfScreen (cdk_x11_screen_get_xscreen (screen)))
    {
        y = HeightOfScreen (cdk_x11_screen_get_xscreen (screen)) - requisition.height;
    }
    else if (cont_y + cont_height < 0)     /* isn't really possible ... */
    {
        y = 0;
    }
    else
    {
        y = cont_y + cont_height;
    }

    ctk_window_move (CTK_WINDOW (search_dialog), x, y);
}

static gboolean
baul_icon_container_real_search_enable_popdown (gpointer data)
{
    BaulIconContainer *container = (BaulIconContainer *)data;

    container->details->disable_popdown = FALSE;

    g_object_unref (container);

    return FALSE;
}

static void
baul_icon_container_search_enable_popdown (CtkWidget *widget G_GNUC_UNUSED,
					   gpointer   data)
{
    BaulIconContainer *container = (BaulIconContainer *) data;

    g_object_ref (container);
    g_timeout_add (200, baul_icon_container_real_search_enable_popdown, data);
}

static void
baul_icon_container_search_disable_popdown (CtkEntry *entry G_GNUC_UNUSED,
					    CtkMenu  *menu,
					    gpointer  data)
{
    BaulIconContainer *container = (BaulIconContainer *) data;

    container->details->disable_popdown = TRUE;
    g_signal_connect (menu, "hide",
                      G_CALLBACK (baul_icon_container_search_enable_popdown),
                      data);
}

/* Cut and paste from ctkwindow.c */
static void
send_focus_change (CtkWidget *widget, gboolean in)
{
    CdkEvent *fevent;

    fevent = cdk_event_new (CDK_FOCUS_CHANGE);

    g_object_ref (widget);
    ((CdkEventFocus *) fevent)->in = in;

    ctk_widget_send_focus_change (widget, fevent);

    fevent->focus_change.type = CDK_FOCUS_CHANGE;
    fevent->focus_change.window = g_object_ref (ctk_widget_get_window (widget));
    fevent->focus_change.in = in;

    ctk_widget_event (widget, fevent);

    g_object_notify (G_OBJECT (widget), "has-focus");

    g_object_unref (widget);
    cdk_event_free (fevent);
}

static void
baul_icon_container_search_dialog_hide (CtkWidget *search_dialog,
                                        BaulIconContainer *container)
{
    if (container->details->disable_popdown)
    {
        return;
    }

    if (container->details->search_entry_changed_id)
    {
        g_signal_handler_disconnect (container->details->search_entry,
                                     container->details->search_entry_changed_id);
        container->details->search_entry_changed_id = 0;
    }
    if (container->details->typeselect_flush_timeout)
    {
        g_source_remove (container->details->typeselect_flush_timeout);
        container->details->typeselect_flush_timeout = 0;
    }

    /* send focus-in event */
    send_focus_change (CTK_WIDGET (container->details->search_entry), FALSE);
    ctk_widget_hide (search_dialog);
    ctk_entry_set_text (CTK_ENTRY (container->details->search_entry), "");
}

static gboolean
baul_icon_container_search_entry_flush_timeout (BaulIconContainer *container)
{
    baul_icon_container_search_dialog_hide (container->details->search_window, container);

    return TRUE;
}

/* Because we're visible but offscreen, we just set a flag in the preedit
 * callback.
 */
static void
baul_icon_container_search_preedit_changed (CtkEntry          *entry G_GNUC_UNUSED,
					    gchar             *preedit G_GNUC_UNUSED,
					    BaulIconContainer *container)
{
    container->details->imcontext_changed = 1;
    if (container->details->typeselect_flush_timeout)
    {
        g_source_remove (container->details->typeselect_flush_timeout);
        container->details->typeselect_flush_timeout =
            g_timeout_add_seconds (BAUL_ICON_CONTAINER_SEARCH_DIALOG_TIMEOUT,
                                   (GSourceFunc) baul_icon_container_search_entry_flush_timeout,
                                   container);
    }
}

static void
baul_icon_container_search_activate (CtkEntry          *entry G_GNUC_UNUSED,
				     BaulIconContainer *container)
{
    baul_icon_container_search_dialog_hide (container->details->search_window,
                                            container);

    activate_selected_items (container);
}

static gboolean
baul_icon_container_search_delete_event (CtkWidget         *widget,
					 CdkEventAny       *event G_GNUC_UNUSED,
					 BaulIconContainer *container)
{
    g_assert (CTK_IS_WIDGET (widget));

    baul_icon_container_search_dialog_hide (widget, container);

    return TRUE;
}

static gboolean
baul_icon_container_search_button_press_event (CtkWidget *widget,
        CdkEventButton *event,
        BaulIconContainer *container)
{
    g_assert (CTK_IS_WIDGET (widget));

    baul_icon_container_search_dialog_hide (widget, container);

    if (event->window == ctk_layout_get_bin_window (CTK_LAYOUT (container)))
    {
        button_press_event (CTK_WIDGET (container), event);
    }

    return TRUE;
}

static void
baul_icon_container_get_icon_text (BaulIconContainer *container,
                                   BaulIconData      *data,
                                   char                 **editable_text,
                                   char                 **additional_text,
                                   gboolean               include_invisible)
{
    BaulIconContainerClass *klass;

    klass = BAUL_ICON_CONTAINER_GET_CLASS (container);
    g_assert (klass->get_icon_text != NULL);

    klass->get_icon_text (container, data, editable_text, additional_text, include_invisible);
}

static gboolean
baul_icon_container_search_iter (BaulIconContainer *container,
                                 const char *key, gint n)
{
    GList *p;
    BaulIcon *icon;
    char *name;
    int count;
    char *normalized_key, *case_normalized_key;
    char *normalized_name, *case_normalized_name;

    g_assert (key != NULL);
    g_assert (n >= 1);

    normalized_key = g_utf8_normalize (key, -1, G_NORMALIZE_ALL);
    if (!normalized_key)
    {
        return FALSE;
    }
    case_normalized_key = g_utf8_casefold (normalized_key, -1);
    g_free (normalized_key);
    if (!case_normalized_key)
    {
        return FALSE;
    }

    icon = NULL;
    name = NULL;
    count = 0;
    for (p = container->details->icons; p != NULL && count != n; p = p->next)
    {
        icon = p->data;
        baul_icon_container_get_icon_text (container, icon->data, &name,
                                           NULL, TRUE);

        /* This can happen if a key event is handled really early while
         * loading the icon container, before the items have all been
         * updated once.
         */
        if (!name)
        {
            continue;
        }

        normalized_name = g_utf8_normalize (name, -1, G_NORMALIZE_ALL);
        if (!normalized_name)
        {
            continue;
        }
        case_normalized_name = g_utf8_casefold (normalized_name, -1);
        g_free (normalized_name);
        if (!case_normalized_name)
        {
            continue;
        }

        if (strncmp (case_normalized_key, case_normalized_name,
                     strlen (case_normalized_key)) == 0)
        {
            count++;
        }

        g_free (case_normalized_name);
        g_free (name);
        name = NULL;
    }

    g_free (case_normalized_key);

    if (count == n)
    {
        if (select_one_unselect_others (container, icon))
        {
            g_signal_emit (container, signals[SELECTION_CHANGED], 0);
        }
        schedule_keyboard_icon_reveal (container, icon);

        return TRUE;
    }

    return FALSE;
}

static void
baul_icon_container_search_move (CtkWidget         *window G_GNUC_UNUSED,
				 BaulIconContainer *container,
				 gboolean           up)
{
    gboolean ret;
    gint len;
    const gchar *text;

    text = ctk_entry_get_text (CTK_ENTRY (container->details->search_entry));

    g_assert (text != NULL);

    if (container->details->selected_iter == 0)
    {
        return;
    }

    if (up && container->details->selected_iter == 1)
    {
        return;
    }

    len = strlen (text);

    if (len < 1)
    {
        return;
    }

    /* search */
    unselect_all (container);

    ret = baul_icon_container_search_iter (container, text,
                                           up?((container->details->selected_iter) - 1):((container->details->selected_iter + 1)));

    if (ret)
    {
        /* found */
        container->details->selected_iter += up?(-1):(1);
    }
    else
    {
        /* return to old iter */
        baul_icon_container_search_iter (container, text,
                                         container->details->selected_iter);
    }
}

static gboolean
baul_icon_container_search_scroll_event (CtkWidget *widget,
        CdkEventScroll *event,
        BaulIconContainer *container)
{
    gboolean retval = FALSE;

    if (event->direction == CDK_SCROLL_UP)
    {
        baul_icon_container_search_move (widget, container, TRUE);
        retval = TRUE;
    }
    else if (event->direction == CDK_SCROLL_DOWN)
    {
        baul_icon_container_search_move (widget, container, FALSE);
        retval = TRUE;
    }

    /* renew the flush timeout */
    if (retval && container->details->typeselect_flush_timeout)
    {
        g_source_remove (container->details->typeselect_flush_timeout);
        container->details->typeselect_flush_timeout =
            g_timeout_add_seconds (BAUL_ICON_CONTAINER_SEARCH_DIALOG_TIMEOUT,
                                   (GSourceFunc) baul_icon_container_search_entry_flush_timeout,
                                   container);
    }

    return retval;
}

static gboolean
baul_icon_container_search_key_press_event (CtkWidget *widget,
        CdkEventKey *event,
        BaulIconContainer *container)
{
    gboolean retval = FALSE;

    g_assert (CTK_IS_WIDGET (widget));
    g_assert (BAUL_IS_ICON_CONTAINER (container));

    /* close window and cancel the search */
    if (event->keyval == CDK_KEY_Escape || event->keyval == CDK_KEY_Tab)
    {
        baul_icon_container_search_dialog_hide (widget, container);
        return TRUE;
    }

    /* close window and activate alternate */
    if (event->keyval == CDK_KEY_Return && event->state & CDK_SHIFT_MASK)
    {
        baul_icon_container_search_dialog_hide (widget,
                                                container);

        activate_selected_items_alternate (container, NULL);
        return TRUE;
    }

    /* select previous matching iter */
    if (event->keyval == CDK_KEY_Up || event->keyval == CDK_KEY_KP_Up)
    {
        baul_icon_container_search_move (widget, container, TRUE);
        retval = TRUE;
    }

    if (((event->state & (CDK_CONTROL_MASK | CDK_SHIFT_MASK)) == (CDK_CONTROL_MASK | CDK_SHIFT_MASK))
            && (event->keyval == CDK_KEY_g || event->keyval == CDK_KEY_G))
    {
        baul_icon_container_search_move (widget, container, TRUE);
        retval = TRUE;
    }

    /* select next matching iter */
    if (event->keyval == CDK_KEY_Down || event->keyval == CDK_KEY_KP_Down)
    {
        baul_icon_container_search_move (widget, container, FALSE);
        retval = TRUE;
    }

    if (((event->state & (CDK_CONTROL_MASK | CDK_SHIFT_MASK)) == CDK_CONTROL_MASK)
            && (event->keyval == CDK_KEY_g || event->keyval == CDK_KEY_G))
    {
        baul_icon_container_search_move (widget, container, FALSE);
        retval = TRUE;
    }

    /* renew the flush timeout */
    if (retval && container->details->typeselect_flush_timeout)
    {
        g_source_remove (container->details->typeselect_flush_timeout);
        container->details->typeselect_flush_timeout =
            g_timeout_add_seconds (BAUL_ICON_CONTAINER_SEARCH_DIALOG_TIMEOUT,
                                   (GSourceFunc) baul_icon_container_search_entry_flush_timeout,
                                   container);
    }

    return retval;
}

static void
baul_icon_container_search_init (CtkWidget   *entry,
                                 BaulIconContainer *container)
{
    gint ret;
    gint len;
    const gchar *text;

    g_assert (CTK_IS_ENTRY (entry));
    g_assert (BAUL_IS_ICON_CONTAINER (container));

    text = ctk_entry_get_text (CTK_ENTRY (entry));
    len = strlen (text);

    /* search */
    unselect_all (container);
    if (container->details->typeselect_flush_timeout)
    {
        g_source_remove (container->details->typeselect_flush_timeout);
        container->details->typeselect_flush_timeout =
            g_timeout_add_seconds (BAUL_ICON_CONTAINER_SEARCH_DIALOG_TIMEOUT,
                                   (GSourceFunc) baul_icon_container_search_entry_flush_timeout,
                                   container);
    }

    if (len < 1)
    {
        return;
    }

    ret = baul_icon_container_search_iter (container, text, 1);

    if (ret)
    {
        container->details->selected_iter = 1;
    }
}

static void
baul_icon_container_ensure_interactive_directory (BaulIconContainer *container)
{
    CtkWidget *frame, *vbox;

    if (container->details->search_window != NULL)
    {
        return;
    }

    container->details->search_window = ctk_window_new (CTK_WINDOW_POPUP);

    ctk_window_set_modal (CTK_WINDOW (container->details->search_window), TRUE);
    ctk_window_set_type_hint (CTK_WINDOW (container->details->search_window),
                              CDK_WINDOW_TYPE_HINT_COMBO);

    g_signal_connect (container->details->search_window, "delete_event",
                      G_CALLBACK (baul_icon_container_search_delete_event),
                      container);
    g_signal_connect (container->details->search_window, "key_press_event",
                      G_CALLBACK (baul_icon_container_search_key_press_event),
                      container);
    g_signal_connect (container->details->search_window, "button_press_event",
                      G_CALLBACK (baul_icon_container_search_button_press_event),
                      container);
    g_signal_connect (container->details->search_window, "scroll_event",
                      G_CALLBACK (baul_icon_container_search_scroll_event),
                      container);

    frame = ctk_frame_new (NULL);
    ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_ETCHED_IN);
    ctk_widget_show (frame);
    ctk_container_add (CTK_CONTAINER (container->details->search_window), frame);

    vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
    ctk_widget_show (vbox);
    ctk_container_add (CTK_CONTAINER (frame), vbox);
    ctk_container_set_border_width (CTK_CONTAINER (vbox), 3);

    /* add entry */
    container->details->search_entry = ctk_entry_new ();
    ctk_widget_show (container->details->search_entry);
    g_signal_connect (container->details->search_entry, "populate_popup",
                      G_CALLBACK (baul_icon_container_search_disable_popdown),
                      container);
    g_signal_connect (container->details->search_entry, "activate",
                      G_CALLBACK (baul_icon_container_search_activate),
                      container);
    g_signal_connect (container->details->search_entry,
                      "preedit-changed",
                      G_CALLBACK (baul_icon_container_search_preedit_changed),
                      container);
    ctk_container_add (CTK_CONTAINER (vbox), container->details->search_entry);

    ctk_widget_realize (container->details->search_entry);
}

/* Pops up the interactive search entry.  If keybinding is TRUE then the user
 * started this by typing the start_interactive_search keybinding.  Otherwise, it came from
 */
static gboolean
baul_icon_container_real_start_interactive_search (BaulIconContainer *container,
        gboolean keybinding)
{
    /* We only start interactive search if we have focus.  If one of our
     * children have focus, we don't want to start the search.
     */
    CtkWidgetClass *entry_parent_class;

    if (container->details->search_window != NULL &&
            ctk_widget_get_visible (container->details->search_window))
    {
        return TRUE;
    }

    if (!ctk_widget_has_focus (CTK_WIDGET (container)))
    {
        return FALSE;
    }

    baul_icon_container_ensure_interactive_directory (container);

    if (keybinding)
    {
        ctk_entry_set_text (CTK_ENTRY (container->details->search_entry), "");
    }

    /* done, show it */
    baul_icon_container_search_position_func (container, container->details->search_window);
    ctk_widget_show (container->details->search_window);
    if (container->details->search_entry_changed_id == 0)
    {
        container->details->search_entry_changed_id =
            g_signal_connect (container->details->search_entry, "changed",
                              G_CALLBACK (baul_icon_container_search_init),
                              container);
    }

    container->details->typeselect_flush_timeout =
        g_timeout_add_seconds (BAUL_ICON_CONTAINER_SEARCH_DIALOG_TIMEOUT,
                               (GSourceFunc) baul_icon_container_search_entry_flush_timeout,
                               container);

    /* Grab focus will select all the text.  We don't want that to happen, so we
    * call the parent instance and bypass the selection change.  This is probably
    * really non-kosher. */
    entry_parent_class = g_type_class_peek_parent (CTK_ENTRY_GET_CLASS (container->details->search_entry));
    (entry_parent_class->grab_focus) (container->details->search_entry);

    /* send focus-in event */
    send_focus_change (container->details->search_entry, TRUE);

    /* search first matching iter */
    baul_icon_container_search_init (container->details->search_entry, container);

    return TRUE;
}

static gboolean
baul_icon_container_start_interactive_search (BaulIconContainer *container)
{
    return baul_icon_container_real_start_interactive_search (container, TRUE);
}

static gboolean
handle_popups (BaulIconContainer *container,
	       CdkEventKey       *event G_GNUC_UNUSED,
	       const char        *signal)
{
    CdkEventButton button_event = { 0 };

    g_signal_emit_by_name (container, signal, &button_event);

    return TRUE;
}

static int
key_press_event (CtkWidget *widget,
                 CdkEventKey *event)
{
    BaulIconContainer *container;
    gboolean handled;

    container = BAUL_ICON_CONTAINER (widget);
    handled = FALSE;

    if (is_renaming (container) || is_renaming_pending (container))
    {
        switch (event->keyval)
        {
        case CDK_KEY_Return:
        case CDK_KEY_KP_Enter:
            end_renaming_mode (container, TRUE);
            handled = TRUE;
            break;
        case CDK_KEY_Escape:
            end_renaming_mode (container, FALSE);
            handled = TRUE;
            break;
        default:
            break;
        }
    }
    else
    {
        switch (event->keyval)
        {
        case CDK_KEY_Home:
        case CDK_KEY_KP_Home:
            keyboard_home (container, event);
            handled = TRUE;
            break;
        case CDK_KEY_End:
        case CDK_KEY_KP_End:
            keyboard_end (container, event);
            handled = TRUE;
            break;
        case CDK_KEY_Left:
        case CDK_KEY_KP_Left:
            /* Don't eat Alt-Left, as that is used for history browsing */
            if ((event->state & CDK_MOD1_MASK) == 0)
            {
                keyboard_left (container, event);
                handled = TRUE;
            }
            break;
        case CDK_KEY_Up:
        case CDK_KEY_KP_Up:
            /* Don't eat Alt-Up, as that is used for alt-shift-Up */
            if ((event->state & CDK_MOD1_MASK) == 0)
            {
                keyboard_up (container, event);
                handled = TRUE;
            }
            break;
        case CDK_KEY_Right:
        case CDK_KEY_KP_Right:
            /* Don't eat Alt-Right, as that is used for history browsing */
            if ((event->state & CDK_MOD1_MASK) == 0)
            {
                keyboard_right (container, event);
                handled = TRUE;
            }
            break;
        case CDK_KEY_Down:
        case CDK_KEY_KP_Down:
            /* Don't eat Alt-Down, as that is used for Open */
            if ((event->state & CDK_MOD1_MASK) == 0)
            {
                keyboard_down (container, event);
                handled = TRUE;
            }
            break;
        case CDK_KEY_space:
            keyboard_space (container, event);
            handled = TRUE;
            break;
#ifndef TAB_NAVIGATION_DISABLED
        case CDK_KEY_Tab:
        case CDK_KEY_ISO_Left_Tab:
            select_previous_or_next_icon (container,
                                          (event->state & CDK_SHIFT_MASK) == 0, event);
            handled = TRUE;
            break;
#endif
        case CDK_KEY_Return:
        case CDK_KEY_KP_Enter:
            if ((event->state & CDK_SHIFT_MASK) != 0)
            {
                activate_selected_items_alternate (container, NULL);
            }
            else
            {
                activate_selected_items (container);
            }

            handled = TRUE;
            break;
        case CDK_KEY_Escape:
            handled = undo_stretching (container);
            break;
        case CDK_KEY_plus:
        case CDK_KEY_minus:
        case CDK_KEY_equal:
        case CDK_KEY_KP_Add:
        case CDK_KEY_KP_Subtract:
        case CDK_KEY_0:
        case CDK_KEY_KP_0:
            if (event->state & CDK_CONTROL_MASK)
            {
                handled = keyboard_stretching (container, event);
            }
            break;
        case CDK_KEY_F10:
            /* handle Ctrl+F10 because we want to display the
             * background popup even if something is selected.
             * The other cases are handled by popup_menu().
             */
            if (event->state & CDK_CONTROL_MASK)
            {
                handled = handle_popups (container, event,
                                         "context_click_background");
            }
            break;
        case CDK_KEY_v:
            /* Eat Control + v to not enable type ahead */
            if ((event->state & CDK_CONTROL_MASK) != 0)
            {
                handled = TRUE;
            }
            break;
        default:
            break;
        }
    }

    if (!handled)
    {
        handled = CTK_WIDGET_CLASS (baul_icon_container_parent_class)->key_press_event (widget, event);
    }

    /* We pass the event to the search_entry.  If its text changes, then we
     * start the typeahead find capabilities.
     * Copied from BaulIconContainer */
    if (!handled &&
            event->keyval != CDK_KEY_slash /* don't steal slash key event, used for "go to" */ &&
            event->keyval != CDK_KEY_BackSpace &&
            event->keyval != CDK_KEY_Delete)
    {
        CdkEvent *new_event;
        CdkWindow *window;
        char *old_text;
        const char *new_text;
        gboolean retval;
        CdkScreen *screen;
        gboolean text_modified;
        gulong popup_menu_id;
        gint scale;

        baul_icon_container_ensure_interactive_directory (container);

        /* Make a copy of the current text */
        old_text = g_strdup (ctk_entry_get_text (CTK_ENTRY (container->details->search_entry)));
        new_event = cdk_event_copy ((CdkEvent *) event);
        window = ((CdkEventKey *) new_event)->window;
        ((CdkEventKey *) new_event)->window = ctk_widget_get_window (container->details->search_entry);
        ctk_widget_realize (container->details->search_window);

        popup_menu_id = g_signal_connect (container->details->search_entry,
                                          "popup_menu", G_CALLBACK (ctk_true), NULL);

        /* Move the entry off screen */
        screen = ctk_widget_get_screen (CTK_WIDGET (container));
        scale = ctk_widget_get_scale_factor (CTK_WIDGET (container));
        ctk_window_move (CTK_WINDOW (container->details->search_window),
                         WidthOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale + 1,
                         HeightOfScreen (cdk_x11_screen_get_xscreen (screen)) / scale + 1);
        ctk_widget_show (container->details->search_window);

        /* Send the event to the window.  If the preedit_changed signal is emitted
         * during this event, we will set priv->imcontext_changed  */
        container->details->imcontext_changed = FALSE;
        retval = ctk_widget_event (container->details->search_entry, new_event);
        ctk_widget_hide (container->details->search_window);

        g_signal_handler_disconnect (container->details->search_entry,
                                     popup_menu_id);

        /* We check to make sure that the entry tried to handle the text, and that
         * the text has changed. */
        new_text = ctk_entry_get_text (CTK_ENTRY (container->details->search_entry));
        text_modified = strcmp (old_text, new_text) != 0;
        g_free (old_text);
        if (container->details->imcontext_changed ||    /* we're in a preedit */
                (retval && text_modified))                  /* ...or the text was modified */
        {
            if (baul_icon_container_real_start_interactive_search (container, FALSE))
            {
                ctk_widget_grab_focus (CTK_WIDGET (container));
                return TRUE;
            }
            else
            {
                ctk_entry_set_text (CTK_ENTRY (container->details->search_entry), "");
                return FALSE;
            }
        }

        ((CdkEventKey *) new_event)->window = window;
        cdk_event_free (new_event);
    }

    return handled;
}

static gboolean
popup_menu (CtkWidget *widget)
{
    BaulIconContainer *container;

    container = BAUL_ICON_CONTAINER (widget);

    if (has_selection (container))
    {
        handle_popups (container, NULL,
                       "context_click_selection");
    }
    else
    {
        handle_popups (container, NULL,
                       "context_click_background");
    }

    return TRUE;
}

static void
draw_canvas_background (EelCanvas *canvas G_GNUC_UNUSED,
			cairo_t   *cr G_GNUC_UNUSED)
{
    /* Don't chain up to the parent to avoid clearing and redrawing */
}

static void
grab_notify_cb  (CtkWidget        *widget,
                 gboolean          was_grabbed)
{
    BaulIconContainer *container;

    container = BAUL_ICON_CONTAINER (widget);

    if (container->details->rubberband_info.active &&
            !was_grabbed)
    {
        /* we got a (un)grab-notify during rubberband.
         * This happens when a new modal dialog shows
         * up (e.g. authentication or an error). Stop
         * the rubberbanding so that we can handle the
         * dialog. */
        stop_rubberbanding (container);
    }
}

static void
text_ellipsis_limit_changed_container_callback (gpointer callback_data)
{
    BaulIconContainer *container;

    container = BAUL_ICON_CONTAINER (callback_data);
    invalidate_label_sizes (container);
    schedule_redo_layout (container);
}

static GObject*
baul_icon_container_constructor (GType                  type,
                                 guint                  n_construct_params,
                                 GObjectConstructParam *construct_params)
{
    BaulIconContainer *container;
    GObject *object;

    object = G_OBJECT_CLASS (baul_icon_container_parent_class)->constructor
             (type,
              n_construct_params,
              construct_params);

    container = BAUL_ICON_CONTAINER (object);
    if (baul_icon_container_get_is_desktop (container))
    {
        g_signal_connect_swapped (baul_desktop_preferences,
                                  "changed::" BAUL_PREFERENCES_DESKTOP_TEXT_ELLIPSIS_LIMIT,
                                  G_CALLBACK (text_ellipsis_limit_changed_container_callback),
                                  container);
    }
    else
    {
        g_signal_connect_swapped (baul_icon_view_preferences,
                                  "changed::" BAUL_PREFERENCES_ICON_VIEW_TEXT_ELLIPSIS_LIMIT,
                                  G_CALLBACK (text_ellipsis_limit_changed_container_callback),
                                  container);
    }

    return object;
}

/* Initialization.  */

static void
baul_icon_container_class_init (BaulIconContainerClass *class)
{
    CtkWidgetClass *widget_class;
    EelCanvasClass *canvas_class;
    CtkBindingSet *binding_set;

    G_OBJECT_CLASS (class)->constructor = baul_icon_container_constructor;
    G_OBJECT_CLASS (class)->finalize = finalize;

    CTK_WIDGET_CLASS (class)->destroy = destroy;

    /* Signals.  */

    signals[SELECTION_CHANGED]
        = g_signal_new ("selection_changed",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         selection_changed),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);
    signals[BUTTON_PRESS]
        = g_signal_new ("button_press",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         button_press),
                        NULL, NULL,
                        baul_marshal_BOOLEAN__POINTER,
                        G_TYPE_BOOLEAN, 1,
                        CDK_TYPE_EVENT);
    signals[ACTIVATE]
        = g_signal_new ("activate",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         activate),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__POINTER,
                        G_TYPE_NONE, 1,
                        G_TYPE_POINTER);
    signals[ACTIVATE_ALTERNATE]
        = g_signal_new ("activate_alternate",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         activate_alternate),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__POINTER,
                        G_TYPE_NONE, 1,
                        G_TYPE_POINTER);
    signals[CONTEXT_CLICK_SELECTION]
        = g_signal_new ("context_click_selection",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         context_click_selection),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__POINTER,
                        G_TYPE_NONE, 1,
                        G_TYPE_POINTER);
    signals[CONTEXT_CLICK_BACKGROUND]
        = g_signal_new ("context_click_background",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         context_click_background),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__POINTER,
                        G_TYPE_NONE, 1,
                        G_TYPE_POINTER);
    signals[MIDDLE_CLICK]
        = g_signal_new ("middle_click",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         middle_click),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__POINTER,
                        G_TYPE_NONE, 1,
                        G_TYPE_POINTER);
    signals[ICON_POSITION_CHANGED]
        = g_signal_new ("icon_position_changed",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         icon_position_changed),
                        NULL, NULL,
                        baul_marshal_VOID__POINTER_POINTER,
                        G_TYPE_NONE, 2,
                        G_TYPE_POINTER,
                        G_TYPE_POINTER);
    signals[ICON_TEXT_CHANGED]
        = g_signal_new ("icon_text_changed",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         icon_text_changed),
                        NULL, NULL,
                        baul_marshal_VOID__POINTER_STRING,
                        G_TYPE_NONE, 2,
                        G_TYPE_POINTER,
                        G_TYPE_STRING);
    signals[ICON_STRETCH_STARTED]
        = g_signal_new ("icon_stretch_started",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         icon_stretch_started),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__POINTER,
                        G_TYPE_NONE, 1,
                        G_TYPE_POINTER);
    signals[ICON_STRETCH_ENDED]
        = g_signal_new ("icon_stretch_ended",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         icon_stretch_ended),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__POINTER,
                        G_TYPE_NONE, 1,
                        G_TYPE_POINTER);
    signals[RENAMING_ICON]
        = g_signal_new ("renaming_icon",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         renaming_icon),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__POINTER,
                        G_TYPE_NONE, 1,
                        G_TYPE_POINTER);
    signals[GET_ICON_URI]
        = g_signal_new ("get_icon_uri",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         get_icon_uri),
                        NULL, NULL,
		                baul_marshal_STRING__POINTER,
                        G_TYPE_STRING, 1,
                        G_TYPE_POINTER);
    signals[GET_ICON_DROP_TARGET_URI]
        = g_signal_new ("get_icon_drop_target_uri",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         get_icon_drop_target_uri),
                        NULL, NULL,
		                baul_marshal_STRING__POINTER,
                        G_TYPE_STRING, 1,
                        G_TYPE_POINTER);
    signals[MOVE_COPY_ITEMS]
        = g_signal_new ("move_copy_items",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         move_copy_items),
                        NULL, NULL,
                        baul_marshal_VOID__POINTER_POINTER_POINTER_ENUM_INT_INT,
                        G_TYPE_NONE, 6,
                        G_TYPE_POINTER,
                        G_TYPE_POINTER,
                        G_TYPE_POINTER,
                        CDK_TYPE_DRAG_ACTION,
                        G_TYPE_INT,
                        G_TYPE_INT);
    signals[HANDLE_NETSCAPE_URL]
        = g_signal_new ("handle_netscape_url",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         handle_netscape_url),
                        NULL, NULL,
                        baul_marshal_VOID__STRING_STRING_ENUM_INT_INT,
                        G_TYPE_NONE, 5,
                        G_TYPE_STRING,
                        G_TYPE_STRING,
                        CDK_TYPE_DRAG_ACTION,
                        G_TYPE_INT,
                        G_TYPE_INT);
    signals[HANDLE_URI_LIST]
        = g_signal_new ("handle_uri_list",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         handle_uri_list),
                        NULL, NULL,
                        baul_marshal_VOID__STRING_STRING_ENUM_INT_INT,
                        G_TYPE_NONE, 5,
                        G_TYPE_STRING,
                        G_TYPE_STRING,
                        CDK_TYPE_DRAG_ACTION,
                        G_TYPE_INT,
                        G_TYPE_INT);
    signals[HANDLE_TEXT]
        = g_signal_new ("handle_text",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         handle_text),
                        NULL, NULL,
                        baul_marshal_VOID__STRING_STRING_ENUM_INT_INT,
                        G_TYPE_NONE, 5,
                        G_TYPE_STRING,
                        G_TYPE_STRING,
                        CDK_TYPE_DRAG_ACTION,
                        G_TYPE_INT,
                        G_TYPE_INT);
    signals[HANDLE_RAW]
        = g_signal_new ("handle_raw",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         handle_raw),
                        NULL, NULL,
                        baul_marshal_VOID__POINTER_INT_STRING_STRING_ENUM_INT_INT,
                        G_TYPE_NONE, 7,
                        G_TYPE_POINTER,
                        G_TYPE_INT,
                        G_TYPE_STRING,
                        G_TYPE_STRING,
                        CDK_TYPE_DRAG_ACTION,
                        G_TYPE_INT,
                        G_TYPE_INT);
    signals[GET_CONTAINER_URI]
        = g_signal_new ("get_container_uri",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         get_container_uri),
                        NULL, NULL,
		                baul_marshal_STRING__VOID,
                        G_TYPE_STRING, 0);
    signals[CAN_ACCEPT_ITEM]
        = g_signal_new ("can_accept_item",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         can_accept_item),
                        NULL, NULL,
		                baul_marshal_INT__POINTER_STRING,
                        G_TYPE_INT, 2,
                        G_TYPE_POINTER,
                        G_TYPE_STRING);
    signals[GET_STORED_ICON_POSITION]
        = g_signal_new ("get_stored_icon_position",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         get_stored_icon_position),
                        NULL, NULL,
		                baul_marshal_BOOLEAN__POINTER_POINTER,
                        G_TYPE_BOOLEAN, 2,
                        G_TYPE_POINTER,
                        G_TYPE_POINTER);
    signals[GET_STORED_LAYOUT_TIMESTAMP]
        = g_signal_new ("get_stored_layout_timestamp",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         get_stored_layout_timestamp),
                        NULL, NULL,
		                baul_marshal_BOOLEAN__POINTER_POINTER,
                        G_TYPE_BOOLEAN, 2,
                        G_TYPE_POINTER,
                        G_TYPE_POINTER);
    signals[STORE_LAYOUT_TIMESTAMP]
        = g_signal_new ("store_layout_timestamp",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         store_layout_timestamp),
                        NULL, NULL,
		                baul_marshal_BOOLEAN__POINTER_POINTER,
                        G_TYPE_BOOLEAN, 2,
                        G_TYPE_POINTER,
                        G_TYPE_POINTER);
    signals[LAYOUT_CHANGED]
        = g_signal_new ("layout_changed",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         layout_changed),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);
    signals[PREVIEW]
        = g_signal_new ("preview",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         preview),
                        NULL, NULL,
                        baul_marshal_INT__POINTER_BOOLEAN,
                        G_TYPE_INT, 2,
                        G_TYPE_POINTER,
                        G_TYPE_BOOLEAN);
    signals[BAND_SELECT_STARTED]
        = g_signal_new ("band_select_started",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         band_select_started),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);
    signals[BAND_SELECT_ENDED]
        = g_signal_new ("band_select_ended",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         band_select_ended),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);
    signals[ICON_ADDED]
        = g_signal_new ("icon_added",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         icon_added),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__POINTER,
                        G_TYPE_NONE, 1, G_TYPE_POINTER);
    signals[ICON_REMOVED]
        = g_signal_new ("icon_removed",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         icon_removed),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__POINTER,
                        G_TYPE_NONE, 1, G_TYPE_POINTER);

    signals[CLEARED]
        = g_signal_new ("cleared",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         cleared),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);

    signals[START_INTERACTIVE_SEARCH]
        = g_signal_new ("start_interactive_search",
                        G_TYPE_FROM_CLASS (class),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                        G_STRUCT_OFFSET (BaulIconContainerClass,
                                         start_interactive_search),
                        NULL, NULL,
                        baul_marshal_BOOLEAN__VOID,
                        G_TYPE_BOOLEAN, 0);

    /* CtkWidget class.  */

    widget_class = CTK_WIDGET_CLASS (class);
    widget_class->size_allocate = size_allocate;
    widget_class->get_request_mode = get_request_mode;
    widget_class->get_preferred_width = get_prefered_width;
    widget_class->get_preferred_height = get_prefered_height;
    widget_class->draw = draw;
    widget_class->realize = realize;
    widget_class->unrealize = unrealize;
    widget_class->button_press_event = button_press_event;
    widget_class->button_release_event = button_release_event;
    widget_class->motion_notify_event = motion_notify_event;
    widget_class->key_press_event = key_press_event;
    widget_class->popup_menu = popup_menu;
    widget_class->style_updated = style_updated;
    widget_class->grab_notify = grab_notify_cb;

    ctk_widget_class_set_accessible_type (widget_class, baul_icon_container_accessible_get_type ());

    canvas_class = EEL_CANVAS_CLASS (class);
    canvas_class->draw_background = draw_canvas_background;
    class->start_interactive_search = baul_icon_container_start_interactive_search;

    ctk_widget_class_install_style_property (widget_class,
            g_param_spec_boxed ("selection_box_rgba",
                                "Selection Box RGBA",
                                "Color of the selection box",
                                CDK_TYPE_RGBA,
                                G_PARAM_READABLE));
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
    ctk_widget_class_install_style_property (widget_class,
            g_param_spec_boolean ("activate_prelight_icon_label",
                                  "Activate Prelight Icon Label",
                                  "Whether icon labels should make use of its prelight color in prelight state",
                                  FALSE,
                                  G_PARAM_READABLE));


    binding_set = ctk_binding_set_by_class (class);

    ctk_binding_entry_add_signal (binding_set, CDK_KEY_f, CDK_CONTROL_MASK, "start_interactive_search", 0);
    ctk_binding_entry_add_signal (binding_set, CDK_KEY_F, CDK_CONTROL_MASK, "start_interactive_search", 0);
}

static void
update_selected (BaulIconContainer *container)
{
    GList *node;
    BaulIcon *icon = NULL;

    for (node = container->details->icons; node != NULL; node = node->next)
    {
        icon = node->data;
        if (icon->is_selected)
        {
            eel_canvas_item_request_update (EEL_CANVAS_ITEM (icon->item));
        }
    }
}

static gboolean
handle_focus_in_event (CtkWidget     *widget,
		       CdkEventFocus *event G_GNUC_UNUSED,
		       gpointer       user_data G_GNUC_UNUSED)
{
    update_selected (BAUL_ICON_CONTAINER (widget));

    return FALSE;
}

static gboolean
handle_focus_out_event (CtkWidget     *widget,
			CdkEventFocus *event G_GNUC_UNUSED,
			gpointer       user_data G_GNUC_UNUSED)
{
    /* End renaming and commit change. */
    end_renaming_mode (BAUL_ICON_CONTAINER (widget), TRUE);
    update_selected (BAUL_ICON_CONTAINER (widget));

    return FALSE;
}

static void
handle_scale_factor_changed (GObject    *object,
			     GParamSpec *pspec G_GNUC_UNUSED,
			     gpointer    user_data G_GNUC_UNUSED)
{
    invalidate_labels (BAUL_ICON_CONTAINER (object));
    baul_icon_container_request_update_all (BAUL_ICON_CONTAINER (object));
}


static int text_ellipsis_limits[BAUL_ZOOM_LEVEL_N_ENTRIES];
static int desktop_text_ellipsis_limit;

static gboolean
get_text_ellipsis_limit_for_zoom (char **strs,
                                  const char *zoom_level,
                                  int *limit)
{
    char *str;
    gboolean success;

    success = FALSE;

    /* default */
    *limit = 3;

    if (zoom_level != NULL)
    {
        str = g_strdup_printf ("%s:%%d", zoom_level);
    }
    else
    {
        str = g_strdup ("%d");
    }

    if (strs != NULL)
    {
        char **p;

        for (p = strs; *p != NULL; p++)
        {
            if (sscanf (*p, str, limit))
            {
                success = TRUE;
            }
        }
    }

    g_free (str);

    return success;
}

static const char * zoom_level_names[] = {
    "smallest",
    "smaller",
    "small",
    "standard",
    "large",
    "larger",
    "largest"
};

static void
text_ellipsis_limit_changed_callback (gpointer callback_data G_GNUC_UNUSED)
{
    char **pref;
    unsigned int i;
    int one_limit;

    pref = g_settings_get_strv (baul_icon_view_preferences, BAUL_PREFERENCES_ICON_VIEW_TEXT_ELLIPSIS_LIMIT);

    /* set default */
    get_text_ellipsis_limit_for_zoom (pref, NULL, &one_limit);
    for (i = 0; i < BAUL_ZOOM_LEVEL_N_ENTRIES; i++)
    {
        text_ellipsis_limits[i] = one_limit;
    }

    /* override for each zoom level */
    for (i = 0; i < G_N_ELEMENTS(zoom_level_names); i++) {
        if (get_text_ellipsis_limit_for_zoom (pref,
                              zoom_level_names[i],
                              &one_limit)) {
            text_ellipsis_limits[i] = one_limit;
        }
    }

    g_strfreev (pref);
}

static void
desktop_text_ellipsis_limit_changed_callback (gpointer callback_data G_GNUC_UNUSED)
{
    int pref;

    pref = g_settings_get_int (baul_desktop_preferences, BAUL_PREFERENCES_DESKTOP_TEXT_ELLIPSIS_LIMIT);
    desktop_text_ellipsis_limit = pref;
}

static void
baul_icon_container_init (BaulIconContainer *container)
{
    BaulIconContainerDetails *details;
    EelBackground *background;
    static gboolean setup_prefs = FALSE;

    details = g_new0 (BaulIconContainerDetails, 1);

    details->icon_set = g_hash_table_new (g_direct_hash, g_direct_equal);
    details->layout_timestamp = UNDEFINED_TIME;

    details->zoom_level = BAUL_ZOOM_LEVEL_STANDARD;

    details->font_size_table[BAUL_ZOOM_LEVEL_SMALLEST] = -2 * PANGO_SCALE;
    details->font_size_table[BAUL_ZOOM_LEVEL_SMALLER] = -2 * PANGO_SCALE;
    details->font_size_table[BAUL_ZOOM_LEVEL_SMALL] = -0 * PANGO_SCALE;
    details->font_size_table[BAUL_ZOOM_LEVEL_STANDARD] = 0 * PANGO_SCALE;
    details->font_size_table[BAUL_ZOOM_LEVEL_LARGE] = 0 * PANGO_SCALE;
    details->font_size_table[BAUL_ZOOM_LEVEL_LARGER] = 0 * PANGO_SCALE;
    details->font_size_table[BAUL_ZOOM_LEVEL_LARGEST] = 0 * PANGO_SCALE;

    container->details = details;

    /* when the background changes, we must set up the label text color */
    background = eel_get_widget_background (CTK_WIDGET (container));

    g_signal_connect (container, "focus-in-event",
                      G_CALLBACK (handle_focus_in_event), NULL);
    g_signal_connect (container, "focus-out-event",
                      G_CALLBACK (handle_focus_out_event), NULL);

    g_signal_connect (container, "notify::scale-factor",
                      G_CALLBACK (handle_scale_factor_changed), NULL);

    eel_background_set_use_base (background, TRUE);

    if (!setup_prefs)
    {
        g_signal_connect_swapped (baul_icon_view_preferences,
                                  "changed::" BAUL_PREFERENCES_ICON_VIEW_TEXT_ELLIPSIS_LIMIT,
                                  G_CALLBACK (text_ellipsis_limit_changed_callback),
                                  NULL);
        text_ellipsis_limit_changed_callback (NULL);

        g_signal_connect_swapped (baul_icon_view_preferences,
                                  "changed::" BAUL_PREFERENCES_DESKTOP_TEXT_ELLIPSIS_LIMIT,
                                  G_CALLBACK (desktop_text_ellipsis_limit_changed_callback),
                                  NULL);
        desktop_text_ellipsis_limit_changed_callback (NULL);

        setup_prefs = TRUE;
    }
}

typedef struct
{
    BaulIconContainer *container;
    CdkEventButton	      *event;
} ContextMenuParameters;

static gboolean
handle_icon_double_click (BaulIconContainer *container,
                          BaulIcon *icon,
                          CdkEventButton *event)
{
    BaulIconContainerDetails *details;

    if (event->button != DRAG_BUTTON)
    {
        return FALSE;
    }

    details = container->details;

    if (!details->single_click_mode &&
            clicked_within_double_click_interval (container) &&
            details->double_click_icon[0] == details->double_click_icon[1] &&
            details->double_click_button[0] == details->double_click_button[1])
    {
        if (!button_event_modifies_selection (event))
        {
            activate_selected_items (container);
            return TRUE;
        }
        else if ((event->state & CDK_CONTROL_MASK) == 0 &&
                 (event->state & CDK_SHIFT_MASK) != 0)
        {
            activate_selected_items_alternate (container, icon);
            return TRUE;
        }
    }

    return FALSE;
}

/* BaulIcon event handling.  */

/* Conceptually, pressing button 1 together with CTRL or SHIFT toggles
 * selection of a single icon without affecting the other icons;
 * without CTRL or SHIFT, it selects a single icon and un-selects all
 * the other icons.  But in this latter case, the de-selection should
 * only happen when the button is released if the icon is already
 * selected, because the user might select multiple icons and drag all
 * of them by doing a simple click-drag.
*/

static gboolean
handle_icon_button_press (BaulIconContainer *container,
                          BaulIcon *icon,
                          CdkEventButton *event)
{
    BaulIconContainerDetails *details;

    details = container->details;

    if (event->type == CDK_2BUTTON_PRESS || event->type == CDK_3BUTTON_PRESS)
    {
        return TRUE;
    }

    if (event->button != DRAG_BUTTON
            && event->button != CONTEXTUAL_MENU_BUTTON
            && event->button != DRAG_MENU_BUTTON)
    {
        return TRUE;
    }

    if ((event->button == DRAG_BUTTON) &&
            event->type == CDK_BUTTON_PRESS)
    {
        /* The next double click has to be on this icon */
        details->double_click_icon[1] = details->double_click_icon[0];
        details->double_click_icon[0] = icon;

        details->double_click_button[1] = details->double_click_button[0];
        details->double_click_button[0] = event->button;
    }

    if (handle_icon_double_click (container, icon, event))
    {
        /* Double clicking does not trigger a D&D action. */
        details->drag_button = 0;
        details->drag_icon = NULL;
        return TRUE;
    }

    if (event->button == DRAG_BUTTON
            || event->button == DRAG_MENU_BUTTON)
    {
        details->drag_button = event->button;
        details->drag_icon = icon;
        details->drag_x = event->x;
        details->drag_y = event->y;
        details->drag_state = DRAG_STATE_MOVE_OR_COPY;
        details->drag_started = FALSE;

        /* Check to see if this is a click on the stretch handles.
         * If so, it won't modify the selection.
         */
        if (icon == container->details->stretch_icon)
        {
            if (start_stretching (container, (CdkEvent *)event))
            {
                return TRUE;
            }
        }
    }

    /* Modify the selection as appropriate. Selection is modified
     * the same way for contextual menu as it would be without.
     */
    details->icon_selected_on_button_down = icon->is_selected;

    if ((event->button == DRAG_BUTTON || event->button == MIDDLE_BUTTON) &&
            (event->state & CDK_SHIFT_MASK) != 0)
    {
        BaulIcon *start_icon;

        start_icon = details->range_selection_base_icon;
        if (start_icon == NULL || !start_icon->is_selected)
        {
            start_icon = icon;
            details->range_selection_base_icon = icon;
        }
        if (select_range (container, start_icon, icon,
                          (event->state & CDK_CONTROL_MASK) == 0))
        {
            g_signal_emit (container,
                           signals[SELECTION_CHANGED], 0);
        }
    }
    else if (!details->icon_selected_on_button_down)
    {
        details->range_selection_base_icon = icon;
        if (button_event_modifies_selection (event))
        {
            icon_toggle_selected (container, icon);
            g_signal_emit (container,
                           signals[SELECTION_CHANGED], 0);
        }
        else
        {
            select_one_unselect_others (container, icon);
            g_signal_emit (container,
                           signals[SELECTION_CHANGED], 0);
        }
    }

    if (event->button == CONTEXTUAL_MENU_BUTTON)
    {
        g_signal_emit (container,
                       signals[CONTEXT_CLICK_SELECTION], 0,
                       event);
    }


    return TRUE;
}

static int
item_event_callback (EelCanvasItem *item,
                     CdkEvent *event,
                     gpointer data)
{
    BaulIconContainer *container;
    BaulIcon *icon;

    container = BAUL_ICON_CONTAINER (data);

    icon = BAUL_ICON_CANVAS_ITEM (item)->user_data;
    g_assert (icon != NULL);

    switch (event->type)
    {
    case CDK_BUTTON_PRESS:
        if (handle_icon_button_press (container, icon, &event->button))
        {
            /* Stop the event from being passed along further. Returning
             * TRUE ain't enough.
             */
            return TRUE;
        }
        return FALSE;
    default:
        return FALSE;
    }
}

CtkWidget *
baul_icon_container_new (void)
{
    return ctk_widget_new (BAUL_TYPE_ICON_CONTAINER, NULL);
}

/* Clear all of the icons in the container. */
void
baul_icon_container_clear (BaulIconContainer *container)
{
    BaulIconContainerDetails *details;
    GList *p;

    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    details = container->details;
    details->layout_timestamp = UNDEFINED_TIME;
    details->store_layout_timestamps_when_finishing_new_icons = FALSE;

    if (details->icons == NULL)
    {
        return;
    }

    end_renaming_mode (container, TRUE);

    clear_keyboard_focus (container);
    clear_keyboard_rubberband_start (container);
    unschedule_keyboard_icon_reveal (container);
    set_pending_icon_to_reveal (container, NULL);
    details->stretch_icon = NULL;
    details->drop_target = NULL;

    for (p = details->icons; p != NULL; p = p->next)
    {
        icon_free (p->data);
    }
    g_list_free (details->icons);
    details->icons = NULL;
    g_list_free (details->new_icons);
    details->new_icons = NULL;

    g_hash_table_destroy (details->icon_set);
    details->icon_set = g_hash_table_new (g_direct_hash, g_direct_equal);

    baul_icon_container_update_scroll_region (container);
}

gboolean
baul_icon_container_is_empty (BaulIconContainer *container)
{
    return container->details->icons == NULL;
}

BaulIconData *
baul_icon_container_get_first_visible_icon (BaulIconContainer *container)
{
    GList *l;
    BaulIcon *icon, *best_icon;
    double x, y;
    double x1, y1, x2, y2;
    double *pos, best_pos;
    double hadj_v, vadj_v, h_page_size;
    gboolean better_icon;
    gboolean compare_lt;

    hadj_v = ctk_adjustment_get_value (ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (container)));
    vadj_v = ctk_adjustment_get_value (ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (container)));
    h_page_size = ctk_adjustment_get_page_size (ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (container)));

    if (baul_icon_container_is_layout_rtl (container))
    {
        x = hadj_v + h_page_size - ICON_PAD_LEFT - 1;
        y = vadj_v;
    }
    else
    {
        x = hadj_v;
        y = vadj_v;
    }

    eel_canvas_c2w (EEL_CANVAS (container),
                    x, y,
                    &x, &y);

    l = container->details->icons;
    best_icon = NULL;
    best_pos = 0;
    while (l != NULL)
    {
        icon = l->data;

        if (icon_is_positioned (icon))
        {
            eel_canvas_item_get_bounds (EEL_CANVAS_ITEM (icon->item),
                                        &x1, &y1, &x2, &y2);

            compare_lt = FALSE;
            if (baul_icon_container_is_layout_vertical (container))
            {
                pos = &x1;
                if (baul_icon_container_is_layout_rtl (container))
                {
                    compare_lt = TRUE;
                    better_icon = x1 < x + ICON_PAD_LEFT;
                }
                else
                {
                    better_icon = x2 > x + ICON_PAD_LEFT;
                }
            }
            else
            {
                pos = &y1;
                better_icon = y2 > y + ICON_PAD_TOP;
            }
            if (better_icon)
            {
                if (best_icon == NULL)
                {
                    better_icon = TRUE;
                }
                else if (compare_lt)
                {
                    better_icon = best_pos < *pos;
                }
                else
                {
                    better_icon = best_pos > *pos;
                }

                if (better_icon)
                {
                    best_icon = icon;
                    best_pos = *pos;
                }
            }
        }

        l = l->next;
    }

    return best_icon ? best_icon->data : NULL;
}

/* puts the icon at the top of the screen */
void
baul_icon_container_scroll_to_icon (BaulIconContainer  *container,
                                    BaulIconData       *data)
{
    GList *l;
    CtkAdjustment *hadj, *vadj;
    EelIRect bounds;
    CtkAllocation allocation;
    BaulIcon *icon = NULL;

    hadj = ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (container));
    vadj = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (container));
    ctk_widget_get_allocation (CTK_WIDGET (container), &allocation);

    /* We need to force a relayout now if there are updates queued
     * since we need the final positions */
    baul_icon_container_layout_now (container);

    l = container->details->icons;
    while (l != NULL) {
        icon = l->data;

        if (icon->data == data &&
                icon_is_positioned (icon)) {

            if (baul_icon_container_is_auto_layout (container)) {
                /* ensure that we reveal the entire row/column */
                icon_get_row_and_column_bounds (container, icon, &bounds, TRUE);
            } else {
                item_get_canvas_bounds (EEL_CANVAS_ITEM (icon->item), &bounds, TRUE);
            }

            if (baul_icon_container_is_layout_vertical (container)) {
                if (baul_icon_container_is_layout_rtl (container)) {
                    ctk_adjustment_set_value (hadj, bounds.x1 - allocation.width);
                } else {
                    ctk_adjustment_set_value (hadj, bounds.x0);
                }
            } else {
                ctk_adjustment_set_value (vadj, bounds.y0);
            }
        }

        l = l->next;
    }
}

/* Call a function for all the icons. */
typedef struct
{
    BaulIconCallback callback;
    gpointer callback_data;
} CallbackAndData;

static void
call_icon_callback (gpointer data, gpointer callback_data)
{
    BaulIcon *icon;
    CallbackAndData *callback_and_data;

    icon = data;
    callback_and_data = callback_data;
    (* callback_and_data->callback) (icon->data, callback_and_data->callback_data);
}

void
baul_icon_container_for_each (BaulIconContainer *container,
                              BaulIconCallback callback,
                              gpointer callback_data)
{
    CallbackAndData callback_and_data;

    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    callback_and_data.callback = callback;
    callback_and_data.callback_data = callback_data;

    g_list_foreach (container->details->icons,
                    call_icon_callback, &callback_and_data);
}

static int
selection_changed_at_idle_callback (gpointer data)
{
    BaulIconContainer *container;

    container = BAUL_ICON_CONTAINER (data);

    g_signal_emit (container,
                   signals[SELECTION_CHANGED], 0);

    container->details->selection_changed_id = 0;
    return FALSE;
}

/* utility routine to remove a single icon from the container */

static void
icon_destroy (BaulIconContainer *container,
              BaulIcon *icon)
{
    BaulIconContainerDetails *details;
    gboolean was_selected;
    BaulIcon *icon_to_focus;
    GList *item;

    details = container->details;

    item = g_list_find (details->icons, icon);
    item = item->next ? item->next : item->prev;
    icon_to_focus = (item != NULL) ? item->data : NULL;

    details->icons = g_list_remove (details->icons, icon);
    details->new_icons = g_list_remove (details->new_icons, icon);
    g_hash_table_remove (details->icon_set, icon->data);

    was_selected = icon->is_selected;

    if (details->keyboard_focus == icon ||
            details->keyboard_focus == NULL)
    {
        if (icon_to_focus != NULL)
        {
            set_keyboard_focus (container, icon_to_focus);
        }
        else
        {
            clear_keyboard_focus (container);
        }
    }

    if (details->keyboard_rubberband_start == icon)
    {
        clear_keyboard_rubberband_start (container);
    }

    if (details->keyboard_icon_to_reveal == icon)
    {
        unschedule_keyboard_icon_reveal (container);
    }
    if (details->drag_icon == icon)
    {
        clear_drag_state (container);
    }
    if (details->drop_target == icon)
    {
        details->drop_target = NULL;
    }
    if (details->range_selection_base_icon == icon)
    {
        details->range_selection_base_icon = NULL;
    }
    if (details->pending_icon_to_reveal == icon)
    {
        set_pending_icon_to_reveal (container, NULL);
    }
    if (details->stretch_icon == icon)
    {
        details->stretch_icon = NULL;
    }

    icon_free (icon);

    if (was_selected)
    {
        /* Coalesce multiple removals causing multiple selection_changed events */
        details->selection_changed_id = g_idle_add (selection_changed_at_idle_callback, container);
    }
}

/* activate any selected items in the container */
static void
activate_selected_items (BaulIconContainer *container)
{
    GList *selection;

    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    selection = baul_icon_container_get_selection (container);
    if (selection != NULL)
    {
        g_signal_emit (container,
                       signals[ACTIVATE], 0,
                       selection);
    }
    g_list_free (selection);
}

static void
activate_selected_items_alternate (BaulIconContainer *container,
                                   BaulIcon *icon)
{
    GList *selection;

    g_assert (BAUL_IS_ICON_CONTAINER (container));

    if (icon != NULL)
    {
        selection = g_list_prepend (NULL, icon->data);
    }
    else
    {
        selection = baul_icon_container_get_selection (container);
    }
    if (selection != NULL)
    {
        g_signal_emit (container,
                       signals[ACTIVATE_ALTERNATE], 0,
                       selection);
    }
    g_list_free (selection);
}

static BaulIcon *
get_icon_being_renamed (BaulIconContainer *container)
{
    BaulIcon *rename_icon;

    if (!is_renaming (container))
    {
        return NULL;
    }

    g_assert (!has_multiple_selection (container));

    rename_icon = get_first_selected_icon (container);
    g_assert (rename_icon != NULL);

    return rename_icon;
}

static BaulIconInfo *
baul_icon_container_get_icon_images (BaulIconContainer *container,
                                     BaulIconData      *data,
                                     int                    size,
                                     GList                **emblem_pixbufs,
                                     char                 **embedded_text,
                                     gboolean               for_drag_accept,
                                     gboolean               need_large_embeddded_text,
                                     gboolean              *embedded_text_needs_loading,
                                     gboolean              *has_open_window)
{
    BaulIconContainerClass *klass;

    klass = BAUL_ICON_CONTAINER_GET_CLASS (container);
    g_assert (klass->get_icon_images != NULL);

    return klass->get_icon_images (container, data, size, emblem_pixbufs, embedded_text, for_drag_accept, need_large_embeddded_text, embedded_text_needs_loading, has_open_window);
}

static void
baul_icon_container_freeze_updates (BaulIconContainer *container)
{
    BaulIconContainerClass *klass;

    klass = BAUL_ICON_CONTAINER_GET_CLASS (container);
    g_assert (klass->freeze_updates != NULL);

    klass->freeze_updates (container);
}

static void
baul_icon_container_unfreeze_updates (BaulIconContainer *container)
{
    BaulIconContainerClass *klass;

    klass = BAUL_ICON_CONTAINER_GET_CLASS (container);
    g_assert (klass->unfreeze_updates != NULL);

    klass->unfreeze_updates (container);
}

static void
baul_icon_container_prioritize_thumbnailing (BaulIconContainer *container,
        BaulIcon *icon)
{
    BaulIconContainerClass *klass;

    klass = BAUL_ICON_CONTAINER_GET_CLASS (container);
    g_assert (klass->prioritize_thumbnailing != NULL);

    klass->prioritize_thumbnailing (container, icon->data);
}

static void
baul_icon_container_update_visible_icons (BaulIconContainer *container)
{
    CtkAdjustment *vadj, *hadj;
    double min_y, max_y;
    double min_x, max_x;
    double x0, y0, x1, y1;
    GList *node;
    gboolean visible;
    CtkAllocation allocation;
    BaulIcon *icon = NULL;

    hadj = ctk_scrollable_get_hadjustment (CTK_SCROLLABLE (container));
    vadj = ctk_scrollable_get_vadjustment (CTK_SCROLLABLE (container));
    ctk_widget_get_allocation (CTK_WIDGET (container), &allocation);

    min_x = ctk_adjustment_get_value (hadj);
    max_x = min_x + allocation.width;

    min_y = ctk_adjustment_get_value (vadj);
    max_y = min_y + allocation.height;

    eel_canvas_c2w (EEL_CANVAS (container),
                    min_x, min_y, &min_x, &min_y);
    eel_canvas_c2w (EEL_CANVAS (container),
                    max_x, max_y, &max_x, &max_y);

    /* Do the iteration in reverse to get the render-order from top to
     * bottom for the prioritized thumbnails.
     */
    for (node = g_list_last (container->details->icons); node != NULL; node = node->prev)
    {
        icon = node->data;

        if (icon_is_positioned (icon))
        {
            eel_canvas_item_get_bounds (EEL_CANVAS_ITEM (icon->item),
                                        &x0,
                                        &y0,
                                        &x1,
                                        &y1);
            eel_canvas_item_i2w (EEL_CANVAS_ITEM (icon->item)->parent,
                                 &x0,
                                 &y0);
            eel_canvas_item_i2w (EEL_CANVAS_ITEM (icon->item)->parent,
                                 &x1,
                                 &y1);

            if (baul_icon_container_is_layout_vertical (container))
            {
                visible = x1 >= min_x && x0 <= max_x;
            }
            else
            {
                visible = y1 >= min_y && y0 <= max_y;
            }

            if (visible)
            {
                baul_icon_canvas_item_set_is_visible (icon->item, TRUE);
                baul_icon_container_prioritize_thumbnailing (container,
                        icon);
            }
            else
            {
                baul_icon_canvas_item_set_is_visible (icon->item, FALSE);
            }
        }
    }
}

static void
handle_vadjustment_changed (CtkAdjustment     *adjustment G_GNUC_UNUSED,
			    BaulIconContainer *container)
{
    if (!baul_icon_container_is_layout_vertical (container))
    {
        baul_icon_container_update_visible_icons (container);
    }
}

static void
handle_hadjustment_changed (CtkAdjustment     *adjustment G_GNUC_UNUSED,
			    BaulIconContainer *container)
{
    if (baul_icon_container_is_layout_vertical (container))
    {
        baul_icon_container_update_visible_icons (container);
    }
}


void
baul_icon_container_update_icon (BaulIconContainer *container,
                                 BaulIcon *icon)
{
    BaulIconContainerDetails *details;
    guint icon_size;
    guint min_image_size, max_image_size;
    BaulIconInfo *icon_info;
    CdkPoint *attach_points;
    int n_attach_points;
    GdkPixbuf *pixbuf;
    GList *emblem_pixbufs;
    char *editable_text, *additional_text;
    char *embedded_text;
    CdkRectangle embedded_text_rect;
    gboolean large_embedded_text;
    gboolean embedded_text_needs_loading;
    gboolean has_open_window;

    if (icon == NULL)
    {
        return;
    }

    details = container->details;

    /* compute the maximum size based on the scale factor */
    min_image_size = MINIMUM_IMAGE_SIZE * EEL_CANVAS (container)->pixels_per_unit;
    max_image_size = MAX (MAXIMUM_IMAGE_SIZE * EEL_CANVAS (container)->pixels_per_unit, BAUL_ICON_MAXIMUM_SIZE);

    /* Get the appropriate images for the file. */
    if (container->details->forced_icon_size > 0)
    {
        icon_size = container->details->forced_icon_size;
    }
    else
    {
        icon_get_size (container, icon, &icon_size);
    }


    icon_size = MAX (icon_size, min_image_size);
    icon_size = MIN (icon_size, max_image_size);

    /* Get the icons. */
    emblem_pixbufs = NULL;
    embedded_text = NULL;
    large_embedded_text = icon_size > ICON_SIZE_FOR_LARGE_EMBEDDED_TEXT;
    icon_info = baul_icon_container_get_icon_images (container, icon->data, icon_size,
                &emblem_pixbufs,
                &embedded_text,
                icon == details->drop_target,
                large_embedded_text, &embedded_text_needs_loading,
                &has_open_window);


    if (container->details->forced_icon_size > 0)
        pixbuf = baul_icon_info_get_pixbuf_at_size (icon_info, icon_size);
    else
        pixbuf = baul_icon_info_get_pixbuf (icon_info);
    baul_icon_info_get_attach_points (icon_info, &attach_points, &n_attach_points);

    baul_icon_container_get_icon_text (container,
                                       icon->data,
                                       &editable_text,
                                       &additional_text,
                                       FALSE);

    /* If name of icon being renamed was changed from elsewhere, end renaming mode.
     * Alternatively, we could replace the characters in the editable text widget
     * with the new name, but that could cause timing problems if the user just
     * happened to be typing at that moment.
     */
    if (icon == get_icon_being_renamed (container) &&
            g_strcmp0 (editable_text,
                        baul_icon_canvas_item_get_editable_text (icon->item)) != 0)
    {
        end_renaming_mode (container, FALSE);
    }

    eel_canvas_item_set (EEL_CANVAS_ITEM (icon->item),
                         "editable_text", editable_text,
                         "additional_text", additional_text,
                         "highlighted_for_drop", icon == details->drop_target,
                         NULL);

    baul_icon_canvas_item_set_image (icon->item, pixbuf);
    baul_icon_canvas_item_set_attach_points (icon->item, attach_points, n_attach_points);
    baul_icon_canvas_item_set_emblems (icon->item, emblem_pixbufs);
    baul_icon_canvas_item_set_embedded_text_rect (icon->item, &embedded_text_rect);
    baul_icon_canvas_item_set_embedded_text (icon->item, embedded_text);

    /* Let the pixbufs go. */
    g_object_unref (pixbuf);
    g_list_free_full (emblem_pixbufs, g_object_unref);

    g_free (editable_text);
    g_free (additional_text);

    g_object_unref (icon_info);
}

static gboolean
assign_icon_position (BaulIconContainer *container,
                      BaulIcon *icon)
{
    gboolean have_stored_position;
    BaulIconPosition position;

    /* Get the stored position. */
    have_stored_position = FALSE;
    position.scale = 1.0;
    g_signal_emit (container,
                   signals[GET_STORED_ICON_POSITION], 0,
                   icon->data,
                   &position,
                   &have_stored_position);
    icon->scale = position.scale;
    if (!container->details->auto_layout)
    {
        if (have_stored_position)
        {
            icon_set_position (icon, position.x, position.y);
            icon->saved_ltr_x = icon->x;
        }
        else
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void
finish_adding_icon (BaulIconContainer *container,
                    BaulIcon *icon)
{
    baul_icon_container_update_icon (container, icon);
    eel_canvas_item_show (EEL_CANVAS_ITEM (icon->item));

    g_signal_connect_object (icon->item, "event",
                             G_CALLBACK (item_event_callback), container, 0);

    g_signal_emit (container, signals[ICON_ADDED], 0, icon->data);
}

static void
finish_adding_new_icons (BaulIconContainer *container)
{
    GList *p, *new_icons, *no_position_icons, *semi_position_icons;
    BaulIcon *icon;
    double bottom;

    new_icons = container->details->new_icons;
    container->details->new_icons = NULL;
    container->details->is_populating_container =
      g_list_length(new_icons) == g_hash_table_size(container->details->icon_set);

    /* Position most icons (not unpositioned manual-layout icons). */
    new_icons = g_list_reverse (new_icons);
    no_position_icons = semi_position_icons = NULL;
    for (p = new_icons; p != NULL; p = p->next)
    {
        icon = p->data;
        if (icon->has_lazy_position)
        {
            assign_icon_position (container, icon);
            semi_position_icons = g_list_prepend (semi_position_icons, icon);
        }
        else if (!assign_icon_position (container, icon))
        {
            no_position_icons = g_list_prepend (no_position_icons, icon);
        }

        finish_adding_icon (container, icon);
    }
    g_list_free (new_icons);

    if (semi_position_icons != NULL)
    {
        PlacementGrid *grid;
        time_t now;
        gboolean dummy;

        g_assert (!container->details->auto_layout);

        semi_position_icons = g_list_reverse (semi_position_icons);

        /* This is currently only used on the desktop.
         * Thus, we pass FALSE for tight, like lay_down_icons_tblr */
        grid = placement_grid_new (container, FALSE);

        for (p = container->details->icons; p != NULL; p = p->next)
        {
            icon = p->data;

            if (icon_is_positioned (icon) && !icon->has_lazy_position)
            {
                placement_grid_mark_icon (grid, icon);
            }
        }

        now = time (NULL);

        for (p = semi_position_icons; p != NULL; p = p->next)
        {
            BaulIcon *icon;
            BaulIconPosition position;
            int x, y;

            icon = p->data;
            x = icon->x;
            y = icon->y;

            find_empty_location (container, grid,
                                 icon, x, y, &x, &y);

            icon_set_position (icon, x, y);

            position.x = icon->x;
            position.y = icon->y;
            position.scale = icon->scale;
            placement_grid_mark_icon (grid, icon);
            g_signal_emit (container, signals[ICON_POSITION_CHANGED], 0,
                           icon->data, &position);
            g_signal_emit (container, signals[STORE_LAYOUT_TIMESTAMP], 0,
                           icon->data, &now, &dummy);

            /* ensure that next time we run this code, the formerly semi-positioned
             * icons are treated as being positioned. */
            icon->has_lazy_position = FALSE;
        }

        placement_grid_free (grid);

        g_list_free (semi_position_icons);
    }

    /* Position the unpositioned manual layout icons. */
    if (no_position_icons != NULL)
    {
        g_assert (!container->details->auto_layout);

        sort_icons (container, &no_position_icons);
        if (baul_icon_container_get_is_desktop (container))
        {
            lay_down_icons (container, no_position_icons, CONTAINER_PAD_TOP);
        }
        else
        {
            get_all_icon_bounds (container, NULL, NULL, NULL, &bottom, BOUNDS_USAGE_FOR_LAYOUT);
            lay_down_icons (container, no_position_icons, bottom + ICON_PAD_BOTTOM);
        }
        g_list_free (no_position_icons);
    }

    if (container->details->store_layout_timestamps_when_finishing_new_icons)
    {
        store_layout_timestamps_now (container);
        container->details->store_layout_timestamps_when_finishing_new_icons = FALSE;
    }
}

static gboolean
is_old_or_unknown_icon_data (BaulIconContainer *container,
                             BaulIconData *data)
{
    time_t timestamp;
    gboolean success;

    if (container->details->layout_timestamp == UNDEFINED_TIME)
    {
        /* don't know */
        return FALSE;
    }

    g_signal_emit (container,
                   signals[GET_STORED_LAYOUT_TIMESTAMP], 0,
                   data, &timestamp, &success);
    return (!success || timestamp < container->details->layout_timestamp);
}

/**
 * baul_icon_container_add:
 * @container: A BaulIconContainer
 * @data: Icon data.
 *
 * Add icon to represent @data to container.
 * Returns FALSE if there was already such an icon.
 **/
gboolean
baul_icon_container_add (BaulIconContainer *container,
                         BaulIconData *data)
{
    BaulIconContainerDetails *details;
    BaulIcon *icon;
    EelCanvasItem *band, *item;

    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (container), FALSE);
    g_return_val_if_fail (data != NULL, FALSE);

    details = container->details;

    if (g_hash_table_lookup (details->icon_set, data) != NULL)
    {
        return FALSE;
    }

    /* Create the new icon, including the canvas item. */
    icon = g_new0 (BaulIcon, 1);
    icon->data = data;
    icon->x = ICON_UNPOSITIONED_VALUE;
    icon->y = ICON_UNPOSITIONED_VALUE;

    /* Whether the saved icon position should only be used
     * if the previous icon position is free. If the position
     * is occupied, another position near the last one will
     */
    icon->has_lazy_position = is_old_or_unknown_icon_data (container, data);
    icon->scale = 1.0;
    icon->item = BAUL_ICON_CANVAS_ITEM
                 (eel_canvas_item_new (EEL_CANVAS_GROUP (EEL_CANVAS (container)->root),
                                       baul_icon_canvas_item_get_type (),
                                       "visible", FALSE,
                                       NULL));
    icon->item->user_data = icon;

    /* Make sure the icon is under the selection_rectangle */
    item = EEL_CANVAS_ITEM (icon->item);
    band = BAUL_ICON_CONTAINER (item->canvas)->details->rubberband_info.selection_rectangle;
    if (band)
    {
        eel_canvas_item_send_behind (item, band);
    }

    /* Put it on both lists. */
    details->icons = g_list_prepend (details->icons, icon);
    details->new_icons = g_list_prepend (details->new_icons, icon);

    g_hash_table_insert (details->icon_set, data, icon);

    /* Run an idle function to add the icons. */
    schedule_redo_layout (container);

    return TRUE;
}

void
baul_icon_container_layout_now (BaulIconContainer *container)
{
    if (container->details->idle_id != 0)
    {
        unschedule_redo_layout (container);
        redo_layout_internal (container);
    }

    /* Also need to make sure we're properly resized, for instance
     * newly added files may trigger a change in the size allocation and
     * thus toggle scrollbars on */
    ctk_container_check_resize (CTK_CONTAINER (ctk_widget_get_parent (CTK_WIDGET (container))));
}

/**
 * baul_icon_container_remove:
 * @container: A BaulIconContainer.
 * @data: Icon data.
 *
 * Remove the icon with this data.
 **/
gboolean
baul_icon_container_remove (BaulIconContainer *container,
                            BaulIconData *data)
{
    BaulIcon *icon;

    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (container), FALSE);
    g_return_val_if_fail (data != NULL, FALSE);

    end_renaming_mode (container, FALSE);

    icon = g_hash_table_lookup (container->details->icon_set, data);

    if (icon == NULL)
    {
        return FALSE;
    }

    g_signal_emit (container, signals[ICON_REMOVED], 0, icon);

    icon_destroy (container, icon);
    schedule_redo_layout (container);

    return TRUE;
}

/**
 * baul_icon_container_request_update:
 * @container: A BaulIconContainer.
 * @data: Icon data.
 *
 * Update the icon with this data.
 **/
void
baul_icon_container_request_update (BaulIconContainer *container,
                                    BaulIconData *data)
{
    BaulIcon *icon;

    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));
    g_return_if_fail (data != NULL);

    icon = g_hash_table_lookup (container->details->icon_set, data);

    if (icon != NULL)
    {
        baul_icon_container_update_icon (container, icon);
        schedule_redo_layout (container);
    }
}

/* zooming */

BaulZoomLevel
baul_icon_container_get_zoom_level (BaulIconContainer *container)
{
    return container->details->zoom_level;
}

void
baul_icon_container_set_zoom_level (BaulIconContainer *container, int new_level)
{
    BaulIconContainerDetails *details;
    int pinned_level;
    double pixels_per_unit;

    details = container->details;

    end_renaming_mode (container, TRUE);

    pinned_level = new_level;
    if (pinned_level < BAUL_ZOOM_LEVEL_SMALLEST)
    {
        pinned_level = BAUL_ZOOM_LEVEL_SMALLEST;
    }
    else if (pinned_level > BAUL_ZOOM_LEVEL_LARGEST)
    {
        pinned_level = BAUL_ZOOM_LEVEL_LARGEST;
    }

    if (pinned_level == details->zoom_level)
    {
        return;
    }

    details->zoom_level = pinned_level;

    pixels_per_unit = (double) baul_get_icon_size_for_zoom_level (pinned_level)
                      / BAUL_ICON_SIZE_STANDARD;
    eel_canvas_set_pixels_per_unit (EEL_CANVAS (container), pixels_per_unit);

    invalidate_labels (container);
    baul_icon_container_request_update_all (container);
}

/**
 * baul_icon_container_request_update_all:
 * For each icon, synchronizes the displayed information (image, text) with the
 * information from the model.
 *
 * @container: An icon container.
 **/
void
baul_icon_container_request_update_all (BaulIconContainer *container)
{
    GList *node;
    BaulIcon *icon = NULL;

    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    container->details->is_loading = TRUE;
    for (node = container->details->icons; node != NULL; node = node->next)
    {
        icon = node->data;
        baul_icon_container_update_icon (container, icon);
    }

    redo_layout (container);
    container->details->is_loading = FALSE;
}

/**
 * baul_icon_container_reveal:
 * Change scroll position as necessary to reveal the specified item.
 */
void
baul_icon_container_reveal (BaulIconContainer *container, BaulIconData *data)
{
    BaulIcon *icon;

    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));
    g_return_if_fail (data != NULL);

    icon = g_hash_table_lookup (container->details->icon_set, data);

    if (icon != NULL)
    {
        reveal_icon (container, icon);
    }
}

/**
 * baul_icon_container_get_selection:
 * @container: An icon container.
 *
 * Get a list of the icons currently selected in @container.
 *
 * Return value: A GList of the programmer-specified data associated to each
 * selected icon, or NULL if no icon is selected.  The caller is expected to
 * free the list when it is not needed anymore.
 **/
GList *
baul_icon_container_get_selection (BaulIconContainer *container)
{
    GList *list, *p;

    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (container), NULL);

    list = NULL;
    for (p = container->details->icons; p != NULL; p = p->next)
    {
        BaulIcon *icon;

        icon = p->data;
        if (icon->is_selected)
        {
            list = g_list_prepend (list, icon->data);
        }
    }

    return g_list_reverse (list);
}

static GList *
baul_icon_container_get_selected_icons (BaulIconContainer *container)
{
    GList *list, *p;

    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (container), NULL);

    list = NULL;
    for (p = container->details->icons; p != NULL; p = p->next)
    {
        BaulIcon *icon;

        icon = p->data;
        if (icon->is_selected)
        {
            list = g_list_prepend (list, icon);
        }
    }

    return g_list_reverse (list);
}

/**
 * baul_icon_container_invert_selection:
 * @container: An icon container.
 *
 * Inverts the selection in @container.
 *
 **/
void
baul_icon_container_invert_selection (BaulIconContainer *container)
{
    GList *p;

    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    for (p = container->details->icons; p != NULL; p = p->next)
    {
        BaulIcon *icon;

        icon = p->data;
        icon_toggle_selected (container, icon);
    }

    g_signal_emit (container, signals[SELECTION_CHANGED], 0);
}


/* Returns an array of CdkPoints of locations of the icons. */
static GArray *
baul_icon_container_get_icon_locations (BaulIconContainer *container G_GNUC_UNUSED,
					GList             *icons)
{
    GArray *result;
    GList *node;
    int index;

    result = g_array_new (FALSE, TRUE, sizeof (CdkPoint));
    result = g_array_set_size (result, g_list_length (icons));

    for (index = 0, node = icons; node != NULL; index++, node = node->next)
    {
        g_array_index (result, CdkPoint, index).x =
            ((BaulIcon *)node->data)->x;
        g_array_index (result, CdkPoint, index).y =
            ((BaulIcon *)node->data)->y;
    }

    return result;
}

/**
 * baul_icon_container_get_selected_icon_locations:
 * @container: An icon container widget.
 *
 * Returns an array of CdkPoints of locations of the selected icons.
 **/
GArray *
baul_icon_container_get_selected_icon_locations (BaulIconContainer *container)
{
    GArray *result;
    GList *icons;

    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (container), NULL);

    icons = baul_icon_container_get_selected_icons (container);
    result = baul_icon_container_get_icon_locations (container, icons);
    g_list_free (icons);

    return result;
}

/**
 * baul_icon_container_select_all:
 * @container: An icon container widget.
 *
 * Select all the icons in @container at once.
 **/
void
baul_icon_container_select_all (BaulIconContainer *container)
{
    gboolean selection_changed;
    GList *p;
    BaulIcon *icon = NULL;

    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    selection_changed = FALSE;

    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;

        selection_changed |= icon_set_selected (container, icon, TRUE);
    }

    if (selection_changed)
    {
        g_signal_emit (container,
                       signals[SELECTION_CHANGED], 0);
    }
}

/**
 * baul_icon_container_set_selection:
 * @container: An icon container widget.
 * @selection: A list of BaulIconData *.
 *
 * Set the selection to exactly the icons in @container which have
 * programmer data matching one of the items in @selection.
 **/
void
baul_icon_container_set_selection (BaulIconContainer *container,
                                   GList *selection)
{
    gboolean selection_changed;
    GHashTable *hash;
    GList *p;
    BaulIcon *icon = NULL;

    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    selection_changed = FALSE;

    hash = g_hash_table_new (NULL, NULL);
    for (p = selection; p != NULL; p = p->next)
    {
        g_hash_table_insert (hash, p->data, p->data);
    }
    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;

        selection_changed |= icon_set_selected
                             (container, icon,
                              g_hash_table_lookup (hash, icon->data) != NULL);
    }
    g_hash_table_destroy (hash);

    if (selection_changed)
    {
        g_signal_emit (container,
                       signals[SELECTION_CHANGED], 0);
    }
}

/**
 * baul_icon_container_select_list_unselect_others.
 * @container: An icon container widget.
 * @selection: A list of BaulIcon *.
 *
 * Set the selection to exactly the icons in @selection.
 **/
void
baul_icon_container_select_list_unselect_others (BaulIconContainer *container,
        GList *selection)
{
    gboolean selection_changed;
    GHashTable *hash;
    GList *p;
    BaulIcon *icon = NULL;

    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    selection_changed = FALSE;

    hash = g_hash_table_new (NULL, NULL);
    for (p = selection; p != NULL; p = p->next)
    {
        g_hash_table_insert (hash, p->data, p->data);
    }
    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;

        selection_changed |= icon_set_selected
                             (container, icon,
                              g_hash_table_lookup (hash, icon) != NULL);
    }
    g_hash_table_destroy (hash);

    if (selection_changed)
    {
        g_signal_emit (container,
                       signals[SELECTION_CHANGED], 0);
    }
}

/**
 * baul_icon_container_unselect_all:
 * @container: An icon container widget.
 *
 * Deselect all the icons in @container.
 **/
void
baul_icon_container_unselect_all (BaulIconContainer *container)
{
    if (unselect_all (container))
    {
        g_signal_emit (container,
                       signals[SELECTION_CHANGED], 0);
    }
}

/**
 * baul_icon_container_get_icon_by_uri:
 * @container: An icon container widget.
 * @uri: The uri of an icon to find.
 *
 * Locate an icon, given the URI. The URI must match exactly.
 * Later we may have to have some way of figuring out if the
 * URI specifies the same object that does not require an exact match.
 **/
BaulIcon *
baul_icon_container_get_icon_by_uri (BaulIconContainer *container,
                                     const char *uri)
{
    BaulIconContainerDetails *details;
    GList *p;

    /* Eventually, we must avoid searching the entire icon list,
       but it's OK for now.
       A hash table mapping uri to icon is one possibility.
    */

    details = container->details;

    for (p = details->icons; p != NULL; p = p->next)
    {
        BaulIcon *icon;
        char *icon_uri;
        gboolean is_match;

        icon = p->data;

        icon_uri = baul_icon_container_get_icon_uri
                   (container, icon);
        is_match = strcmp (uri, icon_uri) == 0;
        g_free (icon_uri);

        if (is_match)
        {
            return icon;
        }
    }

    return NULL;
}

static BaulIcon *
get_nth_selected_icon (BaulIconContainer *container, int index)
{
    GList *p;
    int selection_count;
    BaulIcon *icon = NULL;

    g_assert (index > 0);

    /* Find the nth selected icon. */
    selection_count = 0;
    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;
        if (icon->is_selected)
        {
            if (++selection_count == index)
            {
                return icon;
            }
        }
    }
    return NULL;
}

static BaulIcon *
get_first_selected_icon (BaulIconContainer *container)
{
    return get_nth_selected_icon (container, 1);
}

static gboolean
has_multiple_selection (BaulIconContainer *container)
{
    return get_nth_selected_icon (container, 2) != NULL;
}

static gboolean
all_selected (BaulIconContainer *container)
{
    GList *p;
    BaulIcon *icon = NULL;

    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;
        if (!icon->is_selected)
        {
            return FALSE;
        }
    }
    return TRUE;
}

static gboolean
has_selection (BaulIconContainer *container)
{
    return get_nth_selected_icon (container, 1) != NULL;
}

/**
 * baul_icon_container_show_stretch_handles:
 * @container: An icon container widget.
 *
 * Makes stretch handles visible on the first selected icon.
 **/
void
baul_icon_container_show_stretch_handles (BaulIconContainer *container)
{
    BaulIconContainerDetails *details;
    BaulIcon *icon;
    guint initial_size;

    icon = get_first_selected_icon (container);
    if (icon == NULL)
    {
        return;
    }

    /* Check if it already has stretch handles. */
    details = container->details;
    if (details->stretch_icon == icon)
    {
        return;
    }

    /* Get rid of the existing stretch handles and put them on the new icon. */
    if (details->stretch_icon != NULL)
    {
        baul_icon_canvas_item_set_show_stretch_handles
        (details->stretch_icon->item, FALSE);
        ungrab_stretch_icon (container);
        emit_stretch_ended (container, details->stretch_icon);
    }
    baul_icon_canvas_item_set_show_stretch_handles (icon->item, TRUE);
    details->stretch_icon = icon;

    icon_get_size (container, icon, &initial_size);

    /* only need to keep size in one dimension, since they are constrained to be the same */
    container->details->stretch_initial_x = icon->x;
    container->details->stretch_initial_y = icon->y;
    container->details->stretch_initial_size = initial_size;

    emit_stretch_started (container, icon);
}

/**
 * baul_icon_container_has_stretch_handles
 * @container: An icon container widget.
 *
 * Returns true if the first selected item has stretch handles.
 **/
gboolean
baul_icon_container_has_stretch_handles (BaulIconContainer *container)
{
    BaulIcon *icon;

    icon = get_first_selected_icon (container);
    if (icon == NULL)
    {
        return FALSE;
    }

    return icon == container->details->stretch_icon;
}

/**
 * baul_icon_container_is_stretched
 * @container: An icon container widget.
 *
 * Returns true if the any selected item is stretched to a size other than 1.0.
 **/
gboolean
baul_icon_container_is_stretched (BaulIconContainer *container)
{
    GList *p;
    BaulIcon *icon = NULL;

    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;
        if (icon->is_selected && icon->scale != 1.0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * baul_icon_container_unstretch
 * @container: An icon container widget.
 *
 * Gets rid of any icon stretching.
 **/
void
baul_icon_container_unstretch (BaulIconContainer *container)
{
    GList *p;
    BaulIcon *icon = NULL;

    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;
        if (icon->is_selected)
        {
            baul_icon_container_move_icon (container, icon,
                                           icon->x, icon->y,
                                           1.0,
                                           FALSE, TRUE, TRUE);
        }
    }
}

static void
compute_stretch (StretchState *start,
                 StretchState *current)
{
    gboolean right, bottom;
    int x_stretch, y_stretch;

    /* FIXME bugzilla.gnome.org 45390: This doesn't correspond to
         * the way the handles are drawn.
     */
    /* Figure out which handle we are dragging. */
    right = start->pointer_x > start->icon_x + (int) start->icon_size / 2;
    bottom = start->pointer_y > start->icon_y + (int) start->icon_size / 2;

    /* Figure out how big we should stretch. */
    x_stretch = start->pointer_x - current->pointer_x;
    y_stretch = start->pointer_y - current->pointer_y;
    if (right)
    {
        x_stretch = - x_stretch;
    }
    if (bottom)
    {
        y_stretch = - y_stretch;
    }
    current->icon_size = MAX ((int) start->icon_size + MIN (x_stretch, y_stretch),
                              (int) BAUL_ICON_SIZE_SMALLEST);

    /* Figure out where the corner of the icon should be. */
    current->icon_x = start->icon_x;
    if (!right)
    {
        current->icon_x += start->icon_size - current->icon_size;
    }
    current->icon_y = start->icon_y;
    if (!bottom)
    {
        current->icon_y += start->icon_size - current->icon_size;
    }
}

char *
baul_icon_container_get_icon_uri (BaulIconContainer *container,
                                  BaulIcon *icon)
{
    char *uri;

    uri = NULL;
    g_signal_emit (container,
                   signals[GET_ICON_URI], 0,
                   icon->data,
                   &uri);
    return uri;
}

char *
baul_icon_container_get_icon_drop_target_uri (BaulIconContainer *container,
        BaulIcon *icon)
{
    char *uri;

    uri = NULL;
    g_signal_emit (container,
                   signals[GET_ICON_DROP_TARGET_URI], 0,
                   icon->data,
                   &uri);
    return uri;
}

/* Call to reset the scroll region only if the container is not empty,
 * to avoid having the flag linger until the next file is added.
 */
static void
reset_scroll_region_if_not_empty (BaulIconContainer *container)
{
    if (!baul_icon_container_is_empty (container))
    {
        baul_icon_container_reset_scroll_region (container);
    }
}

/* Switch from automatic layout to manual or vice versa.
 * If we switch to manual layout, we restore the icon positions from the
 * last manual layout.
 */
void
baul_icon_container_set_auto_layout (BaulIconContainer *container,
                                     gboolean auto_layout)
{
    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));
    g_return_if_fail (auto_layout == FALSE || auto_layout == TRUE);

    if (container->details->auto_layout == auto_layout)
    {
        return;
    }

    reset_scroll_region_if_not_empty (container);
    container->details->auto_layout = auto_layout;

    if (!auto_layout)
    {
        reload_icon_positions (container);
        baul_icon_container_freeze_icon_positions (container);
    }

    redo_layout (container);

    g_signal_emit (container, signals[LAYOUT_CHANGED], 0);
}


/* Toggle the tighter layout boolean. */
void
baul_icon_container_set_tighter_layout (BaulIconContainer *container,
                                        gboolean tighter_layout)
{
    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));
    g_return_if_fail (tighter_layout == FALSE || tighter_layout == TRUE);

    if (container->details->tighter_layout == tighter_layout)
    {
        return;
    }

    container->details->tighter_layout = tighter_layout;

    if (container->details->auto_layout)
    {
        invalidate_label_sizes (container);
        redo_layout (container);

        g_signal_emit (container, signals[LAYOUT_CHANGED], 0);
    }
    else
    {
        /* in manual layout, label sizes still change, even though
         * the icons don't move.
         */
        invalidate_label_sizes (container);
        baul_icon_container_request_update_all (container);
    }
}

gboolean
baul_icon_container_is_keep_aligned (BaulIconContainer *container)
{
    return container->details->keep_aligned;
}

static gboolean
align_icons_callback (gpointer callback_data)
{
    BaulIconContainer *container;

    container = BAUL_ICON_CONTAINER (callback_data);
    align_icons (container);
    container->details->align_idle_id = 0;

    return FALSE;
}

static void
unschedule_align_icons (BaulIconContainer *container)
{
    if (container->details->align_idle_id != 0)
    {
        g_source_remove (container->details->align_idle_id);
        container->details->align_idle_id = 0;
    }
}

static void
schedule_align_icons (BaulIconContainer *container)
{
    if (container->details->align_idle_id == 0
            && container->details->has_been_allocated)
    {
        container->details->align_idle_id = g_idle_add
                                            (align_icons_callback, container);
    }
}

void
baul_icon_container_set_keep_aligned (BaulIconContainer *container,
                                      gboolean keep_aligned)
{
    if (container->details->keep_aligned != keep_aligned)
    {
        container->details->keep_aligned = keep_aligned;

        if (keep_aligned && !container->details->auto_layout)
        {
            schedule_align_icons (container);
        }
        else
        {
            unschedule_align_icons (container);
        }
    }
}

void
baul_icon_container_set_layout_mode (BaulIconContainer *container,
                                     BaulIconLayoutMode mode)
{
    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    container->details->layout_mode = mode;
    invalidate_labels (container);

    redo_layout (container);

    g_signal_emit (container, signals[LAYOUT_CHANGED], 0);
}

void
baul_icon_container_set_label_position (BaulIconContainer *container,
                                        BaulIconLabelPosition position)
{
    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    if (container->details->label_position != position)
    {
        container->details->label_position = position;

        invalidate_labels (container);
        baul_icon_container_request_update_all (container);

        schedule_redo_layout (container);
    }
}

/* Switch from automatic to manual layout, freezing all the icons in their
 * current positions instead of restoring icon positions from the last manual
 * layout as set_auto_layout does.
 */
void
baul_icon_container_freeze_icon_positions (BaulIconContainer *container)
{
    gboolean changed;
    GList *p;
    BaulIcon *icon;
    BaulIconPosition position;

    changed = container->details->auto_layout;
    container->details->auto_layout = FALSE;

    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;

        position.x = icon->saved_ltr_x;
        position.y = icon->y;
        position.scale = icon->scale;
        g_signal_emit (container, signals[ICON_POSITION_CHANGED], 0,
                       icon->data, &position);
    }

    if (changed)
    {
        g_signal_emit (container, signals[LAYOUT_CHANGED], 0);
    }
}

/* Re-sort, switching to automatic layout if it was in manual layout. */
void
baul_icon_container_sort (BaulIconContainer *container)
{
    gboolean changed;

    changed = !container->details->auto_layout;
    container->details->auto_layout = TRUE;

    reset_scroll_region_if_not_empty (container);
    redo_layout (container);

    if (changed)
    {
        g_signal_emit (container, signals[LAYOUT_CHANGED], 0);
    }
}

gboolean
baul_icon_container_is_auto_layout (BaulIconContainer *container)
{
    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (container), FALSE);

    return container->details->auto_layout;
}

gboolean
baul_icon_container_is_tighter_layout (BaulIconContainer *container)
{
    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (container), FALSE);

    return container->details->tighter_layout;
}

static void
pending_icon_to_rename_destroy_callback (BaulIconCanvasItem *item, BaulIconContainer *container)
{
    g_assert (container->details->pending_icon_to_rename != NULL);
    g_assert (container->details->pending_icon_to_rename->item == item);
    container->details->pending_icon_to_rename = NULL;
}

static BaulIcon*
get_pending_icon_to_rename (BaulIconContainer *container)
{
    return container->details->pending_icon_to_rename;
}

static void
set_pending_icon_to_rename (BaulIconContainer *container, BaulIcon *icon)
{
    BaulIcon *old_icon;

    old_icon = container->details->pending_icon_to_rename;

    if (icon == old_icon)
    {
        return;
    }

    if (old_icon != NULL)
    {
        g_signal_handlers_disconnect_by_func
        (old_icon->item,
         G_CALLBACK (pending_icon_to_rename_destroy_callback),
         container);
    }

    if (icon != NULL)
    {
        g_signal_connect (icon->item, "destroy",
                          G_CALLBACK (pending_icon_to_rename_destroy_callback), container);
    }

    container->details->pending_icon_to_rename = icon;
}

static void
process_pending_icon_to_rename (BaulIconContainer *container)
{
    BaulIcon *pending_icon_to_rename;

    pending_icon_to_rename = get_pending_icon_to_rename (container);

    if (pending_icon_to_rename != NULL)
    {
        if (pending_icon_to_rename->is_selected && !has_multiple_selection (container))
        {
            baul_icon_container_start_renaming_selected_item (container, FALSE);
        }
        else
        {
            set_pending_icon_to_rename (container, NULL);
        }
    }
}

static gboolean
is_renaming_pending (BaulIconContainer *container)
{
    return get_pending_icon_to_rename (container) != NULL;
}

static gboolean
is_renaming (BaulIconContainer *container)
{
    return container->details->renaming;
}

/**
 * baul_icon_container_start_renaming_selected_item
 * @container: An icon container widget.
 * @select_all: Whether the whole file should initially be selected, or
 *              only its basename (i.e. everything except its extension).
 *
 * Displays the edit name widget on the first selected icon
 **/
void
baul_icon_container_start_renaming_selected_item (BaulIconContainer *container,
        gboolean select_all)
{
    BaulIconContainerDetails *details;
    BaulIcon *icon;
    EelDRect icon_rect;
    EelDRect text_rect;
    PangoFontDescription *desc;
    const char *editable_text;
    int x, y, width;
    int start_offset, end_offset;

    /* Check if it already in renaming mode, if so - select all */
    details = container->details;
    if (details->renaming)
    {
        eel_editable_label_select_region (EEL_EDITABLE_LABEL (details->rename_widget),
                                          0,
                                          -1);
        return;
    }

    /* Find selected icon */
    icon = get_first_selected_icon (container);
    if (icon == NULL)
    {
        return;
    }

    g_assert (!has_multiple_selection (container));


    if (!icon_is_positioned (icon))
    {
        set_pending_icon_to_rename (container, icon);
        return;
    }

    set_pending_icon_to_rename (container, NULL);

    /* Make a copy of the original editable text for a later compare */
    editable_text = baul_icon_canvas_item_get_editable_text (icon->item);

    /* This could conceivably be NULL if a rename was triggered really early. */
    if (editable_text == NULL)
    {
        return;
    }

    details->original_text = g_strdup (editable_text);

    /* Freeze updates so files added while renaming don't cause rename to loose focus, bug #318373 */
    baul_icon_container_freeze_updates (container);

    /* Create text renaming widget, if it hasn't been created already.
     * We deal with the broken icon text item widget by keeping it around
     * so its contents can still be cut and pasted as part of the clipboard
     */
    if (details->rename_widget == NULL)
    {
        details->rename_widget = eel_editable_label_new ("Test text");
        eel_editable_label_set_line_wrap (EEL_EDITABLE_LABEL (details->rename_widget), TRUE);
        eel_editable_label_set_line_wrap_mode (EEL_EDITABLE_LABEL (details->rename_widget), PANGO_WRAP_WORD_CHAR);
        eel_editable_label_set_draw_outline (EEL_EDITABLE_LABEL (details->rename_widget), TRUE);

        if (details->label_position != BAUL_ICON_LABEL_POSITION_BESIDE)
        {
            eel_editable_label_set_justify (EEL_EDITABLE_LABEL (details->rename_widget), CTK_JUSTIFY_CENTER);
        }

        ctk_widget_set_margin_start (details->rename_widget, 1);
        ctk_widget_set_margin_end (details->rename_widget, 1);
        ctk_widget_set_margin_top (details->rename_widget, 1);
        ctk_widget_set_margin_bottom (details->rename_widget, 1);
        ctk_layout_put (CTK_LAYOUT (container),
                        details->rename_widget, 0, 0);
    }

    /* Set the right font */
    if (details->font)
    {
        desc = pango_font_description_from_string (details->font);
    }
    else
    {
        PangoContext *context;

        context = ctk_widget_get_pango_context (CTK_WIDGET (container));
        desc = pango_font_description_copy (pango_context_get_font_description (context));
        pango_font_description_set_size (desc,
                                         pango_font_description_get_size (desc) +
                                         container->details->font_size_table [container->details->zoom_level]);
    }
    eel_editable_label_set_font_description (EEL_EDITABLE_LABEL (details->rename_widget),
            desc);
    pango_font_description_free (desc);

    icon_rect = baul_icon_canvas_item_get_icon_rectangle (icon->item);
    text_rect = baul_icon_canvas_item_get_text_rectangle (icon->item, TRUE);

    if (baul_icon_container_is_layout_vertical (container) &&
            container->details->label_position == BAUL_ICON_LABEL_POSITION_BESIDE)
    {
        /* for one-line editables, the width changes dynamically */
        width = -1;
    }
    else
    {
        width = baul_icon_canvas_item_get_max_text_width (icon->item);
    }

    if (details->label_position == BAUL_ICON_LABEL_POSITION_BESIDE)
    {
        eel_canvas_w2c (EEL_CANVAS_ITEM (icon->item)->canvas,
                        text_rect.x0,
                        text_rect.y0,
                        &x, &y);
    }
    else
    {
        eel_canvas_w2c (EEL_CANVAS_ITEM (icon->item)->canvas,
                        (icon_rect.x0 + icon_rect.x1) / 2,
                        icon_rect.y1,
                        &x, &y);
        x = x - width / 2 - 1;
    }

    ctk_layout_move (CTK_LAYOUT (container),
                     details->rename_widget,
                     x, y);

    ctk_widget_set_size_request (details->rename_widget,
                                 width, -1);
    eel_editable_label_set_text (EEL_EDITABLE_LABEL (details->rename_widget),
                                 editable_text);
    if (select_all)
    {
        start_offset = 0;
        end_offset = -1;
    }
    else
    {
        eel_filename_get_rename_region (editable_text, &start_offset, &end_offset);
    }
    ctk_widget_show (details->rename_widget);
    ctk_widget_grab_focus (details->rename_widget);

    eel_editable_label_select_region (EEL_EDITABLE_LABEL (details->rename_widget),
                                      start_offset,
                                      end_offset);
    g_signal_emit (container,
                   signals[RENAMING_ICON], 0,
                   CTK_EDITABLE (details->rename_widget));

    baul_icon_container_update_icon (container, icon);

    /* We are in renaming mode */
    details->renaming = TRUE;
    baul_icon_canvas_item_set_renaming (icon->item, TRUE);
}

static void
end_renaming_mode (BaulIconContainer *container, gboolean commit)
{
    BaulIcon *icon;

    set_pending_icon_to_rename (container, NULL);

    icon = get_icon_being_renamed (container);
    if (icon == NULL)
    {
        return;
    }

    /* We are not in renaming mode */
    container->details->renaming = FALSE;
    baul_icon_canvas_item_set_renaming (icon->item, FALSE);

    baul_icon_container_unfreeze_updates (container);

    if (commit)
    {
        set_pending_icon_to_reveal (container, icon);
    }

    ctk_widget_grab_focus (CTK_WIDGET (container));

    if (commit)
    {
        const char *changed_text;

        /* Verify that text has been modified before signalling change. */
        changed_text = eel_editable_label_get_text (EEL_EDITABLE_LABEL (container->details->rename_widget));
        if (strcmp (container->details->original_text, changed_text) != 0)
        {
            AtkObject *accessible_icon;

            g_signal_emit (container,
                           signals[ICON_TEXT_CHANGED], 0,
                           icon->data,
                           changed_text);

	    accessible_icon = atk_gobject_accessible_for_object (G_OBJECT(icon->item));
	    g_object_notify (G_OBJECT(accessible_icon), "accessible-name");
        }
    }

    ctk_widget_hide (container->details->rename_widget);

    g_free (container->details->original_text);

}

/* emit preview signal, called by the canvas item */
gboolean
baul_icon_container_emit_preview_signal (BaulIconContainer *icon_container,
        BaulIcon *icon,
        gboolean start_flag)
{
    gboolean result;

    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (icon_container), FALSE);
    g_return_val_if_fail (icon != NULL, FALSE);
    g_return_val_if_fail (start_flag == FALSE || start_flag == TRUE, FALSE);

    result = FALSE;
    g_signal_emit (icon_container,
                   signals[PREVIEW], 0,
                   icon->data,
                   start_flag,
                   &result);

    return result;
}

gboolean
baul_icon_container_has_stored_icon_positions (BaulIconContainer *container)
{
    GList *p;
    gboolean have_stored_position;
    BaulIconPosition position;
    BaulIcon *icon = NULL;

    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;

        have_stored_position = FALSE;
        g_signal_emit (container,
                       signals[GET_STORED_ICON_POSITION], 0,
                       icon->data,
                       &position,
                       &have_stored_position);
        if (have_stored_position)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void
baul_icon_container_set_single_click_mode (BaulIconContainer *container,
        gboolean single_click_mode)
{
    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    container->details->single_click_mode = single_click_mode;
}

/* Return if the icon container is a fixed size */
gboolean
baul_icon_container_get_is_fixed_size (BaulIconContainer *container)
{
    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (container), FALSE);

    return container->details->is_fixed_size;
}

/* Set the icon container to be a fixed size */
void
baul_icon_container_set_is_fixed_size (BaulIconContainer *container,
                                       gboolean is_fixed_size)
{
    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    container->details->is_fixed_size = is_fixed_size;
}

gboolean
baul_icon_container_get_is_desktop (BaulIconContainer *container)
{
    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (container), FALSE);

    return container->details->is_desktop;
}

void
baul_icon_container_set_is_desktop (BaulIconContainer *container,
                                    gboolean is_desktop)
{
    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    container->details->is_desktop = is_desktop;

    if (is_desktop) {
            CtkStyleContext *context;

            context = ctk_widget_get_style_context (CTK_WIDGET (container));
            ctk_style_context_add_class (context, "baul-desktop");
    }
}

void
baul_icon_container_set_margins (BaulIconContainer *container,
                                 int left_margin,
                                 int right_margin,
                                 int top_margin,
                                 int bottom_margin)
{
    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    container->details->left_margin = left_margin;
    container->details->right_margin = right_margin;
    container->details->top_margin = top_margin;
    container->details->bottom_margin = bottom_margin;

    /* redo layout of icons as the margins have changed */
    schedule_redo_layout (container);
}

void
baul_icon_container_set_use_drop_shadows (BaulIconContainer  *container,
        gboolean                use_drop_shadows)
{
    if (container->details->drop_shadows_requested == use_drop_shadows)
    {
        return;
    }

    container->details->drop_shadows_requested = use_drop_shadows;
    container->details->use_drop_shadows = use_drop_shadows;

    ctk_widget_queue_draw (CTK_WIDGET (container));
}

/* handle theme changes */

void
baul_icon_container_set_font (BaulIconContainer *container,
                              const char *font)
{
    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    if (g_strcmp0 (container->details->font, font) == 0)
    {
        return;
    }

    g_free (container->details->font);
    container->details->font = g_strdup (font);

    invalidate_labels (container);
    baul_icon_container_request_update_all (container);
    ctk_widget_queue_draw (CTK_WIDGET (container));
}

void
baul_icon_container_set_font_size_table (BaulIconContainer *container,
        const int font_size_table[BAUL_ZOOM_LEVEL_LARGEST + 1])
{
    int old_font_size;
    int i;

    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));
    g_return_if_fail (font_size_table != NULL);

    old_font_size = container->details->font_size_table[container->details->zoom_level];

    for (i = 0; i <= BAUL_ZOOM_LEVEL_LARGEST; i++)
    {
        if (container->details->font_size_table[i] != font_size_table[i])
        {
            container->details->font_size_table[i] = font_size_table[i];
        }
    }

    if (old_font_size != container->details->font_size_table[container->details->zoom_level])
    {
        invalidate_labels (container);
        baul_icon_container_request_update_all (container);
    }
}

/**
 * baul_icon_container_get_icon_description
 * @container: An icon container widget.
 * @data: Icon data
 *
 * Gets the description for the icon. This function may return NULL.
 **/
char*
baul_icon_container_get_icon_description (BaulIconContainer *container,
        BaulIconData      *data)
{
    BaulIconContainerClass *klass;

    klass = BAUL_ICON_CONTAINER_GET_CLASS (container);

    if (klass->get_icon_description)
    {
        return klass->get_icon_description (container, data);
    }
    else
    {
        return NULL;
    }
}

gboolean
baul_icon_container_get_allow_moves (BaulIconContainer *container)
{
    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (container), FALSE);

    return container->details->drag_allow_moves;
}

void
baul_icon_container_set_allow_moves	(BaulIconContainer *container,
                                     gboolean               allow_moves)
{
    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    container->details->drag_allow_moves = allow_moves;
}

void
baul_icon_container_set_forced_icon_size (BaulIconContainer *container,
        int                    forced_icon_size)
{
    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    if (forced_icon_size != container->details->forced_icon_size)
    {
        container->details->forced_icon_size = forced_icon_size;

        invalidate_label_sizes (container);
        baul_icon_container_request_update_all (container);
    }
}

void
baul_icon_container_set_all_columns_same_width (BaulIconContainer *container,
        gboolean               all_columns_same_width)
{
    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    if (all_columns_same_width != container->details->all_columns_same_width)
    {
        container->details->all_columns_same_width = all_columns_same_width;

        invalidate_labels (container);
        baul_icon_container_request_update_all (container);
    }
}

/**
 * baul_icon_container_set_highlighted_for_clipboard
 * @container: An icon container widget.
 * @data: Icon Data associated with all icons that should be highlighted.
 *        Others will be unhighlighted.
 **/
void
baul_icon_container_set_highlighted_for_clipboard (BaulIconContainer *container,
        GList                 *clipboard_icon_data)
{
    GList *l;
    gboolean highlighted_for_clipboard;
    BaulIcon *icon = NULL;

    g_return_if_fail (BAUL_IS_ICON_CONTAINER (container));

    for (l = container->details->icons; l != NULL; l = l->next)
    {
        icon = l->data;
        highlighted_for_clipboard = (g_list_find (clipboard_icon_data, icon->data) != NULL);

        eel_canvas_item_set (EEL_CANVAS_ITEM (icon->item),
                             "highlighted-for-clipboard", highlighted_for_clipboard,
                             NULL);
    }

}

/* BaulIconContainerAccessible */

static BaulIconContainerAccessiblePrivate *
accessible_get_priv (AtkObject *accessible)
{
    BaulIconContainerAccessiblePrivate *priv;

    priv = g_object_get_qdata (G_OBJECT (accessible),
                               accessible_private_data_quark);

    return priv;
}

/* AtkAction interface */

static gboolean
baul_icon_container_accessible_do_action (AtkAction *accessible, int i)
{
    CtkWidget *widget;
    BaulIconContainer *container;
    GList *selection;

    g_return_val_if_fail (i < LAST_ACTION, FALSE);

    widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
    if (!widget)
    {
        return FALSE;
    }

    container = BAUL_ICON_CONTAINER (widget);
    switch (i)
    {
    case ACTION_ACTIVATE :
        selection = baul_icon_container_get_selection (container);

        if (selection)
        {
            g_signal_emit_by_name (container, "activate", selection);
            g_list_free (selection);
        }
        break;
    case ACTION_MENU :
        handle_popups (container, NULL,"context_click_background");
        break;
    default :
        g_warning ("Invalid action passed to BaulIconContainerAccessible::do_action");
        return FALSE;
    }
    return TRUE;
}

static int
baul_icon_container_accessible_get_n_actions (AtkAction *accessible G_GNUC_UNUSED)
{
    return LAST_ACTION;
}

static const char *
baul_icon_container_accessible_action_get_description (AtkAction *accessible,
        int i)
{
    BaulIconContainerAccessiblePrivate *priv;

    g_assert (i < LAST_ACTION);

    priv = accessible_get_priv (ATK_OBJECT (accessible));

    if (priv->action_descriptions[i])
    {
        return priv->action_descriptions[i];
    }
    else
    {
        return baul_icon_container_accessible_action_descriptions[i];
    }
}

static const char *
baul_icon_container_accessible_action_get_name (AtkAction *accessible G_GNUC_UNUSED,
						int        i)
{
    g_assert (i < LAST_ACTION);

    return baul_icon_container_accessible_action_names[i];
}

static const char *
baul_icon_container_accessible_action_get_keybinding (AtkAction *accessible G_GNUC_UNUSED,
						      int        i)
{
    g_assert (i < LAST_ACTION);

    return NULL;
}

static gboolean
baul_icon_container_accessible_action_set_description (AtkAction *accessible,
        int i,
        const char *description)
{
    BaulIconContainerAccessiblePrivate *priv;

    g_assert (i < LAST_ACTION);

    priv = accessible_get_priv (ATK_OBJECT (accessible));

    if (priv->action_descriptions[i])
    {
        g_free (priv->action_descriptions[i]);
    }
    priv->action_descriptions[i] = g_strdup (description);

    return FALSE;
}

static void
baul_icon_container_accessible_action_interface_init (AtkActionIface *iface)
{
    iface->do_action = baul_icon_container_accessible_do_action;
    iface->get_n_actions = baul_icon_container_accessible_get_n_actions;
    iface->get_description = baul_icon_container_accessible_action_get_description;
    iface->get_name = baul_icon_container_accessible_action_get_name;
    iface->get_keybinding = baul_icon_container_accessible_action_get_keybinding;
    iface->set_description = baul_icon_container_accessible_action_set_description;
}

/* AtkSelection interface */

static void
baul_icon_container_accessible_update_selection (AtkObject *accessible)
{
    BaulIconContainer *container;
    BaulIconContainerAccessiblePrivate *priv;
    GList *l;
    BaulIcon *icon = NULL;

    container = BAUL_ICON_CONTAINER (ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible)));

    priv = accessible_get_priv (accessible);

    if (priv->selection)
    {
        g_list_free (priv->selection);
        priv->selection = NULL;
    }

    for (l = container->details->icons; l != NULL; l = l->next)
    {
        icon = l->data;
        if (icon->is_selected)
        {
            priv->selection = g_list_prepend (priv->selection,
                                              icon);
        }
    }

    priv->selection = g_list_reverse (priv->selection);
}

static void
baul_icon_container_accessible_selection_changed_cb (BaulIconContainer *container G_GNUC_UNUSED,
						     gpointer           data)
{
    g_signal_emit_by_name (data, "selection_changed");
}

static void
baul_icon_container_accessible_icon_added_cb (BaulIconContainer *container,
        BaulIconData *icon_data,
        gpointer data)
{
    BaulIcon *icon;

    // We don't want to emit children_changed signals during any type of load.
    if (container->details->is_loading || container->details->is_populating_container)
        return;

    icon = g_hash_table_lookup (container->details->icon_set, icon_data);
    if (icon)
    {
        AtkObject *atk_parent;
        AtkObject *atk_child;
        int index;

        atk_parent = ATK_OBJECT (data);
        atk_child = atk_gobject_accessible_for_object
                    (G_OBJECT (icon->item));
        index = g_list_index (container->details->icons, icon);

        g_signal_emit_by_name (atk_parent, "children_changed::add",
                               index, atk_child, NULL);
    }
}

static void
baul_icon_container_accessible_icon_removed_cb (BaulIconContainer *container,
        BaulIconData *icon_data,
        gpointer data)
{
    BaulIcon *icon;

    icon = g_hash_table_lookup (container->details->icon_set, icon_data);
    if (icon)
    {
        AtkObject *atk_parent;
        AtkObject *atk_child;
        int index;

        atk_parent = ATK_OBJECT (data);
        atk_child = atk_gobject_accessible_for_object
                    (G_OBJECT (icon->item));
        index = g_list_index (container->details->icons, icon);

        g_signal_emit_by_name (atk_parent, "children_changed::remove",
                               index, atk_child, NULL);
    }
}

static void
baul_icon_container_accessible_cleared_cb (BaulIconContainer *container G_GNUC_UNUSED,
					   gpointer           data)
{
    g_signal_emit_by_name (data, "children_changed", 0, NULL, NULL);
}


static gboolean
baul_icon_container_accessible_add_selection (AtkSelection *accessible,
        int i)
{
    CtkWidget *widget;
    BaulIconContainer *container;
    BaulIcon *icon;

    widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
    if (!widget)
    {
        return FALSE;
    }

    container = BAUL_ICON_CONTAINER (widget);

    icon = g_list_nth_data (container->details->icons, i);
    if (icon)
    {
        GList *selection;

        selection = baul_icon_container_get_selection (container);
        selection = g_list_prepend (selection,
                                    icon->data);
        baul_icon_container_set_selection (container, selection);

        g_list_free (selection);
        return TRUE;
    }

    return FALSE;
}

static gboolean
baul_icon_container_accessible_clear_selection (AtkSelection *accessible)
{
    CtkWidget *widget;
    BaulIconContainer *container;

    widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
    if (!widget)
    {
        return FALSE;
    }

    container = BAUL_ICON_CONTAINER (widget);

    baul_icon_container_unselect_all (container);

    return TRUE;
}

static AtkObject *
baul_icon_container_accessible_ref_selection (AtkSelection *accessible,
        int i)
{
    BaulIconContainerAccessiblePrivate *priv;
    BaulIcon *icon;

    baul_icon_container_accessible_update_selection (ATK_OBJECT (accessible));
    priv = accessible_get_priv (ATK_OBJECT (accessible));

    icon = g_list_nth_data (priv->selection, i);
    if (icon)
    {
        AtkObject *atk_object;

        atk_object = atk_gobject_accessible_for_object (G_OBJECT (icon->item));
        if (atk_object)
        {
            g_object_ref (atk_object);
        }

        return atk_object;
    }
    else
    {
        return NULL;
    }
}

static int
baul_icon_container_accessible_get_selection_count (AtkSelection *accessible)
{
    int count;
    BaulIconContainerAccessiblePrivate *priv;

    baul_icon_container_accessible_update_selection (ATK_OBJECT (accessible));
    priv = accessible_get_priv (ATK_OBJECT (accessible));

    count = g_list_length (priv->selection);

    return count;
}

static gboolean
baul_icon_container_accessible_is_child_selected (AtkSelection *accessible,
        int i)
{
    BaulIconContainer *container;
    BaulIcon *icon;
    CtkWidget *widget;

    widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
    if (!widget)
    {
        return FALSE;
    }

    container = BAUL_ICON_CONTAINER (widget);

    icon = g_list_nth_data (container->details->icons, i);
    return icon ? icon->is_selected : FALSE;
}

static gboolean
baul_icon_container_accessible_remove_selection (AtkSelection *accessible,
        int i)
{
    BaulIconContainer *container;
    BaulIconContainerAccessiblePrivate *priv;
    BaulIcon *icon;
    CtkWidget *widget;

    widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
    if (!widget)
    {
        return FALSE;
    }

    baul_icon_container_accessible_update_selection (ATK_OBJECT (accessible));
    priv = accessible_get_priv (ATK_OBJECT (accessible));

    container = BAUL_ICON_CONTAINER (widget);

    icon = g_list_nth_data (priv->selection, i);
    if (icon)
    {
        GList *selection;

        selection = baul_icon_container_get_selection (container);
        selection = g_list_remove (selection, icon->data);
        baul_icon_container_set_selection (container, selection);

        g_list_free (selection);
        return TRUE;
    }

    return FALSE;
}

static gboolean
baul_icon_container_accessible_select_all_selection (AtkSelection *accessible)
{
    BaulIconContainer *container;
    CtkWidget *widget;

    widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
    if (!widget)
    {
        return FALSE;
    }

    container = BAUL_ICON_CONTAINER (widget);

    baul_icon_container_select_all (container);

    return TRUE;
}

void
baul_icon_container_widget_to_file_operation_position (BaulIconContainer *container,
        CdkPoint              *position)
{
    double x, y;

    g_return_if_fail (position != NULL);

    x = position->x;
    y = position->y;

    eel_canvas_window_to_world (EEL_CANVAS (container), x, y, &x, &y);

    position->x = (int) x;
    position->y = (int) y;

    /* ensure that we end up in the middle of the icon */
    position->x -= baul_get_icon_size_for_zoom_level (container->details->zoom_level) / 2;
    position->y -= baul_get_icon_size_for_zoom_level (container->details->zoom_level) / 2;
}

static void
baul_icon_container_accessible_selection_interface_init (AtkSelectionIface *iface)
{
    iface->add_selection = baul_icon_container_accessible_add_selection;
    iface->clear_selection = baul_icon_container_accessible_clear_selection;
    iface->ref_selection = baul_icon_container_accessible_ref_selection;
    iface->get_selection_count = baul_icon_container_accessible_get_selection_count;
    iface->is_child_selected = baul_icon_container_accessible_is_child_selected;
    iface->remove_selection = baul_icon_container_accessible_remove_selection;
    iface->select_all_selection = baul_icon_container_accessible_select_all_selection;
}


static gint
baul_icon_container_accessible_get_n_children (AtkObject *accessible)
{
    BaulIconContainer *container;
    CtkWidget *widget;
    gint i;

    widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
    if (!widget)
    {
        return FALSE;
    }

    container = BAUL_ICON_CONTAINER (widget);

    i = g_hash_table_size (container->details->icon_set);
    if (container->details->rename_widget)
    {
        i++;
    }
    return i;
}

static AtkObject*
baul_icon_container_accessible_ref_child (AtkObject *accessible, int i)
{
    AtkObject *atk_object;
    BaulIconContainer *container;
    BaulIcon *icon;
    CtkWidget *widget;

    widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
    if (!widget)
    {
        return NULL;
    }

    container = BAUL_ICON_CONTAINER (widget);

    icon = g_list_nth_data (container->details->icons, i);
    if (icon)
    {
        atk_object = atk_gobject_accessible_for_object (G_OBJECT (icon->item));
        g_object_ref (atk_object);

        return atk_object;
    }
    else
    {
        if (i == g_list_length (container->details->icons))
        {
            if (container->details->rename_widget)
            {
                atk_object = ctk_widget_get_accessible (container->details->rename_widget);
                g_object_ref (atk_object);

                return atk_object;
            }
        }
        return NULL;
    }
}

static void
baul_icon_container_accessible_initialize (AtkObject *accessible,
        gpointer data)
{
    BaulIconContainerAccessiblePrivate *priv;

    if (ATK_OBJECT_CLASS (accessible_parent_class)->initialize)
    {
        ATK_OBJECT_CLASS (accessible_parent_class)->initialize (accessible, data);
    }

    priv = g_new0 (BaulIconContainerAccessiblePrivate, 1);
    g_object_set_qdata (G_OBJECT (accessible),
                        accessible_private_data_quark,
                        priv);

    if (CTK_IS_ACCESSIBLE (accessible))
    {
        BaulIconContainer *container;

        baul_icon_container_accessible_update_selection
        (ATK_OBJECT (accessible));

        container = BAUL_ICON_CONTAINER (ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible)));
        g_signal_connect (G_OBJECT (container), "selection_changed",
                          G_CALLBACK (baul_icon_container_accessible_selection_changed_cb),
                          accessible);
        g_signal_connect (G_OBJECT (container), "icon_added",
                          G_CALLBACK (baul_icon_container_accessible_icon_added_cb),
                          accessible);
        g_signal_connect (G_OBJECT (container), "icon_removed",
                          G_CALLBACK (baul_icon_container_accessible_icon_removed_cb),
                          accessible);
        g_signal_connect (G_OBJECT (container), "cleared",
                          G_CALLBACK (baul_icon_container_accessible_cleared_cb),
                          accessible);
    }
}

static void
baul_icon_container_accessible_finalize (GObject *object)
{
    BaulIconContainerAccessiblePrivate *priv;
    int i;

    priv = accessible_get_priv (ATK_OBJECT (object));
    if (priv->selection)
    {
        g_list_free (priv->selection);
    }

    for (i = 0; i < LAST_ACTION; i++)
    {
        if (priv->action_descriptions[i])
        {
            g_free (priv->action_descriptions[i]);
        }
    }

    g_free (priv);

    G_OBJECT_CLASS (accessible_parent_class)->finalize (object);
}

G_DEFINE_TYPE_WITH_CODE (BaulIconContainerAccessible,
                         baul_icon_container_accessible,
                         eel_canvas_accessible_get_type (),
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION,
                                                baul_icon_container_accessible_action_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_SELECTION,
                                                baul_icon_container_accessible_selection_interface_init));

static void
baul_icon_container_accessible_init (BaulIconContainerAccessible *accessible G_GNUC_UNUSED)
{
}

static void
baul_icon_container_accessible_class_init (BaulIconContainerAccessibleClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

    accessible_parent_class = g_type_class_peek_parent (klass);

    gobject_class->finalize = baul_icon_container_accessible_finalize;

    atk_class->get_n_children = baul_icon_container_accessible_get_n_children;
    atk_class->ref_child = baul_icon_container_accessible_ref_child;
    atk_class->initialize = baul_icon_container_accessible_initialize;

    accessible_private_data_quark = g_quark_from_static_string ("icon-container-accessible-private-data");
}

#if ! defined (BAUL_OMIT_SELF_CHECK)

static char *
check_compute_stretch (int icon_x, int icon_y, int icon_size,
                       int start_pointer_x, int start_pointer_y,
                       int end_pointer_x, int end_pointer_y)
{
    StretchState start, current;

    start.icon_x = icon_x;
    start.icon_y = icon_y;
    start.icon_size = icon_size;
    start.pointer_x = start_pointer_x;
    start.pointer_y = start_pointer_y;
    current.pointer_x = end_pointer_x;
    current.pointer_y = end_pointer_y;

    compute_stretch (&start, &current);

    return g_strdup_printf ("%d,%d:%d",
                            current.icon_x,
                            current.icon_y,
                            current.icon_size);
}

void
baul_self_check_icon_container (void)
{
    EEL_CHECK_STRING_RESULT (check_compute_stretch (0, 0, 16, 0, 0, 0, 0), "0,0:16");
    EEL_CHECK_STRING_RESULT (check_compute_stretch (0, 0, 16, 16, 16, 17, 17), "0,0:17");
    EEL_CHECK_STRING_RESULT (check_compute_stretch (0, 0, 16, 16, 16, 17, 16), "0,0:16");
    EEL_CHECK_STRING_RESULT (check_compute_stretch (100, 100, 64, 105, 105, 40, 40), "35,35:129");
}
#endif /* ! BAUL_OMIT_SELF_CHECK */

gboolean
baul_icon_container_is_layout_rtl (BaulIconContainer *container)
{
    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (container), 0);

    return container->details->layout_mode == BAUL_ICON_LAYOUT_T_B_R_L ||
           container->details->layout_mode == BAUL_ICON_LAYOUT_R_L_T_B;
}

gboolean
baul_icon_container_is_layout_vertical (BaulIconContainer *container)
{
    g_return_val_if_fail (BAUL_IS_ICON_CONTAINER (container), FALSE);

    return (container->details->layout_mode == BAUL_ICON_LAYOUT_T_B_L_R ||
            container->details->layout_mode == BAUL_ICON_LAYOUT_T_B_R_L);
}

int
baul_icon_container_get_max_layout_lines_for_pango (BaulIconContainer  *container)
{
    int limit;

    if (baul_icon_container_get_is_desktop (container))
    {
        limit = desktop_text_ellipsis_limit;
    }
    else
    {
        limit = text_ellipsis_limits[container->details->zoom_level];
    }

    if (limit <= 0)
    {
        return G_MININT;
    }

    return -limit;
}

int
baul_icon_container_get_max_layout_lines (BaulIconContainer  *container)
{
    int limit;

    if (baul_icon_container_get_is_desktop (container))
    {
        limit = desktop_text_ellipsis_limit;
    }
    else
    {
        limit = text_ellipsis_limits[container->details->zoom_level];
    }

    if (limit <= 0)
    {
        return G_MAXINT;
    }

    return limit;
}

void
baul_icon_container_begin_loading (BaulIconContainer *container)
{
    gboolean dummy;

    if (baul_icon_container_get_store_layout_timestamps (container))
    {
        container->details->layout_timestamp = UNDEFINED_TIME;
        g_signal_emit (container,
                       signals[GET_STORED_LAYOUT_TIMESTAMP], 0,
                       NULL, &container->details->layout_timestamp, &dummy);
    }
}

static void
store_layout_timestamps_now (BaulIconContainer *container)
{
    GList *p;
    gboolean dummy;
    BaulIcon *icon = NULL;

    container->details->layout_timestamp = time (NULL);
    g_signal_emit (container,
                   signals[STORE_LAYOUT_TIMESTAMP], 0,
                   NULL, &container->details->layout_timestamp, &dummy);

    for (p = container->details->icons; p != NULL; p = p->next)
    {
        icon = p->data;

        g_signal_emit (container,
                       signals[STORE_LAYOUT_TIMESTAMP], 0,
                       icon->data, &container->details->layout_timestamp, &dummy);
    }
}

void
baul_icon_container_end_loading (BaulIconContainer *container,
                                 gboolean               all_icons_added)
{
    if (all_icons_added &&
            baul_icon_container_get_store_layout_timestamps (container))
    {
        if (container->details->new_icons == NULL)
        {
            store_layout_timestamps_now (container);
        }
        else
        {
            container->details->store_layout_timestamps_when_finishing_new_icons = TRUE;
        }
    }
}

gboolean
baul_icon_container_get_store_layout_timestamps (BaulIconContainer *container)
{
    return container->details->store_layout_timestamps;
}

void
baul_icon_container_set_store_layout_timestamps (BaulIconContainer *container,
        gboolean               store_layout_timestamps)
{
    container->details->store_layout_timestamps = store_layout_timestamps;
}
