/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2003 Red Hat, Inc.
 * Copyright (C) 2010 Cosimo Cecchi <cosimoc@gnome.org>
 *
 * Baul is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Baul is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <ctk/ctk.h>

#include <eel/eel-stock-dialogs.h>

#include <libbaul-private/baul-global-preferences.h>
#include <libbaul-private/baul-icon-names.h>

#include "baul-connect-server-dialog.h"
#include "baul-application.h"
#include "baul-bookmark-list.h"
#include "baul-connect-server-operation.h"
#include "baul-window.h"

/* TODO:
 * - name entry + pre-fill
 * - NetworkManager integration
 */

struct _BaulConnectServerDialogPrivate
{
    BaulApplication *application;

    CtkWidget *primary_grid;
    CtkWidget *user_details;
    CtkWidget *port_spinbutton;

    CtkWidget *info_bar;
    CtkWidget *info_bar_content;

    CtkWidget *type_combo;
    CtkWidget *server_entry;
    CtkWidget *share_entry;
    CtkWidget *folder_entry;
    CtkWidget *domain_entry;
    CtkWidget *user_entry;
    CtkWidget *password_entry;
    CtkWidget *remember_checkbox;
    CtkWidget *connect_button;
    CtkWidget *bookmark_checkbox;
    CtkWidget *name_entry;

    GList *iconized_entries;

    GSimpleAsyncResult *fill_details_res;
    GAskPasswordFlags fill_details_flags;
    GMountOperation *fill_operation;

    gboolean last_password_set;
    gulong password_sensitive_id;
    gboolean should_destroy;
};

G_DEFINE_TYPE_WITH_PRIVATE (BaulConnectServerDialog, baul_connect_server_dialog,
	       CTK_TYPE_DIALOG)

static void sensitive_entry_changed_callback (CtkEditable *editable,
					      CtkWidget *widget);
static void iconized_entry_changed_cb (CtkEditable *entry,
				       BaulConnectServerDialog *dialog);

enum
{
    RESPONSE_CONNECT
};

struct MethodInfo
{
    const char *scheme;
    guint flags;
    guint default_port;
};

/* A collection of flags for MethodInfo.flags */
enum
{
    DEFAULT_METHOD = (1 << 0),

	/* Widgets to display in connect_dialog_setup_for_type */
    SHOW_SHARE     = (1 << 1),
    SHOW_PORT      = (1 << 2),
    SHOW_USER      = (1 << 3),
    SHOW_DOMAIN    = (1 << 4),

    IS_ANONYMOUS   = (1 << 5)
};

/* Remember to fill in descriptions below */
static struct MethodInfo methods[] =
{
    { "afp", SHOW_SHARE | SHOW_USER, 548 },
    /* FIXME: we need to alias ssh to sftp */
    { "sftp",  SHOW_PORT | SHOW_USER, 22 },
    { "ftp",  SHOW_PORT | SHOW_USER, 21 },
    { "ftp",  DEFAULT_METHOD | IS_ANONYMOUS | SHOW_PORT, 21 },
    { "smb",  SHOW_SHARE | SHOW_USER | SHOW_DOMAIN, 0 },
    { "dav",  SHOW_PORT | SHOW_USER, 80 },
    /* FIXME: hrm, shouldn't it work? */
    { "davs", SHOW_PORT | SHOW_USER, 443 },
};

/* To get around non constant gettext strings */
static const char*
get_method_description (struct MethodInfo *meth)
{
    if (strcmp (meth->scheme, "sftp") == 0) {
        return _("SSH");
    } else if (strcmp (meth->scheme, "ftp") == 0) {
        if (meth->flags & IS_ANONYMOUS) {
            return _("Public FTP");
        } else {
            return _("FTP (with login)");
        }
    } else if (strcmp (meth->scheme, "smb") == 0) {
        return _("Windows share");
    } else if (strcmp (meth->scheme, "dav") == 0) {
        return _("WebDAV (HTTP)");
    } else if (strcmp (meth->scheme, "davs") == 0) {
        return _("Secure WebDAV (HTTPS)");

    } else if (strcmp (meth->scheme, "afp") == 0) {
        return _("Apple Filing Protocol (AFP)");
    } else {
        /* No descriptive text */
        return meth->scheme;
    }
}

static void
connect_dialog_restore_info_bar (BaulConnectServerDialog *dialog,
				 CtkMessageType message_type)
{
	if (dialog->details->info_bar_content != NULL) {
		ctk_widget_destroy (dialog->details->info_bar_content);
		dialog->details->info_bar_content = NULL;
	}

	ctk_info_bar_set_message_type (CTK_INFO_BAR (dialog->details->info_bar),
				       message_type);
}

static void
connect_dialog_set_connecting (BaulConnectServerDialog *dialog)
{
	CtkWidget *hbox;
	CtkWidget *widget;
	CtkWidget *content_area;
	gint width, height;

	connect_dialog_restore_info_bar (dialog, CTK_MESSAGE_INFO);
	ctk_widget_show (dialog->details->info_bar);

	content_area = ctk_info_bar_get_content_area (CTK_INFO_BAR (dialog->details->info_bar));

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
	ctk_container_add (CTK_CONTAINER (content_area), hbox);
	ctk_widget_show (hbox);

	dialog->details->info_bar_content = hbox;

	widget = ctk_spinner_new ();
	ctk_icon_size_lookup (CTK_ICON_SIZE_SMALL_TOOLBAR, &width, &height);
	ctk_widget_set_size_request (widget, width, height);
	ctk_spinner_start (CTK_SPINNER (widget));
	ctk_box_pack_start (CTK_BOX (hbox), widget, FALSE, FALSE, 6);
	ctk_widget_show (widget);

	widget = ctk_label_new (_("Connecting..."));
	ctk_box_pack_start (CTK_BOX (hbox), widget, FALSE, FALSE, 6);
	ctk_widget_show (widget);

	ctk_widget_set_sensitive (dialog->details->connect_button, FALSE);
}

