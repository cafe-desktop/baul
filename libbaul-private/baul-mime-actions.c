/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* baul-mime-actions.c - uri-specific versions of mime action functions

   Copyright (C) 2000, 2001 Eazel, Inc.

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

   Authors: Maciej Stachowiak <mjs@eazel.com>
*/

#include <config.h>
#include <string.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <cdk/cdkx.h>

#include <eel/eel-glib-extensions.h>
#include <eel/eel-stock-dialogs.h>
#include <eel/eel-string.h>

#include "baul-mime-actions.h"
#include "baul-file-attributes.h"
#include "baul-file.h"
#include "baul-autorun.h"
#include "baul-file-operations.h"
#include "baul-metadata.h"
#include "baul-program-choosing.h"
#include "baul-desktop-icon-file.h"
#include "baul-global-preferences.h"
#include "baul-debug-log.h"
#include "baul-open-with-dialog.h"

typedef enum
{
    ACTIVATION_ACTION_LAUNCH_DESKTOP_FILE,
    ACTIVATION_ACTION_ASK,
    ACTIVATION_ACTION_LAUNCH,
    ACTIVATION_ACTION_LAUNCH_IN_TERMINAL,
    ACTIVATION_ACTION_OPEN_IN_VIEW,
    ACTIVATION_ACTION_OPEN_IN_APPLICATION,
    ACTIVATION_ACTION_DO_NOTHING,
} ActivationAction;

typedef struct
{
    BaulFile *file;
    char *uri;
} LaunchLocation;

typedef struct
{
    GAppInfo *application;
    GList *uris;
} ApplicationLaunchParameters;

typedef struct
{
    BaulWindowSlotInfo *slot_info;
    gpointer window_info;
    CtkWindow *parent_window;
    GCancellable *cancellable;
    GList *locations;
    GList *mountables;
    GList *start_mountables;
    GList *not_mounted;
    BaulWindowOpenMode mode;
    BaulWindowOpenFlags flags;
    char *timed_wait_prompt;
    gboolean timed_wait_active;
    BaulFileListHandle *files_handle;
    gboolean tried_mounting;
    char *activation_directory;
    gboolean user_confirmation;
} ActivateParameters;

/* Number of seconds until cancel dialog shows up */
#define DELAY_UNTIL_CANCEL_MSECS 5000

#define RESPONSE_RUN 1000
#define RESPONSE_DISPLAY 1001
#define RESPONSE_RUN_IN_TERMINAL 1002
#define RESPONSE_MARK_TRUSTED 1003

#define SILENT_WINDOW_OPEN_LIMIT 5

/* This number controls a maximum character count for a URL that is
 * displayed as part of a dialog. It's fairly arbitrary -- big enough
 * to allow most "normal" URIs to display in full, but small enough to
 * prevent the dialog from getting insanely wide.
 */
#define MAX_URI_IN_DIALOG_LENGTH 60

static void cancel_activate_callback                (gpointer            callback_data);
static void activate_activation_uris_ready_callback (GList              *files,
        gpointer            callback_data);
static void activation_mount_mountables             (ActivateParameters *parameters);
static void activation_start_mountables             (ActivateParameters *parameters);
static void activate_callback                       (GList              *files,
        gpointer            callback_data);
static void activation_mount_not_mounted            (ActivateParameters *parameters);


static void
launch_location_free (LaunchLocation *location)
{
    baul_file_unref (location->file);
    g_free (location->uri);
    g_free (location);
}

static void
launch_location_list_free (GList *list)
{
    g_list_foreach (list, (GFunc)launch_location_free, NULL);
    g_list_free (list);
}

static GList *
get_file_list_for_launch_locations (GList *locations)
{
    GList *files, *l;
    LaunchLocation *location = NULL;

    files = NULL;
    for (l = locations; l != NULL; l = l->next)
    {
        location = l->data;

        files = g_list_prepend (files,
                                baul_file_ref (location->file));
    }
    return g_list_reverse (files);
}


static LaunchLocation *
launch_location_from_file (BaulFile *file)
{
    LaunchLocation *location;
    location = g_new (LaunchLocation, 1);
    location->file = baul_file_ref (file);
    location->uri = baul_file_get_uri (file);

    return location;
}

static void
launch_location_update_from_file (LaunchLocation *location,
                                  BaulFile *file)
{
    baul_file_unref (location->file);
    g_free (location->uri);
    location->file = baul_file_ref (file);
    location->uri = baul_file_get_uri (file);
}

static void
launch_location_update_from_uri (LaunchLocation *location,
                                 const char *uri)
{
    baul_file_unref (location->file);
    g_free (location->uri);
    location->file = baul_file_get_by_uri (uri);
    location->uri = g_strdup (uri);
}

static LaunchLocation *
find_launch_location_for_file (GList *list,
                               BaulFile *file)
{
    GList *l;
    LaunchLocation *location = NULL;

    for (l = list; l != NULL; l = l->next)
    {
        location = l->data;

        if (location->file == file)
        {
            return location;
        }
    }
    return NULL;
}

static GList *
launch_locations_from_file_list (GList *list)
{
    GList *new;

    new = NULL;
    while (list)
    {
        new = g_list_prepend (new,
                              launch_location_from_file (list->data));
        list = list->next;
    }
    new = g_list_reverse (new);
    return new;
}

static ApplicationLaunchParameters *
application_launch_parameters_new (GAppInfo *application,
                                   GList *uris)
{
    ApplicationLaunchParameters *result;

    result = g_new0 (ApplicationLaunchParameters, 1);
    result->application = g_object_ref (application);
    result->uris = g_list_copy_deep (uris, (GCopyFunc) g_strdup, NULL);

    return result;
}

static void
application_launch_parameters_free (ApplicationLaunchParameters *parameters)
{
    g_object_unref (parameters->application);
    g_list_free_full (parameters->uris, g_free);

    g_free (parameters);
}

static GList*
filter_baul_handler (GList *apps)
{
    GList *l, *next;
    GAppInfo *application = NULL;

    l = apps;
    while (l != NULL)
    {
        const char *id;

        application = (GAppInfo *) l->data;
        next = l->next;

        id = g_app_info_get_id (application);
        if (id != NULL &&
                strcmp (id,
                        "baul-folder-handler.desktop") == 0)
        {
            g_object_unref (application);
            apps = g_list_delete_link (apps, l);
        }

        l = next;
    }

    return apps;
}

static GList*
filter_non_uri_apps (GList *apps)
{
    GList *l, *next;
    GAppInfo *app = NULL;

    for (l = apps; l != NULL; l = next)
    {
        app = l->data;
        next = l->next;

        if (!g_app_info_supports_uris (app))
        {
            apps = g_list_delete_link (apps, l);
            g_object_unref (app);
        }
    }
    return apps;
}


static gboolean
baul_mime_actions_check_if_required_attributes_ready (BaulFile *file)
{
    BaulFileAttributes attributes;
    gboolean ready;

    attributes = baul_mime_actions_get_required_file_attributes ();
    ready = baul_file_check_if_ready (file, attributes);

    return ready;
}

BaulFileAttributes
baul_mime_actions_get_required_file_attributes (void)
{
    return BAUL_FILE_ATTRIBUTE_INFO |
           BAUL_FILE_ATTRIBUTE_LINK_INFO;
}

static gboolean
file_has_local_path (BaulFile *file)
{
    GFile *location;
    char *path;
    gboolean res;


    /* Don't only check _is_native, because we want to support
       using the fuse path */
    location = baul_file_get_location (file);
    if (g_file_is_native (location))
    {
        res = TRUE;
    }
    else
    {
        path = g_file_get_path (location);

        res = path != NULL;

        g_free (path);
    }
    g_object_unref (location);

    return res;
}

GAppInfo *
baul_mime_get_default_application_for_file (BaulFile *file)
{
    GAppInfo *app;
    char *mime_type;

    if (!baul_mime_actions_check_if_required_attributes_ready (file))
    {
        return NULL;
    }

    mime_type = baul_file_get_mime_type (file);
    app = g_app_info_get_default_for_type (mime_type, !file_has_local_path (file));
    g_free (mime_type);

    if (app == NULL)
    {
        char *uri_scheme;

        uri_scheme = baul_file_get_uri_scheme (file);
        if (uri_scheme != NULL)
        {
            app = g_app_info_get_default_for_uri_scheme (uri_scheme);
            g_free (uri_scheme);
        }
    }

    return app;
}

static int
file_compare_by_mime_type (BaulFile *file_a,
                           BaulFile *file_b)
{
    char *mime_type_a, *mime_type_b;
    int ret;

    mime_type_a = baul_file_get_mime_type (file_a);
    mime_type_b = baul_file_get_mime_type (file_b);

    ret = strcmp (mime_type_a, mime_type_b);

    g_free (mime_type_a);
    g_free (mime_type_b);

    return ret;
}

