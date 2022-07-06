/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* cafe-icon-container.h - Icon container widget.

   Copyright (C) 1999, 2000 Free Software Foundation
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

   Authors: Ettore Perazzoli <ettore@gnu.org>, Darin Adler <darin@bentspoon.com>
*/

#ifndef BAUL_ICON_CONTAINER_H
#define BAUL_ICON_CONTAINER_H

#include <eel/eel-canvas.h>

#include "baul-icon-info.h"

#define BAUL_TYPE_ICON_CONTAINER baul_icon_container_get_type()
#define BAUL_ICON_CONTAINER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_ICON_CONTAINER, BaulIconContainer))
#define BAUL_ICON_CONTAINER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_ICON_CONTAINER, BaulIconContainerClass))
#define BAUL_IS_ICON_CONTAINER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_ICON_CONTAINER))
#define BAUL_IS_ICON_CONTAINER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_ICON_CONTAINER))
#define BAUL_ICON_CONTAINER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_ICON_CONTAINER, BaulIconContainerClass))


#define BAUL_ICON_CONTAINER_ICON_DATA(pointer) \
	((BaulIconData *) (pointer))

typedef struct BaulIconData BaulIconData;

typedef void (* BaulIconCallback) (BaulIconData *icon_data,
                                   gpointer callback_data);

typedef struct
{
    int x;
    int y;
    double scale;
} BaulIconPosition;

typedef enum
{
    BAUL_ICON_LAYOUT_L_R_T_B,
    BAUL_ICON_LAYOUT_R_L_T_B,
    BAUL_ICON_LAYOUT_T_B_L_R,
    BAUL_ICON_LAYOUT_T_B_R_L
} BaulIconLayoutMode;

typedef enum
{
    BAUL_ICON_LABEL_POSITION_UNDER,
    BAUL_ICON_LABEL_POSITION_BESIDE
} BaulIconLabelPosition;

#define	BAUL_ICON_CONTAINER_TYPESELECT_FLUSH_DELAY 1000000

typedef struct BaulIconContainerDetails BaulIconContainerDetails;

typedef struct
{
    EelCanvas canvas;
    BaulIconContainerDetails *details;
} BaulIconContainer;

