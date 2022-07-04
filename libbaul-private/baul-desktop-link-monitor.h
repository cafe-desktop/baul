/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-desktop-link-monitor.h: singleton that manages the desktop links

   Copyright (C) 2003 Red Hat, Inc.

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

   Author: Alexander Larsson <alexl@redhat.com>
*/

#ifndef BAUL_DESKTOP_LINK_MONITOR_H
#define BAUL_DESKTOP_LINK_MONITOR_H

#include <gtk/gtk.h>

#include "baul-desktop-link.h"

#define BAUL_TYPE_DESKTOP_LINK_MONITOR baul_desktop_link_monitor_get_type()
#define BAUL_DESKTOP_LINK_MONITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_DESKTOP_LINK_MONITOR, CajaDesktopLinkMonitor))
#define BAUL_DESKTOP_LINK_MONITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_DESKTOP_LINK_MONITOR, CajaDesktopLinkMonitorClass))
#define BAUL_IS_DESKTOP_LINK_MONITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_DESKTOP_LINK_MONITOR))
#define BAUL_IS_DESKTOP_LINK_MONITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_DESKTOP_LINK_MONITOR))
#define BAUL_DESKTOP_LINK_MONITOR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_DESKTOP_LINK_MONITOR, CajaDesktopLinkMonitorClass))

typedef struct CajaDesktopLinkMonitorDetails CajaDesktopLinkMonitorDetails;

typedef struct
{
    GObject parent_slot;
    CajaDesktopLinkMonitorDetails *details;
} CajaDesktopLinkMonitor;

typedef struct
{
    GObjectClass parent_slot;
} CajaDesktopLinkMonitorClass;

GType   baul_desktop_link_monitor_get_type (void);

CajaDesktopLinkMonitor *   baul_desktop_link_monitor_get (void);

/* Used by baul-desktop-link.c */
char * baul_desktop_link_monitor_make_filename_unique (CajaDesktopLinkMonitor *monitor,
        const char *filename);

#endif /* BAUL_DESKTOP_LINK_MONITOR_H */
