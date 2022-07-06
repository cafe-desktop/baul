/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Baul is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Andy Hertzfeld <andy@eazel.com>
 */

/*
 * This is the header file for the sidebar title, which is part of the sidebar.
 */

#ifndef BAUL_SIDEBAR_TITLE_H
#define BAUL_SIDEBAR_TITLE_H

#include <ctk/ctk.h>

#include <eel/eel-background.h>

#include <libbaul-private/baul-file.h>

#define BAUL_TYPE_SIDEBAR_TITLE baul_sidebar_title_get_type()
#define BAUL_SIDEBAR_TITLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_SIDEBAR_TITLE, BaulSidebarTitle))
#define BAUL_SIDEBAR_TITLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_SIDEBAR_TITLE, BaulSidebarTitleClass))
#define BAUL_IS_SIDEBAR_TITLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_SIDEBAR_TITLE))
#define BAUL_IS_SIDEBAR_TITLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_SIDEBAR_TITLE))
#define BAUL_SIDEBAR_TITLE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_SIDEBAR_TITLE, BaulSidebarTitleClass))

typedef struct _BaulSidebarTitlePrivate BaulSidebarTitlePrivate;

typedef struct
{
    GtkBox box;
    BaulSidebarTitlePrivate *details;
} BaulSidebarTitle;

typedef struct
{
    GtkBoxClass parent_class;
} BaulSidebarTitleClass;

GType      baul_sidebar_title_get_type          (void);
GtkWidget *baul_sidebar_title_new               (void);
void       baul_sidebar_title_set_file          (BaulSidebarTitle *sidebar_title,
        BaulFile         *file,
        const char           *initial_text);
void       baul_sidebar_title_set_text          (BaulSidebarTitle *sidebar_title,
        const char           *new_title);
char *     baul_sidebar_title_get_text          (BaulSidebarTitle *sidebar_title);
gboolean   baul_sidebar_title_hit_test_icon     (BaulSidebarTitle *sidebar_title,
        int                   x,
        int                   y);
void       baul_sidebar_title_select_text_color (BaulSidebarTitle *sidebar_title,
        					 EelBackground        *background);

#endif /* BAUL_SIDEBAR_TITLE_H */
