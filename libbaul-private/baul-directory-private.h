/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   baul-directory-private.h: Baul directory model.

   Copyright (C) 1999, 2000, 2001 Eazel, Inc.

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

   Author: Darin Adler <darin@bentspoon.com>
*/

#include <gio/gio.h>
#include <libxml/tree.h>

#include <eel/eel-vfs-extensions.h>

#include <libbaul-extension/baul-info-provider.h>

#include "baul-directory.h"
#include "baul-file-queue.h"
#include "baul-file.h"
#include "baul-monitor.h"

typedef struct LinkInfoReadState LinkInfoReadState;
typedef struct TopLeftTextReadState TopLeftTextReadState;
typedef struct FileMonitors FileMonitors;
typedef struct DirectoryLoadState DirectoryLoadState;
typedef struct DirectoryCountState DirectoryCountState;
typedef struct DeepCountState DeepCountState;
typedef struct GetInfoState GetInfoState;
typedef struct NewFilesState NewFilesState;
typedef struct MimeListState MimeListState;
typedef struct ThumbnailState ThumbnailState;
typedef struct MountState MountState;
typedef struct FilesystemInfoState FilesystemInfoState;

typedef enum
{
    REQUEST_LINK_INFO,
    REQUEST_DEEP_COUNT,
    REQUEST_DIRECTORY_COUNT,
    REQUEST_FILE_INFO,
    REQUEST_FILE_LIST, /* always FALSE if file != NULL */
    REQUEST_MIME_LIST,
    REQUEST_TOP_LEFT_TEXT,
    REQUEST_LARGE_TOP_LEFT_TEXT,
    REQUEST_EXTENSION_INFO,
    REQUEST_THUMBNAIL,
    REQUEST_MOUNT,
    REQUEST_FILESYSTEM_INFO,
    REQUEST_TYPE_LAST
} RequestType;

/* A request for information about one or more files. */
typedef guint32 Request;
typedef gint32 RequestCounter[REQUEST_TYPE_LAST];

#define REQUEST_WANTS_TYPE(request, type) ((request) & (1<<(type)))
#define REQUEST_SET_TYPE(request, type) (request) |= (1<<(type))

struct _BaulDirectoryPrivate
{
    /* The location. */
    GFile *location;

    /* The file objects. */
    BaulFile *as_file;
    GList *file_list;
    GHashTable *file_hash;

    /* Queues of files needing some I/O done. */
    BaulFileQueue *high_priority_queue;
    BaulFileQueue *low_priority_queue;
    BaulFileQueue *extension_queue;

    /* These lists are going to be pretty short.  If we think they
     * are going to get big, we can use hash tables instead.
     */
    GList *call_when_ready_list;
    RequestCounter call_when_ready_counters;
    GList *monitor_list;
    RequestCounter monitor_counters;
    guint call_ready_idle_id;

    BaulMonitor *monitor;
    gulong 		 mime_db_monitor;

    gboolean in_async_service_loop;
    gboolean state_changed;

    gboolean file_list_monitored;
    gboolean directory_loaded;
    gboolean directory_loaded_sent_notification;
    DirectoryLoadState *directory_load_in_progress;

    GList *pending_file_info; /* list of CafeVFSFileInfo's that are pending */
    int confirmed_file_count;
    guint dequeue_pending_idle_id;

    GList *new_files_in_progress; /* list of NewFilesState * */

    DirectoryCountState *count_in_progress;

    BaulFile *deep_count_file;
    DeepCountState *deep_count_in_progress;

    MimeListState *mime_list_in_progress;

    BaulFile *get_info_file;
    GetInfoState *get_info_in_progress;

    BaulFile *extension_info_file;
    BaulInfoProvider *extension_info_provider;
    BaulOperationHandle *extension_info_in_progress;
    guint extension_info_idle;

    ThumbnailState *thumbnail_state;

    MountState *mount_state;

    FilesystemInfoState *filesystem_info_state;

    TopLeftTextReadState *top_left_read_state;

    LinkInfoReadState *link_info_read_state;

    GList *file_operations_in_progress; /* list of FileOperation * */

    guint64 free_space; /* (guint)-1 for unknown */
    time_t free_space_read; /* The time free_space was updated, or 0 for never */
};

BaulDirectory *baul_directory_get_existing                    (GFile                     *location);

/* async. interface */
void               baul_directory_async_state_changed             (BaulDirectory         *directory);
void               baul_directory_call_when_ready_internal        (BaulDirectory         *directory,
        BaulFile              *file,
        BaulFileAttributes     file_attributes,
        gboolean                   wait_for_file_list,
        BaulDirectoryCallback  directory_callback,
        BaulFileCallback       file_callback,
        gpointer                   callback_data);
