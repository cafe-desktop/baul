/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-file-utilities.c - implementation of file manipulation routines.

   Copyright (C) 1999, 2000, 2001 Eazel, Inc.

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

   Authors: John Sullivan <sullivan@eazel.com>
*/

#include <config.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <stdlib.h>

#include <eel/eel-glib-extensions.h>
#include <eel/eel-stock-dialogs.h>
#include <eel/eel-string.h>
#include <eel/eel-debug.h>

#include "baul-file-utilities.h"
#include "baul-global-preferences.h"
#include "baul-lib-self-check-functions.h"
#include "baul-metadata.h"
#include "baul-file.h"
#include "baul-file-operations.h"
#include "baul-search-directory.h"
#include "baul-signaller.h"

#define DEFAULT_BAUL_DIRECTORY_MODE (0755)

#define DESKTOP_DIRECTORY_NAME "Desktop"
#define DEFAULT_DESKTOP_DIRECTORY_MODE (0755)

static void update_xdg_dir_cache (void);
static void schedule_user_dirs_changed (void);
static void desktop_dir_changed (void);

char *
baul_compute_title_for_location (GFile *location)
{
    char *title;

    /* TODO-gio: This doesn't really work all that great if the
       info about the file isn't known atm... */

    title = NULL;
    if (location)
    {
        BaulFile *file;

        file = baul_file_get (location);
        title = baul_file_get_description (file);
        if (title == NULL)
        {
            title = baul_file_get_display_name (file);
        }
        baul_file_unref (file);
    }

    if (title == NULL)
    {
        title = g_strdup ("");
    }

    return title;
}


/**
 * baul_get_user_directory:
 *
 * Get the path for the directory containing baul settings.
 *
 * Return value: the directory path.
 **/
char* baul_get_user_directory(void)
{
	/* FIXME bugzilla.gnome.org 41286:
	 * How should we handle the case where this mkdir fails?
	 * Note that baul_application_startup will refuse to launch if this
	 * directory doesn't get created, so that case is OK. But the directory
	 * could be deleted after Baul was launched, and perhaps
	 * there is some bad side-effect of not handling that case.
	 * <<<
	 * Si alguien tiene tiempo, puede enviar este codigo a Nautilus.
	 * Obviamente, con los comentarios traducidos al Inglés.
	 */
	char* user_directory = g_build_filename(g_get_user_config_dir(), "baul", NULL);
	/* Se necesita que esta dirección sea una carpeta, con los permisos
	 * DEFAULT_BAUL_DIRECTORY_MODE. Pero si es un archivo, el programa intentará
	 * eliminar el archivo silenciosamente. */
	if (g_file_test(user_directory, G_FILE_TEST_IS_DIR) == FALSE ||
		g_access(user_directory, DEFAULT_BAUL_DIRECTORY_MODE) == -1)
	{
		/* Se puede obtener un enlace simbolico a una carpeta */
		if (g_file_test(user_directory, G_FILE_TEST_IS_SYMLINK) == TRUE)
		{
			/* intentaremos saber si el enlace es una carpeta, y tiene los
			 * permisos adecuados */
			char* link = g_file_read_link(user_directory, NULL);

			if (link)
			{
				/* Si el enlace no es un directorio, o si falla al hacer chmod,
				 * se borra el enlace y se crea la carpeta */
				if (g_file_test(link, G_FILE_TEST_IS_DIR) != TRUE ||
					g_chmod(link, DEFAULT_BAUL_DIRECTORY_MODE) != 0)
				{
					/* podemos borrar el enlace y crear la carpeta */
					g_unlink(user_directory);
					g_mkdir(user_directory, DEFAULT_BAUL_DIRECTORY_MODE);
				}

				g_free(link);
			}
		}
		else if (g_file_test(user_directory, G_FILE_TEST_IS_DIR) == TRUE)
		{
			g_chmod(user_directory, DEFAULT_BAUL_DIRECTORY_MODE);
		}
		else if (g_file_test(user_directory, G_FILE_TEST_EXISTS) == TRUE)
		{
			/* podemos borrar el enlace y crear la carpeta */
			g_unlink(user_directory);
			g_mkdir(user_directory, DEFAULT_BAUL_DIRECTORY_MODE);
		}
		else
		{
			/* Si no existe ningun archivo, se crea la carpeta */
			g_mkdir_with_parents(user_directory, DEFAULT_BAUL_DIRECTORY_MODE);
		}

		/* Faltan permisos */
		if (g_chmod(user_directory, DEFAULT_BAUL_DIRECTORY_MODE) != 0)
		{
			CtkWidget* dialog = ctk_message_dialog_new(
				NULL,
				CTK_DIALOG_DESTROY_WITH_PARENT,
				CTK_MESSAGE_ERROR,
				CTK_BUTTONS_CLOSE,
				"The path for the directory containing baul settings need read and write permissions: %s",
				user_directory);

			ctk_dialog_run(CTK_DIALOG(dialog));
			ctk_widget_destroy(dialog);

			exit(0);
		}
	}

	return user_directory;
}