static void
connect_dialog_gvfs_error (BaulConnectServerDialog *dialog)
{
	CtkWidget *hbox, *image, *content_area, *label;

	connect_dialog_restore_info_bar (dialog, CTK_MESSAGE_ERROR);

	content_area = ctk_info_bar_get_content_area (CTK_INFO_BAR (dialog->details->info_bar));

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
	ctk_container_add (CTK_CONTAINER (content_area), hbox);
	ctk_widget_show (hbox);

	image = ctk_image_new_from_icon_name ("dialog-error", CTK_ICON_SIZE_SMALL_TOOLBAR);
	ctk_box_pack_start (CTK_BOX (hbox), image, FALSE, FALSE, 6);
	ctk_widget_show (image);

	label = ctk_label_new (_("Can't load the supported server method list.\n"
				 "Please check your GVfs installation."));
	ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 6);
	ctk_widget_show (label);

	ctk_widget_set_sensitive (dialog->details->connect_button, FALSE);
	ctk_widget_set_sensitive (dialog->details->primary_grid, FALSE);

	ctk_widget_show (dialog->details->info_bar);
}

static void
iconized_entry_restore (gpointer data,
			gpointer user_data)
{
	CtkEntry *entry;
	BaulConnectServerDialog *dialog;

	entry = data;
	dialog = user_data;

	ctk_entry_set_icon_from_icon_name (CTK_ENTRY (entry),
				       CTK_ENTRY_ICON_SECONDARY,
				       NULL);

	g_signal_handlers_disconnect_by_func (entry,
					      iconized_entry_changed_cb,
					      dialog);
}

static void
iconized_entry_changed_cb (CtkEditable *entry,
			   BaulConnectServerDialog *dialog)
{
	dialog->details->iconized_entries =
		g_list_remove (dialog->details->iconized_entries, entry);

	iconized_entry_restore (entry, dialog);
}

static void
iconize_entry (BaulConnectServerDialog *dialog,
	       CtkWidget *entry)
{
	if (!g_list_find (dialog->details->iconized_entries, entry)) {
		dialog->details->iconized_entries =
			g_list_prepend (dialog->details->iconized_entries, entry);

		ctk_entry_set_icon_from_icon_name (CTK_ENTRY (entry),
					       CTK_ENTRY_ICON_SECONDARY,
					       "dialog-warning");

		ctk_widget_grab_focus (entry);

		g_signal_connect (entry, "changed",
				  G_CALLBACK (iconized_entry_changed_cb), dialog);
	}
}

static void
connect_dialog_set_info_bar_error (BaulConnectServerDialog *dialog,
				   GError *error)
{
	CtkWidget *content_area, *label, *entry, *hbox, *icon;
	gchar *str;
	const gchar *folder, *server;

	connect_dialog_restore_info_bar (dialog, CTK_MESSAGE_WARNING);

	content_area = ctk_info_bar_get_content_area (CTK_INFO_BAR (dialog->details->info_bar));
	entry = NULL;

	switch (error->code) {
	case G_IO_ERROR_FAILED_HANDLED:
		return;
	case G_IO_ERROR_NOT_FOUND:
		folder = ctk_entry_get_text (CTK_ENTRY (dialog->details->folder_entry));
		server = ctk_entry_get_text (CTK_ENTRY (dialog->details->server_entry));
		str = g_strdup_printf (_("The folder \"%s\" cannot be opened on \"%s\"."),
				       folder, server);
		label = ctk_label_new (str);
		entry = dialog->details->folder_entry;

		g_free (str);

		break;
	case G_IO_ERROR_HOST_NOT_FOUND:
		server = ctk_entry_get_text (CTK_ENTRY (dialog->details->server_entry));
		str = g_strdup_printf (_("The server at \"%s\" cannot be found."), server);
		label = ctk_label_new (str);
		entry = dialog->details->server_entry;

		g_free (str);

		break;
	case G_IO_ERROR_FAILED:
	default:
		label = ctk_label_new (error->message);
		break;
	}

	ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
	ctk_widget_show (dialog->details->info_bar);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
	ctk_box_pack_start (CTK_BOX (content_area), hbox, FALSE, FALSE, 6);
	ctk_widget_show (hbox);

	icon = ctk_image_new_from_icon_name ("dialog-warning",
					 CTK_ICON_SIZE_SMALL_TOOLBAR);
	ctk_box_pack_start (CTK_BOX (hbox), icon, FALSE, FALSE, 6);
	ctk_widget_show (icon);

	ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 6);
	ctk_widget_show (label);

	if (entry != NULL) {
		iconize_entry (dialog, entry);
	}

	dialog->details->info_bar_content = hbox;

	ctk_button_set_label (CTK_BUTTON (dialog->details->connect_button),
			      _("Try Again"));
	ctk_widget_set_sensitive (dialog->details->connect_button, TRUE);
}

