/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-labeled-image.c - A labeled image.

   Copyright (C) 2000 Eazel, Inc.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Cafe Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

#include <config.h>
#include "eel-labeled-image.h"

#include "eel-art-extensions.h"
#include "eel-art-ctk-extensions.h"
#include "eel-ctk-container.h"
#include "eel-ctk-extensions.h"
#include "eel-accessibility.h"
#include <ctk/ctk.h>
#include <ctk/ctk-a11y.h>
#include <cdk/cdkkeysyms.h>
#include <atk/atkimage.h>

#define DEFAULT_SPACING 0
#define DEFAULT_X_PADDING 0
#define DEFAULT_Y_PADDING 0
#define DEFAULT_X_ALIGNMENT 0.5
#define DEFAULT_Y_ALIGNMENT 0.5

/* Signals */
enum
{
    ACTIVATE,
    LAST_SIGNAL
};

/* Arguments */
enum
{
    PROP_0,
    PROP_FILL,
    PROP_LABEL,
    PROP_LABEL_POSITION,
    PROP_PIXBUF,
    PROP_SHOW_IMAGE,
    PROP_SHOW_LABEL,
    PROP_SPACING,
    PROP_X_ALIGNMENT,
    PROP_X_PADDING,
    PROP_Y_ALIGNMENT,
    PROP_Y_PADDING
};

/* Detail member struct */
struct EelLabeledImagePrivate
{
    CtkWidget *image;
    CtkWidget *label;
    CtkPositionType label_position;
    gboolean show_label;
    gboolean show_image;
    guint spacing;
    float x_alignment;
    float y_alignment;
    int x_padding;
    int y_padding;
    int fixed_image_height;
    gboolean fill;
};

/* derived types so we can add our accessibility interfaces */
static GType         eel_labeled_image_button_get_type        (void);
static GType         eel_labeled_image_check_button_get_type  (void);
static GType         eel_labeled_image_radio_button_get_type  (void);
static GType         eel_labeled_image_toggle_button_get_type (void);

/* CtkWidgetClass methods */
static GType eel_labeled_image_accessible_get_type (void);

/* Private EelLabeledImage methods */
static EelDimensions labeled_image_get_image_dimensions   (const EelLabeledImage *labeled_image);
static EelDimensions labeled_image_get_label_dimensions   (const EelLabeledImage *labeled_image);
static void          labeled_image_ensure_label           (EelLabeledImage       *labeled_image);
static void          labeled_image_ensure_image           (EelLabeledImage       *labeled_image);
static EelIRect      labeled_image_get_content_bounds     (const EelLabeledImage *labeled_image);
static EelDimensions labeled_image_get_content_dimensions (const EelLabeledImage *labeled_image);
static void          labeled_image_update_alignments      (EelLabeledImage       *labeled_image);
static gboolean      labeled_image_show_label             (const EelLabeledImage *labeled_image);
static gboolean      labeled_image_show_image             (const EelLabeledImage *labeled_image);

static guint labeled_image_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (EelLabeledImage, eel_labeled_image, CTK_TYPE_CONTAINER)

static void
eel_labeled_image_init (EelLabeledImage *labeled_image)
{
    ctk_widget_set_has_window (CTK_WIDGET (labeled_image), FALSE);

    labeled_image->details = eel_labeled_image_get_instance_private (labeled_image);
    labeled_image->details->show_label = TRUE;
    labeled_image->details->show_image = TRUE;
    labeled_image->details->label_position = CTK_POS_BOTTOM;
    labeled_image->details->spacing = DEFAULT_SPACING;
    labeled_image->details->x_padding = DEFAULT_X_PADDING;
    labeled_image->details->y_padding = DEFAULT_Y_PADDING;
    labeled_image->details->x_alignment = DEFAULT_X_ALIGNMENT;
    labeled_image->details->y_alignment = DEFAULT_Y_ALIGNMENT;
    labeled_image->details->fixed_image_height = 0;

    eel_labeled_image_set_fill (labeled_image, FALSE);
}

static void
eel_labeled_image_destroy (CtkWidget *object)
{
    EelLabeledImage *labeled_image;

    labeled_image = EEL_LABELED_IMAGE (object);

    if (labeled_image->details->image != NULL)
    {
        ctk_widget_destroy (labeled_image->details->image);
    }

    if (labeled_image->details->label != NULL)
    {
        ctk_widget_destroy (labeled_image->details->label);
    }

    CTK_WIDGET_CLASS (eel_labeled_image_parent_class)->destroy (object);
}

/* GObjectClass methods */
static void
eel_labeled_image_set_property (GObject      *object,
				guint         property_id,
				const GValue *value,
				GParamSpec   *pspec G_GNUC_UNUSED)
{
    EelLabeledImage *labeled_image;

    g_assert (EEL_IS_LABELED_IMAGE (object));

    labeled_image = EEL_LABELED_IMAGE (object);

    switch (property_id)
    {
    case PROP_PIXBUF:
        eel_labeled_image_set_pixbuf (labeled_image,
                                      g_value_get_object (value));
        break;

    case PROP_LABEL:
        eel_labeled_image_set_text (labeled_image, g_value_get_string (value));
        break;

    case PROP_LABEL_POSITION:
        eel_labeled_image_set_label_position (labeled_image,
                                              g_value_get_enum (value));
        break;

    case PROP_SHOW_LABEL:
        eel_labeled_image_set_show_label (labeled_image,
                                          g_value_get_boolean (value));
        break;

    case PROP_SHOW_IMAGE:
        eel_labeled_image_set_show_image (labeled_image,
                                          g_value_get_boolean (value));
        break;

    case PROP_SPACING:
        eel_labeled_image_set_spacing (labeled_image,
                                       g_value_get_uint (value));
        break;

    case PROP_X_PADDING:
        eel_labeled_image_set_x_padding (labeled_image,
                                         g_value_get_int (value));
        break;

    case PROP_Y_PADDING:
        eel_labeled_image_set_y_padding (labeled_image,
                                         g_value_get_int (value));
        break;

    case PROP_X_ALIGNMENT:
        eel_labeled_image_set_x_alignment (labeled_image,
                                           g_value_get_float (value));
        break;

    case PROP_Y_ALIGNMENT:
        eel_labeled_image_set_y_alignment (labeled_image,
                                           g_value_get_float (value));
        break;

    case PROP_FILL:
        eel_labeled_image_set_fill (labeled_image,
                                    g_value_get_boolean (value));
        break;
    default:
        g_assert_not_reached ();
    }
}

static void
eel_labeled_image_get_property (GObject    *object,
				guint       property_id,
				GValue     *value,
				GParamSpec *pspec G_GNUC_UNUSED)
{
    EelLabeledImage *labeled_image;

    g_assert (EEL_IS_LABELED_IMAGE (object));

    labeled_image = EEL_LABELED_IMAGE (object);

    switch (property_id)
    {
    case PROP_LABEL:
        if (labeled_image->details->label == NULL)
        {
            g_value_set_string (value, NULL);
        }
        else
        {
            g_value_set_string (value,
                                ctk_label_get_text (CTK_LABEL (
                                        labeled_image->details->label)));
        }
        break;

    case PROP_LABEL_POSITION:
        g_value_set_enum (value, eel_labeled_image_get_label_position (labeled_image));
        break;

    case PROP_SHOW_LABEL:
        g_value_set_boolean (value, eel_labeled_image_get_show_label (labeled_image));
        break;

    case PROP_SHOW_IMAGE:
        g_value_set_boolean (value, eel_labeled_image_get_show_image (labeled_image));
        break;

    case PROP_SPACING:
        g_value_set_uint (value, eel_labeled_image_get_spacing (labeled_image));
        break;

    case PROP_X_PADDING:
        g_value_set_int (value, eel_labeled_image_get_x_padding (labeled_image));
        break;

    case PROP_Y_PADDING:
        g_value_set_int (value, eel_labeled_image_get_y_padding (labeled_image));
        break;

    case PROP_X_ALIGNMENT:
        g_value_set_float (value, eel_labeled_image_get_x_alignment (labeled_image));
        break;

    case PROP_Y_ALIGNMENT:
        g_value_set_float (value, eel_labeled_image_get_y_alignment (labeled_image));
        break;

    case PROP_FILL:
        g_value_set_boolean (value, eel_labeled_image_get_fill (labeled_image));
        break;

    default:
        g_assert_not_reached ();
    }
}

/* CtkWidgetClass methods */
static void
eel_labeled_image_size_request (CtkWidget *widget,
                                CtkRequisition *requisition)
{
    EelLabeledImage *labeled_image;
    EelDimensions content_dimensions;

    g_assert (EEL_IS_LABELED_IMAGE (widget));
    g_assert (requisition != NULL);

    labeled_image = EEL_LABELED_IMAGE (widget);

    content_dimensions = labeled_image_get_content_dimensions (labeled_image);

    requisition->width =
        MAX (1, content_dimensions.width) +
        2 * labeled_image->details->x_padding;

    requisition->height =
        MAX (1, content_dimensions.height) +
        2 * labeled_image->details->y_padding;
}