/**
 * baul_get_accel_map_file:
 *
 * Get the path for the filename containing baul accelerator map.
 * The filename need not exist.
 *
 * Return value: the filename path
 **/
char* baul_get_accel_map_file(void)
{
	return g_build_filename (g_get_user_config_dir (), "baul", "accels", NULL);
}

typedef struct {
	char*type;
	char*path;
	BaulFile* file;
} XdgDirEntry;


static XdgDirEntry *
parse_xdg_dirs (const char *config_file)
{
    GArray *array;
    char *config_file_free = NULL;
    XdgDirEntry dir;
    char *data;
    char **lines;
    char *p;
    char *unescaped;
    gboolean relative;

    array = g_array_new (TRUE, TRUE, sizeof (XdgDirEntry));

    if (config_file == NULL)
    {
        config_file_free = g_build_filename (g_get_user_config_dir (),
                                             "user-dirs.dirs", NULL);
        config_file = (const char *)config_file_free;
    }

    if (g_file_get_contents (config_file, &data, NULL, NULL))
    {
        int i;

        lines = g_strsplit (data, "\n", 0);
        g_free (data);
        for (i = 0; lines[i] != NULL; i++)
        {
            char *d;
            char *type_start, *type_end;
            char *value;

            p = lines[i];
            while (g_ascii_isspace (*p))
                p++;

            if (*p == '#')
                continue;

            value = strchr (p, '=');
            if (value == NULL)
                continue;
            *value++ = 0;

            g_strchug (g_strchomp (p));
            if (!g_str_has_prefix (p, "XDG_"))
                continue;
            if (!g_str_has_suffix (p, "_DIR"))
                continue;
            type_start = p + 4;
            type_end = p + strlen (p) - 4;

            while (g_ascii_isspace (*value))
                value++;

            if (*value != '"')
                continue;
            value++;

            relative = FALSE;
            if (g_str_has_prefix (value, "$HOME"))
            {
                relative = TRUE;
                value += 5;
                while (*value == '/')
                    value++;
            }
            else if (*value != '/')
                continue;

            d = unescaped = g_malloc (strlen (value) + 1);
            while (*value && *value != '"')
            {
                if ((*value == '\\') && (*(value + 1) != 0))
                    value++;
                *d++ = *value++;
            }
            *d = 0;

            *type_end = 0;
            dir.type = g_strdup (type_start);
            if (relative)
            {
                dir.path = g_build_filename (g_get_home_dir (), unescaped, NULL);
                g_free (unescaped);
            }
            else
                dir.path = unescaped;

            g_array_append_val (array, dir);
        }

        g_strfreev (lines);
    }

    g_free (config_file_free);

    return (XdgDirEntry *) (gpointer) g_array_free (array, FALSE);
}

static XdgDirEntry *cached_xdg_dirs = NULL;
static GFileMonitor *cached_xdg_dirs_monitor = NULL;