static int
file_compare_by_parent_uri (BaulFile *file_a,
                            BaulFile *file_b)
{
    char *parent_uri_a, *parent_uri_b;
    int ret;

    parent_uri_a = baul_file_get_parent_uri (file_a);
    parent_uri_b = baul_file_get_parent_uri (file_b);

    ret = strcmp (parent_uri_a, parent_uri_b);

    g_free (parent_uri_a);
    g_free (parent_uri_b);

    return ret;
}

static int
application_compare_by_name (const GAppInfo *app_a,
                             const GAppInfo *app_b)
{
    return g_utf8_collate (g_app_info_get_display_name ((GAppInfo *)app_a),
                           g_app_info_get_display_name ((GAppInfo *)app_b));
}

static int
application_compare_by_id (const GAppInfo *app_a,
                           const GAppInfo *app_b)
{
    const char *id_a, *id_b;

    id_a = g_app_info_get_id ((GAppInfo *)app_a);
    id_b = g_app_info_get_id ((GAppInfo *)app_b);

    if (id_a == NULL && id_b == NULL)
    {
        if (g_app_info_equal ((GAppInfo *)app_a, (GAppInfo *)app_b))
        {
            return 0;
        }
        if ((gsize)app_a < (gsize) app_b)
        {
            return -1;
        }
        return 1;
    }

    if (id_a == NULL)
    {
        return -1;
    }

    if (id_b == NULL)
    {
        return 1;
    }


    return strcmp (id_a, id_b);
}

GList *
baul_mime_get_applications_for_file (BaulFile *file)
{
    char *mime_type;
    char *uri_scheme;
    GList *result;

    if (!baul_mime_actions_check_if_required_attributes_ready (file))
    {
        return NULL;
    }
    mime_type = baul_file_get_mime_type (file);
    result = g_app_info_get_all_for_type (mime_type);

    uri_scheme = baul_file_get_uri_scheme (file);
    if (uri_scheme != NULL)
    {
        GAppInfo *uri_handler;

        uri_handler = g_app_info_get_default_for_uri_scheme (uri_scheme);
        if (uri_handler)
        {
            result = g_list_prepend (result, uri_handler);
        }
        g_free (uri_scheme);
    }

    if (!file_has_local_path (file))
    {
        /* Filter out non-uri supporting apps */
        result = filter_non_uri_apps (result);
    }

    result = g_list_sort (result, (GCompareFunc) application_compare_by_name);
    g_free (mime_type);

    return filter_baul_handler (result);
}

gboolean
baul_mime_has_any_applications_for_file (BaulFile *file)
{
    GList *apps;
    char *mime_type;
    gboolean result;
    char *uri_scheme;

    mime_type = baul_file_get_mime_type (file);

    apps = g_app_info_get_all_for_type (mime_type);

    uri_scheme = baul_file_get_uri_scheme (file);
    if (uri_scheme != NULL)
    {
        GAppInfo *uri_handler;

        uri_handler = g_app_info_get_default_for_uri_scheme (uri_scheme);
        if (uri_handler)
        {
            apps = g_list_prepend (apps, uri_handler);
        }
        g_free (uri_scheme);
    }

    if (!file_has_local_path (file))
    {
        /* Filter out non-uri supporting apps */
        apps = filter_non_uri_apps (apps);
    }
    apps = filter_baul_handler (apps);

    if (apps)
    {
        result = TRUE;
        g_list_free_full (apps, g_object_unref);
    }
    else
    {
        result = FALSE;
    }

    g_free (mime_type);

    return result;
}

GAppInfo *
baul_mime_get_default_application_for_files (GList *files)
{
    GList *l, *sorted_files;
    GAppInfo *app, *one_app;
    BaulFile *file = NULL;

    g_assert (files != NULL);

    sorted_files = g_list_sort (g_list_copy (files), (GCompareFunc) file_compare_by_mime_type);

    app = NULL;
    for (l = sorted_files; l != NULL; l = l->next)
    {
        file = l->data;

        if (l->prev &&
                file_compare_by_mime_type (file, l->prev->data) == 0 &&
                file_compare_by_parent_uri (file, l->prev->data) == 0)
        {
            continue;
        }

        one_app = baul_mime_get_default_application_for_file (file);
        if (one_app == NULL || (app != NULL && !g_app_info_equal (app, one_app)))
        {
            if (app)
            {
                g_object_unref (app);
            }
            if (one_app)
            {
                g_object_unref (one_app);
            }
            app = NULL;
            break;
        }

        if (app == NULL)
        {
            app = one_app;
        }
        else
        {
            g_object_unref (one_app);
        }
    }

    g_list_free (sorted_files);

    return app;
}

/* returns an intersection of two mime application lists,
 * and returns a new list, freeing a, b and all applications
 * that are not in the intersection set.
 * The lists are assumed to be pre-sorted by their IDs */
static GList *
intersect_application_lists (GList *a,
                             GList *b)
{
    GList *l, *m;
    GList *ret;
    GAppInfo *a_app = NULL;
    GAppInfo *b_app = NULL;

    ret = NULL;

    l = a;
    m = b;

    while (l != NULL && m != NULL)
    {
        int cmp;

        a_app = (GAppInfo *) l->data;
        b_app = (GAppInfo *) m->data;

        cmp = application_compare_by_id (a_app, b_app);
        if (cmp > 0)
        {
            g_object_unref (b_app);
            m = m->next;
        }
        else if (cmp < 0)
        {
            g_object_unref (a_app);
            l = l->next;
        }
        else
        {
            g_object_unref (b_app);
            ret = g_list_prepend (ret, a_app);
            l = l->next;
            m = m->next;
        }
    }

    g_list_foreach (l, (GFunc) g_object_unref, NULL);
    g_list_foreach (m, (GFunc) g_object_unref, NULL);

    g_list_free (a);
    g_list_free (b);

    return g_list_reverse (ret);
}

GList *
baul_mime_get_applications_for_files (GList *files)
{
    GList *l, *sorted_files;
    GList *one_ret, *ret;
    BaulFile *file = NULL;

    g_assert (files != NULL);

    sorted_files = g_list_sort (g_list_copy (files), (GCompareFunc) file_compare_by_mime_type);

    ret = NULL;
    for (l = sorted_files; l != NULL; l = l->next)
    {
        file = l->data;

        if (l->prev &&
                file_compare_by_mime_type (file, l->prev->data) == 0 &&
                file_compare_by_parent_uri (file, l->prev->data) == 0)
        {
            continue;
        }

        one_ret = baul_mime_get_applications_for_file (file);
        one_ret = g_list_sort (one_ret, (GCompareFunc) application_compare_by_id);
        if (ret != NULL)
        {
            ret = intersect_application_lists (ret, one_ret);
        }
        else
        {
            ret = one_ret;
        }

        if (ret == NULL)
        {
            break;
        }
    }

    g_list_free (sorted_files);

    ret = g_list_sort (ret, (GCompareFunc) application_compare_by_name);

    return ret;
}

static void
trash_or_delete_files (CtkWindow   *parent_window,
		       const GList *files,
		       gboolean     delete_if_all_already_in_trash G_GNUC_UNUSED)
{
    GList *locations;
    const GList *node;

    locations = NULL;
    for (node = files; node != NULL; node = node->next)
    {
        locations = g_list_prepend (locations,
                                    baul_file_get_location ((BaulFile *) node->data));
    }

    locations = g_list_reverse (locations);

    baul_file_operations_trash_or_delete (locations,
                                          parent_window,
                                          NULL, NULL);
    g_list_free_full (locations, g_object_unref);
}

static void
report_broken_symbolic_link (CtkWindow *parent_window, BaulFile *file)
{
    char *target_path;
    char *display_name;
    char *prompt;
    char *detail;
    CtkDialog *dialog;
    GList file_as_list;
    int response;

    g_assert (baul_file_is_broken_symbolic_link (file));

    display_name = baul_file_get_display_name (file);
    if (baul_file_is_in_trash (file))
    {
        prompt = g_strdup_printf (_("The Link \"%s\" is Broken."), display_name);
    }
    else
    {
        prompt = g_strdup_printf (_("The Link \"%s\" is Broken. Move it to Trash?"), display_name);
    }
    g_free (display_name);

    target_path = baul_file_get_symbolic_link_target_path (file);
    if (target_path == NULL)
    {
        detail = g_strdup (_("This link cannot be used, because it has no target."));
    }
    else
    {
        detail = g_strdup_printf (_("This link cannot be used, because its target "
                                    "\"%s\" doesn't exist."), target_path);
    }

    if (baul_file_is_in_trash (file))
    {
        eel_run_simple_dialog (CTK_WIDGET (parent_window), FALSE, CTK_MESSAGE_WARNING,
                               prompt, detail, "process-stop", NULL);
        goto out;
    }

    dialog = eel_show_yes_no_dialog (prompt, detail, _("Mo_ve to Trash"), "process-stop",
                                     parent_window);

    ctk_dialog_set_default_response (dialog, CTK_RESPONSE_CANCEL);

    /* Make this modal to avoid problems with reffing the view & file
     * to keep them around in case the view changes, which would then
     * cause the old view not to be destroyed, which would cause its
     * merged CafeComponent items not to be un-merged. Maybe we need to unmerge
     * explicitly when disconnecting views instead of relying on the
     * unmerge in Destroy. But since CafeComponentUIHandler is probably going
     * to change wildly, I don't want to mess with this now.
     */

    response = ctk_dialog_run (dialog);
    ctk_widget_destroy (CTK_WIDGET (dialog));

    if (response == CTK_RESPONSE_YES)
    {
        file_as_list.data = file;
        file_as_list.next = NULL;
        file_as_list.prev = NULL;
        trash_or_delete_files (parent_window, &file_as_list, TRUE);
    }

out:
    g_free (prompt);
    g_free (target_path);
    g_free (detail);
}

