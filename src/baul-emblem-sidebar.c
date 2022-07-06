/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 1999, 2000, 2001 Eazel, Inc.
 * Copyright (C) 2001 Red Hat, Inc.
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
 * Authors:  James Willcox  <jwillcox@gnome.org>
 *           Alexander Larsson <alexl@redhat.com>
 *
 * This is a sidebar displaying emblems which can be dragged onto files to
 * set/unset the chosen emblem.
 *
 */

#include <config.h>
#include <stdio.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <ctk/ctk.h>
#include <glib/gi18n.h>

#include <eel/eel-wrap-table.h>
#include <eel/eel-labeled-image.h>
#include <eel/eel-graphic-effects.h>
#include <eel/eel-gdk-pixbuf-extensions.h>
#include <eel/eel-stock-dialogs.h>
#include <eel/eel-ctk-extensions.h>

#include <libbaul-private/baul-icon-dnd.h>
#include <libbaul-private/baul-emblem-utils.h>
#include <libbaul-private/baul-file-utilities.h>
#include <libbaul-private/baul-sidebar-provider.h>
#include <libbaul-private/baul-module.h>
#include <libbaul-private/baul-signaller.h>

#include "baul-emblem-sidebar.h"

struct BaulEmblemSidebarDetails
{
    BaulWindowInfo *window;
    GtkWidget *emblems_table;
    GtkWidget *popup;
    GtkWidget *popup_remove;
    GtkWidget *popup_rename;

    char *popup_emblem_keyword;
    char *popup_emblem_display_name;
    GdkPixbuf *popup_emblem_pixbuf;
};

#define ERASE_EMBLEM_KEYWORD			"erase"
#define STANDARD_EMBLEM_HEIGHT			52
#define EMBLEM_LABEL_SPACING			2

static void baul_emblem_sidebar_populate          (BaulEmblemSidebar        *emblem_sidebar);
static void baul_emblem_sidebar_refresh           (BaulEmblemSidebar        *emblem_sidebar);
static void baul_emblem_sidebar_iface_init        (BaulSidebarIface         *iface);
static void sidebar_provider_iface_init           (BaulSidebarProviderIface *iface);
static GType baul_emblem_sidebar_provider_get_type (void);

static const GtkTargetEntry drag_types[] =
{
    {"property/keyword", 0, 0 }
};

enum
{
    TARGET_URI_LIST,
    TARGET_URI,
    TARGET_NETSCAPE_URL
};

static const GtkTargetEntry dest_types[] =
{
    {"text/uri-list", 0, TARGET_URI_LIST},
    {"text/plain", 0, TARGET_URI},
    {"_NETSCAPE_URL", 0, TARGET_NETSCAPE_URL}
};

typedef struct _Emblem
{
    GdkPixbuf *pixbuf;
    char *uri;
    char *name;
    char *keyword;
} Emblem;

typedef struct
{
    GObject parent;
} BaulEmblemSidebarProvider;

typedef struct
{
    GObjectClass parent;
} BaulEmblemSidebarProviderClass;

G_DEFINE_TYPE_WITH_CODE (BaulEmblemSidebar, baul_emblem_sidebar, GTK_TYPE_BOX,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_SIDEBAR,
                                 baul_emblem_sidebar_iface_init));

G_DEFINE_TYPE_WITH_CODE (BaulEmblemSidebarProvider, baul_emblem_sidebar_provider, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BAUL_TYPE_SIDEBAR_PROVIDER,
                                 sidebar_provider_iface_init));

static void
baul_emblem_sidebar_drag_data_get_cb (GtkWidget *widget,
                                      GdkDragContext *context,
                                      GtkSelectionData *data,
                                      guint info,
                                      guint time,
                                      BaulEmblemSidebar *emblem_sidebar)
{
    char *keyword;

    keyword = g_object_get_data (G_OBJECT (widget), "emblem-keyword");

    g_return_if_fail (keyword != NULL);

    ctk_selection_data_set (data, ctk_selection_data_get_target (data), 8,
                            keyword,
                            strlen (keyword));
}

static void
baul_emblem_sidebar_enter_notify_cb (GtkWidget *widget,
                                     BaulEmblemSidebar *emblem_sidebar)
{
    GdkPixbuf *pixbuf;
    EelLabeledImage *image;

    pixbuf = g_object_get_data (G_OBJECT (widget), "prelight-pixbuf");
    image = g_object_get_data (G_OBJECT (widget), "labeled-image");

    eel_labeled_image_set_pixbuf (EEL_LABELED_IMAGE (image), pixbuf);
}