static void
connect_dialog_finish_fill (BaulConnectServerDialog *dialog)
{
	GAskPasswordFlags flags;
	GMountOperation *op;

	flags = dialog->details->fill_details_flags;
	op = G_MOUNT_OPERATION (dialog->details->fill_operation);

	if (flags & G_ASK_PASSWORD_NEED_PASSWORD) {
		g_mount_operation_set_password (op, ctk_entry_get_text (CTK_ENTRY (dialog->details->password_entry)));
	}

	if (flags & G_ASK_PASSWORD_NEED_USERNAME) {
		g_mount_operation_set_username (op, ctk_entry_get_text (CTK_ENTRY (dialog->details->user_entry)));
	}

	if (flags & G_ASK_PASSWORD_NEED_DOMAIN) {
		g_mount_operation_set_domain (op, ctk_entry_get_text (CTK_ENTRY (dialog->details->domain_entry)));
	}

	if (flags & G_ASK_PASSWORD_SAVING_SUPPORTED &&
	    ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (dialog->details->remember_checkbox))) {
		g_mount_operation_set_password_save (op, G_PASSWORD_SAVE_PERMANENTLY);
	}

	connect_dialog_set_connecting (dialog);

	g_simple_async_result_set_op_res_gboolean (dialog->details->fill_details_res, TRUE);
	g_simple_async_result_complete (dialog->details->fill_details_res);

	g_object_unref (dialog->details->fill_details_res);
	dialog->details->fill_details_res = NULL;

	g_object_unref (dialog->details->fill_operation);
	dialog->details->fill_operation = NULL;
}

static void
connect_dialog_request_additional_details (BaulConnectServerDialog *self,
					   GAskPasswordFlags flags,
					   const gchar *default_user,
					   const gchar *default_domain)
{
	CtkWidget *content_area, *label, *hbox, *icon;

	self->details->fill_details_flags = flags;

	connect_dialog_restore_info_bar (self, CTK_MESSAGE_WARNING);

	content_area = ctk_info_bar_get_content_area (CTK_INFO_BAR (self->details->info_bar));

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
	ctk_box_pack_start (CTK_BOX (content_area), hbox, FALSE, FALSE, 6);
	ctk_widget_show (hbox);

	icon = ctk_image_new_from_icon_name ("dialog-warning",
					 CTK_ICON_SIZE_SMALL_TOOLBAR);
	ctk_box_pack_start (CTK_BOX (hbox), icon, FALSE, FALSE, 6);
	ctk_widget_show (icon);

	label = ctk_label_new (_("Please verify your user details."));
	ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 6);
	ctk_widget_show (label);

	if (flags & G_ASK_PASSWORD_NEED_PASSWORD) {
		iconize_entry (self, self->details->password_entry);
	}

	if (flags & G_ASK_PASSWORD_NEED_USERNAME) {
		if (default_user != NULL && g_strcmp0 (default_user, "") != 0) {
			ctk_entry_set_text (CTK_ENTRY (self->details->user_entry),
					    default_user);
		} else {
			iconize_entry (self, self->details->user_entry);
		}
	}

	if (flags & G_ASK_PASSWORD_NEED_DOMAIN) {
		if (default_domain != NULL && g_strcmp0 (default_domain, "") != 0) {
			ctk_entry_set_text (CTK_ENTRY (self->details->domain_entry),
					    default_domain);
		} else {
			iconize_entry (self, self->details->domain_entry);
		}
	}

	self->details->info_bar_content = hbox;

	ctk_widget_set_sensitive (self->details->connect_button, TRUE);
	ctk_button_set_label (CTK_BUTTON (self->details->connect_button),
			      _("Continue"));

	if (!(flags & G_ASK_PASSWORD_SAVING_SUPPORTED)) {
		g_signal_handler_disconnect (self->details->password_entry,
					     self->details->password_sensitive_id);
		self->details->password_sensitive_id = 0;

		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (self->details->remember_checkbox),
					      FALSE);
		ctk_widget_set_sensitive (self->details->remember_checkbox, FALSE);
	}
}

static void
display_location_async_cb (GObject *source,
			   GAsyncResult *res,
			   gpointer user_data)
{
	BaulConnectServerDialog *dialog;
	GError *error;

	dialog = BAUL_CONNECT_SERVER_DIALOG (source);
	error = NULL;

	baul_connect_server_dialog_display_location_finish (dialog,
								res, &error);

	if (error != NULL) {
		connect_dialog_set_info_bar_error (dialog, error);
		g_error_free (error);
	} else {
		ctk_widget_destroy (CTK_WIDGET (dialog));
	}
}

static void
mount_enclosing_ready_cb (GObject *source,
			  GAsyncResult *res,
			  gpointer user_data)
{
	GFile *location;
	BaulConnectServerDialog *dialog;
	GError *error;

	error = NULL;
	location = G_FILE (source);
	dialog = user_data;

	g_file_mount_enclosing_volume_finish (location, res, &error);

	if (!error || g_error_matches (error, G_IO_ERROR, G_IO_ERROR_ALREADY_MOUNTED)) {
		/* volume is mounted, show it */
		baul_connect_server_dialog_display_location_async (dialog,
								       dialog->details->application, location,
								       display_location_async_cb, NULL);
	} else {
		if (dialog->details->should_destroy) {
			ctk_widget_destroy (CTK_WIDGET (dialog));
		} else {
			connect_dialog_set_info_bar_error (dialog, error);
		}
	}

	if (error != NULL) {
		g_error_free (error);
	}
}

static void
connect_dialog_present_uri_async (BaulConnectServerDialog *self,
				  BaulApplication *application,
				  GFile *location)
{
	GMountOperation *op;

	op = baul_connect_server_operation_new (self);
	g_file_mount_enclosing_volume (location,
				       0, op, NULL,
				       mount_enclosing_ready_cb, self);
	g_object_unref (op);
}

