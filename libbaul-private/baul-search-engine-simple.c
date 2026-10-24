/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2005 Red Hat, Inc
 *
 * Baul is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Baul is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 *
 */

#include <config.h>
#include <string.h>
#include <glib.h>

#include <gio/gio.h>

#include <eel/eel-ctk-macros.h>

#include "baul-search-engine-simple.h"

#define BATCH_SIZE 500

typedef struct
{
    BaulSearchEngineSimple *engine;
    GCancellable *cancellable;

    char *contained_text;
    GList *mime_types;
    GList *tags;
    char **words;
    GList *found_list;

    GQueue *directories; /* GFiles */

    GHashTable *visited;

    gint n_processed_files;
    GList *uri_hits;
    gint64 timestamp;
    gint64 size;
} SearchThreadData;


struct BaulSearchEngineSimpleDetails
{
    BaulQuery *query;

    SearchThreadData *active_search;

    gboolean query_finished;

    /* Live updates: keep track of the current hits and monitor the
     * directories containing them, so that files deleted and/or
     * recreated by external programs (e.g. git) are removed from or
     * restored to the results without restarting the search. */
    GHashTable *hit_uris;          /* uri -> uri, current results */
    GHashTable *removed_hit_uris;  /* uri -> uri, hits deleted, awaiting recreation */
    GHashTable *monitored_dirs;    /* GFile dir -> GFileMonitor (NULL if not available) */
    GHashTable *pending_restore_uris;  /* uri -> uri, recreated, awaiting a follow-up event */
};

G_DEFINE_TYPE (BaulSearchEngineSimple,
               baul_search_engine_simple,
               BAUL_TYPE_SEARCH_ENGINE);

static BaulSearchEngineClass *parent_class = NULL;