static void
baul_emblem_sidebar_leave_notify_cb (GtkWidget *widget,
                                     BaulEmblemSidebar *emblem_sidebar)
{
    GdkPixbuf *pixbuf;
    EelLabeledImage *image;

    pixbuf = g_object_get_data (G_OBJECT (widget), "original-pixbuf");
    image = g_object_get_data (G_OBJECT (widget), "labeled-image");

    eel_labeled_image_set_pixbuf (EEL_LABELED_IMAGE (image), pixbuf);
}

static gboolean
baul_emblem_sidebar_button_press_cb (GtkWidget *widget,
                                     GdkEventButton *event,
                                     BaulEmblemSidebar *emblem_sidebar)
{
    char *keyword, *name;
    GdkPixbuf *pixbuf;

    if (event->button == 3)
    {
        keyword = g_object_get_data (G_OBJECT (widget),
                                     "emblem-keyword");
        name = g_object_get_data (G_OBJECT (widget),
                                  "emblem-display-name");
        pixbuf = g_object_get_data (G_OBJECT (widget),
                                    "original-pixbuf");

        emblem_sidebar->details->popup_emblem_keyword = keyword;
        emblem_sidebar->details->popup_emblem_display_name = name;
        emblem_sidebar->details->popup_emblem_pixbuf = pixbuf;

        ctk_widget_set_sensitive (emblem_sidebar->details->popup_remove,
                                  baul_emblem_can_remove_emblem (keyword));
        ctk_widget_set_sensitive (emblem_sidebar->details->popup_rename,
                                  baul_emblem_can_rename_emblem (keyword));

        ctk_menu_popup_at_pointer (GTK_MENU (emblem_sidebar->details->popup),
                                             (const GdkEvent*) event);
    }

    return TRUE;
}

static void
send_emblems_changed (void)
{
    g_signal_emit_by_name (baul_signaller_get_current (),
                           "emblems_changed");
}

static void
emblems_changed_callback (GObject *signaller,
                          BaulEmblemSidebar *emblem_sidebar)
{
    baul_emblem_sidebar_refresh (emblem_sidebar);
}

static void
baul_emblem_sidebar_delete_cb (GtkWidget *menu_item,
                               BaulEmblemSidebar *emblem_sidebar)
{
    if (baul_emblem_remove_emblem (emblem_sidebar->details->popup_emblem_keyword))
    {
        send_emblems_changed ();
    }
    else
    {
        char *error;

        error = g_strdup_printf (_("Could not remove emblem with name '%s'."), emblem_sidebar->details->popup_emblem_display_name);
        eel_show_error_dialog (error, _("This is probably because the emblem is a permanent one, and not one that you added yourself."),
                               NULL);
        g_free (error);
    }
}

static void
rename_dialog_response_cb (GtkWidget *dialog, int response,
                           BaulEmblemSidebar *emblem_sidebar)
{
    GtkWidget *entry;
    char *keyword, *name;

    keyword = g_object_get_data (G_OBJECT (dialog), "emblem-keyword");

    if (response == GTK_RESPONSE_CANCEL)
    {
        g_free (keyword);
        ctk_widget_destroy (dialog);
        return;
    }
    else if (response == GTK_RESPONSE_HELP)
    {
        g_message ("Implement me!");
        return;
    }

    entry = g_object_get_data (G_OBJECT (dialog), "entry");

    name = g_strdup (ctk_entry_get_text (GTK_ENTRY (entry)));

    ctk_widget_destroy (dialog);

    if (baul_emblem_rename_emblem (keyword, name))
    {
        send_emblems_changed ();
    }
    else
    {
        char *error;

        error = g_strdup_printf (_("Could not rename emblem with name '%s'."), name);
        eel_show_error_dialog (error, _("This is probably because the emblem is a permanent one, and not one that you added yourself."),
                               NULL);
        g_free (error);
    }

    g_free (keyword);
    g_free (name);
}