static void
connect_dialog_connect_to_server (BaulConnectServerDialog *dialog)
{
    struct MethodInfo *meth;
    GFile *location;
    int index;
    CtkTreeIter iter;
    char *user, *initial_path, *server, *folder, *domain, *port_str;
    char *t, *join, *uri;
    double port;

    /* Get our method info */
    ctk_combo_box_get_active_iter (CTK_COMBO_BOX (dialog->details->type_combo), &iter);
    ctk_tree_model_get (ctk_combo_box_get_model (CTK_COMBO_BOX (dialog->details->type_combo)),
                        &iter, 0, &index, -1);
    g_assert (index < G_N_ELEMENTS (methods) && index >= 0);
    meth = &(methods[index]);

    server = ctk_editable_get_chars (CTK_EDITABLE (dialog->details->server_entry), 0, -1);

    user = NULL;
	initial_path = g_strdup ("");
    domain = NULL;
    folder = NULL;

    if (meth->flags & IS_ANONYMOUS) {
        /* FTP special case */
    	user = g_strdup ("anonymous");

    } else if ((strcmp (meth->scheme, "smb") == 0) ||
		(strcmp (meth->scheme, "afp") == 0)){
		/* SMB/AFP special case */
		g_free (initial_path);

    	t = ctk_editable_get_chars (CTK_EDITABLE (dialog->details->share_entry), 0, -1);
    	initial_path = g_strconcat ("/", t, NULL);

        g_free (t);
    }

    /* username */
    if (!user) {
    	t = ctk_editable_get_chars (CTK_EDITABLE (dialog->details->user_entry), 0, -1);
    	user = g_uri_escape_string (t, G_URI_RESERVED_CHARS_ALLOWED_IN_USERINFO, FALSE);
    	g_free (t);
    }

    /* domain */
    domain = ctk_editable_get_chars (CTK_EDITABLE (dialog->details->domain_entry), 0, -1);

    if (strlen (domain) != 0) {
    	t = user;

    	user = g_strconcat (domain , ";" , t, NULL);
    	g_free (t);
    }

    /* folder */
    folder = ctk_editable_get_chars (CTK_EDITABLE (dialog->details->folder_entry), 0, -1);

    if (folder[0] != 0 &&
        folder[0] != '/') {
    	join = "/";
    } else {
    	join = "";
    }

	t = folder;
	folder = g_strconcat (initial_path, join, t, NULL);
	g_free (t);

    t = folder;
    folder = g_uri_escape_string (t, G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
    g_free (t);

    /* port */
	port = ctk_spin_button_get_value (CTK_SPIN_BUTTON (dialog->details->port_spinbutton));

    if (port != 0 && port != meth->default_port) {
    	port_str = g_strdup_printf ("%d", (int) port);
    } else {
    	port_str = NULL;
    }

    /* final uri */
    uri = g_strdup_printf ("%s://%s%s%s%s%s%s",
    		       meth->scheme,
    		       (user != NULL) ? user : "",
		       (user != NULL && user[0] != 0) ? "@" : "",
    		       server,
    		       (port_str != NULL) ? ":" : "",
    		       (port_str != NULL) ? port_str : "",
    		       (folder != NULL) ? folder : "");

    g_free (initial_path);
    g_free (server);
    g_free (folder);
    g_free (user);
    g_free (domain);
    g_free (port_str);

    location = g_file_new_for_uri (uri);
    g_free (uri);

    /* add to bookmarks */
    if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (dialog->details->bookmark_checkbox)))
    {
        char *name;
        BaulBookmark *bookmark;
        BaulBookmarkList *list;
        GIcon *icon;

        name = ctk_editable_get_chars (CTK_EDITABLE (dialog->details->name_entry), 0, -1);
        icon = g_themed_icon_new (BAUL_ICON_FOLDER_REMOTE);
        bookmark = baul_bookmark_new (location, strlen (name) ? name : NULL, TRUE, icon);
        list = baul_bookmark_list_new ();
        if (!baul_bookmark_list_contains (list, bookmark))
            baul_bookmark_list_append (list, bookmark);
        g_object_unref (bookmark);
        g_object_unref (list);
        g_object_unref (icon);
        g_free (name);
    }

	connect_dialog_set_connecting (dialog);
	connect_dialog_present_uri_async (dialog,
					  dialog->details->application,
					  location);

    g_object_unref (location);
}

static void
connect_to_server_or_finish_fill (BaulConnectServerDialog *dialog)
{
    if (dialog->details->fill_details_res != NULL) {
		connect_dialog_finish_fill (dialog);
    } else {
		connect_dialog_connect_to_server (dialog);
    }
}

static gboolean
connect_dialog_abort_mount_operation (BaulConnectServerDialog *dialog)
{
    if (dialog->details->fill_details_res != NULL) {
    	g_simple_async_result_set_op_res_gboolean (dialog->details->fill_details_res, FALSE);
    	g_simple_async_result_complete (dialog->details->fill_details_res);

    	g_object_unref (dialog->details->fill_details_res);
    	dialog->details->fill_details_res = NULL;

    	if (dialog->details->fill_operation) {
    		g_object_unref (dialog->details->fill_operation);
    		dialog->details->fill_operation = NULL;
        }

        return TRUE;
    }

    return FALSE;
}

static void
connect_dialog_destroy (BaulConnectServerDialog *dialog)
{
	if (connect_dialog_abort_mount_operation (dialog)) {
    	dialog->details->should_destroy = TRUE;
    } else {
    	ctk_widget_destroy (CTK_WIDGET (dialog));
    }
}