static void
finalize (GObject *object)
{
    BaulSearchEngineSimple *simple;

    simple = BAUL_SEARCH_ENGINE_SIMPLE (object);

    if (simple->details->query)
    {
        g_object_unref (simple->details->query);
        simple->details->query = NULL;
    }

    if (simple->details->monitored_dirs != NULL)
    {
        g_hash_table_destroy (simple->details->monitored_dirs);
        simple->details->monitored_dirs = NULL;
    }

    if (simple->details->hit_uris != NULL)
    {
        g_hash_table_destroy (simple->details->hit_uris);
        simple->details->hit_uris = NULL;
    }

    if (simple->details->removed_hit_uris != NULL)
    {
        g_hash_table_destroy (simple->details->removed_hit_uris);
        simple->details->removed_hit_uris = NULL;
    }

    if (simple->details->pending_restore_uris != NULL)
    {
        g_hash_table_destroy (simple->details->pending_restore_uris);
        simple->details->pending_restore_uris = NULL;
    }

    g_free (simple->details);

    EEL_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static SearchThreadData *
search_thread_data_new (BaulSearchEngineSimple *engine,
                        BaulQuery *query)
{
    SearchThreadData *data;
    char *text, *lower, *normalized, *uri;
    GFile *location;

    data = g_new0 (SearchThreadData, 1);

    data->engine = engine;
    data->directories = g_queue_new ();
    data->visited = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    uri = baul_query_get_location (query);
    location = NULL;
    if (uri != NULL)
    {
        location = g_file_new_for_uri (uri);
        g_free (uri);
    }
    if (location == NULL)
    {
        location = g_file_new_for_path ("/");
    }
    g_queue_push_tail (data->directories, location);

    text = baul_query_get_text (query);
    normalized = g_utf8_normalize (text, -1, G_NORMALIZE_NFD);
    lower = g_utf8_strdown (normalized, -1);
    data->words = g_strsplit (lower, " ", -1);
    g_free (text);
    g_free (lower);
    g_free (normalized);

    data->tags = baul_query_get_tags (query);
    data->mime_types = baul_query_get_mime_types (query);
    data->timestamp = baul_query_get_timestamp (query);
    data->size = baul_query_get_size (query);
    data->contained_text = baul_query_get_contained_text (query);

    data->cancellable = g_cancellable_new ();

    return data;
}

static void
search_thread_data_free (SearchThreadData *data)
{
    g_queue_foreach (data->directories,
                     (GFunc)g_object_unref, NULL);
    g_queue_free (data->directories);
    g_hash_table_destroy (data->visited);
    g_object_unref (data->cancellable);
    g_strfreev (data->words);
    g_list_free_full (data->tags, g_free);
    g_list_free_full (data->mime_types, g_free);
    g_list_free_full (data->uri_hits, g_free);
    g_free (data->contained_text);
    g_free (data);
}

static gboolean
search_thread_done_idle (gpointer user_data)
{
    SearchThreadData *data;

    data = user_data;

    if (!g_cancellable_is_cancelled (data->cancellable))
    {
        baul_search_engine_finished (BAUL_SEARCH_ENGINE (data->engine));
        data->engine->details->active_search = NULL;
    }

    search_thread_data_free (data);

    return FALSE;
}

typedef struct
{
    GList *uris;
    SearchThreadData *thread_data;
} SearchHits;

static void engine_register_hit (BaulSearchEngineSimple *engine,
                                 const char             *uri);
static void parent_dir_changed (GFileMonitor      *monitor,
                                GFile             *child,
                                GFile             *other_file,
                                GFileMonitorEvent  event_type,
                                gpointer           user_data);

static gboolean
search_thread_add_hits_idle (gpointer user_data)
{
    SearchHits *hits;

    hits = user_data;

    if (!g_cancellable_is_cancelled (hits->thread_data->cancellable))
    {
        GList *l;

        for (l = hits->uris; l != NULL; l = l->next)
        {
            engine_register_hit (hits->thread_data->engine, l->data);
        }

        baul_search_engine_hits_added (BAUL_SEARCH_ENGINE (hits->thread_data->engine),
                                       hits->uris);
    }

    g_list_free_full (hits->uris, g_free);
    g_free (hits);

    return FALSE;
}

static void
send_batch (SearchThreadData *data)
{
    data->n_processed_files = 0;

    if (data->uri_hits)
    {
        SearchHits *hits;

        hits = g_new (SearchHits, 1);
        hits->uris = data->uri_hits;
        hits->thread_data = data;
        g_idle_add (search_thread_add_hits_idle, hits);
    }
    data->uri_hits = NULL;
}

static void
monitor_value_free (gpointer data)
{
    if (data != NULL)
    {
        g_object_unref (data);
    }
}

static void
emit_hits_added (BaulSearchEngineSimple *engine, const char *uri)
{
    GList *hits;

    hits = g_list_prepend (NULL, g_strdup (uri));
    baul_search_engine_hits_added (BAUL_SEARCH_ENGINE (engine), hits);
    g_list_free_full (hits, g_free);
}

static void
emit_hits_subtracted (BaulSearchEngineSimple *engine, const char *uri)
{
    GList *hits;

    hits = g_list_prepend (NULL, g_strdup (uri));
    baul_search_engine_hits_subtracted (BAUL_SEARCH_ENGINE (engine), hits);
    g_list_free_full (hits, g_free);
}

/* Make sure the directory containing a hit is monitored, so we get
 * notified when one of the results is deleted or recreated. */
static void
engine_ensure_parent_monitor (BaulSearchEngineSimple *engine,
                              GFile                  *hit)
{
    BaulSearchEngineSimpleDetails *details;
    GFile *parent;
    GFileMonitor *monitor;
    gpointer key, value;

    details = engine->details;

    parent = g_file_get_parent (hit);
    if (parent == NULL)
    {
        return;
    }

    if (g_hash_table_lookup_extended (details->monitored_dirs, parent, &key, &value))
    {
        /* Already monitored (or known to be unmonitorable) */
        g_object_unref (parent);
        return;
    }

    monitor = g_file_monitor_directory (parent, G_FILE_MONITOR_NONE, NULL, NULL);
    if (monitor != NULL)
    {
        g_signal_connect (monitor, "changed",
                          G_CALLBACK (parent_dir_changed), engine);
    }

    /* Store the monitor (or NULL, so we do not retry for every hit
     * of the same directory) together with its parent. */
    g_hash_table_insert (details->monitored_dirs, parent, monitor);
}

static void
engine_register_hit (BaulSearchEngineSimple *engine, const char *uri)
{
    BaulSearchEngineSimpleDetails *details;
    GFile *file;

    details = engine->details;

    if (g_hash_table_contains (details->hit_uris, uri))
    {
        return;
    }

    g_hash_table_remove (details->removed_hit_uris, uri);
    g_hash_table_add (details->hit_uris, g_strdup (uri));

    file = g_file_new_for_uri (uri);
    engine_ensure_parent_monitor (engine, file);
    g_object_unref (file);
}

static void
parent_dir_changed (GFileMonitor      *monitor G_GNUC_UNUSED,
                    GFile             *child,
                    GFile             *other_file,
                    GFileMonitorEvent  event_type,
                    gpointer           user_data)
{
    BaulSearchEngineSimple *engine;
    BaulSearchEngineSimpleDetails *details;
    char *uri;

    engine = BAUL_SEARCH_ENGINE_SIMPLE (user_data);
    details = engine->details;

    if (details->query == NULL)
    {
        return;
    }

    switch (event_type)
    {
    case G_FILE_MONITOR_EVENT_DELETED:
    case G_FILE_MONITOR_EVENT_MOVED_OUT:
        uri = g_file_get_uri (child);
        /* One of our hits disappeared: remove it from the results and
         * remember it, in case it gets recreated later (e.g. by git). */
        g_hash_table_remove (details->pending_restore_uris, uri);
        if (g_hash_table_remove (details->hit_uris, uri))
        {
            g_hash_table_add (details->removed_hit_uris, g_strdup (uri));
            emit_hits_subtracted (engine, uri);
        }
        g_free (uri);
        break;
    case G_FILE_MONITOR_EVENT_CREATED:
    case G_FILE_MONITOR_EVENT_MOVED_IN:
        uri = g_file_get_uri (child);
        /* A previously matched file came back. Do not restore it right
         * away: the directory layer may not have processed the deletion
         * yet, and baul_file_get_by_uri() could then return the old
         * object, which is about to be marked gone (a gone object can
         * never be revived). Park the uri instead; the next event for
         * this file (git always writes the recreated file, so CHANGED
         * follows CREATED) will perform the restore. */
        if (g_hash_table_remove (details->removed_hit_uris, uri))
        {
            g_hash_table_add (details->pending_restore_uris, g_strdup (uri));
        }
        g_free (uri);
        break;
    case G_FILE_MONITOR_EVENT_RENAMED:
        uri = g_file_get_uri (child);
        if (g_hash_table_remove (details->hit_uris, uri))
        {
            g_hash_table_add (details->removed_hit_uris, g_strdup (uri));
            emit_hits_subtracted (engine, uri);
        }
        g_free (uri);

        if (other_file != NULL)
        {
            uri = g_file_get_uri (other_file);
            if (g_hash_table_remove (details->removed_hit_uris, uri))
            {
                g_hash_table_add (details->pending_restore_uris, g_strdup (uri));
            }
            g_free (uri);
        }
        break;
    default:
        /* CHANGED, CHANGES_DONE_HINT, ATTRIBUTE_CHANGED, ... : the file
         * still exists at the same name, so it still matches the query
         * and the view is updated through the normal file-changed
         * events. It is also the safe moment to restore a recreated
         * hit: the directory layer has processed both the deletion and
         * the recreation by now, so the file object we get below is
         * fresh and alive. */
        uri = g_file_get_uri (child);
        if (g_hash_table_remove (details->pending_restore_uris, uri))
        {
            g_hash_table_add (details->hit_uris, g_strdup (uri));
            emit_hits_added (engine, uri);
        }
        g_free (uri);
        break;
    }
}

#define G_FILE_ATTRIBUTE_XATTR_XDG_TAGS "xattr::xdg.tags"

#define STD_ATTRIBUTES \
	G_FILE_ATTRIBUTE_STANDARD_NAME "," \
	G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," \
	G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN "," \
	G_FILE_ATTRIBUTE_STANDARD_TYPE "," \
	G_FILE_ATTRIBUTE_ID_FILE


/* Stolen code
 * file: glocalfileinfo.c
 * function: hex_unescape_string
 * GIO - GLib Input, Output and Streaming Library
 */
static char *
hex_unescape_string (const char *str,
                     int        *out_len,
                     gboolean   *free_return)
{
    int i;
    char *unescaped_str, *p;
    unsigned char c;
    int len;

    len = strlen (str);

    if (strchr (str, '\\') == NULL)
    {
        if (out_len)
            *out_len = len;
        *free_return = FALSE;
        return (char *)str;
    }

    unescaped_str = g_malloc (len + 1);

    p = unescaped_str;
    for (i = 0; i < len; i++)
    {
        if (str[i] == '\\' &&
            str[i+1] == 'x' &&
            len - i >= 4)
        {
            c =
                (g_ascii_xdigit_value (str[i+2]) << 4) |
                 g_ascii_xdigit_value (str[i+3]);
            *p++ = c;
            i += 3;
        }
        else
            *p++ = str[i];
    }
    *p++ = 0;

    if (out_len)
        *out_len = p - unescaped_str;
    *free_return = TRUE;
    return unescaped_str;
}
/* End of stolen code */

static inline gchar **
get_tags_from_info (GFileInfo *info)
{
    char **result;
    const gchar *escaped_tags_string
        = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_XATTR_XDG_TAGS);

    gboolean new_created;
    gchar *tags_string = hex_unescape_string (escaped_tags_string,
                                              NULL,
                                              &new_created);

    gchar *normalized = g_utf8_normalize (tags_string, -1, G_NORMALIZE_NFD);

    if (new_created)
        g_free (tags_string);

    gchar *lower_case = g_utf8_strdown (normalized, -1);
    g_free (normalized);

    result = g_strsplit (lower_case, ",", -1);
    g_free (lower_case);

    return result;
}

