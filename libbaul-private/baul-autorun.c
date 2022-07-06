/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * Baul is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <ctk/ctk.h>
#include <cdk/cdkx.h>
#include <gio/gdesktopappinfo.h>
#include <X11/XKBlib.h>
#include <cdk/cdkkeysyms.h>
#include <cairo-gobject.h>

#include <eel/eel-glib-extensions.h>
#include <eel/eel-stock-dialogs.h>

#include "baul-icon-info.h"
#include "baul-global-preferences.h"
#include "baul-file-operations.h"
#include "baul-autorun.h"
#include "baul-program-choosing.h"
#include "baul-open-with-dialog.h"
#include "baul-desktop-icon-file.h"
#include "baul-file-utilities.h"

enum
{
    AUTORUN_ASK,
    AUTORUN_IGNORE,
    AUTORUN_APP,
    AUTORUN_OPEN_FOLDER,
    AUTORUN_SEP,
    AUTORUN_OTHER_APP,
};
enum
{
    COLUMN_AUTORUN_SURFACE,
    COLUMN_AUTORUN_NAME,
    COLUMN_AUTORUN_APP_INFO,
    COLUMN_AUTORUN_X_CONTENT_TYPE,
    COLUMN_AUTORUN_ITEM_TYPE,
};

static gboolean should_autorun_mount (GMount *mount);

static void baul_autorun_rebuild_combo_box (CtkWidget *combo_box);

void
baul_autorun_get_preferences (const char *x_content_type,
                              gboolean *pref_start_app,
                              gboolean *pref_ignore,
                              gboolean *pref_open_folder)
{
    char **x_content_start_app;
    char **x_content_ignore;
    char **x_content_open_folder;

    g_return_if_fail (pref_start_app != NULL);
    g_return_if_fail (pref_ignore != NULL);
    g_return_if_fail (pref_open_folder != NULL);

    *pref_start_app = FALSE;
    *pref_ignore = FALSE;
    *pref_open_folder = FALSE;
    x_content_start_app = g_settings_get_strv (baul_media_preferences, BAUL_PREFERENCES_MEDIA_AUTORUN_X_CONTENT_START_APP);
    x_content_ignore = g_settings_get_strv (baul_media_preferences, BAUL_PREFERENCES_MEDIA_AUTORUN_X_CONTENT_IGNORE);
    x_content_open_folder = g_settings_get_strv (baul_media_preferences, BAUL_PREFERENCES_MEDIA_AUTORUN_X_CONTENT_OPEN_FOLDER);
    if (x_content_start_app != NULL)
    {
        *pref_start_app = eel_g_strv_find (x_content_start_app, x_content_type) != -1;
    }
    if (x_content_ignore != NULL)
    {
        *pref_ignore = eel_g_strv_find (x_content_ignore, x_content_type) != -1;
    }
    if (x_content_open_folder != NULL)
    {
        *pref_open_folder = eel_g_strv_find (x_content_open_folder, x_content_type) != -1;
    }
    g_strfreev (x_content_ignore);
    g_strfreev (x_content_start_app);
    g_strfreev (x_content_open_folder);
}

static void
remove_elem_from_str_array (char **v, const char *s)
{
    int n, m;

    if (v == NULL)
    {
        return;
    }

    for (n = 0; v[n] != NULL; n++)
    {
        if (strcmp (v[n], s) == 0)
        {
            for (m = n + 1; v[m] != NULL; m++)
            {
                v[m - 1] = v[m];
            }
            v[m - 1] = NULL;
            n--;
        }
    }
}

static char **
add_elem_to_str_array (char **v, const char *s)
{
    guint len;
    char **r;

    len = v != NULL ? g_strv_length (v) : 0;
    r = g_new0 (char *, len + 2);

    if (v)
        memcpy (r, v, len * sizeof (char *));

    r[len] = g_strdup (s);
    r[len+1] = NULL;
    g_free (v);

    return r;
}


void
baul_autorun_set_preferences (const char *x_content_type,
                              gboolean pref_start_app,
                              gboolean pref_ignore,
                              gboolean pref_open_folder)
{
    char **x_content_start_app;
    char **x_content_ignore;
    char **x_content_open_folder;

    g_assert (x_content_type != NULL);

    x_content_start_app = g_settings_get_strv (baul_media_preferences, BAUL_PREFERENCES_MEDIA_AUTORUN_X_CONTENT_START_APP);
    x_content_ignore = g_settings_get_strv (baul_media_preferences, BAUL_PREFERENCES_MEDIA_AUTORUN_X_CONTENT_IGNORE);
    x_content_open_folder = g_settings_get_strv (baul_media_preferences, BAUL_PREFERENCES_MEDIA_AUTORUN_X_CONTENT_OPEN_FOLDER);

    remove_elem_from_str_array (x_content_start_app, x_content_type);
    if (pref_start_app)
    {
        x_content_start_app = add_elem_to_str_array (x_content_start_app, x_content_type);
    }
    g_settings_set_strv (baul_media_preferences, BAUL_PREFERENCES_MEDIA_AUTORUN_X_CONTENT_START_APP, (const gchar * const*) x_content_start_app);

    remove_elem_from_str_array (x_content_ignore, x_content_type);
    if (pref_ignore)
    {
        x_content_ignore = add_elem_to_str_array (x_content_ignore, x_content_type);
    }
    g_settings_set_strv (baul_media_preferences, BAUL_PREFERENCES_MEDIA_AUTORUN_X_CONTENT_IGNORE, (const gchar * const*) x_content_ignore);

    remove_elem_from_str_array (x_content_open_folder, x_content_type);
    if (pref_open_folder)
    {
        x_content_open_folder = add_elem_to_str_array (x_content_open_folder, x_content_type);
    }
    g_settings_set_strv (baul_media_preferences, BAUL_PREFERENCES_MEDIA_AUTORUN_X_CONTENT_OPEN_FOLDER, (const gchar * const*) x_content_open_folder);

    g_strfreev (x_content_open_folder);
    g_strfreev (x_content_ignore);
    g_strfreev (x_content_start_app);

}

