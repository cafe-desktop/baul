/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-window-info.h: Interface for baul windows

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

#ifndef BAUL_WINDOW_INFO_H
#define BAUL_WINDOW_INFO_H

#include <glib-object.h>

#include <gtk/gtk.h>

#include "baul-view.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum
    {
        BAUL_WINDOW_SHOW_HIDDEN_FILES_DEFAULT,
        BAUL_WINDOW_SHOW_HIDDEN_FILES_ENABLE,
        BAUL_WINDOW_SHOW_HIDDEN_FILES_DISABLE
    }
    BaulWindowShowHiddenFilesMode;

    typedef enum
    {
        BAUL_WINDOW_SHOW_BACKUP_FILES_DEFAULT,
        BAUL_WINDOW_SHOW_BACKUP_FILES_ENABLE,
        BAUL_WINDOW_SHOW_BACKUP_FILES_DISABLE
    }
    BaulWindowShowBackupFilesMode;

    typedef enum
    {
        BAUL_WINDOW_OPEN_ACCORDING_TO_MODE,
        BAUL_WINDOW_OPEN_IN_SPATIAL,
        BAUL_WINDOW_OPEN_IN_NAVIGATION
    } BaulWindowOpenMode;

    typedef enum
    {
        /* used in spatial mode */
        BAUL_WINDOW_OPEN_FLAG_CLOSE_BEHIND = 1<<0,
        /* used in navigation mode */
        BAUL_WINDOW_OPEN_FLAG_NEW_WINDOW = 1<<1,
        BAUL_WINDOW_OPEN_FLAG_NEW_TAB = 1<<2
    } BaulWindowOpenFlags;

    typedef	enum
    {
        BAUL_WINDOW_SPATIAL,
        BAUL_WINDOW_NAVIGATION,
        BAUL_WINDOW_DESKTOP
    } BaulWindowType;

#define BAUL_TYPE_WINDOW_INFO           (baul_window_info_get_type ())
#define BAUL_WINDOW_INFO(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_WINDOW_INFO, BaulWindowInfo))
#define BAUL_IS_WINDOW_INFO(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_WINDOW_INFO))
#define BAUL_WINDOW_INFO_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAUL_TYPE_WINDOW_INFO, BaulWindowInfoIface))

#ifndef BAUL_WINDOW_DEFINED
#define BAUL_WINDOW_DEFINED
    /* Using BaulWindow for the vtable to make implementing this in
     * BaulWindow easier */
    typedef struct BaulWindow          BaulWindow;
#endif

#ifndef BAUL_WINDOW_SLOT_DEFINED
#define BAUL_WINDOW_SLOT_DEFINED
    typedef struct BaulWindowSlot      BaulWindowSlot;