static GtkWidget *
create_rename_emblem_dialog (BaulEmblemSidebar *emblem_sidebar,
                             const char *keyword, const char *orig_name,
                             GdkPixbuf *pixbuf)
{
    GtkWidget *dialog, *label, *image, *entry, *hbox;

    image = ctk_image_new_from_pixbuf (pixbuf);
    entry = ctk_entry_new ();

    dialog = ctk_dialog_new ();
    ctk_window_set_title (GTK_WINDOW (dialog), _("Rename Emblem"));
    ctk_window_set_transient_for (GTK_WINDOW (dialog), NULL);

    eel_dialog_add_button (GTK_DIALOG (dialog),
                           _("_Cancel"),
                           "process-stop",
                           GTK_RESPONSE_CANCEL);

    eel_dialog_add_button (GTK_DIALOG (dialog),
                           _("_OK"),
                           "ctk-ok",
                           GTK_RESPONSE_OK);

    eel_dialog_add_button (GTK_DIALOG (dialog),
                           _("_Help"),
                           "help-browser",
                           GTK_RESPONSE_HELP);

    ctk_dialog_set_default_response (GTK_DIALOG (dialog),
                                     GTK_RESPONSE_OK);

    g_object_set_data (G_OBJECT (dialog), "emblem-keyword",
                       g_strdup (keyword));
    g_object_set_data (G_OBJECT (dialog), "entry",
                       entry);

    label = ctk_label_new (_("Enter a new name for the displayed emblem:"));
    ctk_widget_show (label);
    ctk_box_pack_start (GTK_BOX (ctk_dialog_get_content_area (GTK_DIALOG (dialog))), label,
                        FALSE, FALSE, 8);


    hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
    ctk_box_pack_start (GTK_BOX (hbox), image, TRUE, TRUE, 8);

    ctk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

    ctk_box_pack_start (GTK_BOX (hbox), entry, TRUE, FALSE, 8);
    ctk_widget_show_all (hbox);

    /* it would be nice to have the text selected, ready to be overwritten
     * by the user, but that doesn't seem possible.
     */
    ctk_widget_grab_focus (entry);
    ctk_entry_set_text (GTK_ENTRY (entry), orig_name);

    ctk_box_pack_start (GTK_BOX (ctk_dialog_get_content_area (GTK_DIALOG (dialog))), hbox,
                        TRUE, TRUE, 8);


    return dialog;
}

static void
baul_emblem_sidebar_rename_cb (GtkWidget *menu_item,
                               BaulEmblemSidebar *emblem_sidebar)
{
    GtkWidget *dialog;

    dialog = create_rename_emblem_dialog (emblem_sidebar,
                                          emblem_sidebar->details->popup_emblem_keyword,
                                          emblem_sidebar->details->popup_emblem_display_name,
                                          emblem_sidebar->details->popup_emblem_pixbuf);
    g_signal_connect (dialog, "response",
                      G_CALLBACK (rename_dialog_response_cb),
                      emblem_sidebar);
    ctk_widget_show (dialog);
}

static void
create_popup_menu (BaulEmblemSidebar *emblem_sidebar)
{
    GtkWidget *popup, *menu_item;

    popup = ctk_menu_new ();

    ctk_menu_set_reserve_toggle_size (GTK_MENU (popup), FALSE);

    /* add the "rename" menu item */
    menu_item = eel_image_menu_item_new_from_icon ("document-properties", _("Rename"));

    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (baul_emblem_sidebar_rename_cb),
                      emblem_sidebar);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (GTK_MENU_SHELL (popup), menu_item);
    emblem_sidebar->details->popup_rename = menu_item;

    /* add "delete" menu item */
    menu_item = eel_image_menu_item_new_from_icon ("edit-delete", _("_Delete"));

    g_signal_connect (menu_item, "activate",
                      G_CALLBACK (baul_emblem_sidebar_delete_cb),
                      emblem_sidebar);
    ctk_widget_show (menu_item);
    ctk_menu_shell_append (GTK_MENU_SHELL (popup), menu_item);
    emblem_sidebar->details->popup_remove = menu_item;

    emblem_sidebar->details->popup = popup;
}