static gboolean
combo_box_separator_func (CtkTreeModel *model,
                          CtkTreeIter *iter,
                          gpointer data)
{
    char *str;

    ctk_tree_model_get (model, iter,
                        1, &str,
                        -1);
    if (str != NULL)
    {
        g_free (str);
        return FALSE;
    }
    return TRUE;
}

typedef struct
{
    guint changed_signal_id;
    CtkWidget *combo_box;

    char *x_content_type;
    gboolean include_ask;
    gboolean include_open_with_other_app;

    gboolean update_settings;
    BaulAutorunComboBoxChanged changed_cb;
    gpointer user_data;

    gboolean other_application_selected;
} BaulAutorunComboBoxData;

static void
baul_autorun_combobox_data_destroy (BaulAutorunComboBoxData *data)
{
    /* signal handler may be automatically disconnected by destroying the widget */
    if (g_signal_handler_is_connected (G_OBJECT (data->combo_box), data->changed_signal_id))
    {
        g_signal_handler_disconnect (G_OBJECT (data->combo_box), data->changed_signal_id);
    }
    g_free (data->x_content_type);
    g_free (data);
}

static void
other_application_selected (BaulOpenWithDialog *dialog,
                            GAppInfo *app_info,
                            BaulAutorunComboBoxData *data)
{
    if (data->changed_cb != NULL)
    {
        data->changed_cb (TRUE, FALSE, FALSE, app_info, data->user_data);
    }
    if (data->update_settings)
    {
        baul_autorun_set_preferences (data->x_content_type, TRUE, FALSE, FALSE);
        g_app_info_set_as_default_for_type (app_info,
                                            data->x_content_type,
                                            NULL);
        data->other_application_selected = TRUE;
    }

    /* rebuild so we include and select the new application in the list */
    baul_autorun_rebuild_combo_box (data->combo_box);
}

static void
handle_dialog_closure (BaulAutorunComboBoxData *data)
{
    if (!data->other_application_selected)
    {
        /* reset combo box so we don't linger on "Open with other Application..." */
        baul_autorun_rebuild_combo_box (data->combo_box);
    }
}

static void
dialog_response_cb (CtkDialog *dialog,
                    gint response,
                    BaulAutorunComboBoxData *data)
{
    handle_dialog_closure (data);
}

static void
dialog_destroy_cb (CtkWidget *object,
                   BaulAutorunComboBoxData *data)
{
    handle_dialog_closure (data);
}

static void
combo_box_changed (CtkComboBox *combo_box,
                   BaulAutorunComboBoxData *data)
{
    CtkTreeIter iter;
    CtkTreeModel *model;
    GAppInfo *app_info;
    char *x_content_type;
    int type;

    model = NULL;
    app_info = NULL;
    x_content_type = NULL;

    if (!ctk_combo_box_get_active_iter (combo_box, &iter))
    {
        goto out;
    }

    model = ctk_combo_box_get_model (combo_box);
    if (model == NULL)
    {
        goto out;
    }

    ctk_tree_model_get (model, &iter,
                        COLUMN_AUTORUN_APP_INFO, &app_info,
                        COLUMN_AUTORUN_X_CONTENT_TYPE, &x_content_type,
                        COLUMN_AUTORUN_ITEM_TYPE, &type,
                        -1);

    switch (type)
    {
    case AUTORUN_ASK:
        if (data->changed_cb != NULL)
        {
            data->changed_cb (TRUE, FALSE, FALSE, NULL, data->user_data);
        }
        if (data->update_settings)
        {
            baul_autorun_set_preferences (x_content_type, FALSE, FALSE, FALSE);
        }
        break;
    case AUTORUN_IGNORE:
        if (data->changed_cb != NULL)
        {
            data->changed_cb (FALSE, TRUE, FALSE, NULL, data->user_data);
        }
        if (data->update_settings)
        {
            baul_autorun_set_preferences (x_content_type, FALSE, TRUE, FALSE);
        }
        break;
    case AUTORUN_OPEN_FOLDER:
        if (data->changed_cb != NULL)
        {
            data->changed_cb (FALSE, FALSE, TRUE, NULL, data->user_data);
        }
        if (data->update_settings)
        {
            baul_autorun_set_preferences (x_content_type, FALSE, FALSE, TRUE);
        }
        break;

    case AUTORUN_APP:
        if (data->changed_cb != NULL)
        {
            /* TODO TODO?? */
            data->changed_cb (TRUE, FALSE, FALSE, app_info, data->user_data);
        }
        if (data->update_settings)
        {
            baul_autorun_set_preferences (x_content_type, TRUE, FALSE, FALSE);
            g_app_info_set_as_default_for_type (app_info,
                                                x_content_type,
                                                NULL);
        }
        break;

    case AUTORUN_OTHER_APP:
    {
        CtkWidget *dialog;

        data->other_application_selected = FALSE;

        dialog = baul_add_application_dialog_new (NULL, x_content_type);
        ctk_window_set_transient_for (CTK_WINDOW (dialog),
                                      CTK_WINDOW (ctk_widget_get_toplevel (CTK_WIDGET (combo_box))));
        g_signal_connect (dialog, "application_selected",
                          G_CALLBACK (other_application_selected),
                          data);
        g_signal_connect (dialog, "response",
                          G_CALLBACK (dialog_response_cb), data);
        g_signal_connect (dialog, "destroy",
                          G_CALLBACK (dialog_destroy_cb), data);
        ctk_widget_show (CTK_WIDGET (dialog));

        break;
    }

    }

out:
    if (app_info != NULL)
    {
        g_object_unref (app_info);
    }
    g_free (x_content_type);
}

