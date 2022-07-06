/*
 *  baul-property-page.h - Property pages exported by
 *                             BaulPropertyProvider objects.
 *
 *  Copyright (C) 2003 Novell, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Author:  Dave Camp <dave@ximian.com>
 *
 */

#include <config.h>
#include "baul-property-page.h"

#include "baul-extension-i18n.h"

enum {
    PROP_0,
    PROP_NAME,
    PROP_LABEL,
    PROP_PAGE,
    LAST_PROP
};

struct _BaulPropertyPageDetails {
    char *name;
    CtkWidget *label;
    CtkWidget *page;
};

static GObjectClass *parent_class = NULL;

/**
 * SECTION:baul-property-page
 * @title: BaulPropertyPage
 * @short_description: Property page descriptor object
 * @include: libbaul-extension/baul-property-page.h
 *
 * #BaulPropertyPage is an object that describes a page in the file
 * properties dialog. Extensions can provide #BaulPropertyPage objects
 * by registering a #BaulPropertyPageProvider and returning them from
 * baul_property_page_provider_get_pages(), which will be called by the
 * main application when creating file properties dialogs.
 */

/**
 * baul_property_page_new:
 * @name: the identifier for the property page
 * @label: the user-visible label of the property page
 * @page: the property page to display
 *
 * Creates a new #BaulPropertyPage from page_widget.
 *
 * Returns: a newly created #BaulPropertyPage
 */
BaulPropertyPage *
baul_property_page_new (const char *name,
                        CtkWidget *label,
                        CtkWidget *page_widget)
{
    BaulPropertyPage *page;

    g_return_val_if_fail (name != NULL, NULL);
    g_return_val_if_fail (label != NULL && GTK_IS_WIDGET (label), NULL);
    g_return_val_if_fail (page_widget != NULL && GTK_IS_WIDGET (page_widget),
                          NULL);

    page = g_object_new (BAUL_TYPE_PROPERTY_PAGE,
                         "name", name,
                         "label", label,
                         "page", page_widget,
                         NULL);

    return page;
}

static void
baul_property_page_get_property (GObject *object,
                                 guint param_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
    BaulPropertyPage *page;

    page = BAUL_PROPERTY_PAGE (object);

    switch (param_id) {
    case PROP_NAME :
        g_value_set_string (value, page->details->name);
        break;
    case PROP_LABEL :
        g_value_set_object (value, page->details->label);
        break;
    case PROP_PAGE :
        g_value_set_object (value, page->details->page);
        break;
    default :
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
baul_property_page_set_property (GObject *object,
                                 guint param_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
    BaulPropertyPage *page;

    page = BAUL_PROPERTY_PAGE (object);

    switch (param_id) {
    case PROP_NAME :
        g_free (page->details->name);
        page->details->name = g_strdup (g_value_get_string (value));
        g_object_notify (object, "name");
        break;
    case PROP_LABEL :
        if (page->details->label) {
            g_object_unref (page->details->label);
        }

        page->details->label = g_object_ref (g_value_get_object (value));
        g_object_notify (object, "label");
        break;
    case PROP_PAGE :
        if (page->details->page) {
            g_object_unref (page->details->page);
        }

        page->details->page = g_object_ref (g_value_get_object (value));
        g_object_notify (object, "page");
        break;
    default :
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
baul_property_page_dispose (GObject *object)
{
    BaulPropertyPage *page;

    page = BAUL_PROPERTY_PAGE (object);

    if (page->details->label) {
        g_object_unref (page->details->label);
        page->details->label = NULL;
    }
    if (page->details->page) {
        g_object_unref (page->details->page);
        page->details->page = NULL;
    }
}

static void
baul_property_page_finalize (GObject *object)
{
    BaulPropertyPage *page;

    page = BAUL_PROPERTY_PAGE (object);

    g_free (page->details->name);

    g_free (page->details);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
baul_property_page_instance_init (BaulPropertyPage *page)
{
    page->details = g_new0 (BaulPropertyPageDetails, 1);
}

static void
baul_property_page_class_init (BaulPropertyPageClass *class)
{
    parent_class = g_type_class_peek_parent (class);

    G_OBJECT_CLASS (class)->finalize = baul_property_page_finalize;
    G_OBJECT_CLASS (class)->dispose = baul_property_page_dispose;
    G_OBJECT_CLASS (class)->get_property = baul_property_page_get_property;
    G_OBJECT_CLASS (class)->set_property = baul_property_page_set_property;

    g_object_class_install_property (G_OBJECT_CLASS (class),
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                             "Name",
                                             "Name of the page",
                                             NULL,
                                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_READABLE));
    g_object_class_install_property (G_OBJECT_CLASS (class),
                                     PROP_LABEL,
                                     g_param_spec_object ("label",
                                             "Label",
                                             "Label widget to display in the notebook tab",
                                             GTK_TYPE_WIDGET,
                                             G_PARAM_READWRITE));
    g_object_class_install_property (G_OBJECT_CLASS (class),
                                     PROP_PAGE,
                                     g_param_spec_object ("page",
                                             "Page",
                                             "Widget for the property page",
                                             GTK_TYPE_WIDGET,
                                             G_PARAM_READWRITE));
}

GType
baul_property_page_get_type (void)
{
    static GType type = 0;

    if (!type) {
        const GTypeInfo info = {
            sizeof (BaulPropertyPageClass),
            NULL,
            NULL,
            (GClassInitFunc)baul_property_page_class_init,
            NULL,
            NULL,
            sizeof (BaulPropertyPage),
            0,
            (GInstanceInitFunc)baul_property_page_instance_init
        };

        type = g_type_register_static
               (G_TYPE_OBJECT,
                "BaulPropertyPage",
                &info, 0);
    }

    return type;
}