static inline gboolean
file_has_all_tags (GFileInfo *info, GList *tags)
{
    if (g_list_length (tags) == 0) {
        return TRUE;
    }

    if (!g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_XATTR_XDG_TAGS))
    {
        return FALSE;
    }

    char **file_tags = get_tags_from_info (info);

    guint file_tags_len = g_strv_length (file_tags);
    if (file_tags_len < g_list_length (tags)) {
        g_strfreev (file_tags);
        return FALSE;
    }

    GList *l;

    for (l = tags; l != NULL; l = l->next) {
        gboolean found = FALSE;
        int i;

        for (i = 0; i < file_tags_len; ++i) {
            if (g_strcmp0 (l->data, file_tags[i]) == 0) {
                found = TRUE;
                break;
            }
        }

        if (found == FALSE) {
            g_strfreev (file_tags);
            return FALSE;
        }
    }

    g_strfreev (file_tags);
    return TRUE;
}

static inline gboolean
check_odt2txt () {
    gboolean rc = TRUE;
    int exit = 0;
    gchar *output = NULL;

    gboolean cmd_rc = g_spawn_command_line_sync ("odt2txt --version", &output, NULL, &exit, NULL);

    if (!cmd_rc || exit != 0 ||
        !output || !g_str_has_prefix (output, "odt2txt"))
    {
        rc = FALSE;
    }

    g_free (output);
    return rc;
}