static void
eel_labeled_image_get_preferred_width (CtkWidget *widget,
                                       gint *minimum_width,
                                       gint *natural_width)
{
    CtkRequisition req;
    eel_labeled_image_size_request (widget, &req);
    *minimum_width = *natural_width = req.width;
}

static void
eel_labeled_image_get_preferred_height (CtkWidget *widget,
                                        gint *minimum_height,
                                        gint *natural_height)
{
    CtkRequisition req;
    eel_labeled_image_size_request (widget, &req);
    *minimum_height = *natural_height = req.height;
}

static void
eel_labeled_image_size_allocate (CtkWidget *widget,
                                 CtkAllocation *allocation)
{
    EelLabeledImage *labeled_image;
    EelIRect image_bounds;
    EelIRect label_bounds;

    g_assert (EEL_IS_LABELED_IMAGE (widget));
    g_assert (allocation != NULL);

    labeled_image = EEL_LABELED_IMAGE (widget);

    ctk_widget_set_allocation (widget, allocation);

    label_bounds = eel_labeled_image_get_label_bounds (labeled_image);
    eel_ctk_container_child_size_allocate (CTK_CONTAINER (widget),
                                           labeled_image->details->label,
                                           label_bounds);

    image_bounds = eel_labeled_image_get_image_bounds (labeled_image);
    eel_ctk_container_child_size_allocate (CTK_CONTAINER (widget),
                                           labeled_image->details->image,
                                           image_bounds);
}

static int
eel_labeled_image_draw (CtkWidget *widget,
                        cairo_t *cr)
{
    EelLabeledImage *labeled_image;
    EelIRect label_bounds;
    CtkStyleContext *context;

    g_assert (EEL_IS_LABELED_IMAGE (widget));
    g_assert (ctk_widget_get_realized (widget));

    labeled_image = EEL_LABELED_IMAGE (widget);

    context = ctk_widget_get_style_context (widget);
    ctk_style_context_save (context);

    if (ctk_widget_get_state_flags (widget) == CTK_STATE_FLAG_SELECTED ||
            ctk_widget_get_state_flags (widget) == CTK_STATE_FLAG_ACTIVE)
    {
        label_bounds = eel_labeled_image_get_label_bounds (EEL_LABELED_IMAGE (widget));

        ctk_widget_get_state_flags (widget);
        ctk_render_background (context,
                              cr,
                              label_bounds.x0, label_bounds.y0,
                              label_bounds.x1 - label_bounds.x0,
                              label_bounds.y1 - label_bounds.y0);

        ctk_render_frame (context,
                          cr,
                          label_bounds.x0, label_bounds.y0,
                          label_bounds.x1 - label_bounds.x0,
                          label_bounds.y1 - label_bounds.y0);
    }

    if (labeled_image_show_label (labeled_image))
    {
        eel_ctk_container_child_expose_event (CTK_CONTAINER (widget),
                                              labeled_image->details->label,
                                              cr);
    }

    if (labeled_image_show_image (labeled_image))
    {
        eel_ctk_container_child_expose_event (CTK_CONTAINER (widget),
                                              labeled_image->details->image,
                                              cr);
    }

    if (ctk_widget_has_focus (widget))
    {
        label_bounds = eel_labeled_image_get_image_bounds (EEL_LABELED_IMAGE (widget));
        ctk_widget_set_state_flags (widget, CTK_STATE_FLAG_NORMAL, TRUE);
        ctk_render_focus (context,
                          cr,
                          label_bounds.x0, label_bounds.y0,
                          label_bounds.x1 - label_bounds.x0,
                          label_bounds.y1 - label_bounds.y0);
    }

    ctk_style_context_restore (context);

    return FALSE;
}

static void
eel_labeled_image_map (CtkWidget *widget)
{
    EelLabeledImage *labeled_image;

    g_assert (EEL_IS_LABELED_IMAGE (widget));

    labeled_image = EEL_LABELED_IMAGE (widget);

    ctk_widget_set_mapped (widget, TRUE);

    if (labeled_image_show_label (labeled_image))
    {
        eel_ctk_container_child_map (CTK_CONTAINER (widget), labeled_image->details->label);
    }

    if (labeled_image_show_image (labeled_image))
    {
        eel_ctk_container_child_map (CTK_CONTAINER (widget), labeled_image->details->image);
    }
}

static void
eel_labeled_image_unmap (CtkWidget *widget)
{
    EelLabeledImage *labeled_image;

    g_assert (EEL_IS_LABELED_IMAGE (widget));

    labeled_image = EEL_LABELED_IMAGE (widget);

    ctk_widget_set_mapped (widget, FALSE);

    eel_ctk_container_child_unmap (CTK_CONTAINER (widget), labeled_image->details->label);
    eel_ctk_container_child_unmap (CTK_CONTAINER (widget), labeled_image->details->image);
}

/* CtkContainerClass methods */
static void
eel_labeled_image_add (CtkContainer *container,
                       CtkWidget *child)
{
    g_assert (CTK_IS_LABEL (child) || CTK_IS_IMAGE (child));

    eel_ctk_container_child_add (container, child);
}

static void
eel_labeled_image_remove (CtkContainer *container,
                          CtkWidget *child)
{
    EelLabeledImage *labeled_image;

    g_assert (CTK_IS_LABEL (child) || CTK_IS_IMAGE (child));

    labeled_image = EEL_LABELED_IMAGE (container);;

    g_assert (child == labeled_image->details->image || child == labeled_image->details->label);

    eel_ctk_container_child_remove (container, child);

    if (labeled_image->details->image == child)
    {
        labeled_image->details->image = NULL;
    }

    if (labeled_image->details->label == child)
    {
        labeled_image->details->label = NULL;
    }
}

static void
eel_labeled_image_forall (CtkContainer *container,
                          gboolean include_internals,
                          CtkCallback callback,
                          gpointer callback_data)
{
    EelLabeledImage *labeled_image;

    g_assert (EEL_IS_LABELED_IMAGE (container));
    g_assert (callback != NULL);

    labeled_image = EEL_LABELED_IMAGE (container);

    if (include_internals)
    {
        if (labeled_image->details->image != NULL)
        {
            (* callback) (labeled_image->details->image, callback_data);
        }

        if (labeled_image->details->label != NULL)
        {
            (* callback) (labeled_image->details->label, callback_data);
        }
    }
}