static ActivationAction
get_executable_text_file_action (CtkWindow *parent_window, BaulFile *file)
{
    CtkDialog *dialog;
    char *file_name;
    char *prompt;
    char *detail;
    int preferences_value;
    int response;

    g_assert (baul_file_contains_text (file));

    preferences_value = g_settings_get_enum (baul_preferences,
                            BAUL_PREFERENCES_EXECUTABLE_TEXT_ACTIVATION);
    switch (preferences_value)
    {
    case BAUL_EXECUTABLE_TEXT_LAUNCH:
        return ACTIVATION_ACTION_LAUNCH;
    case BAUL_EXECUTABLE_TEXT_DISPLAY:
        return ACTIVATION_ACTION_OPEN_IN_APPLICATION;
    case BAUL_EXECUTABLE_TEXT_ASK:
        break;
    default:
        /* Complain non-fatally, since preference data can't be trusted */
        g_warning ("Unknown value %d for BAUL_PREFERENCES_EXECUTABLE_TEXT_ACTIVATION",
                   preferences_value);

    }


    file_name = baul_file_get_display_name (file);
    prompt = g_strdup_printf (_("Do you want to run \"%s\", or display its contents?"),
                              file_name);
    detail = g_strdup_printf (_("\"%s\" is an executable text file."),
                              file_name);
    g_free (file_name);

    dialog = eel_create_question_dialog (prompt,
                                         detail,
                                         _("Run in _Terminal"), RESPONSE_RUN_IN_TERMINAL,
                                         _("_Display"), RESPONSE_DISPLAY,
                                         parent_window);

    eel_dialog_add_button (dialog,
                           _("_Cancel"),
                           "process-stop",
                           CTK_RESPONSE_CANCEL);

    ctk_dialog_add_button (dialog, _("_Run"), RESPONSE_RUN);
    ctk_dialog_set_default_response (dialog, CTK_RESPONSE_CANCEL);
    ctk_widget_show (CTK_WIDGET (dialog));

    g_free (prompt);
    g_free (detail);

    response = ctk_dialog_run (dialog);
    ctk_widget_destroy (CTK_WIDGET (dialog));

    switch (response)
    {
    case RESPONSE_RUN:
        return ACTIVATION_ACTION_LAUNCH;
    case RESPONSE_RUN_IN_TERMINAL:
        return ACTIVATION_ACTION_LAUNCH_IN_TERMINAL;
    case RESPONSE_DISPLAY:
        return ACTIVATION_ACTION_OPEN_IN_APPLICATION;
    default:
        return ACTIVATION_ACTION_DO_NOTHING;
    }
}

static ActivationAction
get_default_executable_text_file_action (void)
{
    int preferences_value;

    preferences_value = g_settings_get_enum (baul_preferences,
                            BAUL_PREFERENCES_EXECUTABLE_TEXT_ACTIVATION);
    switch (preferences_value)
    {
    case BAUL_EXECUTABLE_TEXT_LAUNCH:
        return ACTIVATION_ACTION_LAUNCH;
    case BAUL_EXECUTABLE_TEXT_DISPLAY:
        return ACTIVATION_ACTION_OPEN_IN_APPLICATION;
    case BAUL_EXECUTABLE_TEXT_ASK:
    default:
        return ACTIVATION_ACTION_ASK;
    }
}

gboolean
baul_mime_file_opens_in_view (BaulFile *file)
{
    return (baul_file_is_directory (file));
}

static ActivationAction
get_activation_action (BaulFile *file)
{
    ActivationAction action;
    char *activation_uri;

    if (baul_file_is_baul_link (file))
    {
        return ACTIVATION_ACTION_LAUNCH_DESKTOP_FILE;
    }

    activation_uri = baul_file_get_activation_uri (file);
    if (activation_uri == NULL)
    {
        activation_uri = baul_file_get_uri (file);
    }

    action = ACTIVATION_ACTION_DO_NOTHING;
    if (baul_file_is_launchable (file))
    {
        char *executable_path;

        action = ACTIVATION_ACTION_LAUNCH;

        executable_path = g_filename_from_uri (activation_uri, NULL, NULL);
        if (!executable_path)
        {
            action = ACTIVATION_ACTION_DO_NOTHING;
        }
        else if (baul_file_contains_text (file))
        {
            action = get_default_executable_text_file_action ();
        }
        g_free (executable_path);
    }

    if (action == ACTIVATION_ACTION_DO_NOTHING)
    {
        if (baul_mime_file_opens_in_view (file))
        {
            action = ACTIVATION_ACTION_OPEN_IN_VIEW;
        }
        else
        {
            action = ACTIVATION_ACTION_OPEN_IN_APPLICATION;
        }
    }
    g_free (activation_uri);

    return action;
}

gboolean
baul_mime_file_opens_in_external_app (BaulFile *file)
{
    ActivationAction activation_action;

    activation_action = get_activation_action (file);

    return (activation_action == ACTIVATION_ACTION_OPEN_IN_APPLICATION);
}


static unsigned int
mime_application_hash (GAppInfo *app)
{
    const char *id;

    id = g_app_info_get_id (app);

    if (id == NULL)
    {
        return GPOINTER_TO_UINT(app);
    }

    return g_str_hash (id);
}

static void
list_to_parameters_foreach (GAppInfo *application,
                            GList *uris,
                            GList **ret)
{
    ApplicationLaunchParameters *parameters;

    uris = g_list_reverse (uris);

    parameters = application_launch_parameters_new
                 (application, uris);
    *ret = g_list_prepend (*ret, parameters);
}


/**
 * make_activation_parameters
 *
 * Construct a list of ApplicationLaunchParameters from a list of BaulFiles,
 * where files that have the same default application are put into the same
 * launch parameter, and others are put into the unhandled_files list.
 *
 * @files: Files to use for construction.
 * @unhandled_files: Files without any default application will be put here.
 *
 * Return value: Newly allocated list of ApplicationLaunchParameters.
 **/
static GList *
make_activation_parameters (GList *uris,
                            GList **unhandled_uris)
{
    GList *ret, *l, *app_uris;
    GHashTable *app_table;
    GAppInfo *old_app;
    GAppInfo *app = NULL;
    BaulFile *file = NULL;

    ret = NULL;
    *unhandled_uris = NULL;

    app_table = g_hash_table_new_full
                ((GHashFunc) mime_application_hash,
                 (GEqualFunc) g_app_info_equal,
                 (GDestroyNotify) g_object_unref,
                 (GDestroyNotify) g_list_free);

    for (l = uris; l != NULL; l = l->next)
    {
        char *uri;

        uri = l->data;
        file = baul_file_get_by_uri (uri);

        /* Double-check if a file's MIME type has changed before we commit to a
           choice of application for it. This can happen if, for instance, file
           was originally created with 0 bytes and then content was added to it
           later-- it will change from plaintext to something else. */
        baul_file_refresh_info (file);

        app = baul_mime_get_default_application_for_file (file);
        if (app != NULL)
        {
            app_uris = NULL;

            if (g_hash_table_lookup_extended (app_table, app,
                                              (gpointer *) &old_app,
                                              (gpointer *) &app_uris))
            {
                g_hash_table_steal (app_table, old_app);

                app_uris = g_list_prepend (app_uris, uri);

                g_object_unref (app);
                app = old_app;
            }
            else
            {
                app_uris = g_list_prepend (NULL, uri);
            }

            g_hash_table_insert (app_table, app, app_uris);
        }
        else
        {
            *unhandled_uris = g_list_prepend (*unhandled_uris, uri);
        }
        baul_file_unref (file);
    }

    g_hash_table_foreach (app_table,
                          (GHFunc) list_to_parameters_foreach,
                          &ret);

    g_hash_table_destroy (app_table);

    *unhandled_uris = g_list_reverse (*unhandled_uris);

    return g_list_reverse (ret);
}

static gboolean
file_was_cancelled (BaulFile *file)
{
    GError *error;

    error = baul_file_get_file_info_error (file);
    return
        error != NULL &&
        error->domain == G_IO_ERROR &&
        error->code == G_IO_ERROR_CANCELLED;
}

static gboolean
file_was_not_mounted (BaulFile *file)
{
    GError *error;

    error = baul_file_get_file_info_error (file);
    return
        error != NULL &&
        error->domain == G_IO_ERROR &&
        error->code == G_IO_ERROR_NOT_MOUNTED;
}