static GtkWidget *
create_emblem_widget_with_pixbuf (BaulEmblemSidebar *emblem_sidebar,
                                  const char *keyword,
                                  const char *display_name,
                                  GdkPixbuf *pixbuf)
{
    GtkWidget *image, *event_box;
    GdkPixbuf *prelight_pixbuf;

    image = eel_labeled_image_new (display_name, pixbuf);

    eel_labeled_image_set_fixed_image_height (EEL_LABELED_IMAGE (image),
            STANDARD_EMBLEM_HEIGHT);
    eel_labeled_image_set_spacing (EEL_LABELED_IMAGE (image),
                                   EMBLEM_LABEL_SPACING);
    event_box = ctk_event_box_new ();
    ctk_container_add (GTK_CONTAINER (event_box), image);

    prelight_pixbuf = eel_create_spotlight_pixbuf (pixbuf);


    ctk_drag_source_set (event_box, GDK_BUTTON1_MASK, drag_types,
                         G_N_ELEMENTS (drag_types),
                         GDK_ACTION_COPY | GDK_ACTION_MOVE);

    ctk_drag_source_set_icon_pixbuf (event_box, pixbuf);



    g_signal_connect (event_box, "button_press_event",
                      G_CALLBACK (baul_emblem_sidebar_button_press_cb),
                      emblem_sidebar);
    g_signal_connect (event_box, "drag-data-get",
                      G_CALLBACK (baul_emblem_sidebar_drag_data_get_cb),
                      emblem_sidebar);
    g_signal_connect (event_box, "enter-notify-event",
                      G_CALLBACK (baul_emblem_sidebar_enter_notify_cb),
                      emblem_sidebar);
    g_signal_connect (event_box, "leave-notify-event",
                      G_CALLBACK (baul_emblem_sidebar_leave_notify_cb),
                      emblem_sidebar);

    g_object_set_data_full (G_OBJECT (event_box),
                            "emblem-keyword",
                            g_strdup (keyword), g_free);
    g_object_set_data_full (G_OBJECT (event_box),
                            "emblem-display-name",
                            g_strdup (display_name), g_free);
    g_object_set_data_full (G_OBJECT (event_box),
                            "original-pixbuf",
                            pixbuf, g_object_unref);
    g_object_set_data_full (G_OBJECT (event_box),
                            "prelight-pixbuf",
                            prelight_pixbuf, g_object_unref);
    g_object_set_data (G_OBJECT (event_box),
                       "labeled-image", image);

    return event_box;

}

static GtkWidget *
create_emblem_widget (BaulEmblemSidebar *emblem_sidebar,
                      const char *name)
{
    GtkWidget *ret;
    const char *display_name;
    char *keyword;
    GdkPixbuf *pixbuf;
    BaulIconInfo *info;

    info = baul_icon_info_lookup_from_name (name, BAUL_ICON_SIZE_STANDARD, 1);

    pixbuf = baul_icon_info_get_pixbuf_at_size (info, BAUL_ICON_SIZE_STANDARD);

    display_name = baul_icon_info_get_display_name (info);

    keyword = baul_emblem_get_keyword_from_icon_name (name);
    if (display_name == NULL)
    {
        display_name = keyword;
    }

    ret = create_emblem_widget_with_pixbuf (emblem_sidebar, keyword,
                                            display_name, pixbuf);
    g_free (keyword);
    g_object_unref (info);
    return ret;
}

static void
emblem_name_entry_changed_cb (GtkWidget *entry, Emblem *emblem)
{
    char *text;

    g_free (emblem->name);

    text = ctk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

    emblem->name = g_strdup (text);
}


static void
destroy_emblem (Emblem *emblem, gpointer user_data)
{
    g_return_if_fail (emblem != NULL);


    if (emblem->pixbuf != NULL)
    {
        g_object_unref (emblem->pixbuf);
        emblem->pixbuf = NULL;
    }

    if (emblem->name != NULL)
    {
        g_free (emblem->name);
        emblem->name = NULL;
    }

    if (emblem->uri != NULL)
    {
        g_free (emblem->uri);
        emblem->uri = NULL;
    }

    if (emblem->keyword != NULL)
    {
        g_free (emblem->keyword);
        emblem->keyword = NULL;
    }

    g_free (emblem);
}

static void
destroy_emblem_list (GSList *list)
{
    g_slist_foreach (list, (GFunc)destroy_emblem, NULL);
    g_slist_free (list);
}

