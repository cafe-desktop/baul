/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-clipboard-monitor.h: lets you notice clipboard changes.

   Copyright (C) 2004 Red Hat, Inc.

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

#ifndef BAUL_CLIPBOARD_MONITOR_H
#define BAUL_CLIPBOARD_MONITOR_H

#include <ctk/ctk.h>

#define BAUL_TYPE_CLIPBOARD_MONITOR baul_clipboard_monitor_get_type()
#define BAUL_CLIPBOARD_MONITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_CLIPBOARD_MONITOR, BaulClipboardMonitor))
#define BAUL_CLIPBOARD_MONITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_CLIPBOARD_MONITOR, BaulClipboardMonitorClass))
#define BAUL_IS_CLIPBOARD_MONITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_CLIPBOARD_MONITOR))
#define BAUL_IS_CLIPBOARD_MONITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_CLIPBOARD_MONITOR))
#define BAUL_CLIPBOARD_MONITOR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_CLIPBOARD_MONITOR, BaulClipboardMonitorClass))

typedef struct _BaulClipboardMonitorPrivate BaulClipboardMonitorPrivate;
typedef struct BaulClipboardInfo BaulClipboardInfo;

typedef struct
{
    GObject parent_slot;

    BaulClipboardMonitorPrivate *details;
} BaulClipboardMonitor;

typedef struct
{
    GObjectClass parent_slot;

    void (* clipboard_changed) (BaulClipboardMonitor *monitor);
    void (* clipboard_info) (BaulClipboardMonitor *monitor,
                             BaulClipboardInfo *info);
} BaulClipboardMonitorClass;

struct BaulClipboardInfo
{
    GList *files;
    gboolean cut;
};

GType   baul_clipboard_monitor_get_type (void);

BaulClipboardMonitor *   baul_clipboard_monitor_get (void);
void baul_clipboard_monitor_set_clipboard_info (BaulClipboardMonitor *monitor,
        BaulClipboardInfo *info);
BaulClipboardInfo * baul_clipboard_monitor_get_clipboard_info (BaulClipboardMonitor *monitor);
void baul_clipboard_monitor_emit_changed (void);

void baul_clear_clipboard_callback (GtkClipboard *clipboard,
                                    gpointer      user_data);
void baul_get_clipboard_callback   (GtkClipboard     *clipboard,
                                    GtkSelectionData *selection_data,
                                    guint             info,
                                    gpointer          user_data);



#endif /* BAUL_CLIPBOARD_MONITOR_H */

