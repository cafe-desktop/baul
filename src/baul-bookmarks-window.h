/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 1999, 2000 Eazel, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Authors: John Sullivan <sullivan@eazel.com>
 */

/* baul-bookmarks-window.h - interface for bookmark-editing window.
 */

#ifndef BAUL_BOOKMARKS_WINDOW_H
#define BAUL_BOOKMARKS_WINDOW_H

#include <ctk/ctk.h>
#include "baul-bookmark-list.h"
#include "baul-window.h"

GtkWindow *create_bookmarks_window                 (BaulBookmarkList *bookmarks,
                                                    BaulWindow       *window_source);
void       baul_bookmarks_window_save_geometry     (GtkWindow        *window);
void	   edit_bookmarks_dialog_set_signals	   (BaulWindow       *window);

#endif /* BAUL_BOOKMARKS_WINDOW_H */