static GtkWidget *
create_add_emblems_dialog (BaulEmblemSidebar *emblem_sidebar,
                           GSList *emblems)
{
    GtkWidget *dialog, *label, *table;
    GtkWidget *first_entry, *entry, *scroller;
    GSList *list;
    int num_emblems;
    GtkWidget *image = NULL;
    GtkWidget *hbox = NULL;
    Emblem *emblem = NULL;

    first_entry = NULL;

    dialog = ctk_dialog_new ();
    ctk_window_set_title (GTK_WINDOW (dialog), _("Add Emblems..."));
    ctk_window_set_transient_for (GTK_WINDOW (dialog), NULL);

    eel_dialog_add_button (GTK_DIALOG (dialog),
                           _("_Cancel"),
                           "process-stop",
                           GTK_RESPONSE_CANCEL);

    eel_dialog_add_button (GTK_DIALOG (dialog),
                           _("_OK"),
                           "ctk-ok",
                           GTK_RESPONSE_OK);

    eel_dialog_add_button (GTK_DIALOG (dialog),
                           _("_Help"),
                           "help-browser",
                           GTK_RESPONSE_HELP);

    ctk_dialog_set_default_response (GTK_DIALOG (dialog),
                                     GTK_RESPONSE_OK);

    /* FIXME:  make a better message */
    if (g_slist_length (emblems) > 1)
    {
        label = ctk_label_new (_("Enter a descriptive name next to each emblem.  This name will be used in other places to identify the emblem."));
    }
    else
    {
        label = ctk_label_new (_("Enter a descriptive name next to the emblem.  This name will be used in other places to identify the emblem."));
    }

    ctk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    ctk_box_pack_start (GTK_BOX (ctk_dialog_get_content_area (GTK_DIALOG (dialog))),
                        label, FALSE, FALSE, 8);
    ctk_widget_show (label);

    scroller = eel_scrolled_wrap_table_new (TRUE, GTK_SHADOW_NONE, &table);
    eel_wrap_table_set_x_spacing (EEL_WRAP_TABLE (table), 8);
    eel_wrap_table_set_y_spacing (EEL_WRAP_TABLE (table), 8);

    num_emblems=0;
    list = emblems;
    while (list != NULL)
    {
        /* walk through the list of emblems, and create an image
         * and entry for each one
         */

        emblem = (Emblem *)list->data;
        list = list->next;

        image = ctk_image_new_from_pixbuf (emblem->pixbuf);

        hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        ctk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

        entry = ctk_entry_new ();
        ctk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
        g_signal_connect (entry, "changed",
                          G_CALLBACK (emblem_name_entry_changed_cb),
                          emblem);

        ctk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
        ctk_container_add (GTK_CONTAINER (table), hbox);

        if (num_emblems == 0)
        {
            first_entry = entry;
        }

        num_emblems++;
    }

    ctk_container_set_border_width (GTK_CONTAINER (dialog), 8);
    ctk_box_pack_start (GTK_BOX (ctk_dialog_get_content_area (GTK_DIALOG (dialog))),
                        scroller, TRUE, TRUE, 8);
    ctk_widget_show_all (scroller);

    ctk_widget_grab_focus (first_entry);

    /* we expand the window to hold up to about 4 emblems, but after that
     * let the scroller do its thing.  Is there a better way to do this?
     */
    ctk_window_set_default_size (GTK_WINDOW (dialog), 400,
                                 MIN (120+(60*num_emblems), 350));

    g_object_set_data_full (G_OBJECT (dialog), "emblems-to-add",
                            emblems, (GDestroyNotify)destroy_emblem_list);

    return dialog;
}

static void
remove_widget (GtkWidget *widget, GtkContainer *container)
{
    ctk_container_remove (container, widget);
}

static void
baul_emblem_sidebar_refresh (BaulEmblemSidebar *emblem_sidebar)
{
    baul_emblem_refresh_list ();

    ctk_container_foreach (GTK_CONTAINER (emblem_sidebar->details->emblems_table),
                           (GtkCallback)remove_widget,
                           emblem_sidebar->details->emblems_table);

    baul_emblem_sidebar_populate (emblem_sidebar);
}

static void
add_emblems_dialog_response_cb (GtkWidget *dialog, int response,
                                BaulEmblemSidebar *emblem_sidebar)
{
    Emblem *emblem;
    GSList *emblems;
    GSList *l;

    switch (response)
    {
    case GTK_RESPONSE_CANCEL:
        ctk_widget_destroy (dialog);
        break;

    case GTK_RESPONSE_HELP:
        g_message ("Implement me!");
        break;

    case GTK_RESPONSE_OK:
        emblems = g_object_get_data (G_OBJECT (dialog),
                                     "emblems-to-add");

        for (l = emblems; l; l = l->next)
        {
            char *keyword;

            emblem = (Emblem *)l->data;
            if (emblem->keyword != NULL)
            {
                /* this one has already been verified */
                continue;
            }

            keyword = baul_emblem_create_unique_keyword (emblem->name);
            if (!baul_emblem_verify_keyword
                    (GTK_WINDOW (dialog), keyword, emblem->name))
            {
                g_free (keyword);
                return;
            }
            else
            {
                emblem->keyword = keyword;
            }

        }

        for (l = emblems; l; l = l->next)
        {
            emblem = (Emblem *)l->data;

            baul_emblem_install_custom_emblem (emblem->pixbuf,
                                               emblem->keyword,
                                               emblem->name,
                                               GTK_WINDOW (dialog));
        }

        ctk_widget_destroy (dialog);

        send_emblems_changed ();
        break;
    }
}

