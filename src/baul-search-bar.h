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

#ifndef BAUL_SEARCH_BAR_H
#define BAUL_SEARCH_BAR_H

#include <ctk/ctk.h>

#include <libbaul-private/baul-query.h>

#include "baul-window.h"

#define BAUL_TYPE_SEARCH_BAR baul_search_bar_get_type()
#define BAUL_SEARCH_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_SEARCH_BAR, BaulSearchBar))
#define BAUL_SEARCH_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_SEARCH_BAR, BaulSearchBarClass))
#define BAUL_IS_SEARCH_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_SEARCH_BAR))
#define BAUL_IS_SEARCH_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_SEARCH_BAR))
#define BAUL_SEARCH_BAR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_SEARCH_BAR, BaulSearchBarClass))

typedef struct BaulSearchBarDetails BaulSearchBarDetails;

typedef struct BaulSearchBar
{
    GtkEventBox parent;
    BaulSearchBarDetails *details;
} BaulSearchBar;

typedef struct
{
    GtkEventBoxClass parent_class;

    void (* activate) (BaulSearchBar *bar);
    void (* cancel)   (BaulSearchBar *bar);
    void (* focus_in) (BaulSearchBar *bar);
} BaulSearchBarClass;

GType      baul_search_bar_get_type     	(void);
GtkWidget* baul_search_bar_new          	(BaulWindow *window);

GtkWidget *    baul_search_bar_borrow_entry  (BaulSearchBar *bar);
void           baul_search_bar_return_entry  (BaulSearchBar *bar);
void           baul_search_bar_grab_focus    (BaulSearchBar *bar);
BaulQuery *baul_search_bar_get_query     (BaulSearchBar *bar);
void           baul_search_bar_clear         (BaulSearchBar *bar);

#endif /* BAUL_SEARCH_BAR_H */
