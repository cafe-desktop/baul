/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* Baul

   Copyright (C) 2008 Red Hat, Inc.

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

   Author: David Zeuthen <davidz@redhat.com>
*/


#include <config.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <ctk/ctk.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#include <libbaul-private/baul-module.h>
#include <libbaul-private/baul-icon-info.h>

typedef struct
{
    CtkWidget *dialog;
    GMount *mount;
} AutorunSoftwareDialogData;

static void autorun_software_dialog_mount_unmounted (GMount *mount, AutorunSoftwareDialogData *data);

static void
autorun_software_dialog_destroy (AutorunSoftwareDialogData *data)
{
    g_signal_handlers_disconnect_by_func (G_OBJECT (data->mount),
                                          G_CALLBACK (autorun_software_dialog_mount_unmounted),
                                          data);

    ctk_widget_destroy (CTK_WIDGET (data->dialog));
    g_object_unref (data->mount);
    g_free (data);
}

static void
autorun_software_dialog_mount_unmounted (GMount                    *mount G_GNUC_UNUSED,
					 AutorunSoftwareDialogData *data)
{
    autorun_software_dialog_destroy (data);
}

static gboolean
_check_file (GFile *mount_root, const char *file_path, gboolean must_be_executable)
{
    GFile *file;
    GFileInfo *file_info;
    gboolean ret;

    ret = FALSE;

    file = g_file_get_child (mount_root, file_path);
    file_info = g_file_query_info (file,
                                   G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE,
                                   G_FILE_QUERY_INFO_NONE,
                                   NULL,
                                   NULL);
    if (file_info != NULL)
    {
        if (must_be_executable)
        {
            if (g_file_info_get_attribute_boolean (file_info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE))
            {
                ret = TRUE;
            }
        }
        else
        {
            ret = TRUE;
        }
        g_object_unref (file_info);
    }
    g_object_unref (file);

    return ret;
}

static void
autorun (GMount *mount)
{
    char *error_string;
    GFile *root;
    GFile *program_to_spawn;
    char *path_to_spawn;
    char *cwd_for_program;

    root = g_mount_get_root (mount);

    /* Careful here, according to
     *
     *  https://standards.freedesktop.org/autostart-spec/autostart-spec-latest.html
     *
     * the ordering does matter.
     */

    program_to_spawn = NULL;
    path_to_spawn = NULL;

    if (_check_file (root, ".autorun", TRUE))
    {
        program_to_spawn = g_file_get_child (root, ".autorun");
    }
    else if (_check_file (root, "autorun", TRUE))
    {
        program_to_spawn = g_file_get_child (root, "autorun");
    }
    else if (_check_file (root, "autorun.sh", TRUE))
    {
        program_to_spawn = g_file_get_child (root, "autorun.sh");
    }
    else if (_check_file (root, "autorun.exe", TRUE))
    {
        /* TODO */
    }
    else if (_check_file (root, "AUTORUN.EXE", TRUE))
    {
        /* TODO */
    }
    else if (_check_file (root, "autorun.inf", FALSE))
    {
        /* TODO */
    }
    else if (_check_file (root, "AUTORUN.INF", FALSE))
    {
        /* TODO */
    }

    if (program_to_spawn != NULL)
    {
        path_to_spawn = g_file_get_path (program_to_spawn);
    }

    cwd_for_program = g_file_get_path (root);

    error_string = NULL;
    if (path_to_spawn != NULL && cwd_for_program != NULL)
    {
        if (chdir (cwd_for_program) == 0)
        {
            execl (path_to_spawn, path_to_spawn, NULL);
            error_string = g_strdup_printf (_("Error starting autorun program: %s"), strerror (errno));
            goto out;
        }
        error_string = g_strdup_printf (_("Error starting autorun program: %s"), strerror (errno));
        goto out;
    }
    error_string = g_strdup_printf (_("Cannot find the autorun program"));

out:
    if (program_to_spawn != NULL)
    {
        g_object_unref (program_to_spawn);
    }
    if (root != NULL)
    {
        g_object_unref (root);
    }
    g_free (path_to_spawn);
    g_free (cwd_for_program);

    if (error_string != NULL)
    {
        CtkWidget *dialog;
        dialog = ctk_message_dialog_new_with_markup (NULL, /* TODO: parent window? */
                 0,
                 CTK_MESSAGE_ERROR,
                 CTK_BUTTONS_OK,
                 _("<big><b>Error autorunning software</b></big>"));
        ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog), "%s", error_string);
        ctk_dialog_run (CTK_DIALOG (dialog));
        ctk_widget_destroy (dialog);
        g_free (error_string);
    }
}