static void
xdg_dir_changed (BaulFile *file,
                 XdgDirEntry *dir)
{
    GFile *location, *dir_location;
    char *path;

    location = baul_file_get_location (file);
    dir_location = g_file_new_for_path (dir->path);
    if (!g_file_equal (location, dir_location))
    {
        path = g_file_get_path (location);

        if (path)
        {
            char *argv[5];
            int i;

            g_free (dir->path);
            dir->path = path;

            i = 0;
            argv[i++] = "xdg-user-dirs-update";
            argv[i++] = "--set";
            argv[i++] = dir->type;
            argv[i++] = dir->path;
            argv[i++] = NULL;

            /* We do this sync, to avoid possible race-conditions
               if multiple dirs change at the same time. Its
               blocking the main thread, but these updates should
               be very rare and very fast. */
            g_spawn_sync (NULL,
                          argv, NULL,
                          G_SPAWN_SEARCH_PATH |
                          G_SPAWN_STDOUT_TO_DEV_NULL |
                          G_SPAWN_STDERR_TO_DEV_NULL,
                          NULL, NULL,
                          NULL, NULL, NULL, NULL);
            g_reload_user_special_dirs_cache ();
            schedule_user_dirs_changed ();
            desktop_dir_changed ();
            /* Icon might have changed */
            baul_file_invalidate_attributes (file, BAUL_FILE_ATTRIBUTE_INFO);
        }
    }
    g_object_unref (location);
    g_object_unref (dir_location);
}

static void
xdg_dir_cache_changed_cb (GFileMonitor     *monitor G_GNUC_UNUSED,
			  GFile            *file G_GNUC_UNUSED,
			  GFile            *other_file G_GNUC_UNUSED,
			  GFileMonitorEvent event_type)
{
    if (event_type == G_FILE_MONITOR_EVENT_CHANGED ||
            event_type == G_FILE_MONITOR_EVENT_CREATED)
    {
        update_xdg_dir_cache ();
    }
}

static int user_dirs_changed_tag = 0;

static gboolean
emit_user_dirs_changed_idle (gpointer data G_GNUC_UNUSED)
{
    g_signal_emit_by_name (baul_signaller_get_current (),
                           "user_dirs_changed");
    user_dirs_changed_tag = 0;
    return FALSE;
}

static void
schedule_user_dirs_changed (void)
{
    if (user_dirs_changed_tag == 0)
    {
        user_dirs_changed_tag = g_idle_add (emit_user_dirs_changed_idle, NULL);
    }
}

static void
unschedule_user_dirs_changed (void)
{
    if (user_dirs_changed_tag != 0)
    {
        g_source_remove (user_dirs_changed_tag);
        user_dirs_changed_tag = 0;
    }
}

static void
free_xdg_dir_cache (void)
{
    if (cached_xdg_dirs != NULL)
    {
        int i;

        for (i = 0; cached_xdg_dirs[i].type != NULL; i++)
        {
            if (cached_xdg_dirs[i].file != NULL)
            {
                baul_file_monitor_remove (cached_xdg_dirs[i].file,
                                          &cached_xdg_dirs[i]);
                g_signal_handlers_disconnect_by_func (cached_xdg_dirs[i].file,
                                                      G_CALLBACK (xdg_dir_changed),
                                                      &cached_xdg_dirs[i]);
                baul_file_unref (cached_xdg_dirs[i].file);
            }
            g_free (cached_xdg_dirs[i].type);
            g_free (cached_xdg_dirs[i].path);
        }
        g_free (cached_xdg_dirs);
    }
}

static void
destroy_xdg_dir_cache (void)
{
    free_xdg_dir_cache ();
    unschedule_user_dirs_changed ();
    desktop_dir_changed ();

    if (cached_xdg_dirs_monitor != NULL)
    {
        g_object_unref  (cached_xdg_dirs_monitor);
        cached_xdg_dirs_monitor = NULL;
    }
}