static void
show_add_emblems_dialog (BaulEmblemSidebar *emblem_sidebar,
                         GSList *emblems)
{
    GtkWidget *dialog;

    g_return_if_fail (emblems != NULL);

    dialog = create_add_emblems_dialog (emblem_sidebar, emblems);

    if (dialog == NULL)
    {
        return;
    }

    g_signal_connect (dialog, "response",
                      G_CALLBACK (add_emblems_dialog_response_cb),
                      emblem_sidebar);

    ctk_window_present (GTK_WINDOW (dialog));
}

static void
baul_emblem_sidebar_drag_received_cb (GtkWidget *widget,
                                      GdkDragContext *drag_context,
                                      gint x,
                                      gint y,
                                      GtkSelectionData *data,
                                      guint info,
                                      guint time,
                                      BaulEmblemSidebar *emblem_sidebar)
{
    GSList *emblems;
    Emblem *emblem;
    GdkPixbuf *pixbuf;
    char *uri, *error;
    char **uris;
    GFile *f;
    int i;
    gboolean had_failure;
    gint data_format, data_length;
    const guchar *data_data;

    had_failure = FALSE;
    emblems = NULL;
    data_format = ctk_selection_data_get_format (data);
    data_length = ctk_selection_data_get_length (data);
    data_data = ctk_selection_data_get_data (data);

    switch (info)
    {
    case TARGET_URI_LIST:
        if (data_format != 8 ||
                data_length == 0)
        {
            g_message ("URI list had wrong format (%d) or length (%d)\n",
                       data_format, data_length);
            return;
        }

        uris = g_uri_list_extract_uris (data_data);
        if (uris == NULL)
        {
            break;
        }

        for (i = 0; uris[i] != NULL; ++i)
        {
            f = g_file_new_for_uri (uris[i]);
            pixbuf = baul_emblem_load_pixbuf_for_emblem (f);

            if (pixbuf == NULL)
            {
                /* this one apparently isn't an image, or
                 * at least not one that we know how to read
                 */
                had_failure = TRUE;
                g_object_unref (f);
                continue;
            }

            emblem = g_new (Emblem, 1);
            emblem->uri = g_file_get_uri (f);
            emblem->name = NULL; /* created later on by the user */
            emblem->keyword = NULL;
            emblem->pixbuf = pixbuf;

            g_object_unref (f);

            emblems = g_slist_prepend (emblems, emblem);
        }

        g_strfreev (uris);

        if (had_failure && emblems != NULL)
        {
            eel_show_error_dialog (_("Some of the files could not be added as emblems."), _("The emblems do not appear to be valid images."), NULL);
        }
        else if (had_failure && emblems == NULL)
        {
            eel_show_error_dialog (_("None of the files could be added as emblems."), _("The emblems do not appear to be valid images."), NULL);

        }

        if (emblems != NULL)
        {
            show_add_emblems_dialog (emblem_sidebar, emblems);
        }

        break;

    case TARGET_URI:
        if (data_format != 8 ||
                data_length == 0)
        {
            g_warning ("URI had wrong format (%d) or length (%d)\n",
                       data_format, data_length);
            return;
        }

        uri = g_strndup (data_data, data_length);

        f = g_file_new_for_uri (uri);
        pixbuf = baul_emblem_load_pixbuf_for_emblem (f);

        if (pixbuf != NULL)
        {
            emblem = g_new (Emblem, 1);
            emblem->uri = uri;
            emblem->name = NULL;
            emblem->keyword = NULL;
            emblem->pixbuf = pixbuf;

            emblems = g_slist_prepend (NULL, emblem);

            show_add_emblems_dialog (emblem_sidebar, emblems);
        }
        else
        {
            char *uri_utf8;

            uri_utf8 = g_file_get_parse_name (f);

            if (uri_utf8)
            {
                error = g_strdup_printf (_("The file '%s' does not appear to be a valid image."), uri_utf8);
            }
            else
            {
                error = g_strdup (_("The dragged file does not appear to be a valid image."));
            }
            eel_show_error_dialog (_("The emblem cannot be added."), error, NULL);
            g_free (error);
            g_free (uri_utf8);
        }

        g_object_unref (f);
        g_free (uri);

        break;

    case TARGET_NETSCAPE_URL:
        if (data_format != 8 ||
                data_length == 0)
        {
            g_message ("URI had wrong format (%d) or length (%d)\n",
                       data_format, data_length);
            return;
        }

        /* apparently, this is a URI/title pair?  or just a pair
         * of identical URIs?  Regardless, this seems to work...
         */

        uris = g_uri_list_extract_uris (data_data);
        if (uris == NULL)
        {
            break;
        }

        uri = uris[0];
        if (uri == NULL)
        {
            g_strfreev (uris);
            break;
        }

        f = g_file_new_for_uri (uri);
        pixbuf = baul_emblem_load_pixbuf_for_emblem (f);
        g_object_unref (f);

        if (pixbuf != NULL)
        {
            emblem = g_new (Emblem, 1);
            emblem->uri = g_strdup (uri);
            emblem->name = NULL;
            emblem->keyword = NULL;
            emblem->pixbuf = pixbuf;

            emblems = g_slist_prepend (NULL, emblem);

            show_add_emblems_dialog (emblem_sidebar, emblems);
        }
        else
        {
            g_warning ("Tried to load '%s', but failed.\n",
                       uri);
            error = g_strdup_printf (_("The file '%s' does not appear to be a valid image."), uri);
            eel_show_error_dialog (_("The emblem cannot be added."), error, NULL);
            g_free (error);
        }

        g_strfreev (uris);

        break;
    }
}

