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

#include <config.h>
#include <math.h>
#include <string.h>

#include <libnotify/notify.h>
#include <glib/gi18n.h>
#include <ctk/ctk.h>

#include <eel/eel-glib-extensions.h>

#include "baul-progress-info.h"
#include "baul-global-preferences.h"

enum
{
    CHANGED,
    PROGRESS_CHANGED,
    STARTED,
    FINISHED,
    LAST_SIGNAL
};

/* TODO:
 * Want an icon for the operation.
 * Add and implement cancel button
 */

#define SIGNAL_DELAY_MSEC 100

#define STARTBT_DATA_IMAGE_PAUSE "pauseimg"
#define STARTBT_DATA_IMAGE_RESUME "resumeimg"
#define STARTBT_DATA_CURIMAGE "curimage"

static guint signals[LAST_SIGNAL] = { 0 };

struct _ProgressWidgetData;

struct _BaulProgressInfo
{
    GObject parent_instance;

    GCancellable *cancellable;

    struct _ProgressWidgetData *widget;

    char *status;
    char *details;
    double progress;
    gboolean activity_mode;
    gboolean started;
    gboolean finished;
    gboolean paused;

    gboolean can_pause;
    gboolean waiting;
    GCond waiting_c;

    GSource *idle_source;
    gboolean source_is_now;

    gboolean start_at_idle;
    gboolean finish_at_idle;
    gboolean changed_at_idle;
    gboolean progress_at_idle;
};

struct _BaulProgressInfoClass
{
    GObjectClass parent_class;
};

static GList *active_progress_infos = NULL;

static CtkStatusIcon *status_icon = NULL;
static int n_progress_ops = 0;
static void update_status_icon_and_window (void);

G_LOCK_DEFINE_STATIC(progress_info);

G_DEFINE_TYPE (BaulProgressInfo, baul_progress_info, G_TYPE_OBJECT)

GList *
baul_get_all_progress_info (void)
{
    GList *l;

    G_LOCK (progress_info);

    l = g_list_copy_deep (active_progress_infos, (GCopyFunc) g_object_ref, NULL);

    G_UNLOCK (progress_info);

    return l;
}

static void
baul_progress_info_finalize (GObject *object)
{
    BaulProgressInfo *info;

    info = BAUL_PROGRESS_INFO (object);

    g_free (info->status);
    g_free (info->details);
    g_object_unref (info->cancellable);

    if (G_OBJECT_CLASS (baul_progress_info_parent_class)->finalize)
    {
        (*G_OBJECT_CLASS (baul_progress_info_parent_class)->finalize) (object);
    }
}

static void
baul_progress_info_dispose (GObject *object)
{
    BaulProgressInfo *info;

    info = BAUL_PROGRESS_INFO (object);

    G_LOCK (progress_info);

    /* Remove from active list in dispose, since a get_all_progress_info()
       call later could revive the object */
    active_progress_infos = g_list_remove (active_progress_infos, object);

    /* Destroy source in dispose, because the callback
       could come here before the destroy, which should
       ressurect the object for a while */
    if (info->idle_source)
    {
        g_source_destroy (info->idle_source);
        g_source_unref (info->idle_source);
        info->idle_source = NULL;
    }
    G_UNLOCK (progress_info);
}

