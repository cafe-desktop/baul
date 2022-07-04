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

#ifndef BAUL_SEARCH_ENGINE_BEAGLE_H
#define BAUL_SEARCH_ENGINE_BEAGLE_H

#include "baul-search-engine.h"

#define BAUL_TYPE_SEARCH_ENGINE_BEAGLE		(baul_search_engine_beagle_get_type ())
#define BAUL_SEARCH_ENGINE_BEAGLE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_SEARCH_ENGINE_BEAGLE, BaulSearchEngineBeagle))
#define BAUL_SEARCH_ENGINE_BEAGLE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_SEARCH_ENGINE_BEAGLE, BaulSearchEngineBeagleClass))
#define BAUL_IS_SEARCH_ENGINE_BEAGLE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_SEARCH_ENGINE_BEAGLE))
#define BAUL_IS_SEARCH_ENGINE_BEAGLE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_SEARCH_ENGINE_BEAGLE))
#define BAUL_SEARCH_ENGINE_BEAGLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_SEARCH_ENGINE_BEAGLE, BaulSearchEngineBeagleClass))

typedef struct BaulSearchEngineBeagleDetails BaulSearchEngineBeagleDetails;

typedef struct BaulSearchEngineBeagle
{
    BaulSearchEngine parent;
    BaulSearchEngineBeagleDetails *details;
} BaulSearchEngineBeagle;

typedef struct
{
    BaulSearchEngineClass parent_class;
} BaulSearchEngineBeagleClass;

GType          baul_search_engine_beagle_get_type  (void);

BaulSearchEngine* baul_search_engine_beagle_new       (void);

#endif /* BAUL_SEARCH_ENGINE_BEAGLE_H */