static GtkWidget *
baul_emblem_sidebar_create_container (BaulEmblemSidebar *emblem_sidebar)
{
    GtkWidget *emblems_table, *scroller;

    /* The emblems wrapped table */
    scroller = eel_scrolled_wrap_table_new (TRUE, GTK_SHADOW_IN, &emblems_table);

    ctk_container_set_border_width (GTK_CONTAINER (emblems_table), 8);

    /* set up dnd for adding emblems */
    ctk_drag_dest_set (scroller,
                       GTK_DEST_DEFAULT_ALL,
                       dest_types, G_N_ELEMENTS (dest_types),
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);

    g_signal_connect (scroller, "drag-data-received",
                      G_CALLBACK (baul_emblem_sidebar_drag_received_cb),
                      emblem_sidebar);

    ctk_widget_show (scroller);

    emblem_sidebar->details->emblems_table = emblems_table;

    return scroller;
}

static gint
emblem_widget_sort_func (gconstpointer a, gconstpointer b)
{
    GObject *obj_a, *obj_b;

    obj_a = G_OBJECT (a);
    obj_b = G_OBJECT (b);

    return strcmp (g_object_get_data (obj_a, "emblem-display-name"),
                   g_object_get_data (obj_b, "emblem-display-name"));
}

static void
baul_emblem_sidebar_populate (BaulEmblemSidebar *emblem_sidebar)
{
    GList *icons, *l, *widgets;
    GtkWidget *emblem_widget;
    char *path;
    GdkPixbuf *erase_pixbuf;

    erase_pixbuf = NULL;

    path = baul_pixmap_file ("erase.png");
    if (path != NULL)
    {
        erase_pixbuf = gdk_pixbuf_new_from_file (path, NULL);
    }
    g_free (path);

    if (erase_pixbuf != NULL)
    {
        emblem_widget = create_emblem_widget_with_pixbuf (emblem_sidebar,
                        ERASE_EMBLEM_KEYWORD,
                        _("Erase"),
                        erase_pixbuf);
        ctk_container_add (GTK_CONTAINER
                           (emblem_sidebar->details->emblems_table),
                           emblem_widget);
    }


    icons = baul_emblem_list_available ();

    l = icons;
    widgets = NULL;
    while (l != NULL)
    {
        char *name;

        name = (char *)l->data;
        l = l->next;

        if (!baul_emblem_should_show_in_list (name))
        {
            continue;
        }

        emblem_widget = create_emblem_widget (emblem_sidebar, name);

        widgets = g_list_prepend (widgets, emblem_widget);
    }
    g_list_free_full (icons, g_free);

    /* sort the emblems by display name */
    widgets = g_list_sort (widgets, emblem_widget_sort_func);

    l = widgets;
    while (l != NULL)
    {
        ctk_container_add
        (GTK_CONTAINER (emblem_sidebar->details->emblems_table),
         l->data);
        l = l->next;
    }
    g_list_free (widgets);

    ctk_widget_show_all (emblem_sidebar->details->emblems_table);
}