static void
update_xdg_dir_cache (void)
{
    char *uri;
    int i;

    free_xdg_dir_cache ();
    g_reload_user_special_dirs_cache ();
    schedule_user_dirs_changed ();
    desktop_dir_changed ();

    cached_xdg_dirs = parse_xdg_dirs (NULL);

    for (i = 0 ; cached_xdg_dirs[i].type != NULL; i++)
    {
        cached_xdg_dirs[i].file = NULL;
        if (strcmp (cached_xdg_dirs[i].path, g_get_home_dir ()) != 0)
        {
            uri = g_filename_to_uri (cached_xdg_dirs[i].path, NULL, NULL);
            cached_xdg_dirs[i].file = baul_file_get_by_uri (uri);
            baul_file_monitor_add (cached_xdg_dirs[i].file,
                                   &cached_xdg_dirs[i],
                                   BAUL_FILE_ATTRIBUTE_INFO);
            g_signal_connect (cached_xdg_dirs[i].file,
                              "changed", G_CALLBACK (xdg_dir_changed), &cached_xdg_dirs[i]);
            g_free (uri);
        }
    }

    if (cached_xdg_dirs_monitor == NULL)
    {
        GFile *file;
        char *config_file;

        config_file = g_build_filename (g_get_user_config_dir (),
                                        "user-dirs.dirs", NULL);
        file = g_file_new_for_path (config_file);
        cached_xdg_dirs_monitor = g_file_monitor_file (file, 0, NULL, NULL);
        g_signal_connect (cached_xdg_dirs_monitor, "changed",
                          G_CALLBACK (xdg_dir_cache_changed_cb), NULL);
        g_object_unref (file);
        g_free (config_file);

        eel_debug_call_at_shutdown (destroy_xdg_dir_cache);
    }
}

char *
baul_get_xdg_dir (const char *type)
{
    int i;

    if (cached_xdg_dirs == NULL)
    {
        update_xdg_dir_cache ();
    }

    for (i = 0 ; cached_xdg_dirs != NULL && cached_xdg_dirs[i].type != NULL; i++)
    {
        if (strcmp (cached_xdg_dirs[i].type, type) == 0)
        {
            return g_strdup (cached_xdg_dirs[i].path);
        }
    }
    if (strcmp ("DESKTOP", type) == 0)
    {
        return g_build_filename (g_get_home_dir (), DESKTOP_DIRECTORY_NAME, NULL);
    }
    if (strcmp ("TEMPLATES", type) == 0)
    {
        return g_build_filename (g_get_home_dir (), "Templates", NULL);
    }

    return g_strdup (g_get_home_dir ());
}

static char *
get_desktop_path (void)
{
    if (g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_DESKTOP_IS_HOME_DIR))
    {
        return g_strdup (g_get_home_dir());
    }
    else
    {
        return baul_get_xdg_dir ("DESKTOP");
    }
}

/**
 * baul_get_desktop_directory:
 *
 * Get the path for the directory containing files on the desktop.
 *
 * Return value: the directory path.
 **/
char *
baul_get_desktop_directory (void)
{
    char *desktop_directory;

    desktop_directory = get_desktop_path ();

    /* Don't try to create a home directory */
    if (!g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_DESKTOP_IS_HOME_DIR))
    {
        if (!g_file_test (desktop_directory, G_FILE_TEST_EXISTS))
        {
            g_mkdir (desktop_directory, DEFAULT_DESKTOP_DIRECTORY_MODE);
            /* FIXME bugzilla.gnome.org 41286:
             * How should we handle the case where this mkdir fails?
             * Note that baul_application_startup will refuse to launch if this
             * directory doesn't get created, so that case is OK. But the directory
             * could be deleted after Baul was launched, and perhaps
             * there is some bad side-effect of not handling that case.
             */
        }
    }

    return desktop_directory;
}

GFile *
baul_get_desktop_location (void)
{
    char *desktop_directory;
    GFile *res;

    desktop_directory = get_desktop_path ();

    res = g_file_new_for_path (desktop_directory);
    g_free (desktop_directory);
    return res;
}


/**
 * baul_get_desktop_directory_uri:
 *
 * Get the uri for the directory containing files on the desktop.
 *
 * Return value: the directory path.
 **/
char *
baul_get_desktop_directory_uri (void)
{
    char *desktop_path;
    char *desktop_uri;

    desktop_path = baul_get_desktop_directory ();
    desktop_uri = g_filename_to_uri (desktop_path, NULL, NULL);
    g_free (desktop_path);

    return desktop_uri;
}

char *
baul_get_home_directory_uri (void)
{
    return  g_filename_to_uri (g_get_home_dir (), NULL, NULL);
}


gboolean
baul_should_use_templates_directory (void)
{
    char *dir;
    gboolean res;

    dir = baul_get_xdg_dir ("TEMPLATES");
    res = strcmp (dir, g_get_home_dir ()) != 0;
    g_free (dir);
    return res;
}