static inline gchar *
read_odt (const char *filepath) {
    gchar *command = g_strdup_printf ("odt2txt \"%s\"", filepath);
    gchar *output  = NULL;
    int exit = 0;

    gboolean rc = g_spawn_command_line_sync (command, &output, NULL, &exit, NULL);
    if (!rc || exit != 0) {
        g_free (output);
        g_free (command);
        return NULL;
    }

    g_free (command);
    return output;
}

static inline gchar *
utf8_normalize_strdown (const char *str) {
    gchar* lower = NULL;
    gchar *normalized = g_utf8_normalize (str, -1, G_NORMALIZE_DEFAULT);

    if (normalized)
        lower = g_utf8_strdown (normalized, -1);

    g_free (normalized);

    return lower;
}

static inline gboolean
is_file_has_str (
    const char *filepath,
    const char *str,
    const char *mime_type,
    gboolean odt2txt_available)
{
    gboolean rc = TRUE;
    gchar *contents = NULL;
    gchar *lower_contents = NULL;
    gchar *lower_str = NULL;

    if (str[0] == '\0') {
        return TRUE;
    }

    if (g_content_type_is_mime_type (mime_type, "text/plain")) {
        rc = g_file_get_contents (filepath, &contents, NULL, NULL);
    }
    else {
        if (!odt2txt_available) {
            g_warning ("Can't search in file '%s'. odt2txt not found.", filepath);
            rc = FALSE;
        }
        else {
            contents = read_odt (filepath);
            if (!contents)
                rc = FALSE;
        }
    }

    if (rc) {
        lower_str = utf8_normalize_strdown (str);
        lower_contents = utf8_normalize_strdown (contents);

        if (lower_str && lower_contents && strstr (lower_contents, lower_str))
            rc = TRUE;
        else
            rc = FALSE;
    }

    g_free (contents);
    g_free (lower_str);
    g_free (lower_contents);

    return rc;
}