static void
activation_parameters_free (ActivateParameters *parameters)
{
    if (parameters->timed_wait_active)
    {
        eel_timed_wait_stop (cancel_activate_callback, parameters);
    }

    if (parameters->slot_info)
    {
        g_object_remove_weak_pointer (G_OBJECT (parameters->slot_info), (gpointer *)&parameters->slot_info);
    }
    if (parameters->parent_window)
    {
        g_object_remove_weak_pointer (G_OBJECT (parameters->parent_window), (gpointer *)&parameters->parent_window);
    }
    g_object_unref (parameters->cancellable);
    launch_location_list_free (parameters->locations);
    baul_file_list_free (parameters->mountables);
    baul_file_list_free (parameters->start_mountables);
    baul_file_list_free (parameters->not_mounted);
    g_free (parameters->activation_directory);
    g_free (parameters->timed_wait_prompt);
    g_assert (parameters->files_handle == NULL);
    g_free (parameters);
}

static void
cancel_activate_callback (gpointer callback_data)
{
    ActivateParameters *parameters = callback_data;

    parameters->timed_wait_active = FALSE;

    g_cancellable_cancel (parameters->cancellable);

    if (parameters->files_handle)
    {
        baul_file_list_cancel_call_when_ready (parameters->files_handle);
        parameters->files_handle = NULL;
        activation_parameters_free (parameters);
    }
}

static void
activation_start_timed_cancel (ActivateParameters *parameters)
{
    parameters->timed_wait_active = TRUE;
    eel_timed_wait_start_with_duration
    (DELAY_UNTIL_CANCEL_MSECS,
     cancel_activate_callback,
     parameters,
     parameters->timed_wait_prompt,
     parameters->parent_window);
}

static void
pause_activation_timed_cancel (ActivateParameters *parameters)
{
    if (parameters->timed_wait_active)
    {
        eel_timed_wait_stop (cancel_activate_callback, parameters);
        parameters->timed_wait_active = FALSE;
    }
}

static void
unpause_activation_timed_cancel (ActivateParameters *parameters)
{
    if (!parameters->timed_wait_active)
    {
        activation_start_timed_cancel (parameters);
    }
}


static void
activate_mount_op_active (CtkMountOperation  *operation,
			  GParamSpec         *pspec G_GNUC_UNUSED,
			  ActivateParameters *parameters)
{
    gboolean is_active;

    g_object_get (operation, "is-showing", &is_active, NULL);

    if (is_active)
    {
        pause_activation_timed_cancel (parameters);
    }
    else
    {
        unpause_activation_timed_cancel (parameters);
    }
}

static gboolean
confirm_multiple_windows (CtkWindow *parent_window,
                          int count,
                          gboolean use_tabs)
{
    CtkDialog *dialog;
    char *prompt;
    char *detail;
    int response;

    if (count <= SILENT_WINDOW_OPEN_LIMIT)
    {
        return TRUE;
    }

    prompt = _("Are you sure you want to open all files?");
    if (use_tabs)
    {
        detail = g_strdup_printf (ngettext("This will open %d separate tab.",
                                           "This will open %d separate tabs.", count), count);
    }
    else
    {
        detail = g_strdup_printf (ngettext("This will open %d separate window.",
                                           "This will open %d separate windows.", count), count);
    }
    dialog = eel_show_yes_no_dialog (prompt, detail,
                                     "ctk-ok", "process-stop",
                                     parent_window);
    g_free (detail);

    response = ctk_dialog_run (dialog);
    ctk_widget_destroy (CTK_WIDGET (dialog));

    return response == CTK_RESPONSE_YES;
}

typedef struct
{
    BaulWindowSlotInfo *slot_info;
    CtkWindow *parent_window;
    BaulFile *file;
    GList *files;
    BaulWindowOpenMode mode;
    BaulWindowOpenFlags flags;
    char *activation_directory;
    gboolean user_confirmation;
    char *uri;
    GDBusProxy *proxy;
    CtkWidget *dialog;
} ActivateParametersInstall;

static void
activate_parameters_install_free (ActivateParametersInstall *parameters_install)
{
    if (parameters_install->slot_info)
    {
        g_object_remove_weak_pointer (G_OBJECT (parameters_install->slot_info), (gpointer *)&parameters_install->slot_info);
    }
    if (parameters_install->parent_window)
    {
        g_object_remove_weak_pointer (G_OBJECT (parameters_install->parent_window), (gpointer *)&parameters_install->parent_window);
    }

    if (parameters_install->proxy != NULL)
    {
        g_object_unref (parameters_install->proxy);
    }

    baul_file_unref (parameters_install->file);
    baul_file_list_free (parameters_install->files);
    g_free (parameters_install->activation_directory);
    g_free (parameters_install->uri);
    g_free (parameters_install);
}

static char *
get_application_no_mime_type_handler_message (BaulFile *file, char *uri)
{
    char *uri_for_display;
    char *nice_uri;
    char *error_message;

    /* For local files, we want to use filename if possible */
    if (baul_file_is_local (file))
    {
        GFile *location;

        location = baul_file_get_location (file);
        nice_uri = g_file_get_parse_name (location);
        g_object_unref (location);
    }
    else
    {
        nice_uri = g_strdup (uri);
    }

    /* Truncate the URI so it doesn't get insanely wide. Note that even
     * though the dialog uses wrapped text, if the URI doesn't contain
     * white space then the text-wrapping code is too stupid to wrap it.
     */
    uri_for_display = eel_str_middle_truncate (nice_uri, MAX_URI_IN_DIALOG_LENGTH);
    error_message = g_strdup_printf (_("Could not display \"%s\"."), uri_for_display);
    g_free (nice_uri);
    g_free (uri_for_display);
    return error_message;
}

static void
application_selected_cb (BaulOpenWithDialog *dialog,
                         GAppInfo *app,
                         gpointer user_data)
{
    CtkWindow *parent_window;
    BaulFile *file;
    GList files;

    parent_window = CTK_WINDOW (user_data);

    file = g_object_get_data (G_OBJECT (dialog), "mime-action:file");

    files.next = NULL;
    files.prev = NULL;
    files.data = file;
    baul_launch_application (app, &files, parent_window);
}

static void
choose_program (CtkDialog *message_dialog, int response, gpointer callback_data)
{
    CtkWidget *dialog;
    char *uri;
    char *mime_type;
    BaulFile *file;

    if (response != CTK_RESPONSE_ACCEPT)
    {
        ctk_widget_destroy (CTK_WIDGET (message_dialog));
        return;
    }

    file = g_object_get_data (G_OBJECT (message_dialog), "mime-action:file");

    g_assert (BAUL_IS_FILE (file));

    baul_file_ref (file);
    uri = baul_file_get_uri (file);
    mime_type = baul_file_get_mime_type (file);

    dialog = baul_open_with_dialog_new (uri, mime_type, NULL);
    g_object_set_data_full (G_OBJECT (dialog),
                            "mime-action:file",
                            baul_file_ref (file),
                            (GDestroyNotify)baul_file_unref);

    ctk_window_set_screen (CTK_WINDOW (dialog),
                           ctk_widget_get_screen (CTK_WIDGET (callback_data)));

    /* Destroy the message dialog after ref:ing the file */
    ctk_widget_destroy (CTK_WIDGET (message_dialog));

    ctk_widget_show (dialog);

    g_signal_connect_object (dialog,
                             "application_selected",
                             G_CALLBACK (application_selected_cb),
                             callback_data,
                             0);

    g_free (uri);
    g_free (mime_type);
    baul_file_unref (file);
}

static void
show_unhandled_type_error (ActivateParametersInstall *parameters)
{
    CtkWidget *dialog;

    char *mime_type = baul_file_get_mime_type (parameters->file);
    char *error_message = get_application_no_mime_type_handler_message (parameters->file, parameters->uri);
    if (g_content_type_is_unknown (mime_type)) {
        dialog = ctk_message_dialog_new (parameters->parent_window,
        				 CTK_DIALOG_DESTROY_WITH_PARENT,
        				 CTK_MESSAGE_ERROR,
        				 0,
        				 NULL);
        g_object_set (dialog,
        	      "text", error_message,
        	      "secondary-text", _("The file is of an unknown type"),
        	      NULL);
    } else {
        char *text;
        text = g_strdup_printf (_("There is no application installed for %s files"), g_content_type_get_description (mime_type));

        dialog = ctk_message_dialog_new (parameters->parent_window,
        				 CTK_DIALOG_DESTROY_WITH_PARENT,
        				 CTK_MESSAGE_ERROR,
        				 0,
        				 NULL);
        g_object_set (dialog,
        	      "text", error_message,
        	      "secondary-text", text,
        	      NULL);

        g_free (text);
    }

    ctk_dialog_add_button (CTK_DIALOG (dialog), _("_Select Application"), CTK_RESPONSE_ACCEPT);

    eel_dialog_add_button (CTK_DIALOG (dialog),
                           _("_OK"),
                           "ctk-ok",
                           CTK_RESPONSE_OK);

    ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);

    g_object_set_data_full (G_OBJECT (dialog),
                            "mime-action:file",
                            baul_file_ref (parameters->file),
                            (GDestroyNotify)baul_file_unref);

    ctk_widget_show (CTK_WIDGET (dialog));

    g_signal_connect (dialog, "response",
                      G_CALLBACK (choose_program), parameters->parent_window);

    g_free (error_message);
    g_free (mime_type);
}

