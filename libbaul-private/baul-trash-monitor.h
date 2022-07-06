/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
   baul-trash-monitor.h: Baul trash state watcher.

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

   Author: Pavel Cisler <pavel@eazel.com>
*/

#ifndef BAUL_TRASH_MONITOR_H
#define BAUL_TRASH_MONITOR_H

#include <ctk/ctk.h>
#include <gio/gio.h>

typedef struct BaulTrashMonitor BaulTrashMonitor;
typedef struct BaulTrashMonitorClass BaulTrashMonitorClass;
typedef struct _BaulTrashMonitorPrivate BaulTrashMonitorPrivate;

#define BAUL_TYPE_TRASH_MONITOR baul_trash_monitor_get_type()
#define BAUL_TRASH_MONITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_TRASH_MONITOR, BaulTrashMonitor))
#define BAUL_TRASH_MONITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_TRASH_MONITOR, BaulTrashMonitorClass))
#define BAUL_IS_TRASH_MONITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_TRASH_MONITOR))
#define BAUL_IS_TRASH_MONITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_TRASH_MONITOR))
#define BAUL_TRASH_MONITOR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_TRASH_MONITOR, BaulTrashMonitorClass))

struct BaulTrashMonitor
{
    GObject object;
    BaulTrashMonitorPrivate *details;
};

struct BaulTrashMonitorClass
{
    GObjectClass parent_class;

    void (* trash_state_changed)		(BaulTrashMonitor 	*trash_monitor,
                                         gboolean 		 new_state);
};

GType			baul_trash_monitor_get_type				(void);

BaulTrashMonitor   *baul_trash_monitor_get 				(void);
gboolean		baul_trash_monitor_is_empty 			(void);
GIcon                  *baul_trash_monitor_get_icon                         (void);

#endif