static void
baul_autorun_rebuild_combo_box (CtkWidget *combo_box)
{
    BaulAutorunComboBoxData *data;
    char *x_content_type;

    data = g_object_get_data (G_OBJECT (combo_box), "baul_autorun_combobox_data");
    if (data == NULL)
    {
        g_warning ("no 'baul_autorun_combobox_data' data!");
        return;
    }

    x_content_type = g_strdup (data->x_content_type);
    baul_autorun_prepare_combo_box (combo_box,
                                    x_content_type,
                                    data->include_ask,
                                    data->include_open_with_other_app,
                                    data->update_settings,
                                    data->changed_cb,
                                    data->user_data);
    g_free (x_content_type);
}

/* TODO: we need some kind of way to remove user-defined associations,
 * e.g. the result of "Open with other Application...".
 *
 * However, this is a bit hard as
 * g_app_info_can_remove_supports_type() will always return TRUE
 * because we now have [Removed Applications] in the file
 * ~/.local/share/applications/mimeapps.list.
 *
 * We need the API outlined in
 *
 *  https://bugzilla.gnome.org/show_bug.cgi?id=545350
 *
 * to do this.
 *
 * Now, there's also the question about what the UI would look like
 * given this API. Ideally we'd include a small button on the right
 * side of the combo box that the user can press to delete an
 * association, e.g.:
 *
 *  +-------------------------------------+
 *  | Ask what to do                      |
 *  | Do Nothing                          |
 *  | Open Folder                         |
 *  +-------------------------------------+
 *  | Open Rhythmbox Music Player         |
 *  | Open Audio CD Extractor             |
 *  | Open Banshee Media Player           |
 *  | Open Frobnicator App            [x] |
 *  +-------------------------------------+
 *  | Open with other Application...      |
 *  +-------------------------------------+
 *
 * where "Frobnicator App" have been set up using "Open with other
 * Application...". However this is not accessible (which is a
 * CTK+ issue) but probably not a big deal.
 *
 * And we only want show these buttons (e.g. [x]) for associations with
 * GAppInfo instances that are deletable.
 */