/* Class init methods */
static void
eel_labeled_image_class_init (EelLabeledImageClass *labeled_image_class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (labeled_image_class);
    CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (labeled_image_class);
    CtkContainerClass *container_class = CTK_CONTAINER_CLASS (labeled_image_class);
    CtkBindingSet *binding_set;

    /* GObjectClass */
    gobject_class->set_property = eel_labeled_image_set_property;
    gobject_class->get_property = eel_labeled_image_get_property;

    widget_class->destroy = eel_labeled_image_destroy;


    /* CtkWidgetClass */
    widget_class->size_allocate = eel_labeled_image_size_allocate;
    widget_class->get_preferred_width = eel_labeled_image_get_preferred_width;
    widget_class->get_preferred_height = eel_labeled_image_get_preferred_height;
    widget_class->draw = eel_labeled_image_draw;

    widget_class->map = eel_labeled_image_map;
    widget_class->unmap = eel_labeled_image_unmap;

    ctk_widget_class_set_accessible_type (widget_class, eel_labeled_image_accessible_get_type ());


    /* CtkContainerClass */
    container_class->add = eel_labeled_image_add;
    container_class->remove = eel_labeled_image_remove;
    container_class->forall = eel_labeled_image_forall;

    labeled_image_signals[ACTIVATE] =
    g_signal_new ("activate",
                  G_TYPE_FROM_CLASS (labeled_image_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (EelLabeledImageClass,
                                   activate),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
    widget_class->activate_signal = labeled_image_signals[ACTIVATE];

    binding_set = ctk_binding_set_by_class (gobject_class);

    ctk_binding_entry_add_signal (binding_set,
                                  CDK_KEY_Return, 0,
                                  "activate", 0);
    ctk_binding_entry_add_signal (binding_set,
                                  CDK_KEY_KP_Enter, 0,
                                  "activate", 0);
    ctk_binding_entry_add_signal (binding_set,
                                  CDK_KEY_space, 0,
                                  "activate", 0);


    /* Properties */
    g_object_class_install_property (
        gobject_class,
        PROP_PIXBUF,
        g_param_spec_object ("pixbuf", NULL, NULL,
                             GDK_TYPE_PIXBUF, G_PARAM_READWRITE));

    g_object_class_install_property (
        gobject_class,
        PROP_LABEL,
        g_param_spec_string ("label", NULL, NULL,
                             "", G_PARAM_READWRITE));


    g_object_class_install_property (
        gobject_class,
        PROP_LABEL_POSITION,
        g_param_spec_enum ("label_position", NULL, NULL,
                           CTK_TYPE_POSITION_TYPE,
                           CTK_POS_BOTTOM,
                           G_PARAM_READWRITE));

    g_object_class_install_property (
        gobject_class,
        PROP_SHOW_LABEL,
        g_param_spec_boolean ("show_label", NULL, NULL,
                              TRUE, G_PARAM_READWRITE));

    g_object_class_install_property (
        gobject_class,
        PROP_SHOW_IMAGE,
        g_param_spec_boolean ("show_image", NULL, NULL,
                              TRUE, G_PARAM_READWRITE));


    g_object_class_install_property (
        gobject_class,
        PROP_SPACING,
        g_param_spec_uint ("spacing", NULL, NULL,
                           0,
                           G_MAXINT,
                           DEFAULT_SPACING,
                           G_PARAM_READWRITE));

    g_object_class_install_property (
        gobject_class,
        PROP_X_PADDING,
        g_param_spec_int ("x_padding", NULL, NULL,
                          0,
                          G_MAXINT,
                          DEFAULT_X_PADDING,
                          G_PARAM_READWRITE));

    g_object_class_install_property (
        gobject_class,
        PROP_Y_PADDING,
        g_param_spec_int ("y_padding", NULL, NULL,
                          0,
                          G_MAXINT,
                          DEFAULT_Y_PADDING,
                          G_PARAM_READWRITE));

    g_object_class_install_property (
        gobject_class,
        PROP_X_ALIGNMENT,
        g_param_spec_float ("x_alignment", NULL, NULL,
                            0.0,
                            1.0,
                            DEFAULT_X_ALIGNMENT,
                            G_PARAM_READWRITE));

    g_object_class_install_property (
        gobject_class,
        PROP_Y_ALIGNMENT,
        g_param_spec_float ("y_alignment", NULL, NULL,
                            0.0,
                            1.0,
                            DEFAULT_Y_ALIGNMENT,
                            G_PARAM_READWRITE));

    g_object_class_install_property (
        gobject_class,
        PROP_FILL,
        g_param_spec_boolean ("fill", NULL, NULL,
                              FALSE,
                              G_PARAM_READWRITE));

}

/* Private EelLabeledImage methods */
static gboolean
is_fixed_height (const EelLabeledImage *labeled_image)
{
    return labeled_image->details->fixed_image_height > 0;
}

static EelDimensions
labeled_image_get_image_dimensions (const EelLabeledImage *labeled_image)
{
    EelDimensions image_dimensions;
    CtkRequisition image_requisition;

    g_assert (EEL_IS_LABELED_IMAGE (labeled_image));

    if (!labeled_image_show_image (labeled_image))
    {
        return eel_dimensions_empty;
    }

    ctk_widget_get_preferred_size (labeled_image->details->image, &image_requisition, NULL);

    image_dimensions.width = (int) image_requisition.width;
    image_dimensions.height = (int) image_requisition.height;

    if (is_fixed_height (labeled_image))
    {
        image_dimensions.height = labeled_image->details->fixed_image_height;
    }

    return image_dimensions;
}

static EelDimensions
labeled_image_get_label_dimensions (const EelLabeledImage *labeled_image)
{
    EelDimensions label_dimensions;
    CtkRequisition label_requisition;

    g_assert (EEL_IS_LABELED_IMAGE (labeled_image));

    if (!labeled_image_show_label (labeled_image))
    {
        return eel_dimensions_empty;
    }

    ctk_widget_get_preferred_size (labeled_image->details->label, &label_requisition, NULL);

    label_dimensions.width = (int) label_requisition.width;
    label_dimensions.height = (int) label_requisition.height;

    return label_dimensions;
}

static EelIRect
labeled_image_get_image_bounds_fill (const EelLabeledImage *labeled_image)
{
    EelIRect image_bounds;
    EelDimensions image_dimensions;
    EelIRect content_bounds;
    EelIRect bounds;

    g_assert (EEL_IS_LABELED_IMAGE (labeled_image));

    image_dimensions = labeled_image_get_image_dimensions (labeled_image);

    if (eel_dimensions_are_empty (image_dimensions))
    {
        return eel_irect_empty;
    }

    content_bounds = labeled_image_get_content_bounds (labeled_image);
    bounds = eel_ctk_widget_get_bounds (CTK_WIDGET (labeled_image));

    if (!labeled_image_show_label (labeled_image))
    {
        image_bounds = bounds;
    }
    else
    {
        switch (labeled_image->details->label_position)
        {
        case CTK_POS_LEFT:
            image_bounds.y0 = bounds.y0;
            image_bounds.x0 = content_bounds.x1 - image_dimensions.width;
            image_bounds.y1 = bounds.y1;
            image_bounds.x1 = bounds.x1;
            break;

        case CTK_POS_RIGHT:
            image_bounds.y0 = bounds.y0;
            image_bounds.x0 = bounds.x0;
            image_bounds.y1 = bounds.y1;
            image_bounds.x1 = content_bounds.x0 + image_dimensions.width;
            break;

        case CTK_POS_TOP:
            image_bounds.x0 = bounds.x0;
            image_bounds.y0 = content_bounds.y1 - image_dimensions.height;
            image_bounds.x1 = bounds.x1;
            image_bounds.y1 = bounds.y1;
            break;

        case CTK_POS_BOTTOM:
            image_bounds.x0 = bounds.x0;
            image_bounds.y0 = bounds.y0;
            image_bounds.x1 = bounds.x1;
            image_bounds.y1 = content_bounds.y0 + image_dimensions.height;
            break;

        default:
            image_bounds.x0 = 0;
            image_bounds.y0 = 0;
            image_bounds.x1 = 0;
            image_bounds.y1 = 0;
            g_assert_not_reached ();
        }
    }

    return image_bounds;
}

EelIRect
eel_labeled_image_get_image_bounds (const EelLabeledImage *labeled_image)
{
    EelDimensions image_dimensions;
    EelDimensions label_dimensions;
    CtkRequisition image_requisition;
    EelIRect image_bounds;
    EelIRect content_bounds;

    g_return_val_if_fail (EEL_IS_LABELED_IMAGE (labeled_image), eel_irect_empty);

    if (labeled_image->details->fill)
    {
        return labeled_image_get_image_bounds_fill (labeled_image);
    }

    /* get true real dimensions if we're in fixed height mode */
    if (is_fixed_height (labeled_image) && labeled_image_show_image (labeled_image))
    {
        ctk_widget_get_preferred_size (labeled_image->details->image, &image_requisition, NULL);
        image_dimensions.width = (int) image_requisition.width;
        image_dimensions.height = (int) image_requisition.height;
    }
    else
    {
        image_dimensions = labeled_image_get_image_dimensions (labeled_image);
    }

    label_dimensions = labeled_image_get_label_dimensions (labeled_image);

    if (eel_dimensions_are_empty (image_dimensions))
    {
        return eel_irect_empty;
    }

    content_bounds = labeled_image_get_content_bounds (labeled_image);

    if (!labeled_image_show_label (labeled_image))
    {
        image_bounds.x0 =
            content_bounds.x0 +
            (eel_irect_get_width (content_bounds) - image_dimensions.width) / 2;
        image_bounds.y0 =
            content_bounds.y0 +
            (eel_irect_get_height (content_bounds) - image_dimensions.height) / 2;
    }
    else
    {
        switch (labeled_image->details->label_position)
        {
        case CTK_POS_LEFT:
            image_bounds.x0 = content_bounds.x1 - image_dimensions.width;
            image_bounds.y0 =
                content_bounds.y0 +
                (eel_irect_get_height (content_bounds) - image_dimensions.height) / 2;
            break;

        case CTK_POS_RIGHT:
            image_bounds.x0 = content_bounds.x0;
            image_bounds.y0 =
                content_bounds.y0 +
                (eel_irect_get_height (content_bounds) - image_dimensions.height) / 2;
            break;

        case CTK_POS_TOP:
            image_bounds.x0 =
                content_bounds.x0 +
                (eel_irect_get_width (content_bounds) - image_dimensions.width) / 2;
            image_bounds.y0 = content_bounds.y1 - image_dimensions.height;
            break;

        case CTK_POS_BOTTOM:
            image_bounds.x0 =
                content_bounds.x0 +
                (eel_irect_get_width (content_bounds) - image_dimensions.width) / 2;

            if (is_fixed_height (labeled_image))
            {
                image_bounds.y0 = content_bounds.y0 + eel_irect_get_height (content_bounds)
                                  - image_dimensions.height
                                  - label_dimensions.height
                                  - labeled_image->details->spacing;
            }
            else
            {
                image_bounds.y0 = content_bounds.y0;
            }

            break;

        default:
            image_bounds.x0 = 0;
            image_bounds.y0 = 0;
            g_assert_not_reached ();
        }
    }

    image_bounds.x1 = image_bounds.x0 + image_dimensions.width;
    image_bounds.y1 = image_bounds.y0 + image_dimensions.height;

    return image_bounds;
}

static EelIRect
labeled_image_get_label_bounds_fill (const EelLabeledImage *labeled_image)
{
    EelIRect label_bounds;
    EelDimensions label_dimensions;
    EelIRect content_bounds;
    EelIRect bounds;

    g_assert (EEL_IS_LABELED_IMAGE (labeled_image));

    label_dimensions = labeled_image_get_label_dimensions (labeled_image);

    if (eel_dimensions_are_empty (label_dimensions))
    {
        return eel_irect_empty;
    }

    content_bounds = labeled_image_get_content_bounds (labeled_image);
    bounds = eel_ctk_widget_get_bounds (CTK_WIDGET (labeled_image));

    /* Only the label is shown */
    if (!labeled_image_show_image (labeled_image))
    {
        label_bounds = bounds;
        /* Both label and image are shown */
    }
    else
    {
        switch (labeled_image->details->label_position)
        {
        case CTK_POS_LEFT:
            label_bounds.y0 = bounds.y0;
            label_bounds.x0 = bounds.x0;
            label_bounds.y1 = bounds.y1;
            label_bounds.x1 = content_bounds.x0 + label_dimensions.width;
            break;

        case CTK_POS_RIGHT:
            label_bounds.y0 = bounds.y0;
            label_bounds.x0 = content_bounds.x1 - label_dimensions.width;
            label_bounds.y1 = bounds.y1;
            label_bounds.x1 = bounds.x1;
            break;

        case CTK_POS_TOP:
            label_bounds.x0 = bounds.x0;
            label_bounds.y0 = bounds.y0;
            label_bounds.x1 = bounds.x1;
            label_bounds.y1 = content_bounds.y0 + label_dimensions.height;
            break;

        case CTK_POS_BOTTOM:
            label_bounds.x0 = bounds.x0;
            label_bounds.y0 = content_bounds.y1 - label_dimensions.height;
            label_bounds.x1 = bounds.x1;
            label_bounds.y1 = bounds.y1;
            break;

        default:
            label_bounds.x0 = 0;
            label_bounds.y0 = 0;
            label_bounds.x1 = 0;
            label_bounds.y1 = 0;
            g_assert_not_reached ();
        }
    }

    return label_bounds;
}

EelIRect
eel_labeled_image_get_label_bounds (const EelLabeledImage *labeled_image)
{
    EelIRect label_bounds;
    EelDimensions label_dimensions;
    EelIRect content_bounds;

    g_return_val_if_fail (EEL_IS_LABELED_IMAGE (labeled_image), eel_irect_empty);

    if (labeled_image->details->fill)
    {
        return labeled_image_get_label_bounds_fill (labeled_image);
    }

    label_dimensions = labeled_image_get_label_dimensions (labeled_image);

    if (eel_dimensions_are_empty (label_dimensions))
    {
        return eel_irect_empty;
    }

    content_bounds = labeled_image_get_content_bounds (labeled_image);

    /* Only the label is shown */
    if (!labeled_image_show_image (labeled_image))
    {
        label_bounds.x0 =
            content_bounds.x0 +
            (eel_irect_get_width (content_bounds) - label_dimensions.width) / 2;
        label_bounds.y0 =
            content_bounds.y0 +
            (eel_irect_get_height (content_bounds) - label_dimensions.height) / 2;
        /* Both label and image are shown */
    }
    else
    {
        switch (labeled_image->details->label_position)
        {
        case CTK_POS_LEFT:
            label_bounds.x0 = content_bounds.x0;
            label_bounds.y0 =
                content_bounds.y0 +
                (eel_irect_get_height (content_bounds) - label_dimensions.height) / 2;
            break;

        case CTK_POS_RIGHT:
            label_bounds.x0 = content_bounds.x1 - label_dimensions.width;
            label_bounds.y0 =
                content_bounds.y0 +
                (eel_irect_get_height (content_bounds) - label_dimensions.height) / 2;
            break;

        case CTK_POS_TOP:
            label_bounds.x0 =
                content_bounds.x0 +
                (eel_irect_get_width (content_bounds) - label_dimensions.width) / 2;
            label_bounds.y0 = content_bounds.y0;
            break;

        case CTK_POS_BOTTOM:
            label_bounds.x0 =
                content_bounds.x0 +
                (eel_irect_get_width (content_bounds) - label_dimensions.width) / 2;
            label_bounds.y0 = content_bounds.y1 - label_dimensions.height;
            break;

        default:
            label_bounds.x0 = 0;
            label_bounds.y0 = 0;
            g_assert_not_reached ();
        }
    }

    label_bounds.x1 = label_bounds.x0 + label_dimensions.width;
    label_bounds.y1 = label_bounds.y0 + label_dimensions.height;

    return label_bounds;
}

static void
labeled_image_update_alignments (EelLabeledImage *labeled_image)
{

    g_assert (EEL_IS_LABELED_IMAGE (labeled_image));

    if (labeled_image->details->label != NULL)
    {
        if (labeled_image->details->fill)
        {
            float x_alignment;
            float y_alignment;

            x_alignment = ctk_label_get_xalign (CTK_LABEL (labeled_image->details->label));
            y_alignment = ctk_label_get_yalign (CTK_LABEL (labeled_image->details->label));

            /* Only the label is shown */
            if (!labeled_image_show_image (labeled_image))
            {
                x_alignment = 0.5;
                y_alignment = 0.5;
                /* Both label and image are shown */
            }
            else
            {
                switch (labeled_image->details->label_position)
                {
                case CTK_POS_LEFT:
                    x_alignment = 1.0;
                    y_alignment = 0.5;
                    break;

                case CTK_POS_RIGHT:
                    x_alignment = 0.0;
                    y_alignment = 0.5;
                    break;

                case CTK_POS_TOP:
                    x_alignment = 0.5;
                    y_alignment = 1.0;
                    break;

                case CTK_POS_BOTTOM:
                    x_alignment = 0.5;
                    y_alignment = 0.0;
                    break;
                }

            }

            ctk_label_set_xalign (CTK_LABEL (labeled_image->details->label), x_alignment);
            ctk_label_set_yalign (CTK_LABEL (labeled_image->details->label), y_alignment);
        }
    }

    if (labeled_image->details->image != NULL)
    {
        if (labeled_image->details->fill)
        {
            float x_alignment;
            float y_alignment;

            x_alignment = ctk_widget_get_halign (labeled_image->details->image);
            y_alignment = ctk_widget_get_valign (labeled_image->details->image);

            /* Only the image is shown */
            if (!labeled_image_show_label (labeled_image))
            {
                x_alignment = 0.5;
                y_alignment = 0.5;
                /* Both label and image are shown */
            }
            else
            {
                switch (labeled_image->details->label_position)
                {
                case CTK_POS_LEFT:
                    x_alignment = 0.0;
                    y_alignment = 0.5;
                    break;

                case CTK_POS_RIGHT:
                    x_alignment = 1.0;
                    y_alignment = 0.5;
                    break;

                case CTK_POS_TOP:
                    x_alignment = 0.5;
                    y_alignment = 0.0;
                    break;

                case CTK_POS_BOTTOM:
                    x_alignment = 0.5;
                    y_alignment = 1.0;
                    break;
                }
            }

            ctk_widget_set_halign (labeled_image->details->image, x_alignment);
            ctk_widget_set_valign (labeled_image->details->image, y_alignment);
        }
    }
}

static EelDimensions
labeled_image_get_content_dimensions (const EelLabeledImage *labeled_image)
{
    EelDimensions image_dimensions;
    EelDimensions label_dimensions;
    EelDimensions content_dimensions;

    g_assert (EEL_IS_LABELED_IMAGE (labeled_image));

    image_dimensions = labeled_image_get_image_dimensions (labeled_image);
    label_dimensions = labeled_image_get_label_dimensions (labeled_image);

    content_dimensions = eel_dimensions_empty;

    /* Both shown */
    if (!eel_dimensions_are_empty (image_dimensions) && !eel_dimensions_are_empty (label_dimensions))
    {
        content_dimensions.width =
            image_dimensions.width + labeled_image->details->spacing + label_dimensions.width;
        switch (labeled_image->details->label_position)
        {
        case CTK_POS_LEFT:
        case CTK_POS_RIGHT:
            content_dimensions.width =
                image_dimensions.width + labeled_image->details->spacing + label_dimensions.width;
            content_dimensions.height = MAX (image_dimensions.height, label_dimensions.height);
            break;

        case CTK_POS_TOP:
        case CTK_POS_BOTTOM:
            content_dimensions.width = MAX (image_dimensions.width, label_dimensions.width);
            content_dimensions.height =
                image_dimensions.height + labeled_image->details->spacing + label_dimensions.height;
            break;
        }
        /* Only image shown */
    }
    else if (!eel_dimensions_are_empty (image_dimensions))
    {
        content_dimensions.width = image_dimensions.width;
        content_dimensions.height = image_dimensions.height;
        /* Only label shown */
    }
    else
    {
        content_dimensions.width = label_dimensions.width;
        content_dimensions.height = label_dimensions.height;
    }

    return content_dimensions;
}

static EelIRect
labeled_image_get_content_bounds (const EelLabeledImage *labeled_image)
{
    EelDimensions content_dimensions;
    EelIRect content_bounds;
    EelIRect bounds;

    g_assert (EEL_IS_LABELED_IMAGE (labeled_image));

    bounds = eel_ctk_widget_get_bounds (CTK_WIDGET (labeled_image));

    content_dimensions = labeled_image_get_content_dimensions (labeled_image);
    content_bounds = eel_irect_align (bounds,
                                      content_dimensions.width,
                                      content_dimensions.height,
                                      labeled_image->details->x_alignment,
                                      labeled_image->details->y_alignment);

    return content_bounds;
}

static void
labeled_image_ensure_label (EelLabeledImage *labeled_image)
{
    g_assert (EEL_IS_LABELED_IMAGE (labeled_image));

    if (labeled_image->details->label != NULL)
    {
        return;
    }

    labeled_image->details->label = ctk_label_new (NULL);
    ctk_container_add (CTK_CONTAINER (labeled_image), labeled_image->details->label);
    ctk_widget_show (labeled_image->details->label);
}

static void
labeled_image_ensure_image (EelLabeledImage *labeled_image)
{
    g_assert (EEL_IS_LABELED_IMAGE (labeled_image));

    if (labeled_image->details->image != NULL)
    {
        return;
    }

    labeled_image->details->image = ctk_image_new ();
    ctk_container_add (CTK_CONTAINER (labeled_image), labeled_image->details->image);
    ctk_widget_show (labeled_image->details->image);
}

static gboolean
labeled_image_show_image (const EelLabeledImage *labeled_image)
{
    g_assert (EEL_IS_LABELED_IMAGE (labeled_image));

    return labeled_image->details->image != NULL && labeled_image->details->show_image;
}

static gboolean
labeled_image_show_label (const EelLabeledImage *labeled_image)
{
    g_assert (EEL_IS_LABELED_IMAGE (labeled_image));

    return labeled_image->details->label != NULL && labeled_image->details->show_label;
}

/**
 * eel_labeled_image_new:
 * @text: Text to use for label or NULL.
 * @pixbuf: Pixbuf to use for image or NULL.
 *
 * Returns A newly allocated EelLabeledImage.  If the &text parameter is not
 * NULL then the LabeledImage will show a label.  If the &pixbuf parameter is not
 * NULL then the LabeledImage will show a pixbuf.  Either of these can be NULL at
 * creation time.
 *
 * Later in the lifetime of the widget you can invoke methods that affect the
 * label and/or the image.  If at creation time these were NULL, then they will
 * be created as neeeded.
 *
 * Thus, using this widget in place of EelImage or EelLabel is "free" with
 * only the CtkWidget and function call overhead.
 *
 */
CtkWidget*
eel_labeled_image_new (const char *text,
                       GdkPixbuf *pixbuf)
{
    EelLabeledImage *labeled_image;

    labeled_image = EEL_LABELED_IMAGE (ctk_widget_new (eel_labeled_image_get_type (), NULL));

    if (text != NULL)
    {
        eel_labeled_image_set_text (labeled_image, text);
    }

    if (pixbuf != NULL)
    {
        eel_labeled_image_set_pixbuf (labeled_image, pixbuf);
    }

    labeled_image_update_alignments (labeled_image);

    return CTK_WIDGET (labeled_image);
}

/**
 * eel_labeled_image_new_from_file_name:
 * @text: Text to use for label or NULL.
 * @file_name: File name of picture to use for pixbuf.  Cannot be NULL.
 *
 * Returns A newly allocated EelLabeledImage.  If the &text parameter is not
 * NULL then the LabeledImage will show a label.
 *
 */
CtkWidget*
eel_labeled_image_new_from_file_name (const char *text,
                                      const char *pixbuf_file_name)
{
    EelLabeledImage *labeled_image;

    g_return_val_if_fail (pixbuf_file_name != NULL, NULL);

    labeled_image = EEL_LABELED_IMAGE (eel_labeled_image_new (text, NULL));
    eel_labeled_image_set_pixbuf_from_file_name (labeled_image, pixbuf_file_name);
    return CTK_WIDGET (labeled_image);
}

/**
 * eel_labeled_image_set_label_position:
 * @labeled_image: A EelLabeledImage.
 * @label_position: The position of the label with respect to the image.
 *
 * Set the position of the label with respect to the image as follows:
 *
 * CTK_POS_LEFT:
 *   [ <label> <image> ]
 *
 * CTK_POS_RIGHT:
 *   [ <image> <label> ]
 *
 * CTK_POS_TOP:
 *   [ <label> ]
 *   [ <image> ]
 *
 * CTK_POS_BOTTOM:
 *   [ <image> ]
 *   [ <label> ]
 *
 */
void
eel_labeled_image_set_label_position (EelLabeledImage *labeled_image,
                                      CtkPositionType label_position)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));
    g_return_if_fail (label_position >= CTK_POS_LEFT);
    g_return_if_fail (label_position <= CTK_POS_BOTTOM);

    if (labeled_image->details->label_position == label_position)
    {
        return;
    }

    labeled_image->details->label_position = label_position;

    labeled_image_update_alignments (labeled_image);

    ctk_widget_queue_resize (CTK_WIDGET (labeled_image));
}

