/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2005 Novell, Inc.
 *
 * Caja is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Caja is distributed in the hope that it will be useful,
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

#include <gtk/gtk.h>

#include <libbaul-private/baul-query.h>

#include "baul-window.h"

#define BAUL_TYPE_SEARCH_BAR baul_search_bar_get_type()
#define BAUL_SEARCH_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_SEARCH_BAR, CajaSearchBar))
#define BAUL_SEARCH_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_SEARCH_BAR, CajaSearchBarClass))
#define BAUL_IS_SEARCH_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_SEARCH_BAR))
#define BAUL_IS_SEARCH_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_SEARCH_BAR))
#define BAUL_SEARCH_BAR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_SEARCH_BAR, CajaSearchBarClass))

typedef struct CajaSearchBarDetails CajaSearchBarDetails;

typedef struct CajaSearchBar
{
    GtkEventBox parent;
    CajaSearchBarDetails *details;
} CajaSearchBar;

typedef struct
{
    GtkEventBoxClass parent_class;

    void (* activate) (CajaSearchBar *bar);
    void (* cancel)   (CajaSearchBar *bar);
    void (* focus_in) (CajaSearchBar *bar);
} CajaSearchBarClass;

GType      baul_search_bar_get_type     	(void);
GtkWidget* baul_search_bar_new          	(CajaWindow *window);

GtkWidget *    baul_search_bar_borrow_entry  (CajaSearchBar *bar);
void           baul_search_bar_return_entry  (CajaSearchBar *bar);
void           baul_search_bar_grab_focus    (CajaSearchBar *bar);
CajaQuery *baul_search_bar_get_query     (CajaSearchBar *bar);
void           baul_search_bar_clear         (CajaSearchBar *bar);

#endif /* BAUL_SEARCH_BAR_H */