typedef struct
{
    EelCanvasClass parent_slot;

    /* Operations on the container. */
    int          (* button_press) 	          (BaulIconContainer *container,
            GdkEventButton *event);
    void         (* context_click_background) (BaulIconContainer *container,
            GdkEventButton *event);
    void         (* middle_click) 		  (BaulIconContainer *container,
                                           GdkEventButton *event);

    /* Operations on icons. */
    void         (* activate)	  	  (BaulIconContainer *container,
                                       BaulIconData *data);
    void         (* activate_alternate)       (BaulIconContainer *container,
            BaulIconData *data);
    void         (* context_click_selection)  (BaulIconContainer *container,
            GdkEventButton *event);
    void	     (* move_copy_items)	  (BaulIconContainer *container,
                                           const GList *item_uris,
                                           GdkPoint *relative_item_points,
                                           const char *target_uri,
                                           GdkDragAction action,
                                           int x,
                                           int y);
    void	     (* handle_netscape_url)	  (BaulIconContainer *container,
            const char *url,
            const char *target_uri,
            GdkDragAction action,
            int x,
            int y);
    void	     (* handle_uri_list)    	  (BaulIconContainer *container,
            const char *uri_list,
            const char *target_uri,
            GdkDragAction action,
            int x,
            int y);
    void	     (* handle_text)		  (BaulIconContainer *container,
                                           const char *text,
                                           const char *target_uri,
                                           GdkDragAction action,
                                           int x,
                                           int y);
    void	     (* handle_raw)		  (BaulIconContainer *container,
                                       char *raw_data,
                                       int length,
                                       const char *target_uri,
                                       const char *direct_save_uri,
                                       GdkDragAction action,
                                       int x,
                                       int y);

    /* Queries on the container for subclass/client.
     * These must be implemented. The default "do nothing" is not good enough.
     */
    char *	     (* get_container_uri)	  (BaulIconContainer *container);

    /* Queries on icons for subclass/client.
     * These must be implemented. The default "do nothing" is not
     * good enough, these are _not_ signals.
     */
    BaulIconInfo *(* get_icon_images)     (BaulIconContainer *container,
                                           BaulIconData *data,
                                           int icon_size,
                                           GList **emblem_pixbufs,
                                           char **embedded_text,
                                           gboolean for_drag_accept,
                                           gboolean need_large_embeddded_text,
                                           gboolean *embedded_text_needs_loading,
                                           gboolean *has_window_open);
    void         (* get_icon_text)            (BaulIconContainer *container,
            BaulIconData *data,
            char **editable_text,
            char **additional_text,
            gboolean include_invisible);
    char *       (* get_icon_description)     (BaulIconContainer *container,
            BaulIconData *data);
    int          (* compare_icons)            (BaulIconContainer *container,
            BaulIconData *icon_a,
            BaulIconData *icon_b);
    int          (* compare_icons_by_name)    (BaulIconContainer *container,
            BaulIconData *icon_a,
            BaulIconData *icon_b);
    void         (* freeze_updates)           (BaulIconContainer *container);
    void         (* unfreeze_updates)         (BaulIconContainer *container);
    void         (* start_monitor_top_left)   (BaulIconContainer *container,
            BaulIconData *data,
            gconstpointer client,
            gboolean large_text);
    void         (* stop_monitor_top_left)    (BaulIconContainer *container,
            BaulIconData *data,
            gconstpointer client);
    void         (* prioritize_thumbnailing)  (BaulIconContainer *container,
            BaulIconData *data);

    /* Queries on icons for subclass/client.
     * These must be implemented => These are signals !
     * The default "do nothing" is not good enough.
     */
    gboolean     (* can_accept_item)	  (BaulIconContainer *container,
                                           BaulIconData *target,
                                           const char *item_uri);
    gboolean     (* get_stored_icon_position) (BaulIconContainer *container,
            BaulIconData *data,
            BaulIconPosition *position);
    char *       (* get_icon_uri)             (BaulIconContainer *container,
            BaulIconData *data);
    char *       (* get_icon_drop_target_uri) (BaulIconContainer *container,
            BaulIconData *data);

    /* If icon data is NULL, the layout timestamp of the container should be retrieved.
     * That is the time when the container displayed a fully loaded directory with
     * all icon positions assigned.
     *
     * If icon data is not NULL, the position timestamp of the icon should be retrieved.
     * That is the time when the file (i.e. icon data payload) was last displayed in a
     * fully loaded directory with all icon positions assigned.
     */
    gboolean     (* get_stored_layout_timestamp) (BaulIconContainer *container,
            BaulIconData *data,
            time_t *time);
    /* If icon data is NULL, the layout timestamp of the container should be stored.
     * If icon data is not NULL, the position timestamp of the container should be stored.
     */
    gboolean     (* store_layout_timestamp) (BaulIconContainer *container,
            BaulIconData *data,
            const time_t *time);

    /* Notifications for the whole container. */
    void	     (* band_select_started)	  (BaulIconContainer *container);
    void	     (* band_select_ended)	  (BaulIconContainer *container);
    void         (* selection_changed) 	  (BaulIconContainer *container);
    void         (* layout_changed)           (BaulIconContainer *container);

    /* Notifications for icons. */
    void         (* icon_position_changed)    (BaulIconContainer *container,
            BaulIconData *data,
            const BaulIconPosition *position);
    void         (* icon_text_changed)        (BaulIconContainer *container,
            BaulIconData *data,
            const char *text);
    void         (* renaming_icon)            (BaulIconContainer *container,
            CtkWidget *renaming_widget);
    void	     (* icon_stretch_started)     (BaulIconContainer *container,
            BaulIconData *data);
    void	     (* icon_stretch_ended)       (BaulIconContainer *container,
            BaulIconData *data);
    int	     (* preview)		  (BaulIconContainer *container,
                                   BaulIconData *data,
                                   gboolean start_flag);
    void         (* icon_added)               (BaulIconContainer *container,
            BaulIconData *data);
    void         (* icon_removed)             (BaulIconContainer *container,
            BaulIconData *data);
    void         (* cleared)                  (BaulIconContainer *container);
    gboolean     (* start_interactive_search) (BaulIconContainer *container);
} BaulIconContainerClass;

/* CtkObject */
GType             baul_icon_container_get_type                      (void);
CtkWidget *       baul_icon_container_new                           (void);


/* adding, removing, and managing icons */
void              baul_icon_container_clear                         (BaulIconContainer  *view);
gboolean          baul_icon_container_add                           (BaulIconContainer  *view,
        BaulIconData       *data);
void              baul_icon_container_layout_now                    (BaulIconContainer *container);
gboolean          baul_icon_container_remove                        (BaulIconContainer  *view,
        BaulIconData       *data);
void              baul_icon_container_for_each                      (BaulIconContainer  *view,
        BaulIconCallback    callback,
        gpointer                callback_data);
void              baul_icon_container_request_update                (BaulIconContainer  *view,
        BaulIconData       *data);
void              baul_icon_container_request_update_all            (BaulIconContainer  *container);
void              baul_icon_container_reveal                        (BaulIconContainer  *container,
        BaulIconData       *data);
gboolean          baul_icon_container_is_empty                      (BaulIconContainer  *container);
BaulIconData *baul_icon_container_get_first_visible_icon        (BaulIconContainer  *container);
void              baul_icon_container_scroll_to_icon                (BaulIconContainer  *container,
        BaulIconData       *data);

void              baul_icon_container_begin_loading                 (BaulIconContainer  *container);
void              baul_icon_container_end_loading                   (BaulIconContainer  *container,
        gboolean                all_icons_added);

/* control the layout */
gboolean          baul_icon_container_is_auto_layout                (BaulIconContainer  *container);
void              baul_icon_container_set_auto_layout               (BaulIconContainer  *container,
        gboolean                auto_layout);
gboolean          baul_icon_container_is_tighter_layout             (BaulIconContainer  *container);
void              baul_icon_container_set_tighter_layout            (BaulIconContainer  *container,
        gboolean                tighter_layout);