/**
 * eel_labeled_image_get_label_postiion:
 * @labeled_image: A EelLabeledImage.
 *
 * Returns an enumeration indicating the position of the label with respect to the image.
 */
CtkPositionType
eel_labeled_image_get_label_position (const EelLabeledImage *labeled_image)
{
    g_return_val_if_fail (EEL_IS_LABELED_IMAGE (labeled_image), 0);

    return labeled_image->details->label_position;
}

/**
 * eel_labeled_image_set_show_label:
 * @labeled_image: A EelLabeledImage.
 * @show_image: A boolean value indicating whether the label should be shown.
 *
 * Update the labeled image to either show or hide the internal label widget.
 * This function doesnt have any effect if the LabeledImage doesnt already
 * contain an label.
 */
void
eel_labeled_image_set_show_label (EelLabeledImage *labeled_image,
                                  gboolean show_label)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    if (labeled_image->details->show_label == show_label)
    {
        return;
    }

    labeled_image->details->show_label = show_label;

    if (labeled_image->details->label != NULL)
    {
        if (labeled_image->details->show_label)
        {
            ctk_widget_show (labeled_image->details->label);
        }
        else
        {
            ctk_widget_hide (labeled_image->details->label);
        }
    }

    labeled_image_update_alignments (labeled_image);

    ctk_widget_queue_resize (CTK_WIDGET (labeled_image));
}