static void
search_for_application_dbus_call_notify_cb (GDBusProxy   *proxy,
        GAsyncResult *result,
        gpointer      user_data)
{
    ActivateParametersInstall *parameters_install = user_data;
    GVariant *variant;
    GError *error = NULL;

    variant = g_dbus_proxy_call_finish (proxy, result, &error);
    if (variant == NULL)
    {
        if (!g_dbus_error_is_remote_error (error) ||
                g_strcmp0 (g_dbus_error_get_remote_error (error), "org.freedesktop.PackageKit.Modify.Failed") == 0)
        {
            char *message;

            message = g_strdup_printf ("%s\n%s",
                                       _("There was an internal error trying to search for applications:"),
                                       error->message);
            eel_show_error_dialog (_("Unable to search for application"), message,
                                   parameters_install->parent_window);
            g_free (message);
        }

        g_error_free (error);
        activate_parameters_install_free (parameters_install);
        return;
    }

    g_variant_unref (variant);

    /* activate the file again */
    baul_mime_activate_files (parameters_install->parent_window,
                              parameters_install->slot_info,
                              parameters_install->files,
                              parameters_install->activation_directory,
                              parameters_install->mode,
                              parameters_install->flags,
                              parameters_install->user_confirmation);

    activate_parameters_install_free (parameters_install);
}

static void
search_for_application_mime_type (ActivateParametersInstall *parameters_install, const gchar *mime_type)
{
    CdkWindow *window;
    guint xid = 0;
    const char *mime_types[2];

    g_assert (parameters_install->proxy != NULL);

    /* get XID from parent window */
    window = ctk_widget_get_window (CTK_WIDGET (parameters_install->parent_window));
    if (window != NULL)
    {
        xid = CDK_WINDOW_XID (window);
    }

    mime_types[0] = mime_type;
    mime_types[1] = NULL;

    g_dbus_proxy_call (parameters_install->proxy,
                       "InstallMimeTypes",
                       g_variant_new ("(u^ass)",
                                      xid,
                                      mime_types,
                                      "hide-confirm-search"),
                       G_DBUS_CALL_FLAGS_NONE,
                       G_MAXINT /* no timeout */,
                       NULL /* cancellable */,
                       (GAsyncReadyCallback) search_for_application_dbus_call_notify_cb,
                       parameters_install);

    baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                    "InstallMimeType method invoked for %s", mime_type);
}

static void
application_unhandled_file_install (CtkDialog *dialog,
                                    gint response_id,
                                    ActivateParametersInstall *parameters_install)
{
    ctk_widget_destroy (CTK_WIDGET (dialog));
    parameters_install->dialog = NULL;

    if (response_id == CTK_RESPONSE_YES)
    {
        char *mime_type;

        mime_type = baul_file_get_mime_type (parameters_install->file);
        search_for_application_mime_type (parameters_install, mime_type);
        g_free (mime_type);
    }
    else
    {
        /* free as we're not going to get the async dbus callback */
        activate_parameters_install_free (parameters_install);
    }
}

static gboolean
delete_cb (CtkDialog *dialog)
{
    ctk_dialog_response (dialog, CTK_RESPONSE_DELETE_EVENT);
    return TRUE;
}

static void
pk_proxy_appeared_cb (GObject      *source G_GNUC_UNUSED,
		      GAsyncResult *res,
		      gpointer      user_data)
{
    ActivateParametersInstall *parameters_install = user_data;
    char *mime_type;
    char *error_message;
    CtkWidget *dialog;
    GDBusProxy *proxy;
    GError *error = NULL;

    proxy = g_dbus_proxy_new_for_bus_finish (res, &error);

    if (error != NULL)
    {
        g_warning ("Couldn't call Modify on the PackageKit interface: %s",
                   error->message);
        g_error_free (error);

        /* show an unhelpful dialog */
        show_unhandled_type_error (parameters_install);
        /* The callback wasn't started, so we have to free the parameters */
        activate_parameters_install_free (parameters_install);

        return;
    }

    mime_type = baul_file_get_mime_type (parameters_install->file);
    error_message = get_application_no_mime_type_handler_message (parameters_install->file,
                    parameters_install->uri);
    /* use a custom dialog to prompt the user to install new software */
    dialog = ctk_message_dialog_new (parameters_install->parent_window, 0,
                                     CTK_MESSAGE_ERROR,
                                     CTK_BUTTONS_YES_NO,
                                     "%s", error_message);
    ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog),
            _("There is no application installed for %s files.\n"
              "Do you want to search for an application to open this file?"),
            g_content_type_get_description (mime_type));
    ctk_window_set_resizable (CTK_WINDOW (dialog), FALSE);

    parameters_install->dialog = dialog;
    parameters_install->proxy = proxy;

    g_signal_connect (dialog, "response",
                      G_CALLBACK (application_unhandled_file_install),
                      parameters_install);
    g_signal_connect (dialog, "delete-event",
                      G_CALLBACK (delete_cb), NULL);
    ctk_widget_show_all (dialog);
    g_free (mime_type);
}

static void
application_unhandled_uri (ActivateParameters *parameters, char *uri)
{
    gboolean show_install_mime;
    char *mime_type;
    BaulFile *file;
    ActivateParametersInstall *parameters_install;

    file = baul_file_get_by_uri (uri);

    mime_type = baul_file_get_mime_type (file);

    /* copy the parts of parameters we are interested in as the orignal will be unref'd */
    parameters_install = g_new0 (ActivateParametersInstall, 1);
    parameters_install->slot_info = parameters->slot_info;
    g_object_add_weak_pointer (G_OBJECT (parameters_install->slot_info), (gpointer *)&parameters_install->slot_info);
    if (parameters->parent_window)
    {
        parameters_install->parent_window = parameters->parent_window;
        g_object_add_weak_pointer (G_OBJECT (parameters_install->parent_window), (gpointer *)&parameters_install->parent_window);
    }
    parameters_install->activation_directory = g_strdup (parameters->activation_directory);
    parameters_install->file = file;
    parameters_install->files = get_file_list_for_launch_locations (parameters->locations);
    parameters_install->mode = parameters->mode;
    parameters_install->flags = parameters->flags;
    parameters_install->user_confirmation = parameters->user_confirmation;
    parameters_install->uri = g_strdup(uri);

#ifdef ENABLE_PACKAGEKIT
    /* allow an admin to disable the PackageKit search functionality */
    show_install_mime = g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_INSTALL_MIME_ACTIVATION);
#else
    /* we have no install functionality */
    show_install_mime = FALSE;
#endif
    /* There is no use trying to look for handlers of application/octet-stream */
    if (g_content_type_is_unknown (mime_type))
    {
        show_install_mime = FALSE;
        goto out;
    }

    if (!show_install_mime)
    {
        goto out;
    }

    g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                              G_DBUS_PROXY_FLAGS_NONE,
                              NULL,
                              "org.freedesktop.PackageKit",
                              "/org/freedesktop/PackageKit",
                              "org.freedesktop.PackageKit.Modify",
                              NULL,
                              pk_proxy_appeared_cb,
                              parameters_install);

    return;

out:
    /* show an unhelpful dialog */
    show_unhandled_type_error (parameters_install);
    /* The callback wasn't started, so we have to free the parameters */
    activate_parameters_install_free (parameters_install);

    g_free (mime_type);
}

typedef struct
{
    CtkWindow *parent_window;
    BaulFile *file;
} ActivateParametersDesktop;

static void
activate_parameters_desktop_free (ActivateParametersDesktop *parameters_desktop)
{
    if (parameters_desktop->parent_window)
    {
        g_object_remove_weak_pointer (G_OBJECT (parameters_desktop->parent_window), (gpointer *)&parameters_desktop->parent_window);
    }
    baul_file_unref (parameters_desktop->file);
    g_free (parameters_desktop);
}

static void
untrusted_launcher_response_callback (CtkDialog *dialog,
                                      int response_id,
                                      ActivateParametersDesktop *parameters)
{
    CdkScreen *screen;
    char *uri;
    GFile *file;

    switch (response_id)
    {
    case RESPONSE_RUN:
        screen = ctk_widget_get_screen (CTK_WIDGET (parameters->parent_window));
        uri = baul_file_get_uri (parameters->file);
        baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                        "directory view activate_callback launch_desktop_file window=%p: %s",
                        parameters->parent_window, uri);
        baul_launch_desktop_file (screen, uri, NULL,
                                  parameters->parent_window);
        g_free (uri);
        break;
    case RESPONSE_MARK_TRUSTED:
        file = baul_file_get_location (parameters->file);
        baul_file_mark_desktop_file_trusted (file,
                                             parameters->parent_window,
                                             TRUE,
                                             NULL, NULL);
        g_object_unref (file);
        break;
    default:
        /* Just destroy dialog */
        break;
    }

    ctk_widget_destroy (CTK_WIDGET (dialog));
    activate_parameters_desktop_free (parameters);
}