static void
baul_progress_info_class_init (BaulProgressInfoClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = baul_progress_info_finalize;
    gobject_class->dispose = baul_progress_info_dispose;

    signals[CHANGED] =
        g_signal_new ("changed",
                      BAUL_TYPE_PROGRESS_INFO,
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[PROGRESS_CHANGED] =
        g_signal_new ("progress-changed",
                      BAUL_TYPE_PROGRESS_INFO,
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[STARTED] =
        g_signal_new ("started",
                      BAUL_TYPE_PROGRESS_INFO,
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[FINISHED] =
        g_signal_new ("finished",
                      BAUL_TYPE_PROGRESS_INFO,
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

}

static gboolean
delete_event (CtkWidget   *widget,
	      CdkEventAny *event G_GNUC_UNUSED)
{
    ctk_widget_hide (widget);
    return TRUE;
}

static void
status_icon_activate_cb (CtkStatusIcon *icon G_GNUC_UNUSED,
			 CtkWidget     *progress_window)
{
    if (ctk_widget_get_visible (progress_window))
    {
        ctk_widget_hide (progress_window);
    }
    else
    {
        ctk_window_present (CTK_WINDOW (progress_window));
    }
}

/* Creates a Singleton progress_window */
static CtkWidget *
get_progress_window ()
{
    static CtkWidget *progress_window = NULL;
    CtkWidget *vbox;

    if (progress_window != NULL)
        return progress_window;

    progress_window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
    ctk_window_set_resizable (CTK_WINDOW (progress_window),
                              FALSE);
    ctk_container_set_border_width (CTK_CONTAINER (progress_window), 10);

    ctk_window_set_title (CTK_WINDOW (progress_window),
                          _("File Operations"));

    ctk_window_set_position (CTK_WINDOW (progress_window),
                             CTK_WIN_POS_CENTER);

    ctk_window_set_icon_name (CTK_WINDOW (progress_window),
                              "system-file-manager");

    vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
    ctk_box_set_spacing (CTK_BOX (vbox), 5);

    ctk_container_add (CTK_CONTAINER (progress_window),
                       vbox);

    g_signal_connect (progress_window,
                      "delete_event",
                      (GCallback)delete_event, NULL);

    status_icon = ctk_status_icon_new_from_icon_name ("system-file-manager");
    g_signal_connect (status_icon, "activate",
                      (GCallback)status_icon_activate_cb,
                      progress_window);

    update_status_icon_and_window ();

    return progress_window;
}

typedef enum
{
    STATE_INITIALIZED,
    STATE_RUNNING,
    STATE_PAUSING,
    STATE_PAUSED,
    STATE_QUEUING,
    STATE_QUEUED
} ProgressWidgetState;

static gboolean
is_op_paused (ProgressWidgetState state) {
    return state == STATE_PAUSED || state == STATE_QUEUED;
}

typedef struct _ProgressWidgetData
{
    CtkWidget *widget;
    BaulProgressInfo *info;
    CtkLabel *status;
    CtkLabel *details;
    CtkProgressBar *progress_bar;
    CtkWidget *btstart;
    CtkWidget *btqueue;
    ProgressWidgetState state;
} ProgressWidgetData;

static void
progress_widget_data_free (ProgressWidgetData *data)
{
    g_object_unref (data->info);
    g_free (data);
}

static void
update_data (ProgressWidgetData *data)
{
    char *status, *details, *curstat;
    char *markup;

    status = baul_progress_info_get_status (data->info);

    switch (data->state) {
        case STATE_PAUSED:
            curstat = _("paused");
            break;
        case STATE_PAUSING:
            curstat = _("pausing");
            break;
        case STATE_QUEUED:
            curstat = _("queued");
            break;
        case STATE_QUEUING:
            curstat = _("queuing");
            break;
        default:
            curstat = NULL;
    }

    if (curstat != NULL) {
        char *t;

        t = status;
        status = g_strconcat (status, " \xE2\x80\x94 ", curstat, NULL);
        g_free (t);
    }

    ctk_label_set_text (data->status, status);
    g_free (status);

    details = baul_progress_info_get_details (data->info);
    markup = g_markup_printf_escaped ("<span size='small'>%s</span>", details);
    ctk_label_set_markup (data->details, markup);
    g_free (details);
    g_free (markup);
}

/* You should always check return value */
static CtkWidget *
get_widgets_container ()
{
    CtkWidget * window = get_progress_window ();
    return ctk_bin_get_child (CTK_BIN (window));
}
static void
foreach_get_running_operations (CtkWidget * widget, int * n)
{
    ProgressWidgetData *data = (ProgressWidgetData*) g_object_get_data (
                G_OBJECT(widget), "data");

    if (! is_op_paused (data->state))
        (*n)++;
}

static int
get_running_operations ()
{
    CtkWidget * container = get_widgets_container();
    int n = 0;

    ctk_container_foreach (CTK_CONTAINER(container),
                        (CtkCallback)foreach_get_running_operations, &n);
    return n;
}

static void
foreach_get_queued_widget (CtkWidget * widget, CtkWidget ** out)
{
    if (*out == NULL) {
        ProgressWidgetData *data;

        data = (ProgressWidgetData*) g_object_get_data (
                G_OBJECT(widget), "data");

        if (data->state == STATE_QUEUED || data->state == STATE_QUEUING)
            *out = widget;
    }
}

static CtkWidget *
get_first_queued_widget ()
{
    CtkWidget * container = get_widgets_container();
    CtkWidget * out = NULL;

    ctk_container_foreach (CTK_CONTAINER(container),
                (CtkCallback)foreach_get_queued_widget, &out);
    return out;
}

static void
start_button_update_view (ProgressWidgetData *data)
{
    CtkWidget *toapply, *curimage;
    CtkWidget *button = data->btstart;
    ProgressWidgetState state = data->state;
    gboolean as_pause;

    if (state == STATE_RUNNING || state == STATE_QUEUING) {
        toapply = g_object_get_data (G_OBJECT(button),
                                    STARTBT_DATA_IMAGE_PAUSE);
        atk_object_set_name (ctk_widget_get_accessible (button), _("Pause"));
        ctk_widget_set_tooltip_text (button, _("Pause"));
        as_pause = TRUE;
    } else {
        toapply = g_object_get_data (G_OBJECT(button),
                                    STARTBT_DATA_IMAGE_RESUME);
        atk_object_set_name (ctk_widget_get_accessible (button), _("Resume"));
        ctk_widget_set_tooltip_text (button, _("Resume"));
        as_pause = FALSE;
    }

    curimage = g_object_get_data (G_OBJECT(button), STARTBT_DATA_CURIMAGE);
    if (curimage != toapply) {
        if (curimage != NULL)
            ctk_container_remove (CTK_CONTAINER(button), curimage);

        ctk_container_add (CTK_CONTAINER(button), toapply);
        ctk_widget_show (toapply);
        g_object_set_data (G_OBJECT(button), STARTBT_DATA_CURIMAGE, toapply);
    }

    if (as_pause && !data->info->can_pause)
        ctk_widget_set_sensitive (button, FALSE);
}

static void
queue_button_update_view (ProgressWidgetData *data)
{
    CtkWidget *button = data->btqueue;
    ProgressWidgetState state = data->state;

    if ( (!data->info->can_pause) ||
         (state == STATE_QUEUING || state == STATE_QUEUED) )
        ctk_widget_set_sensitive (button, FALSE);
    else
        ctk_widget_set_sensitive (button, TRUE);
}

static void
progress_info_set_waiting(BaulProgressInfo *info, gboolean waiting)
{
     G_LOCK (progress_info);
     info->waiting = waiting;
     if (! waiting)
        g_cond_signal (&info->waiting_c);
     G_UNLOCK (progress_info);
}

static void
widget_reposition_as_queued (CtkWidget * widget)
{
    ctk_box_reorder_child (CTK_BOX(get_widgets_container ()), widget, n_progress_ops-1);
}

/* Reposition the widget so that it sits right before the first stopped widget */
static void
widget_reposition_as_paused (CtkWidget * widget)
{
    int i;
    GList *children, *child;
    ProgressWidgetData *data = NULL;
    gboolean abort = FALSE;
    CtkWidget * container = get_widgets_container();

    children = ctk_container_get_children (CTK_CONTAINER(container));

    i = 0;
    for (child = children; child && !abort; child = child->next) {
        data = (ProgressWidgetData*) g_object_get_data (
            G_OBJECT(child->data), "data");

        if (child->data != widget && is_op_paused(data->state)) {
            abort = TRUE;
            i--;
        }

        i++;
    }

    i--;
    g_list_free (children);

    ctk_box_reorder_child (CTK_BOX(container),
                widget, i);
}

/* Reposition the widget so that it sits right after the last running widget */
static void
widget_reposition_as_running (CtkWidget * widget)
{
    GList *children, *child;
    ProgressWidgetData *data = NULL;
    gboolean abort = FALSE;
    int i, mypos = -1;
    CtkWidget * container = get_widgets_container();

    children = ctk_container_get_children (CTK_CONTAINER(container));

    i = 0;
    for (child = children; child && !abort; child = child->next) {
        data = (ProgressWidgetData*) g_object_get_data (
            G_OBJECT(child->data), "data");

        if (child->data == widget)
            mypos = i;

        if (is_op_paused (data->state)) {
            abort = TRUE;
        }

        i++;
    }

    i--;
    g_list_free (children);

    if (mypos == -1 || mypos > i) {
        ctk_box_reorder_child (CTK_BOX(container),
                    widget, i);
    }
}

static void update_queue ();

static void
widget_state_transit_to (ProgressWidgetData *data,
                        ProgressWidgetState newstate)
{
    data->state = newstate;

    if (newstate == STATE_PAUSING ||
        newstate == STATE_QUEUING ||
        newstate == STATE_QUEUED) {
       progress_info_set_waiting (data->info, TRUE);
    } else if (newstate != STATE_PAUSED) {
        progress_info_set_waiting (data->info, FALSE);
    }

    if (newstate == STATE_QUEUED) {
        widget_reposition_as_queued (data->widget);
        update_queue ();
    } else if (newstate == STATE_PAUSED) {
        widget_reposition_as_paused (data->widget);
        update_queue ();
    } else if (newstate == STATE_RUNNING) {
        widget_reposition_as_running (data->widget);
    }

    start_button_update_view (data);
    queue_button_update_view (data);
    update_data (data);
}

static void
update_queue ()
{
    if (get_running_operations () == 0) {
        CtkWidget *next;

        next = get_first_queued_widget ();

        if (next != NULL) {
            ProgressWidgetData *data;

            data = (ProgressWidgetData*) g_object_get_data (
                    G_OBJECT(next), "data");
            widget_state_transit_to (data, STATE_RUNNING);
        }
    }
}

static void
update_progress (ProgressWidgetData *data)
{
    double progress;

    progress = baul_progress_info_get_progress (data->info);
    if (progress < 0)
    {
        ctk_progress_bar_pulse (data->progress_bar);
    }
    else
    {
        ctk_progress_bar_set_fraction (data->progress_bar, progress);
    }
}

static void
update_status_icon_and_window (void)
{
    char *tooltip;
    gboolean toshow;
    static gboolean window_shown = FALSE;

    tooltip = g_strdup_printf (ngettext ("%'d file operation active",
                                         "%'d file operations active",
                                         n_progress_ops),
                               n_progress_ops);

    ctk_status_icon_set_tooltip_text (status_icon, tooltip);
    g_free (tooltip);

    toshow = (n_progress_ops > 0);

    if (!toshow)
    {
        ctk_status_icon_set_visible (status_icon, FALSE);

        if (window_shown)
        {
            if (g_settings_get_boolean (baul_preferences, BAUL_PREFERENCES_SHOW_NOTIFICATIONS) &&
                !ctk_window_is_active (CTK_WINDOW (get_progress_window ())))
            {
                NotifyNotification *notification;

                notification = notify_notification_new ("baul",
                                                        _("Process completed"),
                                                        "system-file-manager");

                notify_notification_show (notification, NULL);

                g_object_unref (notification);
            }

            ctk_widget_hide (get_progress_window ());
            window_shown = FALSE;
        }
    }
    else if (toshow && !window_shown)
    {
        ctk_widget_show_all (get_progress_window ());
        ctk_status_icon_set_visible (status_icon, TRUE);
        ctk_window_present (CTK_WINDOW (get_progress_window ()));
        window_shown = TRUE;
    }
}

static void
op_finished (ProgressWidgetData *data)
{
    ctk_widget_destroy (data->widget);

    n_progress_ops--;
    update_queue ();

    update_status_icon_and_window ();
}

static int
do_disable_pause (BaulProgressInfo *info)
{
    info->can_pause = FALSE;

    start_button_update_view (info->widget);
    queue_button_update_view (info->widget);
    return G_SOURCE_REMOVE;
}

void
baul_progress_info_disable_pause (BaulProgressInfo *info)
{
    GSource *source = g_idle_source_new ();
    g_source_set_callback (source, (GSourceFunc)do_disable_pause, info, NULL);
    g_source_attach (source, NULL);
}

static void
cancel_clicked (CtkWidget *button,
                ProgressWidgetData *data)
{
    baul_progress_info_cancel (data->info);
    ctk_widget_set_sensitive (button, FALSE);
    do_disable_pause(data->info);
}

static void
progress_widget_invalid_state (ProgressWidgetData *data G_GNUC_UNUSED)
{
    // TODO give more info: current state, buttons
    g_warning ("Invalid ProgressWidgetState");
}

static int
widget_state_notify_paused_callback (ProgressWidgetData *data)
{
    if (data != NULL) {
        if (data->state == STATE_PAUSING)
            widget_state_transit_to (data, STATE_PAUSED);
        else if (data->state == STATE_QUEUING)
            widget_state_transit_to (data, STATE_QUEUED);
    }
    return G_SOURCE_REMOVE;
}

void
baul_progress_info_get_ready (BaulProgressInfo *info)
{
    if (info->waiting) {
        G_LOCK (progress_info);
        if (info->waiting) {
            // Notify main thread we have stopped and are waiting
            GSource * source = g_idle_source_new ();
            g_source_set_callback (source, (GSourceFunc)widget_state_notify_paused_callback, info->widget, NULL);
            g_source_attach (source, NULL);

            while (info->waiting)
                g_cond_wait (&info->waiting_c, &G_LOCK_NAME(progress_info));
        }
        G_UNLOCK (progress_info);
    }
}

static void
start_clicked (CtkWidget          *startbt G_GNUC_UNUSED,
	       ProgressWidgetData *data)
{
    switch (data->state) {
        case STATE_RUNNING:
        case STATE_QUEUING:
            widget_state_transit_to (data, STATE_PAUSING);
            break;
        case STATE_PAUSING:
        case STATE_PAUSED:
        case STATE_QUEUED:
            widget_state_transit_to (data, STATE_RUNNING);
            break;
        default:
            progress_widget_invalid_state (data);
    }
}

static void
queue_clicked (CtkWidget          *queuebt G_GNUC_UNUSED,
	       ProgressWidgetData *data)
{
    switch (data->state) {
        case STATE_RUNNING:
        case STATE_PAUSING:
            widget_state_transit_to (data, STATE_QUEUING);
            break;
        case STATE_PAUSED:
            widget_state_transit_to (data, STATE_QUEUED);
            break;
        default:
            progress_widget_invalid_state (data);
    }
}

static void
unref_callback (gpointer data)
{
    g_object_unref (data);
}

static void
start_button_init (ProgressWidgetData *data)
{
    CtkWidget *pauseImage, *resumeImage;
    CtkWidget *button = ctk_button_new ();
    data->btstart = button;

    pauseImage = ctk_image_new_from_icon_name (
                "media-playback-pause", CTK_ICON_SIZE_BUTTON);
    resumeImage = ctk_image_new_from_icon_name (
                "media-playback-start", CTK_ICON_SIZE_BUTTON);

    g_object_ref (pauseImage);
    g_object_ref (resumeImage);

    g_object_set_data_full (G_OBJECT(button), STARTBT_DATA_IMAGE_PAUSE,
                            pauseImage, unref_callback);
    g_object_set_data_full (G_OBJECT(button), STARTBT_DATA_IMAGE_RESUME,
                            resumeImage, unref_callback);
    g_object_set_data (G_OBJECT(button), STARTBT_DATA_CURIMAGE, NULL);

    start_button_update_view (data);

    g_signal_connect (button, "clicked", (GCallback)start_clicked, data);
}

static void
queue_button_init (ProgressWidgetData *data)
{
    CtkWidget *button, *image;

    button = ctk_button_new ();
    data->btqueue = button;

    image = ctk_image_new_from_icon_name ("undo", CTK_ICON_SIZE_BUTTON);

    ctk_container_add (CTK_CONTAINER (button), image);
    atk_object_set_name (ctk_widget_get_accessible (button), _("Queue"));
    ctk_widget_set_tooltip_text (button, _("Queue"));

    g_signal_connect (button, "clicked", (GCallback)queue_clicked, data);
}

static CtkWidget *
progress_widget_new (BaulProgressInfo *info)
{
    ProgressWidgetData *data;
    CtkWidget *label, *progress_bar, *hbox, *vbox, *box, *btcancel, *imgcancel;

    data = g_new0 (ProgressWidgetData, 1);
    data->info = g_object_ref (info);
    data->state = STATE_INITIALIZED;

    vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
    ctk_box_set_spacing (CTK_BOX (vbox), 5);


    data->widget = vbox;
    g_object_set_data_full (G_OBJECT (data->widget),
                            "data", data,
                            (GDestroyNotify)progress_widget_data_free);

    label = ctk_label_new ("status");
    ctk_widget_set_size_request (label, 500, -1);
    ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
    ctk_label_set_line_wrap_mode (CTK_LABEL (label), PANGO_WRAP_WORD_CHAR);
    ctk_label_set_xalign (CTK_LABEL (label), 0.0);
    ctk_box_pack_start (CTK_BOX (vbox),
                        label,
                        TRUE, FALSE,
                        0);
    data->status = CTK_LABEL (label);

    hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);

    imgcancel = ctk_image_new_from_icon_name ("process-stop",
                                              CTK_ICON_SIZE_BUTTON);

    btcancel = ctk_button_new ();
    ctk_container_add (CTK_CONTAINER (btcancel), imgcancel);
    atk_object_set_name (ctk_widget_get_accessible (btcancel), _("Cancel"));
    ctk_widget_set_tooltip_text (btcancel, _("Cancel"));
    g_signal_connect (btcancel, "clicked", (GCallback)cancel_clicked, data);

    progress_bar = ctk_progress_bar_new ();
    data->progress_bar = CTK_PROGRESS_BAR (progress_bar);
    ctk_progress_bar_set_pulse_step (data->progress_bar, 0.05);
    box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
    ctk_box_pack_start (CTK_BOX (box),
                       progress_bar,
                       TRUE,FALSE,
                       0);

    start_button_init (data);
    queue_button_init (data);

    ctk_box_pack_start (CTK_BOX (hbox),
                        btcancel,
                        FALSE,FALSE,
                        0);
    ctk_box_pack_start (CTK_BOX (hbox),
                       box,
                       TRUE,TRUE,
                       0);
    ctk_box_pack_start (CTK_BOX (hbox),
                        data->btstart,
                        FALSE,FALSE,
                        0);
    ctk_box_pack_start (CTK_BOX (hbox),
                        data->btqueue,
                        FALSE,FALSE,
                        0);

    ctk_box_pack_start (CTK_BOX (vbox),
                        hbox,
                        FALSE,FALSE,
                        0);

    label = ctk_label_new ("details");
    ctk_label_set_xalign (CTK_LABEL (label), 0.0);
    ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
    ctk_box_pack_start (CTK_BOX (vbox),
                        label,
                        TRUE, FALSE,
                        0);
    data->details = CTK_LABEL (label);

    ctk_widget_show_all (data->widget);

    update_data (data);
    update_progress (data);

    g_signal_connect_swapped (data->info,
                              "changed",
                              (GCallback)update_data, data);
    g_signal_connect_swapped (data->info,
                              "progress_changed",
                              (GCallback)update_progress, data);
    g_signal_connect_swapped (data->info,
                              "finished",
                              (GCallback)op_finished, data);

    info->widget = data;
    return data->widget;
}

static void
handle_new_progress_info (BaulProgressInfo *info)
{
    CtkWidget *window, *progress;

    window = get_progress_window ();

    progress = progress_widget_new (info);
    ctk_box_pack_start (CTK_BOX (ctk_bin_get_child (CTK_BIN (window))),
                        progress,
                        FALSE, FALSE, 6);

    n_progress_ops++;

    if (info->waiting && get_running_operations () > 0)
        widget_state_transit_to (info->widget, STATE_QUEUED);
    else
        widget_state_transit_to (info->widget, STATE_RUNNING);
}

static gboolean
delayed_window_showup (BaulProgressInfo *info)
{
    if (baul_progress_info_get_is_paused (info))
    {
        return TRUE;
    }
    if (!baul_progress_info_get_is_finished (info))
    {
        update_status_icon_and_window ();
    }
    g_object_unref (info);
    return FALSE;
}

static void
new_op_started (BaulProgressInfo *info)
{
    g_signal_handlers_disconnect_by_func (info, (GCallback)new_op_started, NULL);

    if (!baul_progress_info_get_is_finished (info)) {
        handle_new_progress_info (info);

        /* Start the job when no other job is running */
        // TODO use user defined policies
        if (info->waiting) {
            if (get_running_operations () == 0)
                progress_info_set_waiting (info, FALSE);
        }

        g_timeout_add_seconds (2,
                           (GSourceFunc)delayed_window_showup,
                           g_object_ref (info));
    }
}

static void
baul_progress_info_init (BaulProgressInfo *info)
{
    info->cancellable = g_cancellable_new ();

    G_LOCK (progress_info);
    active_progress_infos = g_list_append (active_progress_infos, info);
    G_UNLOCK (progress_info);

    g_signal_connect (info, "started", (GCallback)new_op_started, NULL);
}

BaulProgressInfo *
baul_progress_info_new (gboolean should_start, gboolean can_pause)
{
    BaulProgressInfo *info;

    info = g_object_new (BAUL_TYPE_PROGRESS_INFO, NULL);
    info->waiting = !should_start;
    info->can_pause = can_pause;
    return info;
}

char *
baul_progress_info_get_status (BaulProgressInfo *info)
{
    char *res;

    G_LOCK (progress_info);

    if (info->status)
    {
        res = g_strdup (info->status);
    }
    else
    {
        res = g_strdup (_("Preparing"));
    }

    G_UNLOCK (progress_info);

    return res;
}

char *
baul_progress_info_get_details (BaulProgressInfo *info)
{
    char *res;

    G_LOCK (progress_info);

    if (info->details)
    {
        res = g_strdup (info->details);
    }
    else
    {
        res = g_strdup (_("Preparing"));
    }

    G_UNLOCK (progress_info);

    return res;
}

double
baul_progress_info_get_progress (BaulProgressInfo *info)
{
    double res;

    G_LOCK (progress_info);

    if (info->activity_mode)
    {
        res = -1.0;
    }
    else
    {
        res = info->progress;
    }

    G_UNLOCK (progress_info);

    return res;
}

void
baul_progress_info_cancel (BaulProgressInfo *info)
{
    G_LOCK (progress_info);

    g_cancellable_cancel (info->cancellable);
    info->waiting = FALSE;
    g_cond_signal (&info->waiting_c);

    G_UNLOCK (progress_info);
}

GCancellable *
baul_progress_info_get_cancellable (BaulProgressInfo *info)
{
    GCancellable *c;

    G_LOCK (progress_info);

    c = g_object_ref (info->cancellable);

    G_UNLOCK (progress_info);

    return c;
}

gboolean
baul_progress_info_get_is_started (BaulProgressInfo *info)
{
    gboolean res;

    G_LOCK (progress_info);

    res = info->started;

    G_UNLOCK (progress_info);

    return res;
}

gboolean
baul_progress_info_get_is_finished (BaulProgressInfo *info)
{
    gboolean res;

    G_LOCK (progress_info);

    res = info->finished;

    G_UNLOCK (progress_info);

    return res;
}

gboolean
baul_progress_info_get_is_paused (BaulProgressInfo *info)
{
    gboolean res;

    G_LOCK (progress_info);

    res = info->paused;

    G_UNLOCK (progress_info);

    return res;
}

static gboolean
idle_callback (gpointer data)
{
    BaulProgressInfo *info = data;
    gboolean start_at_idle;
    gboolean finish_at_idle;
    gboolean changed_at_idle;
    gboolean progress_at_idle;
    GSource *source;

    source = g_main_current_source ();

    G_LOCK (progress_info);

    /* Protect agains races where the source has
       been destroyed on another thread while it
       was being dispatched.
       Similar to what cdk_threads_add_idle does.
    */
    if (g_source_is_destroyed (source))
    {
        G_UNLOCK (progress_info);
        return FALSE;
    }

    /* We hadn't destroyed the source, so take a ref.
     * This might ressurect the object from dispose, but
     * that should be ok.
     */
    g_object_ref (info);

    g_assert (source == info->idle_source);

    g_source_unref (source);
    info->idle_source = NULL;

    start_at_idle = info->start_at_idle;
    finish_at_idle = info->finish_at_idle;
    changed_at_idle = info->changed_at_idle;
    progress_at_idle = info->progress_at_idle;

    info->start_at_idle = FALSE;
    info->finish_at_idle = FALSE;
    info->changed_at_idle = FALSE;
    info->progress_at_idle = FALSE;

    G_UNLOCK (progress_info);

    if (start_at_idle)
    {
        g_signal_emit (info,
                       signals[STARTED],
                       0);
    }

    if (changed_at_idle)
    {
        g_signal_emit (info,
                       signals[CHANGED],
                       0);
    }

    if (progress_at_idle)
    {
        g_signal_emit (info,
                       signals[PROGRESS_CHANGED],
                       0);
    }

    if (finish_at_idle)
    {
        g_signal_emit (info,
                       signals[FINISHED],
                       0);
    }

    g_object_unref (info);

    return FALSE;
}

/* Called with lock held */
static void
queue_idle (BaulProgressInfo *info, gboolean now)
{
    if (info->idle_source == NULL ||
            (now && !info->source_is_now))
    {
        if (info->idle_source)
        {
            g_source_destroy (info->idle_source);
            g_source_unref (info->idle_source);
            info->idle_source = NULL;
        }

        info->source_is_now = now;
        if (now)
        {
            info->idle_source = g_idle_source_new ();
        }
        else
        {
            info->idle_source = g_timeout_source_new (SIGNAL_DELAY_MSEC);
        }
        g_source_set_callback (info->idle_source, idle_callback, info, NULL);
        g_source_attach (info->idle_source, NULL);
    }
}

void
baul_progress_info_pause (BaulProgressInfo *info)
{
    G_LOCK (progress_info);

    if (!info->paused)
    {
        info->paused = TRUE;
    }

    G_UNLOCK (progress_info);
}

void
baul_progress_info_resume (BaulProgressInfo *info)
{
    G_LOCK (progress_info);

    if (info->paused)
    {
        info->paused = FALSE;
    }

    G_UNLOCK (progress_info);
}

void
baul_progress_info_start (BaulProgressInfo *info)
{
    G_LOCK (progress_info);

    if (!info->started)
    {
        info->started = TRUE;

        info->start_at_idle = TRUE;
        queue_idle (info, TRUE);
    }

    G_UNLOCK (progress_info);
}

void
baul_progress_info_finish (BaulProgressInfo *info)
{
    G_LOCK (progress_info);

    if (!info->finished)
    {
        info->finished = TRUE;

        info->finish_at_idle = TRUE;
        queue_idle (info, TRUE);
    }

    G_UNLOCK (progress_info);
}

void
baul_progress_info_take_status (BaulProgressInfo *info,
                                char *status)
{
    G_LOCK (progress_info);

    if (g_strcmp0 (info->status, status) != 0)
    {
        g_free (info->status);
        info->status = status;

        info->changed_at_idle = TRUE;
        queue_idle (info, FALSE);
    }
    else
    {
        g_free (status);
    }

    G_UNLOCK (progress_info);
}

void
baul_progress_info_set_status (BaulProgressInfo *info,
                               const char *status)
{
    G_LOCK (progress_info);

    if (g_strcmp0 (info->status, status) != 0)
    {
        g_free (info->status);
        info->status = g_strdup (status);

        info->changed_at_idle = TRUE;
        queue_idle (info, FALSE);
    }

    G_UNLOCK (progress_info);
}


void
baul_progress_info_take_details (BaulProgressInfo *info,
                                 char           *details)
{
    G_LOCK (progress_info);

    if (g_strcmp0 (info->details, details) != 0)
    {
        g_free (info->details);
        info->details = details;

        info->changed_at_idle = TRUE;
        queue_idle (info, FALSE);
    }
    else
    {
        g_free (details);
    }

    G_UNLOCK (progress_info);
}

void
baul_progress_info_set_details (BaulProgressInfo *info,
                                const char           *details)
{
    G_LOCK (progress_info);

    if (g_strcmp0 (info->details, details) != 0)
    {
        g_free (info->details);
        info->details = g_strdup (details);

        info->changed_at_idle = TRUE;
        queue_idle (info, FALSE);
    }

    G_UNLOCK (progress_info);
}

void
baul_progress_info_pulse_progress (BaulProgressInfo *info)
{
    G_LOCK (progress_info);

    info->activity_mode = TRUE;
    info->progress = 0.0;
    info->progress_at_idle = TRUE;
    queue_idle (info, FALSE);

    G_UNLOCK (progress_info);
}

void
baul_progress_info_set_progress (BaulProgressInfo *info,
                                 double                current,
                                 double                total)
{
    double current_percent;

    if (total <= 0)
    {
        current_percent = 1.0;
    }
    else
    {
        current_percent = current / total;

        if (current_percent < 0)
        {
            current_percent = 0;
        }

        if (current_percent > 1.0)
        {
            current_percent = 1.0;
        }
    }

    G_LOCK (progress_info);

    if (info->activity_mode || /* emit on switch from activity mode */
            fabs (current_percent - info->progress) > 0.005 /* Emit on change of 0.5 percent */
       )
    {
        info->activity_mode = FALSE;
        info->progress = current_percent;
        info->progress_at_idle = TRUE;
        queue_idle (info, FALSE);
    }

    G_UNLOCK (progress_info);
}
