/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2005 Novell, Inc.
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
 * Author: Anders Carlsson <andersca@imendio.com>
 *
 */

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>

#include <eel/eel-gtk-macros.h>
#include <eel/eel-glib-extensions.h>

#include "baul-query.h"
#include "baul-file-utilities.h"

struct BaulQueryDetails
{
    char *text;
    char *location_uri;
    GList *mime_types;
    GList *tags;
    gint64 timestamp;
    gint64 size;
    char *contained_text;
};

G_DEFINE_TYPE (BaulQuery,
               baul_query,
               G_TYPE_OBJECT);

static GObjectClass *parent_class = NULL;

static void
finalize (GObject *object)
{
    BaulQuery *query;

    query = BAUL_QUERY (object);

    g_free (query->details->text);
    g_free (query->details);

    EEL_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
baul_query_class_init (BaulQueryClass *class)
{
    GObjectClass *gobject_class;

    parent_class = g_type_class_peek_parent (class);

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = finalize;
}

static void
baul_query_init (BaulQuery *query)
{
    query->details = g_new0 (BaulQueryDetails, 1);
    query->details->timestamp = 0;
    query->details->size = 0;
}

BaulQuery *
baul_query_new (void)
{
    return g_object_new (BAUL_TYPE_QUERY,  NULL);
}


char *
baul_query_get_text (BaulQuery *query)
{
    return g_strdup (query->details->text);
}

void
baul_query_set_text (BaulQuery *query, const char *text)
{
    g_free (query->details->text);
    query->details->text = g_strdup (text);
}

char *
baul_query_get_location (BaulQuery *query)
{
    return g_strdup (query->details->location_uri);
}

void
baul_query_set_location (BaulQuery *query, const char *uri)
{
    g_free (query->details->location_uri);
    query->details->location_uri = g_strdup (uri);
}

GList *
baul_query_get_mime_types (BaulQuery *query)
{
    return g_list_copy_deep (query->details->mime_types, (GCopyFunc) g_strdup, NULL);
}

void
baul_query_set_mime_types (BaulQuery *query, GList *mime_types)
{
    g_list_free_full (query->details->mime_types, g_free);
    query->details->mime_types = g_list_copy_deep (mime_types, (GCopyFunc) g_strdup, NULL);
}

void
baul_query_add_mime_type (BaulQuery *query, const char *mime_type)
{
    query->details->mime_types = g_list_append (query->details->mime_types,
                                 g_strdup (mime_type));
}

GList *
baul_query_get_tags (BaulQuery *query)
{
    return g_list_copy_deep (query->details->tags, (GCopyFunc) g_strdup, NULL);
}

void
baul_query_set_tags (BaulQuery *query, GList *tags)
{
    g_list_free_full (query->details->tags, g_free);
    query->details->tags = g_list_copy_deep (tags, (GCopyFunc) g_strdup, NULL);
}

void
baul_query_add_tag (BaulQuery *query, const char *tag)
{
    gchar *normalized = g_utf8_normalize (tag, -1, G_NORMALIZE_NFD);
    gchar *lower_case = g_utf8_strdown (normalized, -1);

    g_free (normalized);
    query->details->tags = g_list_append (query->details->tags, lower_case);
}

char *
baul_query_to_readable_string (BaulQuery *query)
{
    if (!query || !query->details->text)
    {
        return g_strdup (_("Search"));
    }

    return g_strdup_printf (_("Search for \"%s\""), query->details->text);
}

static char *
encode_home_uri (const char *uri)
{
    char *home_uri;
    const char *encoded_uri;

    home_uri = baul_get_home_directory_uri ();

    if (g_str_has_prefix (uri, home_uri))
    {
        encoded_uri = uri + strlen (home_uri);
        if (*encoded_uri == '/')
        {
            encoded_uri++;
        }
    }
    else
    {
        encoded_uri = uri;
    }

    g_free (home_uri);

    return g_markup_escape_text (encoded_uri, -1);
}

static char *
decode_home_uri (const char *uri)
{
    char *decoded_uri;

    if (g_str_has_prefix (uri, "file:"))
    {
        decoded_uri = g_strdup (uri);
    }
    else
    {
        char *home_uri;

        home_uri = baul_get_home_directory_uri ();

        decoded_uri = g_strconcat (home_uri, "/", uri, NULL);

        g_free (home_uri);
    }

    return decoded_uri;
}


typedef struct
{
    BaulQuery *query;
    gboolean in_text;
    gboolean in_location;
    gboolean in_mimetypes;
    gboolean in_mimetype;
    gboolean in_tags;
    gboolean in_tag;
} ParserInfo;

static void
start_element_cb (GMarkupParseContext *ctx,
                  const char *element_name,
                  const char **attribute_names,
                  const char **attribute_values,
                  gpointer user_data,
                  GError **err)
{
    ParserInfo *info;

    info = (ParserInfo *) user_data;

    if (strcmp (element_name, "text") == 0)
        info->in_text = TRUE;
    else if (strcmp (element_name, "location") == 0)
        info->in_location = TRUE;
    else if (strcmp (element_name, "mimetypes") == 0)
        info->in_mimetypes = TRUE;
    else if (strcmp (element_name, "mimetype") == 0)
        info->in_mimetype = TRUE;
    else if (strcmp (element_name, "tags") == 0)
        info->in_tags = TRUE;
    else if (strcmp (element_name, "tag") == 0)
        info->in_tag = TRUE;
}

static void
end_element_cb (GMarkupParseContext *ctx,
                const char *element_name,
                gpointer user_data,
                GError **err)
{
    ParserInfo *info;

    info = (ParserInfo *) user_data;

    if (strcmp (element_name, "text") == 0)
        info->in_text = FALSE;
    else if (strcmp (element_name, "location") == 0)
        info->in_location = FALSE;
    else if (strcmp (element_name, "mimetypes") == 0)
        info->in_mimetypes = FALSE;
    else if (strcmp (element_name, "mimetype") == 0)
        info->in_mimetype = FALSE;
    else if (strcmp (element_name, "tags") == 0)
        info->in_tags = FALSE;
    else if (strcmp (element_name, "tag") == 0)
        info->in_tag = FALSE;
}

static void
text_cb (GMarkupParseContext *ctx,
         const char *text,
         gsize text_len,
         gpointer user_data,
         GError **err)
{
    ParserInfo *info;
    char *t, *uri;

    info = (ParserInfo *) user_data;

    t = g_strndup (text, text_len);

    if (info->in_text)
    {
        baul_query_set_text (info->query, t);
    }
    else if (info->in_location)
    {
        uri = decode_home_uri (t);
        baul_query_set_location (info->query, uri);
        g_free (uri);
    }
    else if (info->in_mimetypes && info->in_mimetype)
    {
        baul_query_add_mime_type (info->query, t);
    }
    else if (info->in_tags && info->in_tag)
    {
        baul_query_add_tag (info->query, t);
    }

    g_free (t);

}

static void
error_cb (GMarkupParseContext *ctx,
          GError *err,
          gpointer user_data)
{
}

static GMarkupParser parser =
{
    start_element_cb,
    end_element_cb,
    text_cb,
    NULL,
    error_cb
};


static BaulQuery *
baul_query_parse_xml (char *xml, gsize xml_len)
{
    ParserInfo info = { NULL };
    GMarkupParseContext *ctx;

    if (xml_len == -1)
    {
        xml_len = strlen (xml);
    }

    info.query = baul_query_new ();
    info.in_text = FALSE;

    ctx = g_markup_parse_context_new (&parser, 0, &info, NULL);
    g_markup_parse_context_parse (ctx, xml, xml_len, NULL);
    g_markup_parse_context_free (ctx);

    return info.query;
}


BaulQuery *
baul_query_load (char *file)
{
    BaulQuery *query;
    char *xml;
    gsize xml_len;

    if (!g_file_test (file, G_FILE_TEST_EXISTS))
    {
        return NULL;
    }


    g_file_get_contents (file, &xml, &xml_len, NULL);
    query = baul_query_parse_xml (xml, xml_len);
    g_free (xml);

    return query;
}

static char *
baul_query_to_xml (BaulQuery *query)
{
    GString *xml;
    char *text;
    GList *l;

    xml = g_string_new ("");
    g_string_append (xml,
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                     "<query version=\"1.0\">\n");

    text = g_markup_escape_text (query->details->text, -1);
    g_string_append_printf (xml, "   <text>%s</text>\n", text);
    g_free (text);

    if (query->details->location_uri)
    {
        char *uri;

        uri = encode_home_uri (query->details->location_uri);
        g_string_append_printf (xml, "   <location>%s</location>\n", uri);
        g_free (uri);
    }

    if (query->details->mime_types)
    {
        g_string_append (xml, "   <mimetypes>\n");
        for (l = query->details->mime_types; l != NULL; l = l->next)
        {
            char *mimetype;

            mimetype = g_markup_escape_text (l->data, -1);
            g_string_append_printf (xml, "      <mimetype>%s</mimetype>\n", mimetype);
            g_free (mimetype);
        }
        g_string_append (xml, "   </mimetypes>\n");
    }

    if (query->details->tags)
    {
        g_string_append (xml, "   <tags>\n");
        for (l = query->details->tags; l != NULL; l = l->next)
        {
            char *tag;

            tag = g_markup_escape_text (l->data, -1);
            g_string_append_printf (xml, "      <tag>%s</tag>\n", tag);
            g_free (tag);
        }
        g_string_append (xml, "   </tags>\n");
    }

    if (query->details->timestamp != 0)
    {
        g_string_append_printf(xml, "   <duration>%ld</duration>",
                               query->details->timestamp);
    }

    if (query->details->size != 0)
    {
        g_string_append_printf(xml, "   <size>%ld</size>", query->details->size);
    }

    g_string_append (xml, "</query>\n");

    return g_string_free (xml, FALSE);
}

gboolean
baul_query_save (BaulQuery *query, char *file)
{
    char *xml;
    GError *err = NULL;
    gboolean res;


    res = TRUE;
    xml = baul_query_to_xml (query);
    g_file_set_contents (file, xml, strlen (xml), &err);
    g_free (xml);

    if (err != NULL)
    {
        res = FALSE;
        g_error_free (err);
    }
    return res;
}

void baul_query_set_timestamp(BaulQuery *query, gint64 sec)
{
    query->details->timestamp = sec;
}

gint64 baul_query_get_timestamp(BaulQuery *query)
{
    return query->details->timestamp;
}

void baul_query_set_size(BaulQuery *query, gint64 size)
{
    query->details->size = size;
}

gint64 baul_query_get_size(BaulQuery *query)
{
    return query->details->size;
}

void baul_query_set_contained_text (BaulQuery *query, const char *text)
{
    g_free (query->details->contained_text);
    query->details->contained_text = g_strdup (text);
}

char *baul_query_get_contained_text (BaulQuery *query)
{
    return g_strdup (query->details->contained_text);
}
