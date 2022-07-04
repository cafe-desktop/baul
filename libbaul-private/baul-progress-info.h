/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-progress-info.h: file operation progress info.

   Copyright (C) 2007 Red Hat, Inc.

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

#ifndef BAUL_PROGRESS_INFO_H
#define BAUL_PROGRESS_INFO_H

#include <glib-object.h>
#include <gio/gio.h>

#define BAUL_TYPE_PROGRESS_INFO         (baul_progress_info_get_type ())
#define BAUL_PROGRESS_INFO(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BAUL_TYPE_PROGRESS_INFO, BaulProgressInfo))
#define BAUL_PROGRESS_INFO_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BAUL_TYPE_PROGRESS_INFO, BaulProgressInfoClass))
#define BAUL_IS_PROGRESS_INFO(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BAUL_TYPE_PROGRESS_INFO))
#define BAUL_IS_PROGRESS_INFO_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BAUL_TYPE_PROGRESS_INFO))
#define BAUL_PROGRESS_INFO_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BAUL_TYPE_PROGRESS_INFO, BaulProgressInfoClass))

typedef struct _BaulProgressInfo      BaulProgressInfo;
typedef struct _BaulProgressInfoClass BaulProgressInfoClass;

GType baul_progress_info_get_type (void) G_GNUC_CONST;

/* Signals:
   "changed" - status or details changed
   "progress-changed" - the percentage progress changed (or we pulsed if in activity_mode
   "started" - emited on job start
   "finished" - emitted when job is done

   All signals are emitted from idles in main loop.
   All methods are threadsafe.
 */

BaulProgressInfo *baul_progress_info_new (gboolean should_start, gboolean can_pause);
void baul_progress_info_get_ready (BaulProgressInfo *info);
void baul_progress_info_disable_pause (BaulProgressInfo *info);

GList *       baul_get_all_progress_info (void);

char *        baul_progress_info_get_status      (BaulProgressInfo *info);
char *        baul_progress_info_get_details     (BaulProgressInfo *info);
double        baul_progress_info_get_progress    (BaulProgressInfo *info);
GCancellable *baul_progress_info_get_cancellable (BaulProgressInfo *info);
void          baul_progress_info_cancel          (BaulProgressInfo *info);
gboolean      baul_progress_info_get_is_started  (BaulProgressInfo *info);
gboolean      baul_progress_info_get_is_finished (BaulProgressInfo *info);
gboolean      baul_progress_info_get_is_paused   (BaulProgressInfo *info);

void          baul_progress_info_start           (BaulProgressInfo *info);
void          baul_progress_info_finish          (BaulProgressInfo *info);
void          baul_progress_info_pause           (BaulProgressInfo *info);
void          baul_progress_info_resume          (BaulProgressInfo *info);
void          baul_progress_info_set_status      (BaulProgressInfo *info,
        const char           *status);
void          baul_progress_info_take_status     (BaulProgressInfo *info,
        char                 *status);
void          baul_progress_info_set_details     (BaulProgressInfo *info,
        const char           *details);
void          baul_progress_info_take_details    (BaulProgressInfo *info,
        char                 *details);
void          baul_progress_info_set_progress    (BaulProgressInfo *info,
        double                current,
        double                total);
void          baul_progress_info_pulse_progress  (BaulProgressInfo *info);


#endif /* BAUL_PROGRESS_INFO_H */