void
baul_autorun_prepare_combo_box (CtkWidget *combo_box,
                                const char *x_content_type,
                                gboolean include_ask,
                                gboolean include_open_with_other_app,
                                gboolean update_settings,
                                BaulAutorunComboBoxChanged changed_cb,
                                gpointer user_data)
{
    GList *l;
    GList *app_info_list;
    GAppInfo *default_app_info;
    CtkListStore *list_store;
    CtkTreeIter iter;
    cairo_surface_t *surface;
    int icon_size, icon_scale;
    int set_active;
    int num_apps;
    gboolean pref_ask;
    gboolean pref_start_app;
    gboolean pref_ignore;
    gboolean pref_open_folder;
    BaulAutorunComboBoxData *data;
    CtkCellRenderer *renderer;
    gboolean new_data;

    baul_autorun_get_preferences (x_content_type, &pref_start_app, &pref_ignore, &pref_open_folder);
    pref_ask = !pref_start_app && !pref_ignore && !pref_open_folder;

    icon_size = baul_get_icon_size_for_stock_size (CTK_ICON_SIZE_MENU);
    icon_scale = ctk_widget_get_scale_factor (combo_box);

    set_active = -1;
    data = NULL;
    new_data = TRUE;

    app_info_list = g_app_info_get_all_for_type (x_content_type);
    default_app_info = g_app_info_get_default_for_type (x_content_type, FALSE);
    num_apps = g_list_length (app_info_list);

    list_store = ctk_list_store_new (5,
                                     CAIRO_GOBJECT_TYPE_SURFACE,
                                     G_TYPE_STRING,
                                     G_TYPE_APP_INFO,
                                     G_TYPE_STRING,
                                     G_TYPE_INT);

    /* no apps installed */
    if (num_apps == 0)
    {
        ctk_list_store_append (list_store, &iter);
        surface = ctk_icon_theme_load_surface (ctk_icon_theme_get_default (),
                                               "dialog-error",
                                               icon_size,
                                               icon_scale,
                                               NULL,
                                               0,
                                               NULL);

        /* TODO: integrate with PackageKit-cafe to find applications */

        ctk_list_store_set (list_store, &iter,
                            COLUMN_AUTORUN_SURFACE, surface,
                            COLUMN_AUTORUN_NAME, _("No applications found"),
                            COLUMN_AUTORUN_APP_INFO, NULL,
                            COLUMN_AUTORUN_X_CONTENT_TYPE, x_content_type,
                            COLUMN_AUTORUN_ITEM_TYPE, AUTORUN_ASK,
                            -1);
        cairo_surface_destroy (surface);
    }
    else
    {
        if (include_ask)
        {
            ctk_list_store_append (list_store, &iter);
            surface = ctk_icon_theme_load_surface (ctk_icon_theme_get_default (),
                                                   "dialog-question",
                                                   icon_size,
                                                   icon_scale,
                                                   NULL,
                                                   0,
                                                   NULL);
            ctk_list_store_set (list_store, &iter,
                                COLUMN_AUTORUN_SURFACE, surface,
                                COLUMN_AUTORUN_NAME, _("Ask what to do"),
                                COLUMN_AUTORUN_APP_INFO, NULL,
                                COLUMN_AUTORUN_X_CONTENT_TYPE, x_content_type,
                                COLUMN_AUTORUN_ITEM_TYPE, AUTORUN_ASK,
                                -1);
            cairo_surface_destroy (surface);
        }

        ctk_list_store_append (list_store, &iter);
        surface = ctk_icon_theme_load_surface (ctk_icon_theme_get_default (),
                                               "window-close",
                                               icon_size,
                                               icon_scale,
                                               NULL,
                                               0,
                                               NULL);
        ctk_list_store_set (list_store, &iter,
                            COLUMN_AUTORUN_SURFACE, surface,
                            COLUMN_AUTORUN_NAME, _("Do Nothing"),
                            COLUMN_AUTORUN_APP_INFO, NULL,
                            COLUMN_AUTORUN_X_CONTENT_TYPE, x_content_type,
                            COLUMN_AUTORUN_ITEM_TYPE, AUTORUN_IGNORE,
                            -1);
        cairo_surface_destroy (surface);

        ctk_list_store_append (list_store, &iter);
        surface = ctk_icon_theme_load_surface (ctk_icon_theme_get_default (),
                                               "folder-open",
                                               icon_size,
                                               icon_scale,
                                               NULL,
                                               0,
                                               NULL);
        ctk_list_store_set (list_store, &iter,
                            COLUMN_AUTORUN_SURFACE, surface,
                            COLUMN_AUTORUN_NAME, _("Open Folder"),
                            COLUMN_AUTORUN_APP_INFO, NULL,
                            COLUMN_AUTORUN_X_CONTENT_TYPE, x_content_type,
                            COLUMN_AUTORUN_ITEM_TYPE, AUTORUN_OPEN_FOLDER,
                            -1);
        cairo_surface_destroy (surface);

        ctk_list_store_append (list_store, &iter);
        ctk_list_store_set (list_store, &iter,
                            COLUMN_AUTORUN_SURFACE, NULL,
                            COLUMN_AUTORUN_NAME, NULL,
                            COLUMN_AUTORUN_APP_INFO, NULL,
                            COLUMN_AUTORUN_X_CONTENT_TYPE, NULL,
                            COLUMN_AUTORUN_ITEM_TYPE, AUTORUN_SEP,
                            -1);

        int n;

        for (l = app_info_list, n = include_ask ? 4 : 3; l != NULL; l = l->next, n++)
        {
            GIcon *icon;
            BaulIconInfo *icon_info;
            char *open_string;
            GAppInfo *app_info = l->data;

            /* we deliberately ignore should_show because some apps might want
             * to install special handlers that should be hidden in the regular
             * application launcher menus
             */

            icon = g_app_info_get_icon (app_info);
            icon_info = baul_icon_info_lookup (icon, icon_size, icon_scale);
            surface = baul_icon_info_get_surface_at_size (icon_info, icon_size);
            g_object_unref (icon_info);

            open_string = g_strdup_printf (_("Open %s"), g_app_info_get_display_name (app_info));

            ctk_list_store_append (list_store, &iter);
            ctk_list_store_set (list_store, &iter,
                                COLUMN_AUTORUN_SURFACE, surface,
                                COLUMN_AUTORUN_NAME, open_string,
                                COLUMN_AUTORUN_APP_INFO, app_info,
                                COLUMN_AUTORUN_X_CONTENT_TYPE, x_content_type,
                                COLUMN_AUTORUN_ITEM_TYPE, AUTORUN_APP,
                                -1);
            if (surface != NULL)
            {
                cairo_surface_destroy (surface);
            }
            g_free (open_string);

            if (g_app_info_equal (app_info, default_app_info))
            {
                set_active = n;
            }
        }
    }

    if (include_open_with_other_app)
    {
        ctk_list_store_append (list_store, &iter);
        ctk_list_store_set (list_store, &iter,
                            COLUMN_AUTORUN_SURFACE, NULL,
                            COLUMN_AUTORUN_NAME, NULL,
                            COLUMN_AUTORUN_APP_INFO, NULL,
                            COLUMN_AUTORUN_X_CONTENT_TYPE, NULL,
                            COLUMN_AUTORUN_ITEM_TYPE, AUTORUN_SEP,
                            -1);

        ctk_list_store_append (list_store, &iter);
        surface = ctk_icon_theme_load_surface (ctk_icon_theme_get_default (),
                                               "application-x-executable",
                                               icon_size,
                                               icon_scale,
                                               NULL,
                                               0,
                                               NULL);
        ctk_list_store_set (list_store, &iter,
                            COLUMN_AUTORUN_SURFACE, surface,
                            COLUMN_AUTORUN_NAME, _("Open with other Application..."),
                            COLUMN_AUTORUN_APP_INFO, NULL,
                            COLUMN_AUTORUN_X_CONTENT_TYPE, x_content_type,
                            COLUMN_AUTORUN_ITEM_TYPE, AUTORUN_OTHER_APP,
                            -1);
        cairo_surface_destroy (surface);
    }

    if (default_app_info != NULL)
    {
        g_object_unref (default_app_info);
    }
    g_list_free_full (app_info_list, g_object_unref);

    ctk_combo_box_set_model (CTK_COMBO_BOX (combo_box), CTK_TREE_MODEL (list_store));
    g_object_unref (G_OBJECT (list_store));

    ctk_cell_layout_clear (CTK_CELL_LAYOUT (combo_box));

    renderer = ctk_cell_renderer_pixbuf_new ();
    ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo_box), renderer, FALSE);
    ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combo_box), renderer,
                                    "surface", COLUMN_AUTORUN_SURFACE,
                                    NULL);
    renderer = ctk_cell_renderer_text_new ();
    ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo_box), renderer, TRUE);
    ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (combo_box), renderer,
                                    "text", COLUMN_AUTORUN_NAME,
                                    NULL);
    ctk_combo_box_set_row_separator_func (CTK_COMBO_BOX (combo_box), combo_box_separator_func, NULL, NULL);

    if (num_apps == 0)
    {
        ctk_combo_box_set_active (CTK_COMBO_BOX (combo_box), 0);
        ctk_widget_set_sensitive (combo_box, FALSE);
    }
    else
    {
        ctk_widget_set_sensitive (combo_box, TRUE);
        if (pref_ask && include_ask)
        {
            ctk_combo_box_set_active (CTK_COMBO_BOX (combo_box), 0);
        }
        else if (pref_ignore)
        {
            ctk_combo_box_set_active (CTK_COMBO_BOX (combo_box), include_ask ? 1 : 0);
        }
        else if (pref_open_folder)
        {
            ctk_combo_box_set_active (CTK_COMBO_BOX (combo_box), include_ask ? 2 : 1);
        }
        else if (set_active != -1)
        {
            ctk_combo_box_set_active (CTK_COMBO_BOX (combo_box), set_active);
        }
        else
        {
            ctk_combo_box_set_active (CTK_COMBO_BOX (combo_box), include_ask ? 1 : 0);
        }

        /* See if we have an old data around */
        data = g_object_get_data (G_OBJECT (combo_box), "baul_autorun_combobox_data");
        if (data)
        {
            new_data = FALSE;
            g_free (data->x_content_type);
        }
        else
        {
            data = g_new0 (BaulAutorunComboBoxData, 1);
        }

        data->x_content_type = g_strdup (x_content_type);
        data->include_ask = include_ask;
        data->include_open_with_other_app = include_open_with_other_app;
        data->update_settings = update_settings;
        data->changed_cb = changed_cb;
        data->user_data = user_data;
        data->combo_box = combo_box;
        if (data->changed_signal_id == 0)
        {
            data->changed_signal_id = g_signal_connect (G_OBJECT (combo_box),
                                      "changed",
                                      G_CALLBACK (combo_box_changed),
                                      data);
        }
    }

    if (new_data)
    {
        g_object_set_data_full (G_OBJECT (combo_box),
                                "baul_autorun_combobox_data",
                                data,
                                (GDestroyNotify) baul_autorun_combobox_data_destroy);
    }
}