/**
 * eel_labeled_image_get_show_label:
 * @labeled_image: A EelLabeledImage.
 *
 * Returns a boolean value indicating whether the internal label is shown.
 */
gboolean
eel_labeled_image_get_show_label (const EelLabeledImage *labeled_image)
{
    g_return_val_if_fail (EEL_IS_LABELED_IMAGE (labeled_image), 0);

    return labeled_image->details->show_label;
}

/**
 * eel_labeled_image_set_show_image:
 * @labeled_image: A EelLabeledImage.
 * @show_image: A boolean value indicating whether the image should be shown.
 *
 * Update the labeled image to either show or hide the internal image widget.
 * This function doesnt have any effect if the LabeledImage doesnt already
 * contain an image.
 */
void
eel_labeled_image_set_show_image (EelLabeledImage *labeled_image,
                                  gboolean show_image)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    if (labeled_image->details->show_image == show_image)
    {
        return;
    }

    labeled_image->details->show_image = show_image;

    if (labeled_image->details->image != NULL)
    {
        if (labeled_image->details->show_image)
        {
            ctk_widget_show (labeled_image->details->image);
        }
        else
        {
            ctk_widget_hide (labeled_image->details->image);
        }
    }

    labeled_image_update_alignments (labeled_image);

    ctk_widget_queue_resize (CTK_WIDGET (labeled_image));
}

