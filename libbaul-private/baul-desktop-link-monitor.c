/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-desktop-link-monitor.c: singleton thatn manages the links

   Copyright (C) 2003 Red Hat, Inc.

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
#include <ctk/ctk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <string.h>

#include <eel/eel-debug.h>
#include <eel/eel-ctk-macros.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-vfs-extensions.h>
#include <eel/eel-stock-dialogs.h>

#include "baul-desktop-link-monitor.h"
#include "baul-desktop-link.h"
#include "baul-desktop-icon-file.h"
#include "baul-directory.h"
#include "baul-desktop-directory.h"
#include "baul-global-preferences.h"
#include "baul-trash-monitor.h"

struct BaulDesktopLinkMonitorDetails
{
    GVolumeMonitor *volume_monitor;
    BaulDirectory *desktop_dir;

    BaulDesktopLink *home_link;
    BaulDesktopLink *computer_link;
    BaulDesktopLink *trash_link;
    BaulDesktopLink *network_link;

    gulong mount_id;
    gulong unmount_id;
    gulong changed_id;

    GList *mount_links;
};


static void baul_desktop_link_monitor_init       (gpointer              object,
        gpointer              klass);
static void baul_desktop_link_monitor_class_init (gpointer              klass);

EEL_CLASS_BOILERPLATE (BaulDesktopLinkMonitor,
                       baul_desktop_link_monitor,
                       G_TYPE_OBJECT)

static BaulDesktopLinkMonitor *the_link_monitor = NULL;

static void
destroy_desktop_link_monitor (void)
{
    if (the_link_monitor != NULL)
    {
        g_object_unref (the_link_monitor);
    }
}

BaulDesktopLinkMonitor *
baul_desktop_link_monitor_get (void)
{
    if (the_link_monitor == NULL)
    {
        g_object_new (BAUL_TYPE_DESKTOP_LINK_MONITOR, NULL);
        eel_debug_call_at_shutdown (destroy_desktop_link_monitor);
    }
    return the_link_monitor;
}

static gboolean
volume_file_name_used (BaulDesktopLinkMonitor *monitor,
                       const char *name)
{
    GList *l;
    gboolean same;

    for (l = monitor->details->mount_links; l != NULL; l = l->next)
    {
        char *other_name;

        other_name = baul_desktop_link_get_file_name (l->data);
        same = strcmp (name, other_name) == 0;
        g_free (other_name);

        if (same)
        {
            return TRUE;
        }
    }

    return FALSE;
}

char *
baul_desktop_link_monitor_make_filename_unique (BaulDesktopLinkMonitor *monitor,
        const char *filename)
{
    char *unique_name;
    int i;

    i = 2;
    unique_name = g_strdup (filename);
    while (volume_file_name_used (monitor, unique_name))
    {
        g_free (unique_name);
        unique_name = g_strdup_printf ("%s.%d", filename, i++);
    }
    return unique_name;
}

static gboolean
has_mount (BaulDesktopLinkMonitor *monitor,
           GMount                     *mount)
{
    gboolean ret;
    GList *l;
    GMount *other_mount = NULL;

    ret = FALSE;

    for (l = monitor->details->mount_links; l != NULL; l = l->next)
    {
        other_mount = baul_desktop_link_get_mount (l->data);
        if (mount == other_mount)
        {
            g_object_unref (other_mount);
            ret = TRUE;
            break;
        }
        g_object_unref (other_mount);
    }

    return ret;
}

static void
create_mount_link (BaulDesktopLinkMonitor *monitor,
                   GMount *mount)
{
    if (has_mount (monitor, mount))
        return;

    if ((!g_mount_is_shadowed (mount)) &&
            g_settings_get_boolean (baul_desktop_preferences, BAUL_PREFERENCES_DESKTOP_VOLUMES_VISIBLE))
    {
        BaulDesktopLink *link;

        link = baul_desktop_link_new_from_mount (mount);
        monitor->details->mount_links = g_list_prepend (monitor->details->mount_links, link);
    }
}

static void
remove_mount_link (BaulDesktopLinkMonitor *monitor,
                   GMount *mount)
{
    GList *l;
    BaulDesktopLink *link;
    GMount *other_mount = NULL;

    link = NULL;
    for (l = monitor->details->mount_links; l != NULL; l = l->next)
    {
        other_mount = baul_desktop_link_get_mount (l->data);
        if (mount == other_mount)
        {
            g_object_unref (other_mount);
            link = l->data;
            break;
        }
        g_object_unref (other_mount);
    }

    if (link)
    {
        monitor->details->mount_links = g_list_remove (monitor->details->mount_links, link);
        g_object_unref (link);
    }
}



static void
mount_added_callback (GVolumeMonitor         *volume_monitor G_GNUC_UNUSED,
		      GMount                 *mount,
		      BaulDesktopLinkMonitor *monitor)
{
    create_mount_link (monitor, mount);
}


static void
mount_removed_callback (GVolumeMonitor         *volume_monitor G_GNUC_UNUSED,
			GMount                 *mount,
			BaulDesktopLinkMonitor *monitor)
{
    remove_mount_link (monitor, mount);
}

