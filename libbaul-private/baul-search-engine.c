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

#include <eel/eel-gtk-macros.h>

#include "baul-search-engine.h"
#include "baul-search-engine-beagle.h"
#include "baul-search-engine-simple.h"
#include "baul-search-engine-tracker.h"

struct BaulSearchEngineDetails
{
    int none;
};

enum
{
    HITS_ADDED,
    HITS_SUBTRACTED,
    FINISHED,
    ERROR,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_ABSTRACT_TYPE (BaulSearchEngine,
                        baul_search_engine,
                        G_TYPE_OBJECT);

static GObjectClass *parent_class = NULL;

static void
finalize (GObject *object)
{
    BaulSearchEngine *engine;

    engine = BAUL_SEARCH_ENGINE (object);

    g_free (engine->details);

    EEL_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
baul_search_engine_class_init (BaulSearchEngineClass *class)
{
    GObjectClass *gobject_class;

    parent_class = g_type_class_peek_parent (class);

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = finalize;

    signals[HITS_ADDED] =
        g_signal_new ("hits-added",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulSearchEngineClass, hits_added),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1,
                      G_TYPE_POINTER);

    signals[HITS_SUBTRACTED] =
        g_signal_new ("hits-subtracted",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulSearchEngineClass, hits_subtracted),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1,
                      G_TYPE_POINTER);

    signals[FINISHED] =
        g_signal_new ("finished",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulSearchEngineClass, finished),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[ERROR] =
        g_signal_new ("error",
                      G_TYPE_FROM_CLASS (class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BaulSearchEngineClass, error),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__STRING,
                      G_TYPE_NONE, 1,
                      G_TYPE_STRING);

}

static void
baul_search_engine_init (BaulSearchEngine *engine)
{
    engine->details = g_new0 (BaulSearchEngineDetails, 1);
}

BaulSearchEngine *
baul_search_engine_new (void)
{
    BaulSearchEngine *engine;

    engine = baul_search_engine_tracker_new ();
    if (engine)
    {
        return engine;
    }

    engine = baul_search_engine_beagle_new ();
    if (engine)
    {
        return engine;
    }

    engine = baul_search_engine_simple_new ();
    return engine;
}

void
baul_search_engine_set_query (BaulSearchEngine *engine, BaulQuery *query)
{
    g_return_if_fail (BAUL_IS_SEARCH_ENGINE (engine));
    g_return_if_fail (BAUL_SEARCH_ENGINE_GET_CLASS (engine)->set_query != NULL);

    BAUL_SEARCH_ENGINE_GET_CLASS (engine)->set_query (engine, query);
}

void
baul_search_engine_start (BaulSearchEngine *engine)
{
    g_return_if_fail (BAUL_IS_SEARCH_ENGINE (engine));
    g_return_if_fail (BAUL_SEARCH_ENGINE_GET_CLASS (engine)->start != NULL);

    BAUL_SEARCH_ENGINE_GET_CLASS (engine)->start (engine);
}


void
baul_search_engine_stop (BaulSearchEngine *engine)
{
    g_return_if_fail (BAUL_IS_SEARCH_ENGINE (engine));
    g_return_if_fail (BAUL_SEARCH_ENGINE_GET_CLASS (engine)->stop != NULL);

    BAUL_SEARCH_ENGINE_GET_CLASS (engine)->stop (engine);
}

gboolean
baul_search_engine_is_indexed (BaulSearchEngine *engine)
{
    g_return_val_if_fail (BAUL_IS_SEARCH_ENGINE (engine), FALSE);
    g_return_val_if_fail (BAUL_SEARCH_ENGINE_GET_CLASS (engine)->is_indexed != NULL, FALSE);

    return BAUL_SEARCH_ENGINE_GET_CLASS (engine)->is_indexed (engine);
}

void
baul_search_engine_hits_added (BaulSearchEngine *engine, GList *hits)
{
    g_return_if_fail (BAUL_IS_SEARCH_ENGINE (engine));

    g_signal_emit (engine, signals[HITS_ADDED], 0, hits);
}


void
baul_search_engine_hits_subtracted (BaulSearchEngine *engine, GList *hits)
{
    g_return_if_fail (BAUL_IS_SEARCH_ENGINE (engine));

    g_signal_emit (engine, signals[HITS_SUBTRACTED], 0, hits);
}


void
baul_search_engine_finished (BaulSearchEngine *engine)
{
    g_return_if_fail (BAUL_IS_SEARCH_ENGINE (engine));

    g_signal_emit (engine, signals[FINISHED], 0);
}

void
baul_search_engine_error (BaulSearchEngine *engine, const char *error_message)
{
    g_return_if_fail (BAUL_IS_SEARCH_ENGINE (engine));

    g_signal_emit (engine, signals[ERROR], 0, error_message);
}