/**
 * eel_labeled_image_get_show_image:
 * @labeled_image: A EelLabeledImage.
 *
 * Returns a boolean value indicating whether the internal image is shown.
 */
gboolean
eel_labeled_image_get_show_image (const EelLabeledImage *labeled_image)
{
    g_return_val_if_fail (EEL_IS_LABELED_IMAGE (labeled_image), 0);

    return labeled_image->details->show_image;
}


/**
 * eel_labeled_image_set_fixed_image_height:
 * @labeled_image: A EelLabeledImage.
 * @fixed_image_height: The new fixed image height.
 *
 * Normally, we measure the height of images, but it's sometimes useful
 * to use a fixed height for all the images.  This routine sets the
 * image height to the passed in value
 *
 */
void
eel_labeled_image_set_fixed_image_height (EelLabeledImage *labeled_image,
        int new_height)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    if (labeled_image->details->fixed_image_height == new_height)
    {
        return;
    }

    labeled_image->details->fixed_image_height = new_height;

    labeled_image_update_alignments (labeled_image);

    ctk_widget_queue_resize (CTK_WIDGET (labeled_image));
}

/**
 * eel_labeled_image_set_selected:
 * @labeled_image: A EelLabeledImage.
 * @selected: A boolean value indicating whether the labeled image
 * should be selected.
 *
 * Selects or deselects the labeled image.
 *
 */
void
eel_labeled_image_set_selected (EelLabeledImage *labeled_image,
                                gboolean selected)
{
    CtkStateType state;
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    state = selected ? CTK_STATE_FLAG_SELECTED : CTK_STATE_FLAG_NORMAL;

    ctk_widget_set_state_flags (CTK_WIDGET (labeled_image), state, TRUE);
    ctk_widget_set_state_flags (labeled_image->details->image, state, TRUE);
    ctk_widget_set_state_flags (labeled_image->details->label, state, TRUE);

}

/**
 * eel_labeled_image_get_selected:
 * @labeled_image: A EelLabeledImage.
 *
 * Returns the selected state of the labeled image.
 *
 */
gboolean
eel_labeled_image_get_selected (EelLabeledImage *labeled_image)
{
    g_return_val_if_fail (EEL_IS_LABELED_IMAGE (labeled_image), FALSE);

    return ctk_widget_get_state_flags (CTK_WIDGET (labeled_image)) == CTK_STATE_FLAG_SELECTED;
}

/**
 * eel_labeled_image_set_spacing:
 * @labeled_image: A EelLabeledImage.
 * @spacing: The new spacing between label and image.
 *
 * Set the spacing between label and image.  This will only affect
 * the geometry of the widget if both a label and image are currently
 * visible.
 *
 */
void
eel_labeled_image_set_spacing (EelLabeledImage *labeled_image,
                               guint spacing)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    if (labeled_image->details->spacing == spacing)
    {
        return;
    }

    labeled_image->details->spacing = spacing;

    labeled_image_update_alignments (labeled_image);

    ctk_widget_queue_resize (CTK_WIDGET (labeled_image));
}

/**
 * eel_labeled_image_get_spacing:
 * @labeled_image: A EelLabeledImage.
 *
 * Returns: The spacing between the label and image.
 */
guint
eel_labeled_image_get_spacing (const EelLabeledImage *labeled_image)
{
    g_return_val_if_fail (EEL_IS_LABELED_IMAGE (labeled_image), 0);

    return labeled_image->details->spacing;
}

/**
 * eel_labeled_image_set_x_padding:
 * @labeled_image: A EelLabeledImage.
 * @x_padding: The new horizontal padding.
 *
 * Set horizontal padding for the EelLabeledImage.  The padding
 * attribute work just like that in CtkMisc.
 */
void
eel_labeled_image_set_x_padding (EelLabeledImage *labeled_image,
                                 int x_padding)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    x_padding = MAX (0, x_padding);

    if (labeled_image->details->x_padding == x_padding)
    {
        return;
    }

    labeled_image->details->x_padding = x_padding;
    labeled_image_update_alignments (labeled_image);
    ctk_widget_queue_resize (CTK_WIDGET (labeled_image));
}

/**
 * eel_labeled_image_get_x_padding:
 * @labeled_image: A EelLabeledImage.
 *
 * Returns: The horizontal padding for the LabeledImage's content.
 */
int
eel_labeled_image_get_x_padding (const EelLabeledImage *labeled_image)
{
    g_return_val_if_fail (EEL_IS_LABELED_IMAGE (labeled_image), 0);

    return labeled_image->details->x_padding;
}

/**
 * eel_labeled_image_set_y_padding:
 * @labeled_image: A EelLabeledImage.
 * @x_padding: The new vertical padding.
 *
 * Set vertical padding for the EelLabeledImage.  The padding
 * attribute work just like that in CtkMisc.
 */
void
eel_labeled_image_set_y_padding (EelLabeledImage *labeled_image,
                                 int y_padding)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    y_padding = MAX (0, y_padding);

    if (labeled_image->details->y_padding == y_padding)
    {
        return;
    }

    labeled_image->details->y_padding = y_padding;
    labeled_image_update_alignments (labeled_image);
    ctk_widget_queue_resize (CTK_WIDGET (labeled_image));
}

/**
 * eel_labeled_image_get_x_padding:
 * @labeled_image: A EelLabeledImage.
 *
 * Returns: The vertical padding for the LabeledImage's content.
 */
int
eel_labeled_image_get_y_padding (const EelLabeledImage *labeled_image)
{
    g_return_val_if_fail (EEL_IS_LABELED_IMAGE (labeled_image), 0);

    return labeled_image->details->y_padding;
}

/**
 * eel_labeled_image_set_x_alignment:
 * @labeled_image: A EelLabeledImage.
 * @x_alignment: The new horizontal alignment.
 *
 * Set horizontal alignment for the EelLabeledImage's content.
 * The 'content' is the union of the image and label.  The alignment
 * attribute work just like that in CtkMisc.
 */
void
eel_labeled_image_set_x_alignment (EelLabeledImage *labeled_image,
                                   float x_alignment)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    x_alignment = MAX (0, x_alignment);
    x_alignment = MIN (1.0, x_alignment);

    if (labeled_image->details->x_alignment == x_alignment)
    {
        return;
    }

    labeled_image->details->x_alignment = x_alignment;
    ctk_widget_queue_resize (CTK_WIDGET (labeled_image));
}

/**
 * eel_labeled_image_get_x_alignment:
 * @labeled_image: A EelLabeledImage.
 *
 * Returns: The horizontal alignment for the LabeledImage's content.
 */
float
eel_labeled_image_get_x_alignment (const EelLabeledImage *labeled_image)
{
    g_return_val_if_fail (EEL_IS_LABELED_IMAGE (labeled_image), 0);

    return labeled_image->details->x_alignment;
}

/**
 * eel_labeled_image_set_y_alignment:
 * @labeled_image: A EelLabeledImage.
 * @y_alignment: The new vertical alignment.
 *
 * Set vertical alignment for the EelLabeledImage's content.
 * The 'content' is the union of the image and label.  The alignment
 * attribute work just like that in CtkMisc.
 */
void
eel_labeled_image_set_y_alignment (EelLabeledImage *labeled_image,
                                   float y_alignment)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    y_alignment = MAX (0, y_alignment);
    y_alignment = MIN (1.0, y_alignment);

    if (labeled_image->details->y_alignment == y_alignment)
    {
        return;
    }

    labeled_image->details->y_alignment = y_alignment;
    ctk_widget_queue_resize (CTK_WIDGET (labeled_image));
}

/**
 * eel_labeled_image_get_y_alignment:
 * @labeled_image: A EelLabeledImage.
 *
 * Returns: The vertical alignment for the LabeledImage's content.
 */
float
eel_labeled_image_get_y_alignment (const EelLabeledImage *labeled_image)
{
    g_return_val_if_fail (EEL_IS_LABELED_IMAGE (labeled_image), 0);

    return labeled_image->details->y_alignment;
}

/**
 * eel_labeled_image_set_fill:
 * @labeled_image: A EelLabeledImage.
 * @fill: A boolean value indicating whether the internal image and label
 * widgets should fill all the available allocation.
 *
 * By default the internal image and label wigets are sized to their natural
 * preferred geometry.  You can use the 'fill' attribute of LabeledImage
 * to have the internal widgets fill as much of the LabeledImage allocation
 * as is available.
 */
void
eel_labeled_image_set_fill (EelLabeledImage *labeled_image,
                            gboolean fill)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    if (labeled_image->details->fill == fill)
    {
        return;
    }

    labeled_image->details->fill = fill;

    labeled_image_update_alignments (labeled_image);

    ctk_widget_queue_resize (CTK_WIDGET (labeled_image));
}