static void
mount_changed_callback (GVolumeMonitor         *volume_monitor G_GNUC_UNUSED,
			GMount                 *mount,
			BaulDesktopLinkMonitor *monitor)
{
    /* TODO: update the mount with other details */

    /* remove a mount if it goes into the shadows */
    if (g_mount_is_shadowed (mount) && has_mount (monitor, mount))
    {
        remove_mount_link (monitor, mount);
    }
}

static void
update_link_visibility (BaulDesktopLinkMonitor *monitor G_GNUC_UNUSED,
			BaulDesktopLink       **link_ref,
			BaulDesktopLinkType     link_type,
			const char             *preference_key)
{
    if (g_settings_get_boolean (baul_desktop_preferences, preference_key))
    {
        if (*link_ref == NULL)
        {
            *link_ref = baul_desktop_link_new (link_type);
        }
    }
    else
    {
        if (*link_ref != NULL)
        {
            g_object_unref (*link_ref);
            *link_ref = NULL;
        }
    }
}

static void
desktop_home_visible_changed (gpointer callback_data)
{
    BaulDesktopLinkMonitor *monitor;

    monitor = BAUL_DESKTOP_LINK_MONITOR (callback_data);

    update_link_visibility (BAUL_DESKTOP_LINK_MONITOR (monitor),
                            &monitor->details->home_link,
                            BAUL_DESKTOP_LINK_HOME,
                            BAUL_PREFERENCES_DESKTOP_HOME_VISIBLE);
}

static void
desktop_computer_visible_changed (gpointer callback_data)
{
    BaulDesktopLinkMonitor *monitor;

    monitor = BAUL_DESKTOP_LINK_MONITOR (callback_data);

    update_link_visibility (BAUL_DESKTOP_LINK_MONITOR (callback_data),
                            &monitor->details->computer_link,
                            BAUL_DESKTOP_LINK_COMPUTER,
                            BAUL_PREFERENCES_DESKTOP_COMPUTER_VISIBLE);
}

static void
desktop_trash_visible_changed (gpointer callback_data)
{
    BaulDesktopLinkMonitor *monitor;

    monitor = BAUL_DESKTOP_LINK_MONITOR (callback_data);

    update_link_visibility (BAUL_DESKTOP_LINK_MONITOR (callback_data),
                            &monitor->details->trash_link,
                            BAUL_DESKTOP_LINK_TRASH,
                            BAUL_PREFERENCES_DESKTOP_TRASH_VISIBLE);
}

static void
desktop_network_visible_changed (gpointer callback_data)
{
    BaulDesktopLinkMonitor *monitor;

    monitor = BAUL_DESKTOP_LINK_MONITOR (callback_data);

    update_link_visibility (BAUL_DESKTOP_LINK_MONITOR (callback_data),
                            &monitor->details->network_link,
                            BAUL_DESKTOP_LINK_NETWORK,
                            BAUL_PREFERENCES_DESKTOP_NETWORK_VISIBLE);
}

static void
desktop_volumes_visible_changed (gpointer callback_data)
{
    BaulDesktopLinkMonitor *monitor;
    GList *l, *mounts;

    monitor = BAUL_DESKTOP_LINK_MONITOR (callback_data);

    if (g_settings_get_boolean (baul_desktop_preferences, BAUL_PREFERENCES_DESKTOP_VOLUMES_VISIBLE))
    {
        if (monitor->details->mount_links == NULL)
        {
            mounts = g_volume_monitor_get_mounts (monitor->details->volume_monitor);
            for (l = mounts; l != NULL; l = l->next)
            {
                create_mount_link (monitor, l->data);
                g_object_unref (l->data);
            }
            g_list_free (mounts);
        }
    }
    else
    {
        g_list_foreach (monitor->details->mount_links, (GFunc)g_object_unref, NULL);
        g_list_free (monitor->details->mount_links);
        monitor->details->mount_links = NULL;
    }
}

static void
create_link_and_add_preference (BaulDesktopLink   **link_ref,
                                BaulDesktopLinkType link_type,
                                const char         *preference_key,
                                GCallback           callback,
                                gpointer            callback_data)
{
    gchar *detailed_signal;

    if (g_settings_get_boolean (baul_desktop_preferences, preference_key))
    {
        *link_ref = baul_desktop_link_new (link_type);
    }

    detailed_signal = g_strconcat ("changed::", preference_key, NULL);
    g_signal_connect_swapped (baul_desktop_preferences,
                              detailed_signal,
                              callback, callback_data);
    g_free (detailed_signal);
}