char *
baul_get_templates_directory (void)
{
    return baul_get_xdg_dir ("TEMPLATES");
}

void
baul_create_templates_directory (void)
{
    char *dir;

    dir = baul_get_templates_directory ();
    if (!g_file_test (dir, G_FILE_TEST_EXISTS))
    {
        g_mkdir (dir, DEFAULT_BAUL_DIRECTORY_MODE);
    }
    g_free (dir);
}

char *
baul_get_templates_directory_uri (void)
{
    char *directory, *uri;

    directory = baul_get_templates_directory ();
    uri = g_filename_to_uri (directory, NULL, NULL);
    g_free (directory);
    return uri;
}

/* These need to be reset to NULL when desktop_is_home_dir changes */
static GFile *desktop_dir = NULL;
static GFile *desktop_dir_dir = NULL;
static char *desktop_dir_filename = NULL;
static gboolean desktop_dir_changed_callback_installed = FALSE;


static void
desktop_dir_changed (void)
{
    if (desktop_dir)
    {
        g_object_unref (desktop_dir);
    }
    if (desktop_dir_dir)
    {
        g_object_unref (desktop_dir_dir);
    }
    g_free (desktop_dir_filename);
    desktop_dir = NULL;
    desktop_dir_dir = NULL;
    desktop_dir_filename = NULL;
}

static void
desktop_dir_changed_callback (gpointer callback_data G_GNUC_UNUSED)
{
    desktop_dir_changed ();
}

static void
update_desktop_dir (void)
{
    char *path;
    char *dirname;

    path = get_desktop_path ();
    desktop_dir = g_file_new_for_path (path);

    dirname = g_path_get_dirname (path);
    desktop_dir_dir = g_file_new_for_path (dirname);
    g_free (dirname);
    desktop_dir_filename = g_path_get_basename (path);
    g_free (path);
}

gboolean
baul_is_home_directory_file (GFile *dir,
                             const char *filename)
{
    static GFile *home_dir_dir = NULL;
    static char *home_dir_filename = NULL;

    if (home_dir_dir == NULL)
    {
        char *dirname;

        dirname = g_path_get_dirname (g_get_home_dir ());
        home_dir_dir = g_file_new_for_path (dirname);
        g_free (dirname);
        home_dir_filename = g_path_get_basename (g_get_home_dir ());
    }

    return (g_file_equal (dir, home_dir_dir) &&
            strcmp (filename, home_dir_filename) == 0);
}

gboolean
baul_is_home_directory (GFile *dir)
{
    static GFile *home_dir = NULL;

    if (home_dir == NULL)
    {
        home_dir = g_file_new_for_path (g_get_home_dir ());
    }

    return g_file_equal (dir, home_dir);
}

gboolean
baul_is_root_directory (GFile *dir)
{
    static GFile *root_dir = NULL;

    if (root_dir == NULL)
    {
        root_dir = g_file_new_for_path ("/");
    }

    return g_file_equal (dir, root_dir);
}


gboolean
baul_is_desktop_directory_file (GFile *dir,
                                const char *file)
{

    if (!desktop_dir_changed_callback_installed)
    {
        g_signal_connect_swapped (baul_preferences, "changed::" BAUL_PREFERENCES_DESKTOP_IS_HOME_DIR,
                                  G_CALLBACK(desktop_dir_changed_callback),
                                  NULL);
        desktop_dir_changed_callback_installed = TRUE;
    }

    if (desktop_dir == NULL)
    {
        update_desktop_dir ();
    }

    return (g_file_equal (dir, desktop_dir_dir) &&
            strcmp (file, desktop_dir_filename) == 0);
}

gboolean
baul_is_desktop_directory (GFile *dir)
{

    if (!desktop_dir_changed_callback_installed)
    {
        g_signal_connect_swapped (baul_preferences, "changed::" BAUL_PREFERENCES_DESKTOP_IS_HOME_DIR,
                                  G_CALLBACK(desktop_dir_changed_callback),
                                  NULL);
        desktop_dir_changed_callback_installed = TRUE;
    }

    if (desktop_dir == NULL)
    {
        update_desktop_dir ();
    }

    return g_file_equal (dir, desktop_dir);
}