/**
 * eel_labeled_image_get_fill:
 * @labeled_image: A EelLabeledImage.
 *
 * Retruns a boolean value indicating whether the internal widgets fill
 * all the available allocation.
 */
gboolean
eel_labeled_image_get_fill (const EelLabeledImage *labeled_image)
{
    g_return_val_if_fail (EEL_IS_LABELED_IMAGE (labeled_image), 0);

    return labeled_image->details->fill;
}

static void
eel_labled_set_mnemonic_widget (CtkWidget *image_widget,
                                CtkWidget *mnemonic_widget)
{
    EelLabeledImage *image;

    g_assert (EEL_IS_LABELED_IMAGE (image_widget));

    image = EEL_LABELED_IMAGE (image_widget);

    if (image->details->label)
        ctk_label_set_mnemonic_widget
        (CTK_LABEL (image->details->label), mnemonic_widget);
}

/**
 * eel_labeled_image_button_new:
 * @text: Text to use for label or NULL.
 * @pixbuf: Pixbuf to use for image or NULL.
 *
 * Create a stock CtkButton with a EelLabeledImage child.
 *
 */
CtkWidget *
eel_labeled_image_button_new (const char *text,
                              GdkPixbuf *pixbuf)
{
    CtkWidget *button;
    CtkWidget *labeled_image;

    button = g_object_new (eel_labeled_image_button_get_type (), NULL);
    labeled_image = eel_labeled_image_new (text, pixbuf);
    ctk_container_add (CTK_CONTAINER (button), labeled_image);
    eel_labled_set_mnemonic_widget (labeled_image, button);
    ctk_widget_show (labeled_image);

    return button;
}

/**
 * eel_labeled_image_button_new_from_file_name:
 * @text: Text to use for label or NULL.
 * @pixbuf_file_name: Name of pixbuf to use for image.  Cannot be NULL.
 *
 * Create a stock CtkToggleButton with a EelLabeledImage child.
 *
 */
CtkWidget *
eel_labeled_image_button_new_from_file_name (const char *text,
        const char *pixbuf_file_name)
{
    CtkWidget *button;
    CtkWidget *labeled_image;

    g_return_val_if_fail (pixbuf_file_name != NULL, NULL);

    button = g_object_new (eel_labeled_image_button_get_type (), NULL);
    labeled_image = eel_labeled_image_new_from_file_name (text, pixbuf_file_name);
    ctk_container_add (CTK_CONTAINER (button), labeled_image);
    eel_labled_set_mnemonic_widget (labeled_image, button);
    ctk_widget_show (labeled_image);

    return button;
}

/**
 * eel_labeled_image_toggle_button_new:
 * @text: Text to use for label or NULL.
 * @pixbuf: Pixbuf to use for image or NULL.
 *
 * Create a stock CtkToggleButton with a EelLabeledImage child.
 *
 */
CtkWidget *
eel_labeled_image_toggle_button_new (const char *text,
                                     GdkPixbuf *pixbuf)
{
    CtkWidget *toggle_button;
    CtkWidget *labeled_image;

    toggle_button = g_object_new (eel_labeled_image_toggle_button_get_type (), NULL);
    labeled_image = eel_labeled_image_new (text, pixbuf);
    ctk_container_add (CTK_CONTAINER (toggle_button), labeled_image);
    eel_labled_set_mnemonic_widget (labeled_image, toggle_button);
    ctk_widget_show (labeled_image);

    return toggle_button;
}

/**
 * eel_labeled_image_toggle_button_new_from_file_name:
 * @text: Text to use for label or NULL.
 * @pixbuf_file_name: Name of pixbuf to use for image.  Cannot be NULL.
 *
 * Create a stock CtkToggleButton with a EelLabeledImage child.
 *
 */
CtkWidget *
eel_labeled_image_toggle_button_new_from_file_name (const char *text,
        const char *pixbuf_file_name)
{
    CtkWidget *toggle_button;
    CtkWidget *labeled_image;

    g_return_val_if_fail (pixbuf_file_name != NULL, NULL);

    toggle_button = g_object_new (eel_labeled_image_toggle_button_get_type (), NULL);
    labeled_image = eel_labeled_image_new_from_file_name (text, pixbuf_file_name);
    ctk_container_add (CTK_CONTAINER (toggle_button), labeled_image);
    eel_labled_set_mnemonic_widget (labeled_image, toggle_button);
    ctk_widget_show (labeled_image);

    return toggle_button;
}

/**
 * eel_labeled_image_toggle_button_new:
 * @text: Text to use for label or NULL.
 * @pixbuf: Pixbuf to use for image or NULL.
 *
 * Create a stock CtkToggleButton with a EelLabeledImage child.
 *
 * Returns: the new radio button.
 */
CtkWidget *
eel_labeled_image_radio_button_new (const char *text,
                                    GdkPixbuf  *pixbuf)
{
    CtkWidget *radio_button;
    CtkWidget *labeled_image;

    radio_button = g_object_new (eel_labeled_image_radio_button_get_type (), NULL);
    labeled_image = eel_labeled_image_new (text, pixbuf);
    ctk_container_add (CTK_CONTAINER (radio_button), labeled_image);
    eel_labled_set_mnemonic_widget (labeled_image, radio_button);
    ctk_widget_show (labeled_image);

    return radio_button;
}

/**
 * eel_labeled_image_radio_button_new_from_file_name:
 * @text: Text to use for label or NULL.
 * @pixbuf_file_name: Name of pixbuf to use for image.  Cannot be NULL.
 *
 * Create a stock CtkRadioButton with a EelLabeledImage child.
 *
 * Returns: the new radio button.
 */
CtkWidget *
eel_labeled_image_radio_button_new_from_file_name (const char *text,
        const char *pixbuf_file_name)
{
    CtkWidget *radio_button;
    CtkWidget *labeled_image;

    g_return_val_if_fail (pixbuf_file_name != NULL, NULL);

    radio_button = g_object_new (eel_labeled_image_radio_button_get_type (), NULL);
    labeled_image = eel_labeled_image_new_from_file_name (text, pixbuf_file_name);
    ctk_container_add (CTK_CONTAINER (radio_button), labeled_image);
    eel_labled_set_mnemonic_widget (labeled_image, radio_button);
    ctk_widget_show (labeled_image);

    return radio_button;
}

/*
 * Workaround some bugs in CtkCheckButton where the widget
 * does not redraw properly after leave or focus out events
 *
 * The workaround is to draw a little bit more than the
 * widget itself - 4 pixels worth.  For some reason the
 * widget does not properly redraw its edges.
 */
static void
button_leave_callback (CtkWidget *widget,
		       gpointer   callback_data G_GNUC_UNUSED)
{
    g_assert (CTK_IS_WIDGET (widget));

    if (ctk_widget_is_drawable (widget))
    {
        const int fudge = 4;
        EelIRect bounds;

        bounds = eel_ctk_widget_get_bounds (widget);

        bounds.x0 -= fudge;
        bounds.y0 -= fudge;
        bounds.x1 += fudge;
        bounds.y1 += fudge;

        ctk_widget_queue_draw_area (ctk_widget_get_parent (widget),
                                    bounds.x0,
                                    bounds.y0,
                                    eel_irect_get_width (bounds),
                                    eel_irect_get_height (bounds));
    }
}

static gint
button_focus_out_event_callback (CtkWidget     *widget,
				 CdkEventFocus *event G_GNUC_UNUSED,
				 gpointer       callback_data)
{
    g_assert (CTK_IS_WIDGET (widget));

    button_leave_callback (widget, callback_data);

    return FALSE;
}

/**
 * eel_labeled_image_check_button_new:
 * @text: Text to use for label or NULL.
 * @pixbuf: Pixbuf to use for image or NULL.
 *
 * Create a stock CtkCheckButton with a EelLabeledImage child.
 *
 */
CtkWidget *
eel_labeled_image_check_button_new (const char *text,
                                    GdkPixbuf *pixbuf)
{
    CtkWidget *check_button;
    CtkWidget *labeled_image;

    check_button = g_object_new (eel_labeled_image_check_button_get_type (), NULL);
    labeled_image = eel_labeled_image_new (text, pixbuf);
    ctk_container_add (CTK_CONTAINER (check_button), labeled_image);
    eel_labled_set_mnemonic_widget (labeled_image, check_button);
    ctk_widget_show (labeled_image);

    /*
     * Workaround some bugs in CtkCheckButton where the widget
     * does not redraw properly after leave or focus out events
     */
    g_signal_connect (check_button, "leave",
                      G_CALLBACK (button_leave_callback), NULL);
    g_signal_connect (check_button, "focus_out_event",
                      G_CALLBACK (button_focus_out_event_callback), NULL);

    return check_button;
}