static gboolean
is_shift_pressed (void)
{
    gboolean ret;
    GdkDisplay *display;
    XkbStateRec state;
    Bool status;

    ret = FALSE;

    display = cdk_display_get_default ();
    cdk_x11_display_error_trap_push (display);
    status = XkbGetState (GDK_DISPLAY_XDISPLAY (display),
                          XkbUseCoreKbd, &state);
    cdk_x11_display_error_trap_pop_ignored (display);

    if (status == Success)
    {
        ret = state.mods & ShiftMask;
    }

    return ret;
}

enum
{
    AUTORUN_DIALOG_RESPONSE_EJECT = 0
};

typedef struct
{
    CtkWidget *dialog;

    GMount *mount;
    gboolean should_eject;

    gboolean selected_ignore;
    gboolean selected_open_folder;
    GAppInfo *selected_app;

    gboolean remember;

    char *x_content_type;

    BaulAutorunOpenWindow open_window_func;
    gpointer user_data;
} AutorunDialogData;


void
baul_autorun_launch_for_mount (GMount *mount, GAppInfo *app_info)
{
    GFile *root;
    BaulFile *file;
    GList *files;

    root = g_mount_get_root (mount);
    file = baul_file_get (root);
    g_object_unref (root);
    files = g_list_append (NULL, file);
    baul_launch_application (app_info,
                             files,
                             NULL); /* TODO: what to set here? */
    g_object_unref (file);
    g_list_free (files);
}

static void autorun_dialog_mount_unmounted (GMount *mount, AutorunDialogData *data);

static void
autorun_dialog_destroy (AutorunDialogData *data)
{
    g_signal_handlers_disconnect_by_func (G_OBJECT (data->mount),
                                          G_CALLBACK (autorun_dialog_mount_unmounted),
                                          data);

    ctk_widget_destroy (CTK_WIDGET (data->dialog));
    if (data->selected_app != NULL)
    {
        g_object_unref (data->selected_app);
    }
    g_object_unref (data->mount);
    g_free (data->x_content_type);
    g_free (data);
}

static void
autorun_dialog_mount_unmounted (GMount *mount, AutorunDialogData *data)
{
    /* remove the dialog if the media is unmounted */
    autorun_dialog_destroy (data);
}

static void
autorun_dialog_response (CtkDialog *dialog, gint response, AutorunDialogData *data)
{
    switch (response)
    {
    case AUTORUN_DIALOG_RESPONSE_EJECT:
        baul_file_operations_unmount_mount (CTK_WINDOW (dialog),
                                            data->mount,
                                            data->should_eject,
                                            FALSE);
        break;

    case CTK_RESPONSE_NONE:
        /* window was closed */
        break;
    case CTK_RESPONSE_CANCEL:
        break;
    case CTK_RESPONSE_OK:
        /* do the selected action */

        if (data->remember)
        {
            /* make sure we don't ask again */
            baul_autorun_set_preferences (data->x_content_type, TRUE, data->selected_ignore, data->selected_open_folder);
            if (!data->selected_ignore && !data->selected_open_folder && data->selected_app != NULL)
            {
                g_app_info_set_as_default_for_type (data->selected_app,
                                                    data->x_content_type,
                                                    NULL);
            }
        }
        else
        {
            /* make sure we do ask again */
            baul_autorun_set_preferences (data->x_content_type, FALSE, FALSE, FALSE);
        }

        if (!data->selected_ignore && !data->selected_open_folder && data->selected_app != NULL)
        {
            baul_autorun_launch_for_mount (data->mount, data->selected_app);
        }
        else if (!data->selected_ignore && data->selected_open_folder)
        {
            if (data->open_window_func != NULL)
                data->open_window_func (data->mount, data->user_data);
        }
        break;
    }

    autorun_dialog_destroy (data);
}

static void
autorun_combo_changed (gboolean selected_ask,
                       gboolean selected_ignore,
                       gboolean selected_open_folder,
                       GAppInfo *selected_app,
                       gpointer user_data)
{
    AutorunDialogData *data = user_data;

    if (data->selected_app != NULL)
    {
        g_object_unref (data->selected_app);
    }
    data->selected_app = selected_app != NULL ? g_object_ref (selected_app) : NULL;
    data->selected_ignore = selected_ignore;
    data->selected_open_folder = selected_open_folder;
}