static void
baul_desktop_link_monitor_init (gpointer object,
				gpointer klass G_GNUC_UNUSED)
{
    BaulDesktopLinkMonitor *monitor;
    GList *l, *mounts;
    GMount *mount = NULL;

    monitor = BAUL_DESKTOP_LINK_MONITOR (object);

    the_link_monitor = monitor;

    monitor->details = g_new0 (BaulDesktopLinkMonitorDetails, 1);

    monitor->details->volume_monitor = g_volume_monitor_get ();

    /* We keep around a ref to the desktop dir */
    monitor->details->desktop_dir = baul_directory_get_by_uri (EEL_DESKTOP_URI);

    /* Default links */

    create_link_and_add_preference (&monitor->details->home_link,
                                    BAUL_DESKTOP_LINK_HOME,
                                    BAUL_PREFERENCES_DESKTOP_HOME_VISIBLE,
                                    G_CALLBACK (desktop_home_visible_changed),
                                    monitor);

    create_link_and_add_preference (&monitor->details->computer_link,
                                    BAUL_DESKTOP_LINK_COMPUTER,
                                    BAUL_PREFERENCES_DESKTOP_COMPUTER_VISIBLE,
                                    G_CALLBACK (desktop_computer_visible_changed),
                                    monitor);

    create_link_and_add_preference (&monitor->details->trash_link,
                                    BAUL_DESKTOP_LINK_TRASH,
                                    BAUL_PREFERENCES_DESKTOP_TRASH_VISIBLE,
                                    G_CALLBACK (desktop_trash_visible_changed),
                                    monitor);

    create_link_and_add_preference (&monitor->details->network_link,
                                    BAUL_DESKTOP_LINK_NETWORK,
                                    BAUL_PREFERENCES_DESKTOP_NETWORK_VISIBLE,
                                    G_CALLBACK (desktop_network_visible_changed),
                                    monitor);

    /* Mount links */

    mounts = g_volume_monitor_get_mounts (monitor->details->volume_monitor);
    for (l = mounts; l != NULL; l = l->next)
    {
        mount = l->data;
        create_mount_link (monitor, mount);
        g_object_unref (mount);
    }
    g_list_free (mounts);

    g_signal_connect_swapped (baul_desktop_preferences,
                              "changed::" BAUL_PREFERENCES_DESKTOP_VOLUMES_VISIBLE,
                              G_CALLBACK (desktop_volumes_visible_changed),
                              monitor);

    monitor->details->mount_id =
        g_signal_connect_object (monitor->details->volume_monitor, "mount_added",
                                 G_CALLBACK (mount_added_callback), monitor, 0);
    monitor->details->unmount_id =
        g_signal_connect_object (monitor->details->volume_monitor, "mount_removed",
                                 G_CALLBACK (mount_removed_callback), monitor, 0);
    monitor->details->changed_id =
        g_signal_connect_object (monitor->details->volume_monitor, "mount_changed",
                                 G_CALLBACK (mount_changed_callback), monitor, 0);

}

static void
remove_link_and_preference (BaulDesktopLink **link_ref,
			    const char       *preference_key G_GNUC_UNUSED,
			    GCallback         callback,
			    gpointer          callback_data)
{
    if (*link_ref != NULL)
    {
        g_object_unref (*link_ref);
        *link_ref = NULL;
    }

    g_signal_handlers_disconnect_by_func (baul_desktop_preferences,
                                          callback, callback_data);
}

static void
desktop_link_monitor_finalize (GObject *object)
{
    BaulDesktopLinkMonitor *monitor;

    monitor = BAUL_DESKTOP_LINK_MONITOR (object);

    g_object_unref (monitor->details->volume_monitor);

    /* Default links */

    remove_link_and_preference (&monitor->details->home_link,
                                BAUL_PREFERENCES_DESKTOP_HOME_VISIBLE,
                                G_CALLBACK (desktop_home_visible_changed),
                                monitor);

    remove_link_and_preference (&monitor->details->computer_link,
                                BAUL_PREFERENCES_DESKTOP_COMPUTER_VISIBLE,
                                G_CALLBACK (desktop_computer_visible_changed),
                                monitor);

    remove_link_and_preference (&monitor->details->trash_link,
                                BAUL_PREFERENCES_DESKTOP_TRASH_VISIBLE,
                                G_CALLBACK (desktop_trash_visible_changed),
                                monitor);

    remove_link_and_preference (&monitor->details->network_link,
                                BAUL_PREFERENCES_DESKTOP_NETWORK_VISIBLE,
                                G_CALLBACK (desktop_network_visible_changed),
                                monitor);

    /* Mounts */

    g_list_foreach (monitor->details->mount_links, (GFunc)g_object_unref, NULL);
    g_list_free (monitor->details->mount_links);
    monitor->details->mount_links = NULL;

    baul_directory_unref (monitor->details->desktop_dir);
    monitor->details->desktop_dir = NULL;

    g_signal_handlers_disconnect_by_func (baul_desktop_preferences,
                                          desktop_volumes_visible_changed,
                                          monitor);

/*  These sources are already gone,  this just causes errors
    if (monitor->details->mount_id != 0)
    {
        g_source_remove (monitor->details->mount_id);
    }
    if (monitor->details->unmount_id != 0)
    {
        g_source_remove (monitor->details->unmount_id);
    }
    if (monitor->details->changed_id != 0)
    {
        g_source_remove (monitor->details->changed_id);
    }
*/
    g_free (monitor->details);

    EEL_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
baul_desktop_link_monitor_class_init (gpointer klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = desktop_link_monitor_finalize;

}