GMount *
baul_get_mounted_mount_for_root (GFile *location)
{
	GVolumeMonitor *volume_monitor;
	GList *mounts;
	GList *l;
	GMount *mount = NULL;
	GMount *result = NULL;
	GFile *root = NULL;
	GFile *default_location = NULL;

	volume_monitor = g_volume_monitor_get ();
	mounts = g_volume_monitor_get_mounts (volume_monitor);

	for (l = mounts; l != NULL; l = l->next) {
		mount = l->data;

		if (g_mount_is_shadowed (mount)) {
			continue;
		}

		root = g_mount_get_root (mount);
		if (g_file_equal (location, root)) {
			result = g_object_ref (mount);
			break;
		}

		default_location = g_mount_get_default_location (mount);
		if (!g_file_equal (default_location, root) &&
		    g_file_equal (location, default_location)) {
			result = g_object_ref (mount);
			break;
		}
	}

	g_clear_object (&root);
	g_clear_object (&default_location);
	g_list_free_full (mounts, g_object_unref);

	return result;
}

/**
 * baul_get_pixmap_directory
 *
 * Get the path for the directory containing Baul pixmaps.
 *
 * Return value: the directory path.
 **/
char *
baul_get_pixmap_directory (void)
{
    return g_strdup (DATADIR "/pixmaps/baul");
}

/* FIXME bugzilla.gnome.org 42423:
 * Callers just use this and dereference so we core dump if
 * pixmaps are missing. That is lame.
 */
char *
baul_pixmap_file (const char *partial_path)
{
    char *path;

    path = g_build_filename (DATADIR "/pixmaps/baul", partial_path, NULL);
    if (g_file_test (path, G_FILE_TEST_EXISTS))
    {
        return path;
    }
    else
    {
        char *tmp;
        tmp = baul_get_pixmap_directory ();
        g_debug ("Failed to locate \"%s\" in Baul pixmap path \"%s\". Incomplete installation?", partial_path, tmp);
        g_free (tmp);
    }
    g_free (path);
    return NULL;
}

char *
baul_get_data_file_path (const char *partial_path)
{
    char *path;
    char *user_directory;

    /* first try the user's home directory */
    user_directory = baul_get_user_directory ();
    path = g_build_filename (user_directory, partial_path, NULL);
    g_free (user_directory);
    if (g_file_test (path, G_FILE_TEST_EXISTS))
    {
        return path;
    }
    g_free (path);

    /* next try the shared directory */
    path = g_build_filename (BAUL_DATADIR, partial_path, NULL);
    if (g_file_test (path, G_FILE_TEST_EXISTS))
    {
        return path;
    }
    g_free (path);

    return NULL;
}

char *
baul_ensure_unique_file_name (const char *directory_uri,
                              const char *base_name,
                              const char *extension)
{
    GFileInfo *info;
    char *filename;
    GFile *dir, *child;
    int copy;
    char *res;

    dir = g_file_new_for_uri (directory_uri);

    info = g_file_query_info (dir, G_FILE_ATTRIBUTE_STANDARD_TYPE, 0, NULL, NULL);
    if (info == NULL)
    {
        g_object_unref (dir);
        return NULL;
    }
    g_object_unref (info);

    filename = g_strdup_printf ("%s%s",
                                base_name,
                                extension);
    child = g_file_get_child (dir, filename);
    g_free (filename);

    copy = 1;
    while ((info = g_file_query_info (child, G_FILE_ATTRIBUTE_STANDARD_TYPE, 0, NULL, NULL)) != NULL)
    {
        g_object_unref (info);
        g_object_unref (child);

        filename = g_strdup_printf ("%s-%d%s",
                                    base_name,
                                    copy,
                                    extension);
        child = g_file_get_child (dir, filename);
        g_free (filename);

        copy++;
    }

    res = g_file_get_uri (child);
    g_object_unref (child);
    g_object_unref (dir);

    return res;
}