static void
activate_desktop_file (ActivateParameters *parameters,
                       BaulFile *file)
{
    ActivateParametersDesktop *parameters_desktop;
    CdkScreen *screen;
    char *uri;

    screen = ctk_widget_get_screen (CTK_WIDGET (parameters->parent_window));

    if (!baul_file_is_trusted_link (file))
    {
        char *primary, *secondary, *display_name;
        CtkWidget *dialog;

        /* copy the parts of parameters we are interested in as the orignal will be freed */
        parameters_desktop = g_new0 (ActivateParametersDesktop, 1);
        if (parameters->parent_window)
        {
            parameters_desktop->parent_window = parameters->parent_window;
            g_object_add_weak_pointer (G_OBJECT (parameters_desktop->parent_window), (gpointer *)&parameters_desktop->parent_window);
        }
        parameters_desktop->file = baul_file_ref (file);

        primary = _("Untrusted application launcher");
        display_name = baul_file_get_display_name (file);
        secondary =
            g_strdup_printf (_("The application launcher \"%s\" has not been marked as trusted. "
                               "If you do not know the source of this file, launching it may be unsafe."
                              ),
                             display_name);

		dialog = ctk_message_dialog_new (parameters->parent_window,
						 0,
						 CTK_MESSAGE_WARNING,
						 CTK_BUTTONS_NONE,
						 NULL);
		g_object_set (dialog,
			      "text", primary,
			      "secondary-text", secondary,
			      NULL);
        ctk_dialog_add_button (CTK_DIALOG (dialog),
                               _("_Launch Anyway"), RESPONSE_RUN);
        if (baul_file_can_set_permissions (file))
        {
            ctk_dialog_add_button (CTK_DIALOG (dialog),
                                   _("Mark as _Trusted"), RESPONSE_MARK_TRUSTED);
        }

        eel_dialog_add_button (CTK_DIALOG (dialog),
                               _("_Cancel"),
                               "process-stop",
                               CTK_RESPONSE_CANCEL);

        ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_CANCEL);

        g_signal_connect (dialog, "response",
                          G_CALLBACK (untrusted_launcher_response_callback),
                          parameters_desktop);
        ctk_widget_show (dialog);

        g_free (display_name);
        g_free (secondary);
        return;
    }

    uri = baul_file_get_uri (file);
    baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                    "directory view activate_callback launch_desktop_file window=%p: %s",
                    parameters->parent_window, uri);
    baul_launch_desktop_file (screen, uri, NULL,
                              parameters->parent_window);
    g_free (uri);
}

static void
activate_files (ActivateParameters *parameters)
{
    BaulWindowInfo *window_info;
    BaulWindowOpenFlags flags;
    BaulFile *file;
    GList *launch_desktop_files;
    GList *launch_files;
    GList *launch_in_terminal_files;
    GList *open_in_app_uris;
    GList *open_in_app_parameters;
    GList *unhandled_open_in_app_uris;
    GList *open_in_view_files;
    GList *l;
    int count;
    char *uri;
    char *executable_path, *quoted_path, *name;
    char *old_working_dir;
    ActivationAction action;
    CdkScreen *screen;
    LaunchLocation *location;
    ApplicationLaunchParameters *one_parameters = NULL;

    screen = ctk_widget_get_screen (CTK_WIDGET (parameters->parent_window));

    launch_desktop_files = NULL;
    launch_files = NULL;
    launch_in_terminal_files = NULL;
    open_in_app_uris = NULL;
    open_in_view_files = NULL;

    for (l = parameters->locations; l != NULL; l = l->next)
    {
        location = l->data;
        file = location->file;

        if (file_was_cancelled (file))
        {
            continue;
        }

        action = get_activation_action (file);
        if (action == ACTIVATION_ACTION_ASK)
        {
            /* Special case for executable text files, since it might be
             * dangerous & unexpected to launch these.
             */
            pause_activation_timed_cancel (parameters);
            action = get_executable_text_file_action (parameters->parent_window, file);
            unpause_activation_timed_cancel (parameters);
        }

        switch (action)
        {
        case ACTIVATION_ACTION_LAUNCH_DESKTOP_FILE :
            launch_desktop_files = g_list_prepend (launch_desktop_files, file);
            break;
        case ACTIVATION_ACTION_LAUNCH :
            launch_files = g_list_prepend (launch_files, file);
            break;
        case ACTIVATION_ACTION_LAUNCH_IN_TERMINAL :
            launch_in_terminal_files = g_list_prepend (launch_in_terminal_files, file);
            break;
        case ACTIVATION_ACTION_OPEN_IN_VIEW :
            open_in_view_files = g_list_prepend (open_in_view_files, file);
            break;
        case ACTIVATION_ACTION_OPEN_IN_APPLICATION :
            open_in_app_uris = g_list_prepend (open_in_app_uris, location->uri);
            break;
        case ACTIVATION_ACTION_DO_NOTHING :
            break;
        case ACTIVATION_ACTION_ASK :
            g_assert_not_reached ();
            break;
        }
    }

    launch_desktop_files = g_list_reverse (launch_desktop_files);
    for (l = launch_desktop_files; l != NULL; l = l->next)
    {
        file = BAUL_FILE (l->data);

        activate_desktop_file (parameters, file);
    }

    old_working_dir = NULL;
    if (parameters->activation_directory &&
            (launch_files != NULL || launch_in_terminal_files != NULL))
    {
        old_working_dir = g_get_current_dir ();
        g_chdir (parameters->activation_directory);

    }

    launch_files = g_list_reverse (launch_files);
    for (l = launch_files; l != NULL; l = l->next)
    {
        file = BAUL_FILE (l->data);

        uri = baul_file_get_activation_uri (file);
        executable_path = g_filename_from_uri (uri, NULL, NULL);
        quoted_path = g_shell_quote (executable_path);
        name = baul_file_get_name (file);

        baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                        "directory view activate_callback launch_file window=%p: %s",
                        parameters->parent_window, quoted_path);

        baul_launch_application_from_command (screen, name, quoted_path, FALSE, NULL);
        g_free (name);
        g_free (quoted_path);
        g_free (executable_path);
        g_free (uri);

    }

    launch_in_terminal_files = g_list_reverse (launch_in_terminal_files);
    for (l = launch_in_terminal_files; l != NULL; l = l->next)
    {
        file = BAUL_FILE (l->data);

        uri = baul_file_get_activation_uri (file);
        executable_path = g_filename_from_uri (uri, NULL, NULL);
        quoted_path = g_shell_quote (executable_path);
        name = baul_file_get_name (file);

        baul_debug_log (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER,
                        "directory view activate_callback launch_in_terminal window=%p: %s",
                        parameters->parent_window, quoted_path);

        baul_launch_application_from_command (screen, name, quoted_path, TRUE, NULL);
        g_free (name);
        g_free (quoted_path);
        g_free (executable_path);
        g_free (uri);
    }

    if (old_working_dir != NULL)
    {
        g_chdir (old_working_dir);
        g_free (old_working_dir);
    }

    open_in_view_files = g_list_reverse (open_in_view_files);
    count = g_list_length (open_in_view_files);

    flags = parameters->flags;
    if (count > 1)
    {
        if ((parameters->flags & BAUL_WINDOW_OPEN_FLAG_NEW_WINDOW) == 0)
        {
            flags |= BAUL_WINDOW_OPEN_FLAG_NEW_TAB;
        }
        else
        {
            flags |= BAUL_WINDOW_OPEN_FLAG_NEW_WINDOW;
        }
    }

    if (parameters->slot_info != NULL &&
            (!parameters->user_confirmation ||
             confirm_multiple_windows (parameters->parent_window, count,
                                       (flags & BAUL_WINDOW_OPEN_FLAG_NEW_TAB) != 0)))
    {

        if ((flags & BAUL_WINDOW_OPEN_FLAG_NEW_TAB) != 0 &&
                g_settings_get_enum (baul_preferences, BAUL_PREFERENCES_NEW_TAB_POSITION) ==
                BAUL_NEW_TAB_POSITION_AFTER_CURRENT_TAB)
        {
            /* When inserting N tabs after the current one,
             * we first open tab N, then tab N-1, ..., then tab 0.
             * Each of them is appended to the current tab, i.e.
             * prepended to the list of tabs to open.
             */
            open_in_view_files = g_list_reverse (open_in_view_files);
        }


        for (l = open_in_view_files; l != NULL; l = l->next)
        {
            GFile *f;
            /* The ui should ask for navigation or object windows
             * depending on what the current one is */
            file = BAUL_FILE (l->data);

            uri = baul_file_get_activation_uri (file);
            f = g_file_new_for_uri (uri);
            baul_window_slot_info_open_location (parameters->slot_info,
                                                 f, parameters->mode, flags, NULL);
            g_object_unref (f);
            g_free (uri);
        }
    }

    open_in_app_parameters = NULL;
    unhandled_open_in_app_uris = NULL;

    if (open_in_app_uris != NULL)
    {
        open_in_app_uris = g_list_reverse (open_in_app_uris);

        open_in_app_parameters = make_activation_parameters
                                 (open_in_app_uris, &unhandled_open_in_app_uris);
    }

    for (l = open_in_app_parameters; l != NULL; l = l->next)
    {
        one_parameters = l->data;

        baul_launch_application_by_uri (one_parameters->application,
                                        one_parameters->uris,
                                        parameters->parent_window);
        application_launch_parameters_free (one_parameters);
    }

    for (l = unhandled_open_in_app_uris; l != NULL; l = l->next)
    {
        uri = l->data;

        /* this does not block */
        application_unhandled_uri (parameters, uri);
    }

    window_info = NULL;
    if (parameters->slot_info != NULL)
    {
        window_info = baul_window_slot_info_get_window (parameters->slot_info);
    }

    if (open_in_app_parameters != NULL ||
            unhandled_open_in_app_uris != NULL)
    {
        if ((parameters->flags & BAUL_WINDOW_OPEN_FLAG_CLOSE_BEHIND) != 0 &&
                window_info != NULL &&
                baul_window_info_get_window_type (window_info) == BAUL_WINDOW_SPATIAL)
        {
            baul_window_info_close (window_info);
        }
    }

    g_list_free (launch_desktop_files);
    g_list_free (launch_files);
    g_list_free (launch_in_terminal_files);
    g_list_free (open_in_view_files);
    g_list_free (open_in_app_uris);
    g_list_free (open_in_app_parameters);
    g_list_free (unhandled_open_in_app_uris);

    activation_parameters_free (parameters);
}

