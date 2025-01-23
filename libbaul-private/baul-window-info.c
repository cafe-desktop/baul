/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-window-info.c: Interface for baul window

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

#include <config.h>
#include "baul-window-info.h"

enum
{
    LOADING_URI,
    SELECTION_CHANGED,
    TITLE_CHANGED,
    HIDDEN_FILES_MODE_CHANGED,
    BACKUP_FILES_MODE_CHANGED,
    LAST_SIGNAL
};

static guint baul_window_info_signals[LAST_SIGNAL] = { 0 };

static void
baul_window_info_base_init (gpointer g_class G_GNUC_UNUSED)
{
    static gboolean initialized = FALSE;

    if (! initialized)
    {
        baul_window_info_signals[LOADING_URI] =
            g_signal_new ("loading_uri",
                          BAUL_TYPE_WINDOW_INFO,
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (BaulWindowInfoIface, loading_uri),
                          NULL, NULL,
                          g_cclosure_marshal_VOID__STRING,
                          G_TYPE_NONE, 1,
                          G_TYPE_STRING);

        baul_window_info_signals[SELECTION_CHANGED] =
            g_signal_new ("selection_changed",
                          BAUL_TYPE_WINDOW_INFO,
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (BaulWindowInfoIface, selection_changed),
                          NULL, NULL,
                          g_cclosure_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

        baul_window_info_signals[TITLE_CHANGED] =
            g_signal_new ("title_changed",
                          BAUL_TYPE_WINDOW_INFO,
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (BaulWindowInfoIface, title_changed),
                          NULL, NULL,
                          g_cclosure_marshal_VOID__STRING,
                          G_TYPE_NONE, 1,
                          G_TYPE_STRING);

        baul_window_info_signals[HIDDEN_FILES_MODE_CHANGED] =
            g_signal_new ("hidden_files_mode_changed",
                          BAUL_TYPE_WINDOW_INFO,
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (BaulWindowInfoIface, hidden_files_mode_changed),
                          NULL, NULL,
                          g_cclosure_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

        baul_window_info_signals[BACKUP_FILES_MODE_CHANGED] =
            g_signal_new ("backup_files_mode_changed",
                          BAUL_TYPE_WINDOW_INFO,
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (BaulWindowInfoIface, backup_files_mode_changed),
                          NULL, NULL,
                          g_cclosure_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

        initialized = TRUE;
    }
}

GType
baul_window_info_get_type (void)
{
    static GType type = 0;

    if (!type)
    {
        const GTypeInfo info =
        {
            sizeof (BaulWindowInfoIface),
            baul_window_info_base_init,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            NULL
        };

        type = g_type_register_static (G_TYPE_INTERFACE,
                                       "BaulWindowInfo",
                                       &info, 0);
        g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }

    return type;
}

void
baul_window_info_report_load_underway (BaulWindowInfo      *window,
                                       BaulView            *view)
{
    g_return_if_fail (BAUL_IS_WINDOW_INFO (window));
    g_return_if_fail (BAUL_IS_VIEW (view));

    (* BAUL_WINDOW_INFO_GET_IFACE (window)->report_load_underway) (window,
            view);
}

void
baul_window_info_report_load_complete (BaulWindowInfo      *window,
                                       BaulView            *view)
{
    g_return_if_fail (BAUL_IS_WINDOW_INFO (window));
    g_return_if_fail (BAUL_IS_VIEW (view));

    (* BAUL_WINDOW_INFO_GET_IFACE (window)->report_load_complete) (window,
            view);
}

void
baul_window_info_report_view_failed (BaulWindowInfo      *window,
                                     BaulView            *view)
{
    g_return_if_fail (BAUL_IS_WINDOW_INFO (window));
    g_return_if_fail (BAUL_IS_VIEW (view));

    (* BAUL_WINDOW_INFO_GET_IFACE (window)->report_view_failed) (window,
            view);
}

void
baul_window_info_report_selection_changed (BaulWindowInfo      *window)
{
    g_return_if_fail (BAUL_IS_WINDOW_INFO (window));

    (* BAUL_WINDOW_INFO_GET_IFACE (window)->report_selection_changed) (window);
}

void
baul_window_info_view_visible (BaulWindowInfo      *window,
                               BaulView            *view)
{
    g_return_if_fail (BAUL_IS_WINDOW_INFO (window));

    (* BAUL_WINDOW_INFO_GET_IFACE (window)->view_visible) (window, view);
}

void
baul_window_info_close (BaulWindowInfo      *window)
{
    g_return_if_fail (BAUL_IS_WINDOW_INFO (window));

    (* BAUL_WINDOW_INFO_GET_IFACE (window)->close_window) (window);
}

void
baul_window_info_push_status (BaulWindowInfo      *window,
                              const char              *status)
{
    g_return_if_fail (BAUL_IS_WINDOW_INFO (window));

    (* BAUL_WINDOW_INFO_GET_IFACE (window)->push_status) (window,
            status);
}

BaulWindowType
baul_window_info_get_window_type (BaulWindowInfo *window)
{
    g_return_val_if_fail (BAUL_IS_WINDOW_INFO (window), BAUL_WINDOW_SPATIAL);

    return (* BAUL_WINDOW_INFO_GET_IFACE (window)->get_window_type) (window);
}

char *
baul_window_info_get_title (BaulWindowInfo *window)
{
    g_return_val_if_fail (BAUL_IS_WINDOW_INFO (window), NULL);

    return (* BAUL_WINDOW_INFO_GET_IFACE (window)->get_title) (window);
}

GList *
baul_window_info_get_history (BaulWindowInfo *window)
{
    g_return_val_if_fail (BAUL_IS_WINDOW_INFO (window), NULL);

    return (* BAUL_WINDOW_INFO_GET_IFACE (window)->get_history) (window);
}

char *
baul_window_info_get_current_location (BaulWindowInfo *window)
{
    g_return_val_if_fail (BAUL_IS_WINDOW_INFO (window), NULL);

    return (* BAUL_WINDOW_INFO_GET_IFACE (window)->get_current_location) (window);
}

int
baul_window_info_get_selection_count (BaulWindowInfo *window)
{
    g_return_val_if_fail (BAUL_IS_WINDOW_INFO (window), 0);

    return (* BAUL_WINDOW_INFO_GET_IFACE (window)->get_selection_count) (window);
}

GList *
baul_window_info_get_selection (BaulWindowInfo *window)
{
    g_return_val_if_fail (BAUL_IS_WINDOW_INFO (window), NULL);

    return (* BAUL_WINDOW_INFO_GET_IFACE (window)->get_selection) (window);
}

BaulWindowShowHiddenFilesMode
baul_window_info_get_hidden_files_mode (BaulWindowInfo *window)
{
    g_return_val_if_fail (BAUL_IS_WINDOW_INFO (window), BAUL_WINDOW_SHOW_HIDDEN_FILES_DEFAULT);

    return (* BAUL_WINDOW_INFO_GET_IFACE (window)->get_hidden_files_mode) (window);
}

void
baul_window_info_set_hidden_files_mode (BaulWindowInfo *window,
                                        BaulWindowShowHiddenFilesMode  mode)
{
    g_return_if_fail (BAUL_IS_WINDOW_INFO (window));

    (* BAUL_WINDOW_INFO_GET_IFACE (window)->set_hidden_files_mode) (window,
            mode);
}

BaulWindowShowBackupFilesMode
baul_window_info_get_backup_files_mode (BaulWindowInfo *window)
{
    g_return_val_if_fail (BAUL_IS_WINDOW_INFO (window), BAUL_WINDOW_SHOW_BACKUP_FILES_DEFAULT);

    return (* BAUL_WINDOW_INFO_GET_IFACE (window)->get_backup_files_mode) (window);
}

void
baul_window_info_set_backup_files_mode (BaulWindowInfo *window,
                                        BaulWindowShowBackupFilesMode  mode)
{
    g_return_if_fail (BAUL_IS_WINDOW_INFO (window));

    (* BAUL_WINDOW_INFO_GET_IFACE (window)->set_backup_files_mode) (window,
            mode);
}

CtkUIManager *
baul_window_info_get_ui_manager (BaulWindowInfo *window)
{
    g_return_val_if_fail (BAUL_IS_WINDOW_INFO (window), NULL);

    return (* BAUL_WINDOW_INFO_GET_IFACE (window)->get_ui_manager) (window);
}

BaulWindowSlotInfo *
baul_window_info_get_active_slot (BaulWindowInfo *window)
{
    g_return_val_if_fail (BAUL_IS_WINDOW_INFO (window), NULL);

    return (* BAUL_WINDOW_INFO_GET_IFACE (window)->get_active_slot) (window);
}

BaulWindowSlotInfo *
baul_window_info_get_extra_slot (BaulWindowInfo *window)
{
    g_return_val_if_fail (BAUL_IS_WINDOW_INFO (window), NULL);

    return (* BAUL_WINDOW_INFO_GET_IFACE (window)->get_extra_slot) (window);
}

gboolean
baul_window_info_get_initiated_unmount (BaulWindowInfo *window)
{
    g_return_val_if_fail (BAUL_IS_WINDOW_INFO (window), FALSE);

    return (* BAUL_WINDOW_INFO_GET_IFACE (window)->get_initiated_unmount) (window);
}

void
baul_window_info_set_initiated_unmount (BaulWindowInfo *window, gboolean initiated_unmount)
{
    g_return_if_fail (BAUL_IS_WINDOW_INFO (window));

    (* BAUL_WINDOW_INFO_GET_IFACE (window)->set_initiated_unmount) (window,
            initiated_unmount);

}
