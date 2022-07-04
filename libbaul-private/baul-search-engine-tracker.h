/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2005 Mr Jamie McCracken
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
 * Author: Jamie McCracken (jamiemcc@gnome.org)
 *
 */

#ifndef BAUL_SEARCH_ENGINE_TRACKER_H
#define BAUL_SEARCH_ENGINE_TRACKER_H

#include "baul-search-engine.h"

#define BAUL_TYPE_SEARCH_ENGINE_TRACKER		(baul_search_engine_tracker_get_type ())
#define BAUL_SEARCH_ENGINE_TRACKER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_SEARCH_ENGINE_TRACKER, BaulSearchEngineTracker))
#define BAUL_SEARCH_ENGINE_TRACKER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_SEARCH_ENGINE_TRACKER, BaulSearchEngineTrackerClass))
#define BAUL_IS_SEARCH_ENGINE_TRACKER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_SEARCH_ENGINE_TRACKER))
#define BAUL_IS_SEARCH_ENGINE_TRACKER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_SEARCH_ENGINE_TRACKER))
#define BAUL_SEARCH_ENGINE_TRACKER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_SEARCH_ENGINE_TRACKER, BaulSearchEngineTrackerClass))

typedef struct BaulSearchEngineTrackerDetails BaulSearchEngineTrackerDetails;

typedef struct BaulSearchEngineTracker
{
    BaulSearchEngine parent;
    BaulSearchEngineTrackerDetails *details;
} BaulSearchEngineTracker;

typedef struct
{
    BaulSearchEngineClass parent_class;
} BaulSearchEngineTrackerClass;

GType baul_search_engine_tracker_get_type (void);

BaulSearchEngine* baul_search_engine_tracker_new (void);

#endif /* BAUL_SEARCH_ENGINE_TRACKER_H */