GFile *
baul_find_existing_uri_in_hierarchy (GFile *location)
{
    GFileInfo *info = NULL;
    GFile *tmp = NULL;

    g_assert (location != NULL);

    location = g_object_ref (location);
    while (location != NULL)
    {
        info = g_file_query_info (location,
                                  G_FILE_ATTRIBUTE_STANDARD_NAME,
                                  0, NULL, NULL);
        g_object_unref (info);
        if (info != NULL)
        {
            return location;
        }
        tmp = location;
        location = g_file_get_parent (location);
        g_object_unref (tmp);
    }

    return location;
}

gboolean
baul_is_grapa_installed (void)
{
    static int installed = -1;

    if (installed < 0)
    {
        gchar *found = g_find_program_in_path ("grapa");
        installed = found ? 1 : 0;
        g_free (found);
    }

    return installed > 0 ? TRUE : FALSE;
}

#define GSM_NAME  "org.gnome.SessionManager"
#define GSM_PATH "/org/gnome/SessionManager"
#define GSM_INTERFACE "org.gnome.SessionManager"

/* The following values come from
 * https://people.gnome.org/~mccann/gnome-session/docs/gnome-session.html#org.gnome.SessionManager.Inhibit
 */
#define INHIBIT_LOGOUT (1U)
#define INHIBIT_SUSPEND (4U)

static GDBusConnection *
get_dbus_connection (void)
{
    static GDBusConnection *conn = NULL;

    if (conn == NULL)
    {
        GError *error = NULL;

        conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);

        if (conn == NULL)
        {
            g_warning ("Could not connect to session bus: %s", error->message);
            g_error_free (error);
        }
    }

    return conn;
}

/**
 * baul_inhibit_power_manager:
 * @message: a human readable message for the reason why power management
 *       is being suspended.
 *
 * Inhibits the power manager from logging out or suspending the machine
 * (e.g. whenever Baul is doing file operations).
 *
 * Returns: an integer cookie, which must be passed to
 *    baul_uninhibit_power_manager() to resume
 *    normal power management.
 */
int
baul_inhibit_power_manager (const char *message)
{
    GDBusConnection *connection;
    GVariant *result;
    GError *error = NULL;
    guint cookie = 0;

    g_return_val_if_fail (message != NULL, -1);

    connection = get_dbus_connection ();

    if (connection == NULL)
    {
        return -1;
    }

    result = g_dbus_connection_call_sync (connection,
                                          GSM_NAME,
                                          GSM_PATH,
                                          GSM_INTERFACE,
                                          "Inhibit",
                                          g_variant_new ("(susu)",
                                                  "Baul",
                                                  (guint) 0,
                                                  message,
                                                  (guint) (INHIBIT_LOGOUT | INHIBIT_SUSPEND)),
                                          G_VARIANT_TYPE ("(u)"),
                                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                          -1,
                                          NULL,
                                          &error);

    if (error != NULL)
    {
        g_warning ("Could not inhibit power management: %s", error->message);
        g_error_free (error);
        return -1;
    }

    g_variant_get (result, "(u)", &cookie);
    g_variant_unref (result);

    return (int) cookie;
}

/**
 * baul_uninhibit_power_manager:
 * @cookie: the cookie value returned by baul_inhibit_power_manager()
 *
 * Uninhibits power management. This function must be called after the task
 * which inhibited power management has finished, or the system will not
 * return to normal power management.
 */
void
baul_uninhibit_power_manager (gint cookie)
{
    GDBusConnection *connection;
    GVariant *result;
    GError *error = NULL;

    g_return_if_fail (cookie > 0);

    connection = get_dbus_connection ();

    if (connection == NULL)
    {
        return;
    }

    result = g_dbus_connection_call_sync (connection,
                                          GSM_NAME,
                                          GSM_PATH,
                                          GSM_INTERFACE,
                                          "Uninhibit",
                                          g_variant_new ("(u)", (guint) cookie),
                                          NULL,
                                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                          -1,
                                          NULL,
                                          &error);

    if (result == NULL)
    {
        g_warning ("Could not uninhibit power management: %s", error->message);
        g_error_free (error);
        return;
    }

    g_variant_unref (result);
}

/* Returns TRUE if the file is in XDG_DATA_DIRS. This is used for
   deciding if a desktop file is "trusted" based on the path */