#endif


    typedef BaulWindowSlot              BaulWindowSlotInfo;
    typedef BaulWindow                  BaulWindowInfo;

    typedef struct _BaulWindowInfoIface BaulWindowInfoIface;

    typedef void (* BaulWindowGoToCallback) (BaulWindow *window,
    					     GError *error,
    					     gpointer user_data);

    struct _BaulWindowInfoIface
    {
        GTypeInterface g_iface;

        /* signals: */

        void           (* loading_uri)              (BaulWindowInfo *window,
                const char        *uri);
        /* Emitted when the view in the window changes the selection */
        void           (* selection_changed)        (BaulWindowInfo *window);
        void           (* title_changed)            (BaulWindowInfo *window,
                const char         *title);
        void           (* hidden_files_mode_changed)(BaulWindowInfo *window);
	void           (* backup_files_mode_changed)(BaulWindowInfo *window);

        /* VTable: */
        /* A view calls this once after a load_location, once it starts loading the
         * directory. Might be called directly, or later on the mainloop.
         * This can also be called at any other time if the view needs to
         * re-load the location. But the view needs to call load_complete first if
         * its currently loading. */
        void (* report_load_underway) (BaulWindowInfo *window,
                                       BaulView *view);
        /* A view calls this once after reporting load_underway, when the location
           has been fully loaded, or when the load was stopped
           (by an error or by the user). */
        void (* report_load_complete) (BaulWindowInfo *window,
                                       BaulView *view);
        /* This can be called at any time when there has been a catastrophic failure of
           the view. It will result in the view being removed. */
        void (* report_view_failed)   (BaulWindowInfo *window,
                                       BaulView *view);
        void (* report_selection_changed) (BaulWindowInfo *window);

        /* Returns the number of selected items in the view */
        int  (* get_selection_count)  (BaulWindowInfo    *window);

        /* Returns a list of uris for th selected items in the view, caller frees it */
        GList *(* get_selection)      (BaulWindowInfo    *window);

        char * (* get_current_location)  (BaulWindowInfo *window);
        void   (* push_status)           (BaulWindowInfo *window,
                                          const char *status);
        char * (* get_title)             (BaulWindowInfo *window);
        GList *(* get_history)           (BaulWindowInfo *window);
        BaulWindowType
        (* get_window_type)       (BaulWindowInfo *window);
        BaulWindowShowHiddenFilesMode
        (* get_hidden_files_mode) (BaulWindowInfo *window);
        void   (* set_hidden_files_mode) (BaulWindowInfo *window,
                                          BaulWindowShowHiddenFilesMode mode);
        BaulWindowShowBackupFilesMode
        (* get_backup_files_mode) (BaulWindowInfo *window);
        void   (* set_backup_files_mode) (BaulWindowInfo *window,
                                          BaulWindowShowBackupFilesMode mode);

        BaulWindowSlotInfo * (* get_active_slot) (BaulWindowInfo *window);
        BaulWindowSlotInfo * (* get_extra_slot)  (BaulWindowInfo *window);

        gboolean (* get_initiated_unmount) (BaulWindowInfo *window);
        void   (* set_initiated_unmount) (BaulWindowInfo *window,
                                          gboolean initiated_unmount);

        void   (* view_visible)        (BaulWindowInfo *window,
                                        BaulView *view);
        void   (* close_window)       (BaulWindowInfo *window);
        GtkUIManager *     (* get_ui_manager)   (BaulWindowInfo *window);
    };

    GType                             baul_window_info_get_type                 (void);
    void                              baul_window_info_report_load_underway     (BaulWindowInfo                *window,
            BaulView                      *view);
    void                              baul_window_info_report_load_complete     (BaulWindowInfo                *window,
            BaulView                      *view);
    void                              baul_window_info_report_view_failed       (BaulWindowInfo                *window,
            BaulView                      *view);
    void                              baul_window_info_report_selection_changed (BaulWindowInfo                *window);
    BaulWindowSlotInfo *          baul_window_info_get_active_slot          (BaulWindowInfo                *window);
    BaulWindowSlotInfo *          baul_window_info_get_extra_slot           (BaulWindowInfo                *window);
    void                              baul_window_info_view_visible             (BaulWindowInfo                *window,
            BaulView                      *view);
    void                              baul_window_info_close                    (BaulWindowInfo                *window);
    void                              baul_window_info_push_status              (BaulWindowInfo                *window,
            const char                        *status);
    BaulWindowType                baul_window_info_get_window_type          (BaulWindowInfo                *window);
    char *                            baul_window_info_get_title                (BaulWindowInfo                *window);
    GList *                           baul_window_info_get_history              (BaulWindowInfo                *window);
    char *                            baul_window_info_get_current_location     (BaulWindowInfo                *window);
    int                               baul_window_info_get_selection_count      (BaulWindowInfo                *window);
    GList *                           baul_window_info_get_selection            (BaulWindowInfo                *window);
    BaulWindowShowHiddenFilesMode baul_window_info_get_hidden_files_mode    (BaulWindowInfo                *window);
    void                              baul_window_info_set_hidden_files_mode    (BaulWindowInfo                *window,
            BaulWindowShowHiddenFilesMode  mode);
    BaulWindowShowBackupFilesMode     baul_window_info_get_backup_files_mode    (BaulWindowInfo                *window);
    void                              baul_window_info_set_backup_files_mode    (BaulWindowInfo                *window,
            BaulWindowShowBackupFilesMode  mode);

    gboolean                          baul_window_info_get_initiated_unmount    (BaulWindowInfo                *window);
    void                              baul_window_info_set_initiated_unmount    (BaulWindowInfo                *window,
            gboolean initiated_unmount);
    GtkUIManager *                    baul_window_info_get_ui_manager           (BaulWindowInfo                *window);

#ifdef __cplusplus
}
#endif

#endif /* BAUL_WINDOW_INFO_H */
