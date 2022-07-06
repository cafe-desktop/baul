/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2005 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 *
 */

#ifndef BAUL_QUERY_EDITOR_H
#define BAUL_QUERY_EDITOR_H

#include <ctk/ctk.h>

#include <libbaul-private/baul-query.h>
#include <libbaul-private/baul-window-info.h>

#include "baul-search-bar.h"

#define BAUL_TYPE_QUERY_EDITOR baul_query_editor_get_type()
#define BAUL_QUERY_EDITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_QUERY_EDITOR, BaulQueryEditor))
#define BAUL_QUERY_EDITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_QUERY_EDITOR, BaulQueryEditorClass))
#define BAUL_IS_QUERY_EDITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_QUERY_EDITOR))
#define BAUL_IS_QUERY_EDITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_QUERY_EDITOR))
#define BAUL_QUERY_EDITOR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_QUERY_EDITOR, BaulQueryEditorClass))

typedef struct BaulQueryEditorDetails BaulQueryEditorDetails;

typedef struct BaulQueryEditor
{
    GtkBox parent;
    BaulQueryEditorDetails *details;
} BaulQueryEditor;

typedef struct
{
    GtkBoxClass parent_class;

    void (* changed) (BaulQueryEditor  *editor,
                      BaulQuery        *query,
                      gboolean              reload);
    void (* cancel)   (BaulQueryEditor *editor);
} BaulQueryEditorClass;

GType      baul_query_editor_get_type     	   (void);
GtkWidget* baul_query_editor_new          	   (gboolean start_hidden,
        gboolean is_indexed);
GtkWidget* baul_query_editor_new_with_bar      (gboolean start_hidden,
        gboolean is_indexed,
        gboolean start_attached,
        BaulSearchBar *bar,
        BaulWindowSlot *slot);
void       baul_query_editor_set_default_query (BaulQueryEditor *editor);

void	   baul_query_editor_grab_focus (BaulQueryEditor *editor);
void       baul_query_editor_clear_query (BaulQueryEditor *editor);

BaulQuery *baul_query_editor_get_query   (BaulQueryEditor *editor);
void           baul_query_editor_set_query   (BaulQueryEditor *editor,
        BaulQuery       *query);
void           baul_query_editor_set_visible (BaulQueryEditor *editor,
        gboolean             visible);

#endif /* BAUL_QUERY_EDITOR_H */