static void
activation_mount_not_mounted_callback (GObject *source_object,
                                       GAsyncResult *res,
                                       gpointer user_data)
{
    ActivateParameters *parameters = user_data;
    GError *error;
    BaulFile *file;

    file = parameters->not_mounted->data;

    error = NULL;
    if (!g_file_mount_enclosing_volume_finish (G_FILE (source_object), res, &error))
    {
        if (error->domain != G_IO_ERROR ||
                (error->code != G_IO_ERROR_CANCELLED &&
                 error->code != G_IO_ERROR_FAILED_HANDLED &&
                 error->code != G_IO_ERROR_ALREADY_MOUNTED))
        {
            eel_show_error_dialog (_("Unable to mount location"),
                                   error->message, parameters->parent_window);
        }

        if (error->domain != G_IO_ERROR ||
                error->code != G_IO_ERROR_ALREADY_MOUNTED)
        {
            LaunchLocation *loc;

            loc = find_launch_location_for_file (parameters->locations,
                                                 file);
            if (loc)
            {
                parameters->locations =
                    g_list_remove (parameters->locations, loc);
                launch_location_free (loc);
            }
        }

        g_error_free (error);
    }

    parameters->not_mounted = g_list_delete_link (parameters->not_mounted,
                              parameters->not_mounted);
    baul_file_unref (file);

    activation_mount_not_mounted (parameters);
}

static void
activation_mount_not_mounted (ActivateParameters *parameters)
{
    LaunchLocation *loc = NULL;
    GList *l, *next, *files;

    if (parameters->not_mounted != NULL)
    {
        BaulFile *file;
        GFile *location;
        GMountOperation *mount_op;

        file = parameters->not_mounted->data;
        mount_op = ctk_mount_operation_new (parameters->parent_window);
        g_mount_operation_set_password_save (mount_op, G_PASSWORD_SAVE_FOR_SESSION);
        g_signal_connect (mount_op, "notify::is-showing",
                          G_CALLBACK (activate_mount_op_active), parameters);
        location = baul_file_get_location (file);
        g_file_mount_enclosing_volume (location, 0, mount_op, parameters->cancellable,
                                       activation_mount_not_mounted_callback, parameters);
        g_object_unref (location);
        /* unref mount_op here - g_file_mount_enclosing_volume() does ref for itself */
        g_object_unref (mount_op);
        return;
    }

    parameters->tried_mounting = TRUE;

    if (parameters->locations == NULL)
    {
        activation_parameters_free (parameters);
        return;
    }

    /*  once the mount is finished, refresh all attributes        */
    /*  - fixes new windows not appearing after successful mount  */
    for (l = parameters->locations; l != NULL; l = next)
    {
        loc = l->data;
        next = l->next;
        baul_file_invalidate_all_attributes (loc->file);
    }

    files = get_file_list_for_launch_locations (parameters->locations);
    baul_file_list_call_when_ready
    (files,
     baul_mime_actions_get_required_file_attributes () | BAUL_FILE_ATTRIBUTE_LINK_INFO,
     &parameters->files_handle,
     activate_callback, parameters);
    baul_file_list_free (files);
}


static void
activate_callback (GList   *files G_GNUC_UNUSED,
		   gpointer callback_data)
{
    ActivateParameters *parameters = callback_data;
    GList *l, *next;
    LaunchLocation *location;
    BaulFile *file = NULL;

    parameters->files_handle = NULL;

    for (l = parameters->locations; l != NULL; l = next)
    {
        location = l->data;
        file = location->file;
        next = l->next;

        if (file_was_cancelled (file))
        {
            launch_location_free (location);
            parameters->locations = g_list_delete_link (parameters->locations, l);
            continue;
        }

        if (file_was_not_mounted (file))
        {
            if (parameters->tried_mounting)
            {
                launch_location_free (location);
                parameters->locations = g_list_delete_link (parameters->locations, l);
            }
            else
            {
                parameters->not_mounted = g_list_prepend (parameters->not_mounted,
                                          baul_file_ref (file));
            }
            continue;
        }
    }


    if (parameters->not_mounted != NULL)
    {
        activation_mount_not_mounted (parameters);
    }
    else
    {
        activate_files (parameters);
    }
}

static void
activate_activation_uris_ready_callback (GList   *files_ignore G_GNUC_UNUSED,
					 gpointer callback_data)
{
    ActivateParameters *parameters = callback_data;
    GList *l, *next, *files;
    LaunchLocation *location;
    BaulFile *file = NULL;

    parameters->files_handle = NULL;

    for (l = parameters->locations; l != NULL; l = next)
    {
        location = l->data;
        file = location->file;
        next = l->next;

        if (file_was_cancelled (file))
        {
            launch_location_free (location);
            parameters->locations = g_list_delete_link (parameters->locations, l);
            continue;
        }

        if (baul_file_is_broken_symbolic_link (file))
        {
            launch_location_free (location);
            parameters->locations = g_list_delete_link (parameters->locations, l);
            pause_activation_timed_cancel (parameters);
            report_broken_symbolic_link (parameters->parent_window, file);
            unpause_activation_timed_cancel (parameters);
            continue;
        }

        if (baul_file_get_file_type (file) == G_FILE_TYPE_MOUNTABLE &&
                !baul_file_has_activation_uri (file))
        {
            /* Don't launch these... There is nothing we
               can do */
            launch_location_free (location);
            parameters->locations = g_list_delete_link (parameters->locations, l);
            continue;
        }

    }

    if (parameters->locations == NULL)
    {
        activation_parameters_free (parameters);
        return;
    }

    /* Convert the files to the actual activation uri files */
    for (l = parameters->locations; l != NULL; l = l->next)
    {
        char *uri;
        location = l->data;

        /* We want the file for the activation URI since we care
         * about the attributes for that, not for the original file.
         */
        uri = baul_file_get_activation_uri (location->file);
        if (uri != NULL)
        {
            launch_location_update_from_uri (location, uri);
        }
        g_free (uri);
    }


    /* get the parameters for the actual files */
    files = get_file_list_for_launch_locations (parameters->locations);
    baul_file_list_call_when_ready
    (files,
     baul_mime_actions_get_required_file_attributes () | BAUL_FILE_ATTRIBUTE_LINK_INFO,
     &parameters->files_handle,
     activate_callback, parameters);
    baul_file_list_free (files);
}

static void
activation_get_activation_uris (ActivateParameters *parameters)
{
    GList *l, *files;
    LaunchLocation *location;
    BaulFile *file = NULL;

    /* link target info might be stale, re-read it */
    for (l = parameters->locations; l != NULL; l = l->next)
    {
        location = l->data;
        file = location->file;

        if (file_was_cancelled (file))
        {
            launch_location_free (location);
            parameters->locations = g_list_delete_link (parameters->locations, l);
            continue;
        }

        if (baul_file_is_symbolic_link (file))
        {
            baul_file_invalidate_attributes
            (file,
             BAUL_FILE_ATTRIBUTE_INFO |
             BAUL_FILE_ATTRIBUTE_LINK_INFO);
        }
    }

    if (parameters->locations == NULL)
    {
        activation_parameters_free (parameters);
        return;
    }

    files = get_file_list_for_launch_locations (parameters->locations);
    baul_file_list_call_when_ready
    (files,
     BAUL_FILE_ATTRIBUTE_INFO |
     BAUL_FILE_ATTRIBUTE_LINK_INFO,
     &parameters->files_handle,
     activate_activation_uris_ready_callback, parameters);
    baul_file_list_free (files);
}

