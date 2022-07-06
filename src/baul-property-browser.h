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
 * Baul is distributed in the hope that it will be useful,
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

/* This is the header file for the property browser window, which
 * gives the user access to an extensible palette of properties which
 * can be dropped on various elements of the user interface to
 * customize them
 */

#ifndef BAUL_PROPERTY_BROWSER_H
#define BAUL_PROPERTY_BROWSER_H

#include <gdk/gdk.h>
#include <ctk/ctk.h>

typedef struct BaulPropertyBrowser BaulPropertyBrowser;
typedef struct BaulPropertyBrowserClass  BaulPropertyBrowserClass;

#define BAUL_TYPE_PROPERTY_BROWSER baul_property_browser_get_type()
#define BAUL_PROPERTY_BROWSER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_PROPERTY_BROWSER, BaulPropertyBrowser))
#define BAUL_PROPERTY_BROWSER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_PROPERTY_BROWSER, BaulPropertyBrowserClass))
#define BAUL_IS_PROPERTY_BROWSER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_PROPERTY_BROWSER))
#define BAUL_IS_PROPERTY_BROWSER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_PROPERTY_BROWSER))
#define BAUL_PROPERTY_BROWSER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_PROPERTY_BROWSER, BaulPropertyBrowserClass))

typedef struct _BaulPropertyBrowserPrivate BaulPropertyBrowserPrivate;

struct BaulPropertyBrowser
{
    CtkWindow window;
    BaulPropertyBrowserPrivate *details;
};

struct BaulPropertyBrowserClass
{
    CtkWindowClass parent_class;
};

GType                    baul_property_browser_get_type (void);
BaulPropertyBrowser *baul_property_browser_new      (GdkScreen               *screen);
void                     baul_property_browser_show     (GdkScreen               *screen);
void                     baul_property_browser_set_path (BaulPropertyBrowser *panel,
        const char              *new_path);

#endif /* BAUL_PROPERTY_BROWSER_H */