static void
autorun_always_toggled (CtkToggleButton *togglebutton, AutorunDialogData *data)
{
    data->remember = ctk_toggle_button_get_active (togglebutton);
}

static gboolean
combo_box_enter_ok (CtkWidget *togglebutton, GdkEventKey *event, CtkDialog *dialog)
{
    if (event->keyval == GDK_KEY_KP_Enter || event->keyval == GDK_KEY_Return)
    {
        ctk_dialog_response (dialog, CTK_RESPONSE_OK);
        return TRUE;
    }
    return FALSE;
}

/* returns TRUE if a folder window should be opened */
static gboolean
do_autorun_for_content_type (GMount *mount, const char *x_content_type, BaulAutorunOpenWindow open_window_func, gpointer user_data)
{
    AutorunDialogData *data;
    CtkWidget *dialog;
    CtkWidget *hbox;
    CtkWidget *vbox;
    CtkWidget *label;
    CtkWidget *combo_box;
    CtkWidget *always_check_button;
    CtkWidget *eject_button;
    CtkWidget *image;
    CtkWidget *action_area;
    char *markup;
    char *content_description;
    char *mount_name;
    GIcon *icon;
    GdkPixbuf *pixbuf;
    cairo_surface_t *surface;
    BaulIconInfo *icon_info;
    int icon_size, icon_scale;
    gboolean user_forced_dialog;
    gboolean pref_ask;
    gboolean pref_start_app;
    gboolean pref_ignore;
    gboolean pref_open_folder;
    char *media_greeting;
    gboolean ret;

    ret = FALSE;
    mount_name = NULL;

    if (g_content_type_is_a (x_content_type, "x-content/win32-software"))
    {
        /* don't pop up the dialog anyway if the content type says
         * windows software.
         */
        goto out;
    }

    user_forced_dialog = is_shift_pressed ();

    baul_autorun_get_preferences (x_content_type, &pref_start_app, &pref_ignore, &pref_open_folder);
    pref_ask = !pref_start_app && !pref_ignore && !pref_open_folder;

    if (user_forced_dialog)
    {
        goto show_dialog;
    }

    if (!pref_ask && !pref_ignore && !pref_open_folder)
    {
        GAppInfo *app_info;
        app_info = g_app_info_get_default_for_type (x_content_type, FALSE);
        if (app_info != NULL)
        {
            baul_autorun_launch_for_mount (mount, app_info);
        }
        goto out;
    }

    if (pref_open_folder)
    {
        ret = TRUE;
        goto out;
    }

    if (pref_ignore)
    {
        goto out;
    }

show_dialog:

    mount_name = g_mount_get_name (mount);

    dialog = ctk_dialog_new ();

    hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
    ctk_box_pack_start (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dialog))), hbox, TRUE, TRUE, 0);
    ctk_container_set_border_width (CTK_CONTAINER (hbox), 12);

    icon = g_mount_get_icon (mount);
    icon_size = baul_get_icon_size_for_stock_size (CTK_ICON_SIZE_DIALOG);
    icon_scale = ctk_widget_get_scale_factor (dialog);
    icon_info = baul_icon_info_lookup (icon, icon_size, icon_scale);
    pixbuf = baul_icon_info_get_pixbuf_at_size (icon_info, icon_size);
    surface = baul_icon_info_get_surface_at_size (icon_info, icon_size);
    g_object_unref (icon_info);
    g_object_unref (icon);
    image = ctk_image_new_from_surface (surface);
    ctk_widget_set_halign (image, CTK_ALIGN_CENTER);
    ctk_widget_set_valign (image, CTK_ALIGN_START);
    ctk_box_pack_start (CTK_BOX (hbox), image, TRUE, TRUE, 0);
    /* also use the icon on the dialog */
    ctk_window_set_title (CTK_WINDOW (dialog), mount_name);
    ctk_window_set_icon (CTK_WINDOW (dialog), pixbuf);
    ctk_window_set_position (CTK_WINDOW (dialog), CTK_WIN_POS_CENTER);
    g_object_unref (pixbuf);
    cairo_surface_destroy (surface);

    vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
    ctk_box_pack_start (CTK_BOX (hbox), vbox, TRUE, TRUE, 0);

    label = ctk_label_new (NULL);


    /* Customize greeting for well-known x-content types */
    if (strcmp (x_content_type, "x-content/audio-cdda") == 0)
    {
        media_greeting = _("You have just inserted an Audio CD.");
    }
    else if (strcmp (x_content_type, "x-content/audio-dvd") == 0)
    {
        media_greeting = _("You have just inserted an Audio DVD.");
    }
    else if (strcmp (x_content_type, "x-content/video-dvd") == 0)
    {
        media_greeting = _("You have just inserted a Video DVD.");
    }
    else if (strcmp (x_content_type, "x-content/video-vcd") == 0)
    {
        media_greeting = _("You have just inserted a Video CD.");
    }
    else if (strcmp (x_content_type, "x-content/video-svcd") == 0)
    {
        media_greeting = _("You have just inserted a Super Video CD.");
    }
    else if (strcmp (x_content_type, "x-content/blank-cd") == 0)
    {
        media_greeting = _("You have just inserted a blank CD.");
    }
    else if (strcmp (x_content_type, "x-content/blank-dvd") == 0)
    {
        media_greeting = _("You have just inserted a blank DVD.");
    }
    else if (strcmp (x_content_type, "x-content/blank-bd") == 0)
    {
        media_greeting = _("You have just inserted a blank Blu-Ray disc.");
    }
    else if (strcmp (x_content_type, "x-content/blank-hddvd") == 0)
    {
        media_greeting = _("You have just inserted a blank HD DVD.");
    }
    else if (strcmp (x_content_type, "x-content/image-photocd") == 0)
    {
        media_greeting = _("You have just inserted a Photo CD.");
    }
    else if (strcmp (x_content_type, "x-content/image-picturecd") == 0)
    {
        media_greeting = _("You have just inserted a Picture CD.");
    }
    else if (strcmp (x_content_type, "x-content/image-dcf") == 0)
    {
        media_greeting = _("You have just inserted a medium with digital photos.");
    }
    else if (strcmp (x_content_type, "x-content/audio-player") == 0)
    {
        media_greeting = _("You have just inserted a digital audio player.");
    }
    else if (g_content_type_is_a (x_content_type, "x-content/software"))
    {
        media_greeting = _("You have just inserted a medium with software intended to be automatically started.");
    }
    else
    {
        /* fallback to generic greeting */
        media_greeting = _("You have just inserted a medium.");
    }
    markup = g_strdup_printf ("<big><b>%s %s</b></big>", media_greeting, _("Choose what application to launch."));
    ctk_label_set_markup (CTK_LABEL (label), markup);
    g_free (markup);
    ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
    ctk_label_set_max_width_chars (CTK_LABEL (label), 50);
    ctk_label_set_xalign (CTK_LABEL (label), 0);
    ctk_box_pack_start (CTK_BOX (vbox), label, TRUE, TRUE, 0);

    label = ctk_label_new (NULL);
    content_description = g_content_type_get_description (x_content_type);
    markup = g_strdup_printf (_("Select how to open \"%s\" and whether to perform this action in the future for other media of type \"%s\"."), mount_name, content_description);
    g_free (content_description);
    ctk_label_set_markup (CTK_LABEL (label), markup);
    g_free (markup);
    ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
    ctk_label_set_max_width_chars (CTK_LABEL (label), 50);
    ctk_label_set_xalign (CTK_LABEL (label), 0);
    ctk_box_pack_start (CTK_BOX (vbox), label, TRUE, TRUE, 0);

    data = g_new0 (AutorunDialogData, 1);
    data->dialog = dialog;
    data->mount = g_object_ref (mount);
    data->remember = !pref_ask;
    data->selected_ignore = pref_ignore;
    data->x_content_type = g_strdup (x_content_type);
    data->selected_app = g_app_info_get_default_for_type (x_content_type, FALSE);
    data->open_window_func = open_window_func;
    data->user_data = user_data;

    combo_box = ctk_combo_box_new ();
    baul_autorun_prepare_combo_box (combo_box, x_content_type, FALSE, TRUE, FALSE, autorun_combo_changed, data);
    g_signal_connect (G_OBJECT (combo_box),
                      "key-press-event",
                      G_CALLBACK (combo_box_enter_ok),
                      dialog);

    ctk_box_pack_start (CTK_BOX (vbox), combo_box, TRUE, TRUE, 0);

    always_check_button = ctk_check_button_new_with_mnemonic (_("_Always perform this action"));
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (always_check_button), data->remember);
    g_signal_connect (G_OBJECT (always_check_button),
                      "toggled",
                      G_CALLBACK (autorun_always_toggled),
                      data);
    ctk_box_pack_start (CTK_BOX (vbox), always_check_button, TRUE, TRUE, 0);

    eel_dialog_add_button (CTK_DIALOG (dialog),
                           _("_Cancel"),
                           "process-stop",
                           CTK_RESPONSE_CANCEL);

    action_area = ctk_widget_get_parent (eel_dialog_add_button (CTK_DIALOG (dialog),
                                                                _("_OK"),
                                                                "ctk-ok",
                                                                CTK_RESPONSE_OK));

    ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);

    if (g_mount_can_eject (mount))
    {
        CtkWidget *eject_image;
        eject_button = ctk_button_new_with_mnemonic (_("_Eject"));
        surface = ctk_icon_theme_load_surface (ctk_icon_theme_get_default (),
                                               "media-eject",
                                               baul_get_icon_size_for_stock_size (CTK_ICON_SIZE_BUTTON),
                                               icon_scale,
                                               NULL,
                                               0,
                                               NULL);
        eject_image = ctk_image_new_from_surface (surface);
        cairo_surface_destroy (surface);
        ctk_button_set_image (CTK_BUTTON (eject_button), eject_image);
        data->should_eject = TRUE;
    }
    else
    {
        eject_button = ctk_button_new_with_mnemonic (_("_Unmount"));
        data->should_eject = FALSE;
    }
    ctk_dialog_add_action_widget (CTK_DIALOG (dialog), eject_button, AUTORUN_DIALOG_RESPONSE_EJECT);
    ctk_button_box_set_child_secondary (CTK_BUTTON_BOX (action_area), eject_button, TRUE);

    /* show the dialog */
    ctk_widget_show_all (dialog);

    g_signal_connect (G_OBJECT (dialog),
                      "response",
                      G_CALLBACK (autorun_dialog_response),
                      data);

    g_signal_connect (G_OBJECT (data->mount),
                      "unmounted",
                      G_CALLBACK (autorun_dialog_mount_unmounted),
                      data);