static void
visit_directory (GFile *dir, SearchThreadData *data)
{
    GFileEnumerator *enumerator;
    GFileInfo *info;
    GFile *child;
    const char *mime_type, *display_name;
    char *lower_name, *normalized;
    gboolean hit;
    int i;
    GList *l;
    const char *id;
    gboolean visited;
    gchar *attributes;
    GString *attr_string;
    GDateTime *mod_dt = NULL;
    gchar *filepath = NULL;
    gboolean odt2txt_available = FALSE;

    attr_string = g_string_new (STD_ATTRIBUTES);
    if (data->mime_types != NULL || data->contained_text != NULL) {
        g_string_append (attr_string, "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
    }
    if (data->tags != NULL) {
        g_string_append (attr_string, "," G_FILE_ATTRIBUTE_XATTR_XDG_TAGS);
    }
    if (data->timestamp != 0) {
        g_string_append (attr_string, "," G_FILE_ATTRIBUTE_TIME_MODIFIED);
    }
    if (data->size != 0) {
        g_string_append (attr_string, "," G_FILE_ATTRIBUTE_STANDARD_SIZE);
    }

    if (data->contained_text != NULL) {
        odt2txt_available = check_odt2txt();
    }

    attributes = g_string_free (attr_string, FALSE);
    enumerator = g_file_enumerate_children (dir, (const char*)attributes, 0,
                                            data->cancellable, NULL);

    g_free (attributes);
    if (enumerator == NULL)
    {
        return;
    }

    while ((info = g_file_enumerator_next_file (enumerator, data->cancellable, NULL)) != NULL)
    {
        if (g_file_info_get_is_hidden (info))
        {
            goto next;
        }

        display_name = g_file_info_get_display_name (info);
        if (display_name == NULL)
        {
            goto next;
        }

        normalized = g_utf8_normalize (display_name, -1, G_NORMALIZE_NFD);
        lower_name = g_utf8_strdown (normalized, -1);
        g_free (normalized);

        hit = TRUE;
        for (i = 0; data->words[i] != NULL; i++)
        {
            if (strstr (lower_name, data->words[i]) == NULL)
            {
                hit = FALSE;
                break;
            }
        }
        g_free (lower_name);

        if (hit && data->mime_types)
        {
            mime_type = g_file_info_get_content_type (info);
            hit = FALSE;

            for (l = data->mime_types; mime_type != NULL && l != NULL; l = l->next)
            {
                if (g_content_type_equals (mime_type, l->data))
                {
                    hit = TRUE;
                    break;
                }
            }
        }

        if (hit && data->tags)
        {
            hit = file_has_all_tags (info, data->tags);
        }

        if (hit && data->timestamp != 0) {
            mod_dt = g_file_info_get_modification_date_time (info);
            if (mod_dt) {
                gint64 mod_sec = g_date_time_to_unix (mod_dt);

                if (data->timestamp > 0) {
                    if (data->timestamp < mod_sec)
                        hit = FALSE;
                } else {
                    if (mod_sec < llabs (data->timestamp))
                        hit = FALSE;
                }
                g_date_time_unref (mod_dt);
                mod_dt = NULL;
            } else {
                hit = FALSE;
            }
        }

        if (hit && data->size != 0) {
            gint64 file_size = g_file_info_get_size (info);
            if (data->size > 0) {
                if (file_size < data->size)
                    hit = FALSE;
            } else {
                if (ABS(data->size) < file_size)
                    hit = FALSE;
            }
        }

        child = g_file_get_child (dir, g_file_info_get_name (info));

        if (hit && data->contained_text) {
            mime_type = g_file_info_get_content_type (info);

            if (g_content_type_is_mime_type (mime_type, "text/plain") ||
                g_content_type_equals (mime_type, "application/vnd.oasis.opendocument.text") ||
                g_content_type_equals (mime_type, "application/vnd.oasis.opendocument.text-template") ||
                g_content_type_equals (mime_type, "application/vnd.oasis.opendocument.spreadsheet") ||
                g_content_type_equals (mime_type, "application/vnd.oasis.opendocument.spreadsheet-template") ||
                g_content_type_equals (mime_type, "application/vnd.oasis.opendocument.presentation") ||
                g_content_type_equals (mime_type, "application/vnd.oasis.opendocument.presentation-template")
            ) {
                g_free (filepath);
                filepath = g_file_get_path (child);
                hit = is_file_has_str (filepath, data->contained_text, mime_type, odt2txt_available);
            }
            else {
                hit = FALSE;
            }
        }

        if (hit)
        {
            data->uri_hits = g_list_prepend (data->uri_hits, g_file_get_uri (child));
        }

        data->n_processed_files++;
        if (data->n_processed_files > BATCH_SIZE)
        {
            send_batch (data);
        }

        if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY)
        {
            id = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_ID_FILE);
            visited = FALSE;
            if (id)
            {
                if (g_hash_table_lookup_extended (data->visited,
                                                  id, NULL, NULL))
                {
                    visited = TRUE;
                }
                else
                {
                    g_hash_table_insert (data->visited, g_strdup (id), NULL);
                }
            }

            if (!visited)
            {
                g_queue_push_tail (data->directories, g_object_ref (child));
            }
        }

        g_object_unref (child);
next:
        g_object_unref (info);
    }

    g_free (filepath);
    g_object_unref (enumerator);
}