static void
connect_dialog_response_cb (BaulConnectServerDialog *dialog,
			    int response_id,
			    gpointer data)
{
    GError *error;

    switch (response_id)
    {
    case RESPONSE_CONNECT:
		connect_to_server_or_finish_fill (dialog);
        break;
    case CTK_RESPONSE_NONE:
    case CTK_RESPONSE_DELETE_EVENT:
    case CTK_RESPONSE_CANCEL:
		connect_dialog_destroy (dialog);
        break;
    case CTK_RESPONSE_HELP :
        error = NULL;
        ctk_show_uri_on_window (CTK_WINDOW (dialog),
                                "help:cafe-user-guide/baul-server-connect",
                                ctk_get_current_event_time (), &error);
        if (error)
        {
            eel_show_error_dialog (_("There was an error displaying help."), error->message,
                                   CTK_WINDOW (dialog));
            g_error_free (error);
        }
        break;
    default :
        g_assert_not_reached ();
    }
}

static void
connect_dialog_cleanup (BaulConnectServerDialog *dialog)
{
	/* hide the infobar */
	ctk_widget_hide (dialog->details->info_bar);

	/* set the connect button label back to 'Connect' */
	ctk_button_set_label (CTK_BUTTON (dialog->details->connect_button),
			      _("C_onnect"));

	/* if there was a pending mount operation, cancel it. */
	connect_dialog_abort_mount_operation (dialog);

	/* restore password checkbox sensitivity */
	if (dialog->details->password_sensitive_id == 0) {
		dialog->details->password_sensitive_id =
			g_signal_connect (dialog->details->password_entry, "changed",
					  G_CALLBACK (sensitive_entry_changed_callback),
					  dialog->details->remember_checkbox);
		sensitive_entry_changed_callback (CTK_EDITABLE (dialog->details->password_entry),
						  dialog->details->remember_checkbox);
	}

	/* remove icons on the entries */
	g_list_foreach (dialog->details->iconized_entries,
			(GFunc) iconized_entry_restore, dialog);
	g_list_free (dialog->details->iconized_entries);
	dialog->details->iconized_entries = NULL;

	dialog->details->last_password_set = FALSE;
}

static void
connect_dialog_setup_for_type (BaulConnectServerDialog *dialog)
{
    struct MethodInfo *meth;
    int index;
    CtkTreeIter iter;

	connect_dialog_cleanup (dialog);

	/* get our method info */
	if (!ctk_combo_box_get_active_iter (CTK_COMBO_BOX (dialog->details->type_combo),
					    &iter)) {
		/* there are no entries in the combo, something is wrong
		 * with our GVfs installation.
		 */
		connect_dialog_gvfs_error (dialog);

		return;
	}

    ctk_tree_model_get (ctk_combo_box_get_model (CTK_COMBO_BOX (dialog->details->type_combo)),
                        &iter, 0, &index, -1);
    g_assert (index < G_N_ELEMENTS (methods) && index >= 0);
    meth = &(methods[index]);

    g_object_set (dialog->details->share_entry,
    	      "visible",
    	      (meth->flags & SHOW_SHARE) != 0,
    	      NULL);

    g_object_set (dialog->details->port_spinbutton,
    	      "sensitive",
    	      (meth->flags & SHOW_PORT) != 0,
    	      "value", (gdouble) meth->default_port,
    	      NULL);

    g_object_set (dialog->details->user_details,
    	      "visible",
    	      (meth->flags & SHOW_USER) != 0 ||
    	      (meth->flags & SHOW_DOMAIN) != 0,
    	      NULL);

    g_object_set (dialog->details->user_entry,
    	      "visible",
    	      (meth->flags & SHOW_USER) != 0,
    	      NULL);

	g_object_set (dialog->details->password_entry,
		      "visible",
		      (meth->flags & SHOW_USER) != 0,
		      NULL);

    g_object_set (dialog->details->domain_entry,
    	      "visible",
    	      (meth->flags & SHOW_DOMAIN) != 0,
    	      NULL);
}

static void
sensitive_entry_changed_callback (CtkEditable *editable,
				  CtkWidget *widget)
{
    guint length;

    length = ctk_entry_get_text_length (CTK_ENTRY (editable));

	ctk_widget_set_sensitive (widget, length > 0);
}

static void
bind_visibility (BaulConnectServerDialog *dialog,
		 CtkWidget *source,
		 CtkWidget *dest)
{
    g_object_bind_property (source,
    			"visible",
    			dest,
    			"visible",
    			G_BINDING_DEFAULT);
}