out:
    g_free (mount_name);
    return ret;
}

typedef struct
{
    GMount *mount;
    BaulAutorunOpenWindow open_window_func;
    gpointer user_data;
} AutorunData;

static void
autorun_guessed_content_type_callback (GObject *source_object,
                                       GAsyncResult *res,
                                       gpointer user_data)
{
    GError *error;
    char **guessed_content_type;
    AutorunData *data = user_data;
    gboolean open_folder;

    open_folder = FALSE;

    error = NULL;
    guessed_content_type = g_mount_guess_content_type_finish (G_MOUNT (source_object), res, &error);
    g_object_set_data_full (source_object,
                            "baul-content-type-cache",
                            g_strdupv (guessed_content_type),
                            (GDestroyNotify)g_strfreev);
    if (error != NULL)
    {
        g_warning ("Unabled to guess content type for mount: %s", error->message);
        g_error_free (error);
    }
    else
    {
        if (guessed_content_type != NULL && g_strv_length (guessed_content_type) > 0)
        {
            int n;
            for (n = 0; guessed_content_type[n] != NULL; n++)
            {
                if (do_autorun_for_content_type (data->mount, guessed_content_type[n],
                                                 data->open_window_func, data->user_data))
                {
                    open_folder = TRUE;
                }
            }
            g_strfreev (guessed_content_type);
        }
        else
        {
            if (g_settings_get_boolean (baul_media_preferences, BAUL_PREFERENCES_MEDIA_AUTOMOUNT_OPEN))
                open_folder = TRUE;
        }
    }

    /* only open the folder once.. */
    if (open_folder && data->open_window_func != NULL)
    {
        data->open_window_func (data->mount, data->user_data);
    }

    g_object_unref (data->mount);
    g_free (data);
}