static gpointer
search_thread_func (gpointer user_data)
{
    SearchThreadData *data;
    GFile *dir;
    GFileInfo *info;

    data = user_data;

    /* Insert id for toplevel directory into visited */
    dir = g_queue_peek_head (data->directories);
    info = g_file_query_info (dir, G_FILE_ATTRIBUTE_ID_FILE, 0, data->cancellable, NULL);
    if (info)
    {
        const char *id;

        id = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_ID_FILE);
        if (id)
        {
            g_hash_table_insert (data->visited, g_strdup (id), NULL);
        }
        g_object_unref (info);
    }

    while (!g_cancellable_is_cancelled (data->cancellable) &&
            (dir = g_queue_pop_head (data->directories)) != NULL)
    {
        visit_directory (dir, data);
        g_object_unref (dir);
    }
    send_batch (data);

    g_idle_add (search_thread_done_idle, data);

    return NULL;
}

static void
baul_search_engine_simple_start (BaulSearchEngine *engine)
{
    BaulSearchEngineSimple *simple;
    SearchThreadData *data;
    GThread *thread;

    simple = BAUL_SEARCH_ENGINE_SIMPLE (engine);

    if (simple->details->active_search != NULL)
    {
        return;
    }

    if (simple->details->query == NULL)
    {
        return;
    }

    data = search_thread_data_new (simple, simple->details->query);

    thread = g_thread_new ("baul-search-simple", search_thread_func, data);
    simple->details->active_search = data;

    g_thread_unref (thread);
}