static void
baul_connect_server_dialog_init (BaulConnectServerDialog *dialog)
{
    CtkWidget *label;
    CtkWidget *content_area;
    CtkWidget *combo, *grid;
    CtkWidget *vbox, *hbox, *connect_button, *checkbox;
    CtkListStore *store;
    CtkCellRenderer *renderer;
    gchar *str;
    int i;

    dialog->details = baul_connect_server_dialog_get_instance_private (dialog);

    content_area = ctk_dialog_get_content_area (CTK_DIALOG (dialog));

    /* set dialog properties */
    ctk_window_set_title (CTK_WINDOW (dialog), _("Connect to Server"));
    ctk_container_set_border_width (CTK_CONTAINER (dialog), 6);
    ctk_box_set_spacing (CTK_BOX (content_area), 2);
    ctk_window_set_resizable (CTK_WINDOW (dialog), FALSE);

	/* infobar */
	dialog->details->info_bar = ctk_info_bar_new ();
	ctk_info_bar_set_message_type (CTK_INFO_BAR (dialog->details->info_bar),
				       CTK_MESSAGE_INFO);
	ctk_box_pack_start (CTK_BOX (content_area), dialog->details->info_bar,
			    FALSE, FALSE, 6);

    /* server settings label */
    label = ctk_label_new (NULL);
    str = g_strdup_printf ("<b>%s</b>", _("Server Details"));
    ctk_label_set_markup (CTK_LABEL (label), str);
    g_free (str);
    ctk_label_set_xalign (CTK_LABEL (label), 0);
    ctk_box_pack_start (CTK_BOX (content_area), label, FALSE, FALSE, 6);
    ctk_widget_show (label);

    grid = ctk_grid_new ();
    ctk_orientable_set_orientation (CTK_ORIENTABLE (grid), CTK_ORIENTATION_VERTICAL);
    ctk_grid_set_row_spacing (CTK_GRID (grid), 6);
    ctk_grid_set_column_spacing (CTK_GRID (grid), 3);
    ctk_widget_set_halign (grid, CTK_ALIGN_START);
    ctk_widget_set_valign (grid, CTK_ALIGN_START);
    ctk_widget_set_margin_start (grid, 12);
    ctk_widget_show (grid);
    ctk_box_pack_start (CTK_BOX (content_area), grid, TRUE, TRUE, 0);

    dialog->details->primary_grid = grid;

    /* first row: server entry + port spinbutton */
    label = ctk_label_new_with_mnemonic (_("_Server:"));
    ctk_label_set_xalign (CTK_LABEL (label), 0);
    ctk_container_add (CTK_CONTAINER (grid), label);
    ctk_widget_show (label);

    hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
    ctk_widget_show (hbox);
    ctk_grid_attach_next_to (CTK_GRID (grid), hbox, label,
                             CTK_POS_RIGHT,
                             1, 1);

    dialog->details->server_entry = ctk_entry_new ();
    ctk_entry_set_activates_default (CTK_ENTRY (dialog->details->server_entry), TRUE);
    ctk_box_pack_start (CTK_BOX (hbox), dialog->details->server_entry, FALSE, FALSE, 0);
    ctk_label_set_mnemonic_widget (CTK_LABEL (label), dialog->details->server_entry);
    ctk_widget_show (dialog->details->server_entry);

    /* port */
    label = ctk_label_new_with_mnemonic (_("_Port:"));
    ctk_label_set_xalign (CTK_LABEL (label), 0);
    ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);
    ctk_widget_show (label);

    dialog->details->port_spinbutton =
    	ctk_spin_button_new_with_range (0.0, 65535.0, 1.0);
    g_object_set (dialog->details->port_spinbutton,
    	      "digits", 0,
    	      "numeric", TRUE,
    	      "update-policy", CTK_UPDATE_IF_VALID,
    	      NULL);
    ctk_box_pack_start (CTK_BOX (hbox), dialog->details->port_spinbutton,
    		    FALSE, FALSE, 0);
    ctk_label_set_mnemonic_widget (CTK_LABEL (label), dialog->details->port_spinbutton);
    ctk_widget_show (dialog->details->port_spinbutton);

    /* second row: type combobox */
    label = ctk_label_new (_("Type:"));
    ctk_label_set_xalign (CTK_LABEL (label), 0);
    ctk_container_add (CTK_CONTAINER (grid), label);
    ctk_widget_show (label);

    dialog->details->type_combo = combo = ctk_combo_box_new ();

    /* each row contains: method index, textual description */
    store = ctk_list_store_new (2, G_TYPE_INT, G_TYPE_STRING);
    ctk_combo_box_set_model (CTK_COMBO_BOX (combo), CTK_TREE_MODEL (store));
    g_object_unref (store);

    renderer = ctk_cell_renderer_text_new ();
    ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (combo), renderer, TRUE);
    ctk_cell_layout_add_attribute (CTK_CELL_LAYOUT (combo), renderer, "text", 1);

    for (i = 0; i < G_N_ELEMENTS (methods); i++)
    {
        CtkTreeIter iter;
        const gchar * const *supported;

		/* skip methods that don't have corresponding gvfs uri schemes */
        supported = g_vfs_get_supported_uri_schemes (g_vfs_get_default ());

        if (methods[i].scheme != NULL)
        {
            gboolean found;
            int j;

            found = FALSE;
            for (j = 0; supported[j] != NULL; j++)
            {
                if (strcmp (methods[i].scheme, supported[j]) == 0)
                {
                    found = TRUE;
                    break;
                }
            }

            if (!found)
            {
                continue;
            }
        }

        ctk_list_store_append (store, &iter);
        ctk_list_store_set (store, &iter,
                            0, i,
                            1, get_method_description (&(methods[i])),
                            -1);


        if (methods[i].flags & DEFAULT_METHOD)
        {
            ctk_combo_box_set_active_iter (CTK_COMBO_BOX (combo), &iter);
        }
    }

    if (ctk_combo_box_get_active (CTK_COMBO_BOX (combo)) < 0)
    {
        /* default method not available, use any other */
        ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 0);
    }

    ctk_widget_show (combo);
    ctk_label_set_mnemonic_widget (CTK_LABEL (label), combo);
    ctk_grid_attach_next_to (CTK_GRID (grid), combo, label,
                             CTK_POS_RIGHT, 1, 1);
    g_signal_connect_swapped (combo, "changed",
				  G_CALLBACK (connect_dialog_setup_for_type),
    			  dialog);

    /* third row: share entry */
    label = ctk_label_new (_("Share:"));
    ctk_label_set_xalign (CTK_LABEL (label), 0);
    ctk_container_add (CTK_CONTAINER (grid), label);

    dialog->details->share_entry = ctk_entry_new ();
    ctk_entry_set_activates_default (CTK_ENTRY (dialog->details->share_entry), TRUE);
    ctk_grid_attach_next_to (CTK_GRID (grid), dialog->details->share_entry, label,
                             CTK_POS_RIGHT, 1, 1);

    bind_visibility (dialog, dialog->details->share_entry, label);

    /* fourth row: folder entry */
    label = ctk_label_new (_("Folder:"));
    ctk_label_set_xalign (CTK_LABEL (label), 0);
    ctk_container_add (CTK_CONTAINER (grid), label);
    ctk_widget_show (label);
    dialog->details->folder_entry = ctk_entry_new ();
    ctk_entry_set_text (CTK_ENTRY (dialog->details->folder_entry), "/");
    ctk_entry_set_activates_default (CTK_ENTRY (dialog->details->folder_entry), TRUE);
    ctk_grid_attach_next_to (CTK_GRID (grid), dialog->details->folder_entry, label,
                             CTK_POS_RIGHT, 1, 1);
    ctk_widget_show (dialog->details->folder_entry);

    /* user details label */
    label = ctk_label_new (NULL);
    str = g_strdup_printf ("<b>%s</b>", _("User Details"));
    ctk_label_set_markup (CTK_LABEL (label), str);
    g_free (str);
    ctk_label_set_xalign (CTK_LABEL (label), 0);
    ctk_box_pack_start (CTK_BOX (content_area), label, FALSE, FALSE, 6);

    grid = ctk_grid_new ();
    ctk_grid_set_row_spacing (CTK_GRID (grid), 6);
    ctk_grid_set_column_spacing (CTK_GRID (grid), 3);
    ctk_orientable_set_orientation (CTK_ORIENTABLE (grid), CTK_ORIENTATION_VERTICAL);
    ctk_widget_set_halign (grid, CTK_ALIGN_START);
    ctk_widget_set_valign (grid, CTK_ALIGN_START);
    ctk_widget_set_margin_start (grid, 12);
    ctk_widget_show (grid);
    ctk_box_pack_start (CTK_BOX (content_area), grid, TRUE, TRUE, 0);

    bind_visibility (dialog, grid, label);
    dialog->details->user_details = grid;

    /* first row: domain entry */
    label = ctk_label_new (_("Domain Name:"));
    ctk_label_set_xalign (CTK_LABEL (label), 0);
    ctk_container_add (CTK_CONTAINER (grid), label);

    dialog->details->domain_entry = ctk_entry_new ();
    ctk_entry_set_activates_default (CTK_ENTRY (dialog->details->domain_entry), TRUE);
    ctk_grid_attach_next_to (CTK_GRID (grid), dialog->details->domain_entry, label,
                             CTK_POS_RIGHT, 1, 1);

    bind_visibility (dialog, dialog->details->domain_entry, label);

    /* second row: username entry */
    label = ctk_label_new (_("User Name:"));
    ctk_label_set_xalign (CTK_LABEL (label), 0);
    ctk_container_add (CTK_CONTAINER (grid), label);

    dialog->details->user_entry = ctk_entry_new ();
    ctk_entry_set_activates_default (CTK_ENTRY (dialog->details->user_entry), TRUE);
    ctk_grid_attach_next_to (CTK_GRID (grid), dialog->details->user_entry, label,
                             CTK_POS_RIGHT, 1, 1);

    bind_visibility (dialog, dialog->details->user_entry, label);

    /* third row: password entry */
    label = ctk_label_new (_("Password:"));
    ctk_label_set_xalign (CTK_LABEL (label), 0);
    ctk_container_add (CTK_CONTAINER (grid), label);

    dialog->details->password_entry = ctk_entry_new ();
    ctk_entry_set_activates_default (CTK_ENTRY (dialog->details->password_entry), TRUE);
    ctk_entry_set_visibility (CTK_ENTRY (dialog->details->password_entry), FALSE);
    ctk_grid_attach_next_to (CTK_GRID (grid), dialog->details->password_entry, label,
                             CTK_POS_RIGHT, 1, 1);

    bind_visibility (dialog, dialog->details->password_entry, label);

    /* fourth row: remember checkbox */
    checkbox = ctk_check_button_new_with_label (_("Remember this password"));
    ctk_grid_attach_next_to (CTK_GRID (grid), checkbox, dialog->details->password_entry,
                             CTK_POS_BOTTOM, 1, 1);
    dialog->details->remember_checkbox = checkbox;

    bind_visibility (dialog, dialog->details->password_entry, checkbox);

    /* add as bookmark */
    vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
    ctk_box_pack_start (CTK_BOX (content_area), vbox, FALSE, FALSE, 6);

    dialog->details->bookmark_checkbox = ctk_check_button_new_with_mnemonic (_("Add _bookmark"));
    ctk_box_pack_start (CTK_BOX (vbox), dialog->details->bookmark_checkbox, TRUE, TRUE, 0);
    ctk_widget_show (dialog->details->bookmark_checkbox);

    hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
    ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    label = ctk_label_new (_("Bookmark Name:"));
    ctk_box_pack_start (CTK_BOX (hbox), label, TRUE, TRUE, 0);

    dialog->details->name_entry = ctk_entry_new ();
    ctk_box_pack_start (CTK_BOX (hbox), dialog->details->name_entry, TRUE, TRUE, 0);

    ctk_widget_show_all (vbox);

    g_object_bind_property (dialog->details->bookmark_checkbox, "active",
                            dialog->details->name_entry, "sensitive",
                            G_BINDING_DEFAULT |
                            G_BINDING_SYNC_CREATE);

    eel_dialog_add_button (CTK_DIALOG (dialog),
                           _("_Help"),
                           "help-browser",
                           CTK_RESPONSE_HELP);

    eel_dialog_add_button (CTK_DIALOG (dialog),
                           _("_Cancel"),
                           "process-stop",
                           CTK_RESPONSE_CANCEL);

    connect_button = ctk_dialog_add_button (CTK_DIALOG (dialog),
    					_("C_onnect"),
    					RESPONSE_CONNECT);
    ctk_dialog_set_default_response (CTK_DIALOG (dialog),
                                     RESPONSE_CONNECT);
	dialog->details->connect_button = connect_button;

    g_signal_connect (dialog->details->server_entry, "changed",
			  G_CALLBACK (sensitive_entry_changed_callback),
    		  connect_button);
	sensitive_entry_changed_callback (CTK_EDITABLE (dialog->details->server_entry),
					  connect_button);

    g_signal_connect (dialog, "response",
			  G_CALLBACK (connect_dialog_response_cb),
                      dialog);

	connect_dialog_setup_for_type (dialog);
}