void
baul_autorun (GMount *mount, BaulAutorunOpenWindow open_window_func, gpointer user_data)
{
    AutorunData *data;

    if (!should_autorun_mount (mount) ||
            g_settings_get_boolean (baul_media_preferences, BAUL_PREFERENCES_MEDIA_AUTORUN_NEVER))
    {
        return;
    }

    data = g_new0 (AutorunData, 1);
    data->mount = g_object_ref (mount);
    data->open_window_func = open_window_func;
    data->user_data = user_data;

    g_mount_guess_content_type (mount,
                                FALSE,
                                NULL,
                                autorun_guessed_content_type_callback,
                                data);
}

typedef struct
{
    BaulAutorunGetContent callback;
    gpointer user_data;
} GetContentTypesData;

static void
get_types_cb (GObject *source_object,
              GAsyncResult *res,
              gpointer user_data)
{
    GetContentTypesData *data;
    char **types;

    data = user_data;
    types = g_mount_guess_content_type_finish (G_MOUNT (source_object), res, NULL);

    g_object_set_data_full (source_object,
                            "baul-content-type-cache",
                            g_strdupv (types),
                            (GDestroyNotify)g_strfreev);

    if (data->callback)
    {
        data->callback (types, data->user_data);
    }
    g_strfreev (types);
    g_free (data);
}

void
baul_autorun_get_x_content_types_for_mount_async (GMount *mount,
        BaulAutorunGetContent callback,
        GCancellable *cancellable,
        gpointer user_data)
{
    char **cached;
    GetContentTypesData *data;

    if (mount == NULL)
    {
        if (callback)
        {
            callback (NULL, user_data);
        }
        return;
    }

    cached = g_object_get_data (G_OBJECT (mount), "baul-content-type-cache");
    if (cached != NULL)
    {
        if (callback)
        {
            callback (cached, user_data);
        }
        return;
    }

    data = g_new (GetContentTypesData, 1);
    data->callback = callback;
    data->user_data = user_data;

    g_mount_guess_content_type (mount,
                                FALSE,
                                cancellable,
                                get_types_cb,
                                data);
}


char **
baul_autorun_get_cached_x_content_types_for_mount (GMount      *mount)
{
    char **cached;

    if (mount == NULL)
    {
        return NULL;
    }

    cached = g_object_get_data (G_OBJECT (mount), "baul-content-type-cache");
    if (cached != NULL)
    {
        return g_strdupv (cached);
    }

    return NULL;
}

static gboolean
remove_allow_volume (gpointer data)
{
    GVolume *volume = data;

    g_object_set_data (G_OBJECT (volume), "baul-allow-autorun", NULL);
    return FALSE;
}

void
baul_allow_autorun_for_volume (GVolume *volume)
{
    g_object_set_data (G_OBJECT (volume), "baul-allow-autorun", GINT_TO_POINTER (1));
}

#define INHIBIT_AUTORUN_SECONDS 10

void
baul_allow_autorun_for_volume_finish (GVolume *volume)
{
    if (g_object_get_data (G_OBJECT (volume), "baul-allow-autorun") != NULL)
    {
        g_timeout_add_seconds_full (0,
                                    INHIBIT_AUTORUN_SECONDS,
                                    remove_allow_volume,
                                    g_object_ref (volume),
                                    g_object_unref);
    }
}

static gboolean
should_skip_native_mount_root (GFile *root)
{
    char *path;
    gboolean should_skip;

    /* skip any mounts in hidden directory hierarchies */
    path = g_file_get_path (root);
    should_skip = strstr (path, "/.") != NULL;
    g_free (path);

    return should_skip;
}

static gboolean
should_autorun_mount (GMount *mount)
{
    GFile *root;
    GVolume *enclosing_volume;
    gboolean ignore_autorun;

    ignore_autorun = TRUE;
    enclosing_volume = g_mount_get_volume (mount);
    if (enclosing_volume != NULL)
    {
        if (g_object_get_data (G_OBJECT (enclosing_volume), "baul-allow-autorun") != NULL)
        {
            ignore_autorun = FALSE;
            g_object_set_data (G_OBJECT (enclosing_volume), "baul-allow-autorun", NULL);
        }
    }

    if (ignore_autorun)
    {
        if (enclosing_volume != NULL)
        {
            g_object_unref (enclosing_volume);
        }
        return FALSE;
    }

    root = g_mount_get_root (mount);

    /* only do autorun on local files or files where g_volume_should_automount() returns TRUE */
    ignore_autorun = TRUE;
    if ((g_file_is_native (root) && !should_skip_native_mount_root (root)) ||
            (enclosing_volume != NULL && g_volume_should_automount (enclosing_volume)))
    {
        ignore_autorun = FALSE;
    }
    if (enclosing_volume != NULL)
    {
        g_object_unref (enclosing_volume);
    }
    g_object_unref (root);

    return !ignore_autorun;
}
