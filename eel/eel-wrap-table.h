/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-wrap-table.h - A table that can wrap its contents as needed.

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

#ifndef EEL_WRAP_TABLE_H
#define EEL_WRAP_TABLE_H

#include <glib.h>
#include <ctk/ctk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EEL_TYPE_WRAP_TABLE eel_wrap_table_get_type()
#define EEL_WRAP_TABLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EEL_TYPE_WRAP_TABLE, EelWrapTable))
#define EEL_WRAP_TABLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EEL_TYPE_WRAP_TABLE, EelWrapTableClass))
#define EEL_IS_WRAP_TABLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EEL_TYPE_WRAP_TABLE))
#define EEL_IS_WRAP_TABLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EEL_TYPE_WRAP_TABLE))
#define EEL_WRAP_TABLE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EEL_TYPE_WRAP_TABLE, EelWrapTableClass))

    typedef struct EelWrapTable	       EelWrapTable;
    typedef struct EelWrapTableClass       EelWrapTableClass;
    typedef struct EelWrapTablePrivate     EelWrapTablePrivate;

    struct EelWrapTable
    {
        /* Superclass */
        CtkContainer container;

        /* Private things */
        EelWrapTablePrivate *details;
    };

    struct EelWrapTableClass
    {
        CtkContainerClass parent_class;
    };

    typedef enum
    {
        EEL_JUSTIFICATION_BEGINNING,
        EEL_JUSTIFICATION_MIDDLE,
        EEL_JUSTIFICATION_END
    } EelJustification;

    /* Public CtkWrapTable methods */
    GType            eel_wrap_table_get_type                  (void);
    CtkWidget *      eel_wrap_table_new                       (gboolean            homogeneous);
    void             eel_wrap_table_set_x_spacing             (EelWrapTable       *wrap_table,
            guint               x_spacing);
    guint            eel_wrap_table_get_x_spacing             (const EelWrapTable *wrap_table);
    void             eel_wrap_table_set_y_spacing             (EelWrapTable       *wrap_table,
            guint               y_spacing);
    guint            eel_wrap_table_get_y_spacing             (const EelWrapTable *wrap_table);
    CtkWidget *      eel_wrap_table_find_child_at_event_point (const EelWrapTable *wrap_table,
            int                 x,
            int                 y);
    void             eel_wrap_table_set_x_justification       (EelWrapTable       *wrap_table,
            EelJustification    justification);
    EelJustification eel_wrap_table_get_x_justification       (const EelWrapTable *wrap_table);
    void             eel_wrap_table_set_y_justification       (EelWrapTable       *wrap_table,
            EelJustification    justification);
    EelJustification eel_wrap_table_get_y_justification       (const EelWrapTable *wrap_table);
    void             eel_wrap_table_set_homogeneous           (EelWrapTable       *wrap_table,
            gboolean            homogeneous);
    gboolean         eel_wrap_table_get_homogeneous           (const EelWrapTable *wrap_table);
    void             eel_wrap_table_reorder_child             (EelWrapTable       *wrap_table,
            CtkWidget          *child,
            int                 position);
    guint            eel_wrap_table_get_num_children          (const EelWrapTable *wrap_table);

    CtkWidget *      eel_scrolled_wrap_table_new              (gboolean            homogeneous,
            CtkShadowType       shadow_type,
            CtkWidget         **wrap_table_out);

#ifdef __cplusplus
}
#endif

#endif /* EEL_WRAP_TABLE_H */