static void
baul_connect_server_dialog_finalize (GObject *object)
{
	BaulConnectServerDialog *dialog;

	dialog = BAUL_CONNECT_SERVER_DIALOG (object);

	connect_dialog_abort_mount_operation (dialog);

	if (dialog->details->iconized_entries != NULL) {
		g_list_free (dialog->details->iconized_entries);
		dialog->details->iconized_entries = NULL;
	}

	G_OBJECT_CLASS (baul_connect_server_dialog_parent_class)->finalize (object);
}

static void
baul_connect_server_dialog_class_init (BaulConnectServerDialogClass *class)
{
	GObjectClass *oclass;

	oclass = G_OBJECT_CLASS (class);
	oclass->finalize = baul_connect_server_dialog_finalize;
}

CtkWidget *
baul_connect_server_dialog_new (BaulWindow *window)
{
    BaulConnectServerDialog *conndlg;
    CtkWidget *dialog;

    dialog = ctk_widget_new (BAUL_TYPE_CONNECT_SERVER_DIALOG, NULL);
    conndlg = BAUL_CONNECT_SERVER_DIALOG (dialog);

    if (window)
    {
        ctk_window_set_screen (CTK_WINDOW (dialog),
                               ctk_window_get_screen (CTK_WINDOW (window)));
        conndlg->details->application = window->application;
    }

    return dialog;
}