static void
baul_emblem_sidebar_init (BaulEmblemSidebar *emblem_sidebar)
{
    GtkWidget *widget;

    emblem_sidebar->details = g_new0 (BaulEmblemSidebarDetails, 1);

    create_popup_menu (emblem_sidebar);

    widget = baul_emblem_sidebar_create_container (emblem_sidebar);
    baul_emblem_sidebar_populate (emblem_sidebar);

    g_signal_connect_object (baul_signaller_get_current (),
                             "emblems_changed",
                             G_CALLBACK (emblems_changed_callback), emblem_sidebar, 0);

    ctk_orientable_set_orientation (GTK_ORIENTABLE (emblem_sidebar), GTK_ORIENTATION_VERTICAL);
    ctk_box_pack_start (GTK_BOX (emblem_sidebar), widget,
                        TRUE, TRUE, 0);
}

static void
baul_emblem_sidebar_finalize (GObject *object)
{
    BaulEmblemSidebar *emblem_sidebar;

    g_assert (BAUL_IS_EMBLEM_SIDEBAR (object));
    emblem_sidebar = BAUL_EMBLEM_SIDEBAR (object);

    if (emblem_sidebar->details != NULL)
    {
        g_free (emblem_sidebar->details);
    }

    G_OBJECT_CLASS (baul_emblem_sidebar_parent_class)->finalize (object);
}

static void
baul_emblem_sidebar_class_init (BaulEmblemSidebarClass *object_klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (object_klass);

    gobject_class->finalize = baul_emblem_sidebar_finalize;
}

static const char *
baul_emblem_sidebar_get_sidebar_id (BaulSidebar *sidebar)
{
    return BAUL_EMBLEM_SIDEBAR_ID;
}

static char *
baul_emblem_sidebar_get_tab_label (BaulSidebar *sidebar)
{
    return g_strdup (_("Emblems"));
}

static char *
baul_emblem_sidebar_get_tab_tooltip (BaulSidebar *sidebar)
{
    return g_strdup (_("Show Emblems"));
}

static GdkPixbuf *
baul_emblem_sidebar_get_tab_icon (BaulSidebar *sidebar)
{
    return NULL;
}

static void
baul_emblem_sidebar_is_visible_changed (BaulSidebar *sidebar,
                                        gboolean         is_visible)
{
    /* Do nothing */
}

static void
baul_emblem_sidebar_iface_init (BaulSidebarIface *iface)
{
    iface->get_sidebar_id = baul_emblem_sidebar_get_sidebar_id;
    iface->get_tab_label = baul_emblem_sidebar_get_tab_label;
    iface->get_tab_tooltip = baul_emblem_sidebar_get_tab_tooltip;
    iface->get_tab_icon = baul_emblem_sidebar_get_tab_icon;
    iface->is_visible_changed = baul_emblem_sidebar_is_visible_changed;
}

static void
baul_emblem_sidebar_set_parent_window (BaulEmblemSidebar *sidebar,
                                       BaulWindowInfo *window)
{
    sidebar->details->window = window;
}

static BaulSidebar *
baul_emblem_sidebar_create (BaulSidebarProvider *provider,
                            BaulWindowInfo *window)
{
    BaulEmblemSidebar *sidebar;

    sidebar = g_object_new (baul_emblem_sidebar_get_type (), NULL);
    baul_emblem_sidebar_set_parent_window (sidebar, window);
    g_object_ref_sink (sidebar);

    return BAUL_SIDEBAR (sidebar);
}

static void
sidebar_provider_iface_init (BaulSidebarProviderIface *iface)
{
    iface->create = baul_emblem_sidebar_create;
}

static void
baul_emblem_sidebar_provider_init (BaulEmblemSidebarProvider *sidebar)
{
}

static void
baul_emblem_sidebar_provider_class_init (BaulEmblemSidebarProviderClass *class)
{
}

void
baul_emblem_sidebar_register (void)
{
    baul_module_add_type (baul_emblem_sidebar_provider_get_type ());
}

