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

#ifndef BAUL_QUERY_H
#define BAUL_QUERY_H

#include <glib-object.h>

#define BAUL_TYPE_QUERY		(baul_query_get_type ())
#define BAUL_QUERY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_QUERY, BaulQuery))
#define BAUL_QUERY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_QUERY, BaulQueryClass))
#define BAUL_IS_QUERY(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_QUERY))
#define BAUL_IS_QUERY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_QUERY))
#define BAUL_QUERY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_QUERY, BaulQueryClass))

typedef struct BaulQueryDetails BaulQueryDetails;

typedef struct BaulQuery
{
    GObject parent;
    BaulQueryDetails *details;
} BaulQuery;

typedef struct
{
    GObjectClass parent_class;
} BaulQueryClass;

GType          baul_query_get_type (void);
gboolean       baul_query_enabled  (void);

BaulQuery* baul_query_new      (void);

char *         baul_query_get_text           (BaulQuery *query);
void           baul_query_set_text           (BaulQuery *query, const char *text);

char *         baul_query_get_location       (BaulQuery *query);
void           baul_query_set_location       (BaulQuery *query, const char *uri);

GList *        baul_query_get_tags           (BaulQuery *query);
void           baul_query_set_tags           (BaulQuery *query, GList *tags);
void           baul_query_add_tag            (BaulQuery *query, const char *tag);

GList *        baul_query_get_mime_types     (BaulQuery *query);
void           baul_query_set_mime_types     (BaulQuery *query, GList *mime_types);
void           baul_query_add_mime_type      (BaulQuery *query, const char *mime_type);

char *         baul_query_to_readable_string (BaulQuery *query);
BaulQuery *    baul_query_load               (char *file);
gboolean       baul_query_save               (BaulQuery *query, char *file);

gint64         baul_query_get_timestamp      (BaulQuery *query);
void           baul_query_set_timestamp      (BaulQuery *query, gint64 sec);

gint64         baul_query_get_size           (BaulQuery *query);
void           baul_query_set_size           (BaulQuery *query, gint64 size);

char *         baul_query_get_contained_text (BaulQuery *query);
void           baul_query_set_contained_text (BaulQuery *query, const char *text);

#endif /* BAUL_QUERY_H */
