/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-metadata.h: #defines and other metadata-related info

   Copyright (C) 2000 Eazel, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Author: John Sullivan <sullivan@eazel.com>
*/

#ifndef BAUL_METADATA_H
#define BAUL_METADATA_H

/* Keys for getting/setting Baul metadata. All metadata used in Baul
 * should define its key here, so we can keep track of the whole set easily.
 * Any updates here needs to be added in baul-metadata.c too.
 */

#include <glib.h>

/* Per-file */

#define BAUL_METADATA_KEY_DEFAULT_VIEW		 	"baul-default-view"

#define BAUL_METADATA_KEY_LOCATION_BACKGROUND_COLOR 	"folder-background-color"
#define BAUL_METADATA_KEY_LOCATION_BACKGROUND_IMAGE 	"folder-background-image"

#define BAUL_METADATA_KEY_ICON_VIEW_ZOOM_LEVEL       	"baul-icon-view-zoom-level"
#define BAUL_METADATA_KEY_ICON_VIEW_AUTO_LAYOUT      	"baul-icon-view-auto-layout"
#define BAUL_METADATA_KEY_ICON_VIEW_TIGHTER_LAYOUT      	"baul-icon-view-tighter-layout"
#define BAUL_METADATA_KEY_ICON_VIEW_SORT_BY          	"baul-icon-view-sort-by"
#define BAUL_METADATA_KEY_ICON_VIEW_SORT_REVERSED    	"baul-icon-view-sort-reversed"
#define BAUL_METADATA_KEY_ICON_VIEW_KEEP_ALIGNED            "baul-icon-view-keep-aligned"
#define BAUL_METADATA_KEY_ICON_VIEW_LAYOUT_TIMESTAMP	"baul-icon-view-layout-timestamp"

#define BAUL_METADATA_KEY_LIST_VIEW_ZOOM_LEVEL       	"baul-list-view-zoom-level"
#define BAUL_METADATA_KEY_LIST_VIEW_SORT_COLUMN      	"baul-list-view-sort-column"
#define BAUL_METADATA_KEY_LIST_VIEW_SORT_REVERSED    	"baul-list-view-sort-reversed"
#define BAUL_METADATA_KEY_LIST_VIEW_VISIBLE_COLUMNS    	"baul-list-view-visible-columns"
#define BAUL_METADATA_KEY_LIST_VIEW_COLUMN_ORDER    	"baul-list-view-column-order"

#define BAUL_METADATA_KEY_COMPACT_VIEW_ZOOM_LEVEL		"baul-compact-view-zoom-level"

#define BAUL_METADATA_KEY_WINDOW_GEOMETRY			"baul-window-geometry"
#define BAUL_METADATA_KEY_WINDOW_SCROLL_POSITION		"baul-window-scroll-position"
#define BAUL_METADATA_KEY_WINDOW_SHOW_HIDDEN_FILES		"baul-window-show-hidden-files"
#define BAUL_METADATA_KEY_WINDOW_SHOW_BACKUP_FILES		"baul-window-show-backup-files"
#define BAUL_METADATA_KEY_WINDOW_MAXIMIZED			"baul-window-maximized"
#define BAUL_METADATA_KEY_WINDOW_STICKY			"baul-window-sticky"
#define BAUL_METADATA_KEY_WINDOW_KEEP_ABOVE			"baul-window-keep-above"

#define BAUL_METADATA_KEY_SIDEBAR_BACKGROUND_COLOR   	"baul-sidebar-background-color"
#define BAUL_METADATA_KEY_SIDEBAR_BACKGROUND_IMAGE   	"baul-sidebar-background-image"
#define BAUL_METADATA_KEY_SIDEBAR_BUTTONS			"baul-sidebar-buttons"

#define BAUL_METADATA_KEY_ICON_POSITION              	"baul-icon-position"
#define BAUL_METADATA_KEY_ICON_POSITION_TIMESTAMP		"baul-icon-position-timestamp"
#define BAUL_METADATA_KEY_ANNOTATION                 	"annotation"
#define BAUL_METADATA_KEY_ICON_SCALE                 	"icon-scale"
#define BAUL_METADATA_KEY_CUSTOM_ICON                	"custom-icon"
#define BAUL_METADATA_KEY_SCREEN				"screen"
#define BAUL_METADATA_KEY_EMBLEMS				"emblems"

guint baul_metadata_get_id (const char *metadata);

#endif /* BAUL_METADATA_H */