static void
present_autorun_for_software_dialog (GMount *mount)
{
    GIcon *icon;
    int icon_size;
    BaulIconInfo *icon_info;
    GdkPixbuf *pixbuf;
    CtkWidget *image;
    char *mount_name;
    CtkWidget *dialog;
    AutorunSoftwareDialogData *data;

    mount_name = g_mount_get_name (mount);

    dialog = ctk_message_dialog_new_with_markup (NULL, /* TODO: parent window? */
             0,
             CTK_MESSAGE_OTHER,
             CTK_BUTTONS_CANCEL,
             _("<big><b>This medium contains software intended to be automatically started. Would you like to run it?</b></big>"));
    ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog),
            _("The software will run directly from the medium \"%s\". "
              "You should never run software that you don't trust.\n"
              "\n"
              "If in doubt, press Cancel."),
            mount_name);

    /* TODO: in a star trek future add support for verifying
     * software on media (e.g. if it has a certificate, check it
     * etc.)
     */


    icon = g_mount_get_icon (mount);
    icon_size = baul_get_icon_size_for_stock_size (CTK_ICON_SIZE_DIALOG);
    icon_info = baul_icon_info_lookup (icon, icon_size,
                                       ctk_widget_get_scale_factor (CTK_WIDGET (dialog)));
    pixbuf = baul_icon_info_get_pixbuf_at_size (icon_info, icon_size);
    image = ctk_image_new_from_pixbuf (pixbuf);
    ctk_widget_set_halign (image, CTK_ALIGN_CENTER);
    ctk_widget_set_valign (image, CTK_ALIGN_START);
    ctk_message_dialog_set_image (CTK_MESSAGE_DIALOG (dialog), image);

    ctk_window_set_title (CTK_WINDOW (dialog), mount_name);
    ctk_window_set_icon (CTK_WINDOW (dialog), pixbuf);

    data = g_new0 (AutorunSoftwareDialogData, 1);
    data->dialog = dialog;
    data->mount = g_object_ref (mount);

    g_signal_connect (G_OBJECT (mount),
                      "unmounted",
                      G_CALLBACK (autorun_software_dialog_mount_unmounted),
                      data);

    ctk_dialog_add_button (CTK_DIALOG (dialog),
                           _("_Run"),
                           CTK_RESPONSE_OK);

    ctk_widget_show_all (dialog);

    if (ctk_dialog_run (CTK_DIALOG (dialog)) == CTK_RESPONSE_OK)
    {
        ctk_widget_destroy (dialog);
        autorun (mount);
    }

    g_object_unref (icon_info);
    g_object_unref (pixbuf);
    g_free (mount_name);
}

int
main (int argc, char *argv[])
{
    GVolumeMonitor *monitor;
    GFile *file;
    GMount *mount;

    bindtextdomain (GETTEXT_PACKAGE, CAFELOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    ctk_init (&argc, &argv);

    if (argc != 2)
    {
        goto out;
    }

    /* instantiate monitor so we get the "unmounted" signal properly */
    monitor = g_volume_monitor_get ();
    if (monitor == NULL)
    {
        goto out;
    }

    file = g_file_new_for_commandline_arg (argv[1]);
    if (file == NULL)
    {
        g_object_unref (monitor);
        goto out;
    }

    mount = g_file_find_enclosing_mount (file, NULL, NULL);
    if (mount == NULL)
    {
        g_object_unref (file);
        g_object_unref (monitor);
        goto out;
    }

    present_autorun_for_software_dialog (mount);
    g_object_unref (file);
    g_object_unref (monitor);
    g_object_unref (mount);

out:
    return 0;
}