static void
activation_mountable_mounted (BaulFile  *file,
                              GFile         *result_location,
                              GError        *error,
                              gpointer       callback_data)
{
    ActivateParameters *parameters = callback_data;
    LaunchLocation *location;

    /* Remove from list of files that have to be mounted */
    parameters->mountables = g_list_remove (parameters->mountables, file);
    baul_file_unref (file);


    if (error == NULL)
    {
        BaulFile *target_file;

        /* Replace file with the result of the mount */
        target_file = baul_file_get (result_location);

        location = find_launch_location_for_file (parameters->locations,
                   file);
        if (location)
        {
            launch_location_update_from_file (location, target_file);
        }
        baul_file_unref (target_file);
    }
    else
    {
        /* Remove failed file */

        if (error->domain != G_IO_ERROR ||
                (error->code != G_IO_ERROR_FAILED_HANDLED &&
                 error->code != G_IO_ERROR_ALREADY_MOUNTED))
        {
            location = find_launch_location_for_file (parameters->locations,
                       file);
            if (location)
            {
                parameters->locations =
                    g_list_remove (parameters->locations,
                                   location);
                launch_location_free (location);
            }
        }

        if (error->domain != G_IO_ERROR ||
                (error->code != G_IO_ERROR_CANCELLED &&
                 error->code != G_IO_ERROR_FAILED_HANDLED &&
                 error->code != G_IO_ERROR_ALREADY_MOUNTED))
        {
            eel_show_error_dialog (_("Unable to mount location"),
                                   error->message, parameters->parent_window);
        }

        if (error->code == G_IO_ERROR_CANCELLED)
        {
            activation_parameters_free (parameters);
            return;
        }
    }

    /* Mount more mountables */
    activation_mount_mountables (parameters);
}


static void
activation_mount_mountables (ActivateParameters *parameters)
{
    if (parameters->mountables != NULL)
    {
        BaulFile *file;
        GMountOperation *mount_op;

        file = parameters->mountables->data;
        mount_op = ctk_mount_operation_new (parameters->parent_window);
        g_mount_operation_set_password_save (mount_op, G_PASSWORD_SAVE_FOR_SESSION);
        g_signal_connect (mount_op, "notify::is-showing",
                          G_CALLBACK (activate_mount_op_active), parameters);
        baul_file_mount (file,
                         mount_op,
                         parameters->cancellable,
                         activation_mountable_mounted,
                         parameters);
        g_object_unref (mount_op);
        return;
    }

    if (parameters->mountables == NULL && parameters->start_mountables == NULL)
        activation_get_activation_uris (parameters);
}


static void
activation_mountable_started (BaulFile *file,
			      GFile    *gfile_of_file G_GNUC_UNUSED,
			      GError   *error,
			      gpointer  callback_data)
{
    ActivateParameters *parameters = callback_data;
    LaunchLocation *location;

    /* Remove from list of files that have to be mounted */
    parameters->start_mountables = g_list_remove (parameters->start_mountables, file);
    baul_file_unref (file);

    if (error == NULL)
    {
        /* Remove file */
        location = find_launch_location_for_file (parameters->locations, file);
        if (location != NULL)
        {
            parameters->locations = g_list_remove (parameters->locations, location);
            launch_location_free (location);
        }

    }
    else
    {
        /* Remove failed file */
        if (error->domain != G_IO_ERROR ||
                (error->code != G_IO_ERROR_FAILED_HANDLED))
        {
            location = find_launch_location_for_file (parameters->locations,
                       file);
            if (location)
            {
                parameters->locations =
                    g_list_remove (parameters->locations,
                                   location);
                launch_location_free (location);
            }
        }

        if (error->domain != G_IO_ERROR ||
                (error->code != G_IO_ERROR_CANCELLED &&
                 error->code != G_IO_ERROR_FAILED_HANDLED))
        {
            eel_show_error_dialog (_("Unable to start location"),
                                   error->message, NULL);
        }

        if (error->code == G_IO_ERROR_CANCELLED)
        {
            activation_parameters_free (parameters);
            return;
        }
    }

    /* Start more mountables */
    activation_start_mountables (parameters);
}

static void
activation_start_mountables (ActivateParameters *parameters)
{
    if (parameters->start_mountables != NULL)
    {
        BaulFile *file;
        GMountOperation *start_op;

        file = parameters->start_mountables->data;
        start_op = ctk_mount_operation_new (parameters->parent_window);
        g_signal_connect (start_op, "notify::is-showing",
                          G_CALLBACK (activate_mount_op_active), parameters);
        baul_file_start (file,
                         start_op,
                         parameters->cancellable,
                         activation_mountable_started,
                         parameters);
        g_object_unref (start_op);
        return;
    }

    if (parameters->mountables == NULL && parameters->start_mountables == NULL)
        activation_get_activation_uris (parameters);
}

/**
 * baul_mime_activate_files:
 *
 * Activate a list of files. Each one might launch with an application or
 * with a component. This is normally called only by subclasses.
 * @view: FMDirectoryView in question.
 * @files: A GList of BaulFiles to activate.
 *
 **/
void
baul_mime_activate_files (CtkWindow *parent_window,
                          BaulWindowSlotInfo *slot_info,
                          GList *files,
                          const char *launch_directory,
                          BaulWindowOpenMode mode,
                          BaulWindowOpenFlags flags,
                          gboolean user_confirmation)
{
    ActivateParameters *parameters;
    int file_count;
    GList *l, *next;
    LaunchLocation *location;
    BaulFile *file = NULL;

    if (files == NULL)
    {
        return;
    }

    baul_debug_log_with_file_list (FALSE, BAUL_DEBUG_LOG_DOMAIN_USER, files,
                                   "baul_mime_activate_files window=%p",
                                   parent_window);

    parameters = g_new0 (ActivateParameters, 1);
    parameters->slot_info = slot_info;
    g_object_add_weak_pointer (G_OBJECT (parameters->slot_info), (gpointer *)&parameters->slot_info);
    if (parent_window)
    {
        parameters->parent_window = parent_window;
        g_object_add_weak_pointer (G_OBJECT (parameters->parent_window), (gpointer *)&parameters->parent_window);
    }
    parameters->cancellable = g_cancellable_new ();
    parameters->activation_directory = g_strdup (launch_directory);
    parameters->locations = launch_locations_from_file_list (files);
    parameters->mode = mode;
    parameters->flags = flags;
    parameters->user_confirmation = user_confirmation;

    file_count = g_list_length (files);
    if (file_count == 1)
    {
        char *file_name;

        file_name = baul_file_get_display_name (files->data);
        parameters->timed_wait_prompt = g_strdup_printf (_("Opening \"%s\"."), file_name);
        g_free (file_name);
    }
    else
    {
        parameters->timed_wait_prompt = g_strdup_printf (ngettext ("Opening %d item.",
                                        "Opening %d items.",
                                        file_count),
                                        file_count);
    }


    for (l = parameters->locations; l != NULL; l = next)
    {
        location = l->data;
        file = location->file;
        next = l->next;

        if (baul_file_can_mount (file))
        {
            parameters->mountables = g_list_prepend (parameters->mountables,
                                     baul_file_ref (file));
        }

        if (baul_file_can_start (file))
        {
            parameters->start_mountables = g_list_prepend (parameters->start_mountables,
                                           baul_file_ref (file));
        }
    }

    activation_start_timed_cancel (parameters);
    if (parameters->mountables != NULL)
        activation_mount_mountables (parameters);
    if (parameters->start_mountables != NULL)
        activation_start_mountables (parameters);
    if (parameters->mountables == NULL && parameters->start_mountables == NULL)
        activation_get_activation_uris (parameters);
}

/**
 * baul_mime_activate_file:
 *
 * Activate a file in this view. This might involve switching the displayed
 * location for the current window, or launching an application.
 * @view: FMDirectoryView in question.
 * @file: A BaulFile representing the file in this view to activate.
 * @use_new_window: Should this item be opened in a new window?
 *
 **/

void
baul_mime_activate_file (CtkWindow *parent_window,
                         BaulWindowSlotInfo *slot_info,
                         BaulFile *file,
                         const char *launch_directory,
                         BaulWindowOpenMode mode,
                         BaulWindowOpenFlags flags)
{
    GList *files;

    g_return_if_fail (BAUL_IS_FILE (file));

    files = g_list_prepend (NULL, file);
    baul_mime_activate_files (parent_window, slot_info, files, launch_directory, mode, flags, FALSE);
    g_list_free (files);
}
