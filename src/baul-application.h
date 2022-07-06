/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2000 Red Hat, Inc.
 *
 * Baul is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Baul is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/* baul-application.h
 */

#ifndef BAUL_APPLICATION_H
#define BAUL_APPLICATION_H

#include <config.h>
#include <gdk/gdk.h>
#include <gio/gio.h>
#include <ctk/ctk.h>

#include <libegg/eggsmclient.h>

#define BAUL_DESKTOP_ICON_VIEW_IID "OAFIID:Baul_File_Manager_Desktop_Icon_View"

#define BAUL_TYPE_APPLICATION \
	baul_application_get_type()
#define BAUL_APPLICATION(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), BAUL_TYPE_APPLICATION, BaulApplication))
#define BAUL_APPLICATION_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), BAUL_TYPE_APPLICATION, BaulApplicationClass))
#define BAUL_IS_APPLICATION(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), BAUL_TYPE_APPLICATION))
#define BAUL_IS_APPLICATION_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), BAUL_TYPE_APPLICATION))
#define BAUL_APPLICATION_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), BAUL_TYPE_APPLICATION, BaulApplicationClass))

#ifndef BAUL_WINDOW_DEFINED
#define BAUL_WINDOW_DEFINED
typedef struct BaulWindow BaulWindow;
#endif

#ifndef BAUL_SPATIAL_WINDOW_DEFINED
#define BAUL_SPATIAL_WINDOW_DEFINED
typedef struct _BaulSpatialWindow BaulSpatialWindow;
#endif

typedef struct _BaulApplicationPrivate BaulApplicationPrivate;

typedef struct
{
    CtkApplication parent;
    BaulApplicationPrivate *priv;

    EggSMClient* smclient;
    GVolumeMonitor* volume_monitor;
    unsigned int automount_idle_id;
    gboolean screensaver_active;
    guint ss_watch_id;
    GDBusProxy *ss_proxy;
    GList *volume_queue;
} BaulApplication;

typedef struct
{
	CtkApplicationClass parent_class;
} BaulApplicationClass;

GType baul_application_get_type (void);

BaulApplication *baul_application_new (void);

BaulWindow *     baul_application_get_spatial_window     (BaulApplication *application,
        BaulWindow      *requesting_window,
        const char      *startup_id,
        GFile           *location,
        GdkScreen       *screen,
        gboolean        *existing);

BaulWindow *     baul_application_create_navigation_window     (BaulApplication *application,
        GdkScreen           *screen);
void baul_application_close_all_navigation_windows (BaulApplication *self);
void baul_application_close_parent_windows     (BaulSpatialWindow *window);
void baul_application_close_all_spatial_windows  (void);

void baul_application_open_location (BaulApplication *application,
        GFile *location,
        GFile *selection,
        const char *startup_id,
        const gboolean open_in_tabs);

#endif /* BAUL_APPLICATION_H */
