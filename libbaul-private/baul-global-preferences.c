/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-global-preferences.c - Caja specific preference keys and
                                   functions.

   Copyright (C) 1999, 2000, 2001 Eazel, Inc.

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

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

#include <config.h>
#include <glib/gi18n.h>

#include <eel/eel-glib-extensions.h>
#include <eel/eel-gtk-extensions.h>
#include <eel/eel-stock-dialogs.h>
#include <eel/eel-string.h>

#include "baul-global-preferences.h"
#include "baul-file-utilities.h"
#include "baul-file.h"

GSettings *baul_preferences;
GSettings *baul_media_preferences;
GSettings *baul_window_state;
GSettings *baul_icon_view_preferences;
GSettings *baul_desktop_preferences;
GSettings *baul_tree_sidebar_preferences;
GSettings *baul_compact_view_preferences;
GSettings *baul_list_view_preferences;
GSettings *baul_extension_preferences;

GSettings *mate_background_preferences;
GSettings *mate_lockdown_preferences;

/*
 * Public functions
 */
char *
baul_global_preferences_get_default_folder_viewer_preference_as_iid (void)
{
    int preference_value;
    const char *viewer_iid;

    preference_value =
        g_settings_get_enum (baul_preferences, BAUL_PREFERENCES_DEFAULT_FOLDER_VIEWER);

    if (preference_value == BAUL_DEFAULT_FOLDER_VIEWER_LIST_VIEW)
    {
        viewer_iid = BAUL_LIST_VIEW_IID;
    }
    else if (preference_value == BAUL_DEFAULT_FOLDER_VIEWER_COMPACT_VIEW)
    {
        viewer_iid = BAUL_COMPACT_VIEW_IID;
    }
    else
    {
        viewer_iid = BAUL_ICON_VIEW_IID;
    }

    return g_strdup (viewer_iid);
}

void
baul_global_preferences_init (void)
{
    static gboolean initialized = FALSE;

    if (initialized)
    {
        return;
    }

    initialized = TRUE;

    baul_preferences = g_settings_new("org.mate.baul.preferences");
    baul_media_preferences = g_settings_new("org.mate.media-handling");
    baul_window_state = g_settings_new("org.mate.baul.window-state");
    baul_icon_view_preferences = g_settings_new("org.mate.baul.icon-view");
    baul_compact_view_preferences = g_settings_new("org.mate.baul.compact-view");
    baul_desktop_preferences = g_settings_new("org.mate.baul.desktop");
    baul_tree_sidebar_preferences = g_settings_new("org.mate.baul.sidebar-panels.tree");
    baul_list_view_preferences = g_settings_new("org.mate.baul.list-view");
    baul_extension_preferences = g_settings_new("org.mate.baul.extensions");

    mate_background_preferences = g_settings_new("org.mate.background");
    mate_lockdown_preferences = g_settings_new("org.mate.lockdown");
}