/**
 * eel_labeled_image_check_button_new_from_file_name:
 * @text: Text to use for label or NULL.
 * @pixbuf_file_name: Name of pixbuf to use for image.  Cannot be NULL.
 *
 * Create a stock CtkCheckButton with a EelLabeledImage child.
 *
 */
CtkWidget *
eel_labeled_image_check_button_new_from_file_name (const char *text,
        const char *pixbuf_file_name)
{
    CtkWidget *check_button;
    CtkWidget *labeled_image;

    g_return_val_if_fail (pixbuf_file_name != NULL, NULL);

    check_button = g_object_new (eel_labeled_image_check_button_get_type (), NULL);
    labeled_image = eel_labeled_image_new_from_file_name (text, pixbuf_file_name);
    ctk_container_add (CTK_CONTAINER (check_button), labeled_image);
    eel_labled_set_mnemonic_widget (labeled_image, check_button);
    ctk_widget_show (labeled_image);

    return check_button;
}

/*
 * The rest of the methods are proxies for those in EelImage and
 * EelLabel.  We have all these so that we dont have to expose
 * our internal widgets at all.  Probably more of these will be added
 * as they are needed.
 */

/**
 * eel_labeled_image_set_pixbuf:
 * @labaled_image: A EelLabeledImage.
 * @pixbuf: New pixbuf to use or NULL.
 *
 * Change the pixbuf displayed by the LabeledImage.  Note that the widget display
 * is only updated if the show_image attribute is TRUE.
 *
 * If no internal image widget exists as of yet, a new one will be created.
 *
 * A NULL &pixbuf will cause the internal image widget (if alive) to be destroyed.
 */
void
eel_labeled_image_set_pixbuf (EelLabeledImage *labeled_image,
                              GdkPixbuf *pixbuf)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    if (pixbuf == NULL)
    {
        if (labeled_image->details->image != NULL)
        {
            ctk_widget_destroy (labeled_image->details->image);
            labeled_image->details->image = NULL;
        }

        ctk_widget_queue_resize (CTK_WIDGET (labeled_image));
    }
    else
    {
        labeled_image_ensure_image (labeled_image);
        ctk_image_set_from_pixbuf (CTK_IMAGE (labeled_image->details->image), pixbuf);
    }
}

void
eel_labeled_image_set_pixbuf_from_file_name (EelLabeledImage *labeled_image,
        const char *pixbuf_file_name)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    labeled_image_ensure_image (labeled_image);
    ctk_image_set_from_file (CTK_IMAGE (labeled_image->details->image), pixbuf_file_name);
}

/**
 * eel_labeled_image_set_text:
 * @labaled_image: A EelLabeledImage.
 * @text: New text (with mnemnonic) to use or NULL.
 *
 * Change the text displayed by the LabeledImage.  Note that the widget display
 * is only updated if the show_label attribute is TRUE.
 *
 * If no internal label widget exists as of yet, a new one will be created.
 *
 * A NULL &text will cause the internal label widget (if alive) to be destroyed.
 */
void
eel_labeled_image_set_text (EelLabeledImage *labeled_image,
                            const char *text)
{
    g_return_if_fail (EEL_IS_LABELED_IMAGE (labeled_image));

    if (text == NULL)
    {
        if (labeled_image->details->label)
        {
            ctk_widget_destroy (labeled_image->details->label);
            labeled_image->details->label = NULL;
        }

        ctk_widget_queue_resize (CTK_WIDGET (labeled_image));
    }
    else
    {
        labeled_image_ensure_label (labeled_image);
        ctk_label_set_text_with_mnemonic
        (CTK_LABEL (labeled_image->details->label), text);
    }
}

char *
eel_labeled_image_get_text (const EelLabeledImage *labeled_image)
{
    g_return_val_if_fail (EEL_IS_LABELED_IMAGE (labeled_image), NULL);

    if (labeled_image->details->label == NULL)
    {
        return NULL;
    }

    return g_strdup (ctk_label_get_text (CTK_LABEL (labeled_image->details->label)));
}

void
eel_labeled_image_set_can_focus (EelLabeledImage *labeled_image,
                                 gboolean         can_focus)
{
    ctk_widget_set_can_focus (CTK_WIDGET (labeled_image), can_focus);
}

static AtkObjectClass *a11y_parent_class = NULL;

static void
eel_labeled_image_accessible_initialize (AtkObject *accessible,
        gpointer   widget)
{
    a11y_parent_class->initialize (accessible, widget);
    atk_object_set_role (accessible, ATK_ROLE_IMAGE);

}

static EelLabeledImage *
get_image (gpointer object)
{
    CtkWidget *widget;

    if (!(widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (object))))
    {
        return NULL;
    }

    if (CTK_IS_BUTTON (widget))
        widget = ctk_bin_get_child (CTK_BIN (widget));

    return EEL_LABELED_IMAGE (widget);
}

static const gchar* eel_labeled_image_accessible_get_name(AtkObject* accessible)
{
    EelLabeledImage* labeled_image;

    labeled_image = get_image(accessible);

    if (labeled_image && labeled_image->details && labeled_image->details->label)
    {
        return ctk_label_get_text(CTK_LABEL(labeled_image->details->label));
    }

    g_warning("no label on '%p'", labeled_image);

    return NULL;
}

static void
eel_labeled_image_accessible_image_get_size (AtkImage *image,
        gint     *width,
        gint     *height)
{
    EelLabeledImage *labeled_image;
    CtkAllocation allocation;

    labeled_image = get_image (image);

    if (!labeled_image || !labeled_image->details->image)
    {
        *width = *height = 0;
        return;
    }

    ctk_widget_get_allocation (labeled_image->details->image, &allocation);
    *width = allocation.width;
    *height = allocation.height;
}

static void
eel_labeled_image_accessible_image_interface_init (AtkImageIface *iface)
{
    iface->get_image_size = eel_labeled_image_accessible_image_get_size;
}

typedef struct _EelLabeledImageAccessible EelLabeledImageAccessible;
typedef struct _EelLabeledImageAccessibleClass EelLabeledImageAccessibleClass;

struct _EelLabeledImageAccessible
{
    CtkContainerAccessible parent;
};

struct _EelLabeledImageAccessibleClass
{
    CtkContainerAccessibleClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (EelLabeledImageAccessible,
                         eel_labeled_image_accessible,
                         CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_IMAGE,
                                                eel_labeled_image_accessible_image_interface_init));
static void
eel_labeled_image_accessible_class_init (EelLabeledImageAccessibleClass *klass)
{
    AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);
    a11y_parent_class = g_type_class_peek_parent (klass);

    atk_class->get_name = eel_labeled_image_accessible_get_name;
    atk_class->initialize = eel_labeled_image_accessible_initialize;
}

static void
eel_labeled_image_accessible_init (EelLabeledImageAccessible *accessible G_GNUC_UNUSED)
{
}

static void
eel_labeled_image_button_class_init (CtkWidgetClass *klass G_GNUC_UNUSED)
{
}

static GType
eel_labeled_image_button_get_type (void)
{
    static GType type = 0;

    if (!type)
    {
        GTypeInfo info =
        {
            sizeof (CtkButtonClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) eel_labeled_image_button_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkButton),
            0, /* n_preallocs */
            (GInstanceInitFunc) NULL
        };

        type = g_type_register_static
               (CTK_TYPE_BUTTON,
                "EelLabeledImageButton", &info, 0);
    }

    return type;
}

static GType
eel_labeled_image_check_button_get_type (void)
{
    static GType type = 0;

    if (!type)
    {
        GTypeInfo info =
        {
            sizeof (CtkCheckButtonClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) eel_labeled_image_button_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkCheckButton),
            0, /* n_preallocs */
            (GInstanceInitFunc) NULL
        };

        type = g_type_register_static
               (CTK_TYPE_CHECK_BUTTON,
                "EelLabeledImageCheckButton", &info, 0);
    }

    return type;
}

static GType
eel_labeled_image_toggle_button_get_type (void)
{
    static GType type = 0;

    if (!type)
    {
        GTypeInfo info =
        {
            sizeof (CtkToggleButtonClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) eel_labeled_image_button_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkToggleButton),
            0, /* n_preallocs */
            (GInstanceInitFunc) NULL
        };

        type = g_type_register_static
               (CTK_TYPE_TOGGLE_BUTTON,
                "EelLabeledImageToggleButton", &info, 0);
    }

    return type;
}


static GType
eel_labeled_image_radio_button_get_type (void)
{
    static GType type = 0;

    if (!type)
    {
        GTypeInfo info =
        {
            sizeof (CtkRadioButtonClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) eel_labeled_image_button_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkRadioButton),
            0, /* n_preallocs */
            (GInstanceInitFunc) NULL
        };

        type = g_type_register_static
               (CTK_TYPE_RADIO_BUTTON,
                "EelLabeledImageRadioButton", &info, 0);
    }

    return type;
}