static void
baul_search_engine_simple_stop (BaulSearchEngine *engine)
{
    BaulSearchEngineSimple *simple;

    simple = BAUL_SEARCH_ENGINE_SIMPLE (engine);

    if (simple->details->active_search != NULL)
    {
        g_cancellable_cancel (simple->details->active_search->cancellable);
        simple->details->active_search = NULL;
    }

    /* Drop the live-update state; it is rebuilt by the next search. */
    if (simple->details->monitored_dirs != NULL)
    {
        g_hash_table_remove_all (simple->details->monitored_dirs);
    }

    if (simple->details->hit_uris != NULL)
    {
        g_hash_table_remove_all (simple->details->hit_uris);
    }

    if (simple->details->removed_hit_uris != NULL)
    {
        g_hash_table_remove_all (simple->details->removed_hit_uris);
    }

    if (simple->details->pending_restore_uris != NULL)
    {
        g_hash_table_remove_all (simple->details->pending_restore_uris);
    }
}

static gboolean
baul_search_engine_simple_is_indexed (BaulSearchEngine *engine G_GNUC_UNUSED)
{
    return FALSE;
}

static void
baul_search_engine_simple_set_query (BaulSearchEngine *engine, BaulQuery *query)
{
    BaulSearchEngineSimple *simple;

    simple = BAUL_SEARCH_ENGINE_SIMPLE (engine);

    if (query)
    {
        g_object_ref (query);
    }

    if (simple->details->query)
    {
        g_object_unref (simple->details->query);
    }

    simple->details->query = query;
}

static void
baul_search_engine_simple_class_init (BaulSearchEngineSimpleClass *class)
{
    GObjectClass *gobject_class;
    BaulSearchEngineClass *engine_class;

    parent_class = g_type_class_peek_parent (class);

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = finalize;

    engine_class = BAUL_SEARCH_ENGINE_CLASS (class);
    engine_class->set_query = baul_search_engine_simple_set_query;
    engine_class->start = baul_search_engine_simple_start;
    engine_class->stop = baul_search_engine_simple_stop;
    engine_class->is_indexed = baul_search_engine_simple_is_indexed;
}

static void
baul_search_engine_simple_init (BaulSearchEngineSimple *engine)
{
    engine->details = g_new0 (BaulSearchEngineSimpleDetails, 1);

    engine->details->hit_uris =
        g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    engine->details->removed_hit_uris =
        g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    engine->details->monitored_dirs =
        g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal,
                               (GDestroyNotify) g_object_unref,
                               monitor_value_free);
    engine->details->pending_restore_uris =
        g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}


BaulSearchEngine *
baul_search_engine_simple_new (void)
{
    BaulSearchEngine *engine;

    engine = g_object_new (BAUL_TYPE_SEARCH_ENGINE_SIMPLE, NULL);

    return engine;
}
