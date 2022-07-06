/* vi: set sw=4 ts=4 wrap ai: */
/*
 * baul-widget-view-provider.h: This file is part of baul.
 *
 * Copyright (C) 2019 Wu Xiaotian <yetist@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * */

#ifndef __BAUL_WIDGET_VIEW_PROVIDER_H__
#define __BAUL_WIDGET_VIEW_PROVIDER_H__  1

#include <glib-object.h>
#include <ctk/ctk.h>
#include "baul-file-info.h"
#include "baul-extension-types.h"

G_BEGIN_DECLS

#define BAUL_TYPE_WIDGET_VIEW_PROVIDER           (baul_widget_view_provider_get_type ())
#define BAUL_WIDGET_VIEW_PROVIDER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_WIDGET_VIEW_PROVIDER, BaulWidgetViewProvider))
#define BAUL_IS_WIDGET_VIEW_PROVIDER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_WIDGET_VIEW_PROVIDER))
#define BAUL_WIDGET_VIEW_PROVIDER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAUL_TYPE_WIDGET_VIEW_PROVIDER, BaulWidgetViewProviderIface))

typedef struct _BaulWidgetViewProvider       BaulWidgetViewProvider;
typedef struct _BaulWidgetViewProviderIface  BaulWidgetViewProviderIface;

/**
 * BaulWidgetViewProviderIface:
 * @supports_uri: Whether this extension works for this uri
 * @get_widget: Returns a #GtkWidget.
 *   See baul_widget_view_provider_get_widget() for details.
 * @add_file: Adds a file to this widget view.
 * @set_location: Set location to this widget view.
 * @set_window: Set the main window to this widget view.
 * @get_item_count: Return the item count of this widget view.
 * @get_first_visible_file: Return the first visible file from this widget view.
 * @clear: Clear items in this widget view.
 *
 * Interface for extensions to provide widgets view for content.
 */
struct _BaulWidgetViewProviderIface {
    GTypeInterface g_iface;

    gboolean  (*supports_uri)   (BaulWidgetViewProvider *provider,
                                 const char *uri,
                                 GFileType file_type,
                                 const char *mime_type);
    GtkWidget* (*get_widget)     (BaulWidgetViewProvider *provider);
    void       (*add_file)       (BaulWidgetViewProvider *provider, BaulFile *file, BaulFile *directory);
    void       (*set_location)   (BaulWidgetViewProvider *provider, const char *location);
    void       (*set_window)     (BaulWidgetViewProvider *provider, GtkWindow *window);
    guint      (*get_item_count) (BaulWidgetViewProvider *provider);
    gchar*     (*get_first_visible_file) (BaulWidgetViewProvider *provider);
    void       (*clear)          (BaulWidgetViewProvider *provider);
};

/* Interface Functions */
GType      baul_widget_view_provider_get_type       (void);

GtkWidget *baul_widget_view_provider_get_widget     (BaulWidgetViewProvider *provider);
void       baul_widget_view_provider_add_file       (BaulWidgetViewProvider *provider,
                                                     BaulFile *file,
                                                     BaulFile *directory);
void       baul_widget_view_provider_set_location   (BaulWidgetViewProvider *provider,
                                                     const char *location);
void       baul_widget_view_provider_set_window     (BaulWidgetViewProvider *provider,
                                                     GtkWindow *window);
guint      baul_widget_view_provider_get_item_count (BaulWidgetViewProvider *provider);
gchar*     baul_widget_view_provider_get_first_visible_file (BaulWidgetViewProvider *provider);
void       baul_widget_view_provider_clear          (BaulWidgetViewProvider *provider);
gboolean   baul_widget_view_provider_supports_uri   (BaulWidgetViewProvider *provider,
                                                     const char *uri,
                                                     GFileType file_type,
                                                     const char *mime_type);
G_END_DECLS

#endif /* __BAUL_WIDGET_VIEW_PROVIDER_H__ */