gboolean          baul_icon_container_is_keep_aligned               (BaulIconContainer  *container);
void              baul_icon_container_set_keep_aligned              (BaulIconContainer  *container,
        gboolean                keep_aligned);
void              baul_icon_container_set_layout_mode               (BaulIconContainer  *container,
        BaulIconLayoutMode  mode);
void              baul_icon_container_set_label_position            (BaulIconContainer  *container,
        BaulIconLabelPosition pos);
void              baul_icon_container_sort                          (BaulIconContainer  *container);
void              baul_icon_container_freeze_icon_positions         (BaulIconContainer  *container);

int               baul_icon_container_get_max_layout_lines           (BaulIconContainer  *container);
int               baul_icon_container_get_max_layout_lines_for_pango (BaulIconContainer  *container);

void              baul_icon_container_set_highlighted_for_clipboard (BaulIconContainer  *container,
        GList                  *clipboard_icon_data);

/* operations on all icons */
void              baul_icon_container_unselect_all                  (BaulIconContainer  *view);
void              baul_icon_container_select_all                    (BaulIconContainer  *view);


/* operations on the selection */
GList     *       baul_icon_container_get_selection                 (BaulIconContainer  *view);
void			  baul_icon_container_invert_selection				(BaulIconContainer  *view);
void              baul_icon_container_set_selection                 (BaulIconContainer  *view,
        GList                  *selection);
GArray    *       baul_icon_container_get_selected_icon_locations   (BaulIconContainer  *view);
gboolean          baul_icon_container_has_stretch_handles           (BaulIconContainer  *container);
gboolean          baul_icon_container_is_stretched                  (BaulIconContainer  *container);
void              baul_icon_container_show_stretch_handles          (BaulIconContainer  *container);
void              baul_icon_container_unstretch                     (BaulIconContainer  *container);
void              baul_icon_container_start_renaming_selected_item  (BaulIconContainer  *container,
        gboolean                select_all);

/* options */
BaulZoomLevel baul_icon_container_get_zoom_level                (BaulIconContainer  *view);
void              baul_icon_container_set_zoom_level                (BaulIconContainer  *view,
        int                     new_zoom_level);
void              baul_icon_container_set_single_click_mode         (BaulIconContainer  *container,
        gboolean                single_click_mode);
void              baul_icon_container_enable_linger_selection       (BaulIconContainer  *view,
        gboolean                enable);
gboolean          baul_icon_container_get_is_fixed_size             (BaulIconContainer  *container);
void              baul_icon_container_set_is_fixed_size             (BaulIconContainer  *container,
        gboolean                is_fixed_size);
gboolean          baul_icon_container_get_is_desktop                (BaulIconContainer  *container);
void              baul_icon_container_set_is_desktop                (BaulIconContainer  *container,
        gboolean                is_desktop);
void              baul_icon_container_reset_scroll_region           (BaulIconContainer  *container);
void              baul_icon_container_set_font                      (BaulIconContainer  *container,
        const char             *font);
void              baul_icon_container_set_font_size_table           (BaulIconContainer  *container,
        const int               font_size_table[BAUL_ZOOM_LEVEL_LARGEST + 1]);
void              baul_icon_container_set_margins                   (BaulIconContainer  *container,
        int                     left_margin,
        int                     right_margin,
        int                     top_margin,
        int                     bottom_margin);
void              baul_icon_container_set_use_drop_shadows          (BaulIconContainer  *container,
        gboolean                use_drop_shadows);
char*             baul_icon_container_get_icon_description          (BaulIconContainer  *container,
        BaulIconData       *data);
gboolean          baul_icon_container_get_allow_moves               (BaulIconContainer  *container);
void              baul_icon_container_set_allow_moves               (BaulIconContainer  *container,
        gboolean                allow_moves);
void		  baul_icon_container_set_forced_icon_size		(BaulIconContainer  *container,
        int                     forced_icon_size);
void		  baul_icon_container_set_all_columns_same_width	(BaulIconContainer  *container,
        gboolean                all_columns_same_width);

gboolean	  baul_icon_container_is_layout_rtl			(BaulIconContainer  *container);
gboolean	  baul_icon_container_is_layout_vertical		(BaulIconContainer  *container);

gboolean          baul_icon_container_get_store_layout_timestamps   (BaulIconContainer  *container);
void              baul_icon_container_set_store_layout_timestamps   (BaulIconContainer  *container,
        gboolean                store_layout);

void              baul_icon_container_widget_to_file_operation_position (BaulIconContainer *container,
        GdkPoint              *position);


#define CANVAS_WIDTH(container,allocation) ((allocation.width	  \
				- container->details->left_margin \
				- container->details->right_margin) \
				/  EEL_CANVAS (container)->pixels_per_unit)

#define CANVAS_HEIGHT(container,allocation) ((allocation.height \
			 - container->details->top_margin \
			 - container->details->bottom_margin) \
			 / EEL_CANVAS (container)->pixels_per_unit)

#endif /* BAUL_ICON_CONTAINER_H */