gboolean           baul_directory_check_if_ready_internal         (BaulDirectory         *directory,
        BaulFile              *file,
        BaulFileAttributes     file_attributes);
void               baul_directory_cancel_callback_internal        (BaulDirectory         *directory,
        BaulFile              *file,
        BaulDirectoryCallback  directory_callback,
        BaulFileCallback       file_callback,
        gpointer                   callback_data);
void               baul_directory_monitor_add_internal            (BaulDirectory         *directory,
        BaulFile              *file,
        gconstpointer              client,
        gboolean                   monitor_hidden_files,
        BaulFileAttributes     attributes,
        BaulDirectoryCallback  callback,
        gpointer                   callback_data);
void               baul_directory_monitor_remove_internal         (BaulDirectory         *directory,
        BaulFile              *file,
        gconstpointer              client);
void               baul_directory_get_info_for_new_files          (BaulDirectory         *directory,
        GList                     *vfs_uris);
BaulFile *     baul_directory_get_existing_corresponding_file (BaulDirectory         *directory);
void               baul_directory_invalidate_count_and_mime_list  (BaulDirectory         *directory);
gboolean           baul_directory_is_file_list_monitored          (BaulDirectory         *directory);
gboolean           baul_directory_is_anyone_monitoring_file_list  (BaulDirectory         *directory);
gboolean           baul_directory_has_active_request_for_file     (BaulDirectory         *directory,
        BaulFile              *file);
void               baul_directory_remove_file_monitor_link        (BaulDirectory         *directory,
        GList                     *link);
void               baul_directory_schedule_dequeue_pending        (BaulDirectory         *directory);
void               baul_directory_stop_monitoring_file_list       (BaulDirectory         *directory);
void               baul_directory_cancel                          (BaulDirectory         *directory);
void               baul_async_destroying_file                     (BaulFile              *file);
void               baul_directory_force_reload_internal           (BaulDirectory         *directory,
        BaulFileAttributes     file_attributes);
void               baul_directory_cancel_loading_file_attributes  (BaulDirectory         *directory,
        BaulFile              *file,
        BaulFileAttributes     file_attributes);

/* Calls shared between directory, file, and async. code. */
void               baul_directory_emit_files_added                (BaulDirectory         *directory,
        GList                     *added_files);
void               baul_directory_emit_files_changed              (BaulDirectory         *directory,
        GList                     *changed_files);
void               baul_directory_emit_change_signals             (BaulDirectory         *directory,
        GList                     *changed_files);
void               emit_change_signals_for_all_files		      (BaulDirectory	 *directory);
void               emit_change_signals_for_all_files_in_all_directories (void);
void               baul_directory_emit_done_loading               (BaulDirectory         *directory);
void               baul_directory_emit_load_error                 (BaulDirectory         *directory,
        GError                    *error);
BaulDirectory *baul_directory_get_internal                    (GFile                     *location,
        gboolean                   create);
char *             baul_directory_get_name_for_self_as_new_file   (BaulDirectory         *directory);
Request            baul_directory_set_up_request                  (BaulFileAttributes     file_attributes);

/* Interface to the file list. */
BaulFile *     baul_directory_find_file_by_name               (BaulDirectory         *directory,
        const char                *filename);

void               baul_directory_add_file                        (BaulDirectory         *directory,
        BaulFile              *file);
void               baul_directory_remove_file                     (BaulDirectory         *directory,
        BaulFile              *file);
FileMonitors *     baul_directory_remove_file_monitors            (BaulDirectory         *directory,
        BaulFile              *file);
void               baul_directory_add_file_monitors               (BaulDirectory         *directory,
        BaulFile              *file,
        FileMonitors              *monitors);
GList *            baul_directory_begin_file_name_change          (BaulDirectory         *directory,
        BaulFile              *file);
void               baul_directory_end_file_name_change            (BaulDirectory         *directory,
        BaulFile              *file,
        GList                     *node);
void               baul_directory_moved                           (const char                *from_uri,
        const char                *to_uri);
/* Interface to the work queue. */

void               baul_directory_add_file_to_work_queue          (BaulDirectory *directory,
        BaulFile *file);
void               baul_directory_remove_file_from_work_queue     (BaulDirectory *directory,
        BaulFile *file);

/* debugging functions */
int                baul_directory_number_outstanding              (void);
