/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-view.h: Interface for baul views

   Copyright (C) 2004 Red Hat Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Author: Alexander Larsson <alexl@redhat.com>
*/

#ifndef BAUL_VIEW_H
#define BAUL_VIEW_H

#include <glib-object.h>
#include <ctk/ctk.h>

/* For BaulZoomLevel */
#include "baul-icon-info.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BAUL_TYPE_VIEW           (baul_view_get_type ())
#define BAUL_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_VIEW, BaulView))
#define BAUL_IS_VIEW(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_VIEW))
#define BAUL_VIEW_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAUL_TYPE_VIEW, BaulViewIface))


    typedef struct _BaulView BaulView; /* dummy typedef */
    typedef struct _BaulViewIface BaulViewIface;

    struct _BaulViewIface
    {
        GTypeInterface g_iface;

        /* Signals: */

        /* emitted when the view-specific title as returned by get_title changes */
        void           (* title_changed)          (BaulView          *view);

        void           (* zoom_level_changed)     (BaulView          *view);

        /* VTable: */

        /* Get the id string for this view. Its a constant string, not memory managed */
        const char *   (* get_view_id)            (BaulView          *view);

        /* Get the widget for this view, can be the same object or a different
           object owned by the view. Doesn't ref the widget. */
        CtkWidget *    (* get_widget)             (BaulView          *view);

        /* Called to tell the view to start loading a location, or to reload it.
           The view responds with a load_underway as soon as it starts loading,
           and a load_complete when the location is completely read. */
        void           (* load_location)          (BaulView          *view,
                const char            *location_uri);

        /* Called to tell the view to stop loading the location its currently loading */
        void           (* stop_loading)           (BaulView          *view);

        /* Returns the number of selected items in the view */
        int            (* get_selection_count)    (BaulView          *view);

        /* Returns a list of uris for th selected items in the view, caller frees it */
        GList *        (* get_selection)          (BaulView          *view);

        /* This is called when the window wants to change the selection in the view */
        void           (* set_selection)          (BaulView          *view,
                GList                 *list);

        /* Inverts the selection in the view */
        void           (* invert_selection)       (BaulView          *view);

        /* Return the uri of the first visible file */
        char *         (* get_first_visible_file) (BaulView          *view);
        /* Scroll the view so that the file specified by the uri is at the top
           of the view */
        void           (* scroll_to_file)	  (BaulView          *view,
                                               const char            *uri);

        /* This function can supply a special window title, if you don't want one
           have this function return NULL, or just don't supply a function  */
        char *         (* get_title)              (BaulView          *view);


        /* Zoom support */
        gboolean       (* supports_zooming)       (BaulView          *view);
        void           (* bump_zoom_level)     	  (BaulView          *view,
                int                    zoom_increment);
        void           (* zoom_to_level) 	  (BaulView          *view,
                                               BaulZoomLevel     level);
        BaulZoomLevel (* get_zoom_level) 	  (BaulView          *view);
        void           (* restore_default_zoom_level) (BaulView          *view);
        gboolean       (* can_zoom_in)	 	  (BaulView          *view);
        gboolean       (* can_zoom_out)	 	  (BaulView          *view);

        void           (* grab_focus)             (BaulView          *view);
        void           (* update_menus)           (BaulView          *view);

        /* Request popup of context menu referring to the open location.
         * This is triggered in spatial windows by right-clicking the location button,
         * in navigational windows by right-clicking the "Location:" label in the
         * navigation bar or any of the buttons in the pathbar.
         * The location parameter specifies the location this popup should be displayed for.
         * If it is NULL, the currently displayed location should be used.
         * The view may display the popup synchronously, asynchronously
         * or not react to the popup request at all. */
        void           (* pop_up_location_context_menu) (BaulView   *view,
                GdkEventButton *event,
                const char     *location);

        void           (* drop_proxy_received_uris)     (BaulView         *view,
                GList                *uris,
                const char           *target_location,
                GdkDragAction         action);
        void           (* drop_proxy_received_netscape_url) (BaulView         *view,
                const char           *source_url,
                const char           *target_location,
                GdkDragAction         action);
        void           (* set_is_active)                    (BaulView         *view,
                gboolean              is_active);

        /* Padding for future expansion */
        void (*_reserved1) (void);
        void (*_reserved2) (void);
        void (*_reserved3) (void);
        void (*_reserved4) (void);
        void (*_reserved5) (void);
        void (*_reserved6) (void);
        void (*_reserved7) (void);
    };

    GType             baul_view_get_type             (void);

    const char *      baul_view_get_view_id                (BaulView      *view);
    CtkWidget *       baul_view_get_widget                 (BaulView      *view);
    void              baul_view_load_location              (BaulView      *view,
            const char        *location_uri);
    void              baul_view_stop_loading               (BaulView      *view);
    int               baul_view_get_selection_count        (BaulView      *view);
    GList *           baul_view_get_selection              (BaulView      *view);
    void              baul_view_set_selection              (BaulView      *view,
            GList             *list);
    void              baul_view_invert_selection           (BaulView      *view);
    char *            baul_view_get_first_visible_file     (BaulView      *view);
    void              baul_view_scroll_to_file             (BaulView      *view,
            const char        *uri);
    char *            baul_view_get_title                  (BaulView      *view);
    gboolean          baul_view_supports_zooming           (BaulView      *view);
    void              baul_view_bump_zoom_level            (BaulView      *view,
            int                zoom_increment);
    void              baul_view_zoom_to_level              (BaulView      *view,
            BaulZoomLevel  level);
    void              baul_view_restore_default_zoom_level (BaulView      *view);
    gboolean          baul_view_can_zoom_in                (BaulView      *view);
    gboolean          baul_view_can_zoom_out               (BaulView      *view);
    BaulZoomLevel baul_view_get_zoom_level             (BaulView      *view);
    void              baul_view_pop_up_location_context_menu (BaulView    *view,
            GdkEventButton  *event,
            const char      *location);
    void              baul_view_grab_focus                 (BaulView      *view);
    void              baul_view_update_menus               (BaulView      *view);
    void              baul_view_drop_proxy_received_uris   (BaulView         *view,
            GList                *uris,
            const char           *target_location,
            GdkDragAction         action);
    void              baul_view_drop_proxy_received_netscape_url (BaulView         *view,
            const char           *source_url,
            const char           *target_location,
            GdkDragAction         action);
    void              baul_view_set_is_active              (BaulView      *view,
            gboolean           is_active);

#ifdef __cplusplus
}
#endif

#endif /* BAUL_VIEW_H */
