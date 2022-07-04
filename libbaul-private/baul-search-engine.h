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

#ifndef BAUL_SEARCH_ENGINE_H
#define BAUL_SEARCH_ENGINE_H

#include <glib-object.h>

#include "baul-query.h"

#define BAUL_TYPE_SEARCH_ENGINE		(baul_search_engine_get_type ())
#define BAUL_SEARCH_ENGINE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_SEARCH_ENGINE, BaulSearchEngine))
#define BAUL_SEARCH_ENGINE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_SEARCH_ENGINE, BaulSearchEngineClass))
#define BAUL_IS_SEARCH_ENGINE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_SEARCH_ENGINE))
#define BAUL_IS_SEARCH_ENGINE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_SEARCH_ENGINE))
#define BAUL_SEARCH_ENGINE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_SEARCH_ENGINE, BaulSearchEngineClass))

typedef struct BaulSearchEngineDetails BaulSearchEngineDetails;

typedef struct BaulSearchEngine
{
    GObject parent;
    BaulSearchEngineDetails *details;
} BaulSearchEngine;

typedef struct
{
    GObjectClass parent_class;

    /* VTable */
    void (*set_query) (BaulSearchEngine *engine, BaulQuery *query);
    void (*start) (BaulSearchEngine *engine);
    void (*stop) (BaulSearchEngine *engine);
    gboolean (*is_indexed) (BaulSearchEngine *engine);

    /* Signals */
    void (*hits_added) (BaulSearchEngine *engine, GList *hits);
    void (*hits_subtracted) (BaulSearchEngine *engine, GList *hits);
    void (*finished) (BaulSearchEngine *engine);
    void (*error) (BaulSearchEngine *engine, const char *error_message);
} BaulSearchEngineClass;

GType          baul_search_engine_get_type  (void);
gboolean       baul_search_engine_enabled (void);

BaulSearchEngine* baul_search_engine_new       (void);

void           baul_search_engine_set_query (BaulSearchEngine *engine, BaulQuery *query);
void	       baul_search_engine_start (BaulSearchEngine *engine);
void	       baul_search_engine_stop (BaulSearchEngine *engine);
gboolean       baul_search_engine_is_indexed (BaulSearchEngine *engine);

void	       baul_search_engine_hits_added (BaulSearchEngine *engine, GList *hits);
void	       baul_search_engine_hits_subtracted (BaulSearchEngine *engine, GList *hits);
void	       baul_search_engine_finished (BaulSearchEngine *engine);
void	       baul_search_engine_error (BaulSearchEngine *engine, const char *error_message);

#endif /* BAUL_SEARCH_ENGINE_H */
