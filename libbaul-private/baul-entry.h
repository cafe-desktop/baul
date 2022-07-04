/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* CajaEntry: one-line text editing widget. This consists of bug fixes
 * and other improvements to GtkEntry, and all the changes could be rolled
 * into GtkEntry some day.
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Author: John Sullivan <sullivan@eazel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef BAUL_ENTRY_H
#define BAUL_ENTRY_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BAUL_TYPE_ENTRY baul_entry_get_type()
#define BAUL_ENTRY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_ENTRY, CajaEntry))
#define BAUL_ENTRY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_ENTRY, CajaEntryClass))
#define BAUL_IS_ENTRY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_ENTRY))
#define BAUL_IS_ENTRY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_ENTRY))
#define BAUL_ENTRY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_ENTRY, CajaEntryClass))

    typedef struct CajaEntryDetails CajaEntryDetails;

    typedef struct
    {
        GtkEntry parent;
        CajaEntryDetails *details;
    } CajaEntry;

    typedef struct
    {
        GtkEntryClass parent_class;

        void (*user_changed)      (CajaEntry *entry);
        void (*selection_changed) (CajaEntry *entry);
    } CajaEntryClass;

    GType       baul_entry_get_type                 (void);
    GtkWidget  *baul_entry_new                      (void);
    void        baul_entry_set_text                 (CajaEntry *entry,
            const char    *text);
    void        baul_entry_select_all               (CajaEntry *entry);
    void        baul_entry_select_all_at_idle       (CajaEntry *entry);
    void        baul_entry_set_special_tab_handling (CajaEntry *entry,
            gboolean       special_tab_handling);

#ifdef __cplusplus
}
#endif

#endif /* BAUL_ENTRY_H */