gboolean
baul_is_in_system_dir (GFile *file)
{
    const char * const * data_dirs;
    char *path;
    int i;
    gboolean res;

    if (!g_file_is_native (file))
    {
        return FALSE;
    }

    path = g_file_get_path (file);

    res = FALSE;

    data_dirs = g_get_system_data_dirs ();
    for (i = 0; path != NULL && data_dirs[i] != NULL; i++)
    {
        if (g_str_has_prefix (path, data_dirs[i]))
        {
            res = TRUE;
            break;
        }

    }

    g_free (path);

    return res;
}

GHashTable *
baul_trashed_files_get_original_directories (GList *files,
        GList **unhandled_files)
{
    GHashTable *directories;
    GList *l, *m;
    BaulFile *file = NULL;
    BaulFile *original_file = NULL;
    BaulFile *original_dir = NULL;

    directories = NULL;

    if (unhandled_files != NULL)
    {
        *unhandled_files = NULL;
    }

    for (l = files; l != NULL; l = l->next)
    {
        file = BAUL_FILE (l->data);
        original_file = baul_file_get_trash_original_file (file);

        original_dir = NULL;
        if (original_file != NULL)
        {
            original_dir = baul_file_get_parent (original_file);
        }

        if (original_dir != NULL)
        {
            if (directories == NULL)
            {
                directories = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                     (GDestroyNotify) baul_file_unref,
                                                     (GDestroyNotify) baul_file_list_unref);
            }
            baul_file_ref (original_dir);
            m = g_hash_table_lookup (directories, original_dir);
            if (m != NULL)
            {
                g_hash_table_steal (directories, original_dir);
                baul_file_unref (original_dir);
            }
            m = g_list_append (m, baul_file_ref (file));
            g_hash_table_insert (directories, original_dir, m);
        }
        else if (unhandled_files != NULL)
        {
            *unhandled_files = g_list_append (*unhandled_files, baul_file_ref (file));
        }

        if (original_file != NULL)
        {
            baul_file_unref (original_file);
        }

        if (original_dir != NULL)
        {
            baul_file_unref (original_dir);
        }
    }

    return directories;
}

static GList *
locations_from_file_list (GList *file_list)
{
    GList *l, *ret;
    BaulFile *file = NULL;

    ret = NULL;

    for (l = file_list; l != NULL; l = l->next)
    {
        file = BAUL_FILE (l->data);
        ret = g_list_prepend (ret, baul_file_get_location (file));
    }

    return g_list_reverse (ret);
}

void
baul_restore_files_from_trash (GList *files,
                               CtkWindow *parent_window)
{
    GHashTable *original_dirs_hash;
    GList *original_dirs, *unhandled_files;
    GList *l;
    BaulFile *file = NULL;

    original_dirs_hash = baul_trashed_files_get_original_directories (files, &unhandled_files);

    for (l = unhandled_files; l != NULL; l = l->next)
    {
        char *message, *file_name;

        file = BAUL_FILE (l->data);
        file_name = baul_file_get_display_name (file);
        message = g_strdup_printf (_("Could not determine original location of \"%s\" "), file_name);
        g_free (file_name);

        eel_show_warning_dialog (message,
                                 _("The item cannot be restored from trash"),
                                 parent_window);
        g_free (message);
    }

    if (original_dirs_hash != NULL)
    {
        BaulFile *original_dir = NULL;
        GFile *original_dir_location = NULL;
        GList *locations = NULL;

        original_dirs = g_hash_table_get_keys (original_dirs_hash);
        for (l = original_dirs; l != NULL; l = l->next)
        {
            original_dir = BAUL_FILE (l->data);
            original_dir_location = baul_file_get_location (original_dir);

            files = g_hash_table_lookup (original_dirs_hash, original_dir);
            locations = locations_from_file_list (files);

            baul_file_operations_move
            (locations, NULL,
             original_dir_location,
             parent_window,
             NULL, NULL);

            g_list_free_full (locations, g_object_unref);
            g_object_unref (original_dir_location);
        }

        g_list_free (original_dirs);
        g_hash_table_destroy (original_dirs_hash);
    }

    baul_file_list_unref (unhandled_files);
}

#if !defined (BAUL_OMIT_SELF_CHECK)

void
baul_self_check_file_utilities (void)
{
}

#endif /* !BAUL_OMIT_SELF_CHECK */