gboolean
baul_connect_server_dialog_fill_details_finish (BaulConnectServerDialog *self,
						    GAsyncResult *result)
{
	return g_simple_async_result_get_op_res_gboolean (G_SIMPLE_ASYNC_RESULT (result));
}

void
baul_connect_server_dialog_fill_details_async (BaulConnectServerDialog *self,
						   GMountOperation *operation,
						   const gchar *default_user,
						   const gchar *default_domain,
						   GAskPasswordFlags flags,
						   GAsyncReadyCallback callback,
						   gpointer user_data)
{
	GSimpleAsyncResult *fill_details_res;
	const gchar *str;
	GAskPasswordFlags set_flags;

	fill_details_res = g_simple_async_result_new (G_OBJECT (self), callback, user_data,
						      baul_connect_server_dialog_fill_details_async);

	self->details->fill_details_res = fill_details_res;
	set_flags = (flags & G_ASK_PASSWORD_NEED_PASSWORD) |
		(flags & G_ASK_PASSWORD_NEED_USERNAME) |
		(flags & G_ASK_PASSWORD_NEED_DOMAIN);

	if (set_flags & G_ASK_PASSWORD_NEED_PASSWORD) {
		/* provide the password */
		str = ctk_entry_get_text (CTK_ENTRY (self->details->password_entry));

		if (str != NULL && g_strcmp0 (str, "") != 0 &&
		    !self->details->last_password_set) {
			g_mount_operation_set_password (G_MOUNT_OPERATION (operation),
							str);
			set_flags ^= G_ASK_PASSWORD_NEED_PASSWORD;

			if (flags & G_ASK_PASSWORD_SAVING_SUPPORTED &&
			    ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (self->details->remember_checkbox))) {
				g_mount_operation_set_password_save (G_MOUNT_OPERATION (operation),
								     G_PASSWORD_SAVE_PERMANENTLY);
			}

			self->details->last_password_set = TRUE;
		}
	}

	if (set_flags & G_ASK_PASSWORD_NEED_USERNAME) {
		/* see if the default username is different from ours */
		str = ctk_entry_get_text (CTK_ENTRY (self->details->user_entry));

		if (str != NULL && g_strcmp0 (str, "") != 0 &&
		    g_strcmp0 (str, default_user) != 0) {
			g_mount_operation_set_username (G_MOUNT_OPERATION (operation),
							str);
			set_flags ^= G_ASK_PASSWORD_NEED_USERNAME;
		}
	}

	if (set_flags & G_ASK_PASSWORD_NEED_DOMAIN) {
		/* see if the default domain is different from ours */
		str = ctk_entry_get_text (CTK_ENTRY (self->details->domain_entry));

		if (str != NULL && g_strcmp0 (str, "") &&
		    g_strcmp0 (str, default_domain) != 0) {
			g_mount_operation_set_domain (G_MOUNT_OPERATION (operation),
						      str);
			set_flags ^= G_ASK_PASSWORD_NEED_DOMAIN;
		}
	}

	if (set_flags != 0) {
		set_flags |= (flags & G_ASK_PASSWORD_SAVING_SUPPORTED);
		self->details->fill_operation = g_object_ref (operation);
		connect_dialog_request_additional_details (self, set_flags, default_user, default_domain);
	} else {
		g_simple_async_result_set_op_res_gboolean (fill_details_res, TRUE);
		g_simple_async_result_complete (fill_details_res);
		g_object_unref (self->details->fill_details_res);

		self->details->fill_details_res = NULL;
	}
}
