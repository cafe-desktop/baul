/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Baul
 *
 * Copyright (C) 2000 Eazel, Inc.
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
 *
 * Author: Maciej Stachowiak <mjs@eazel.com>
 *         Ettore Perazzoli <ettore@gnu.org>
 */

#ifndef BAUL_LOCATION_ENTRY_H
#define BAUL_LOCATION_ENTRY_H

#include <libbaul-private/baul-entry.h>

#define BAUL_TYPE_LOCATION_ENTRY baul_location_entry_get_type()
#define BAUL_LOCATION_ENTRY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAUL_TYPE_LOCATION_ENTRY, BaulLocationEntry))
#define BAUL_LOCATION_ENTRY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), BAUL_TYPE_LOCATION_ENTRY, BaulLocationEntryClass))
#define BAUL_IS_LOCATION_ENTRY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAUL_TYPE_LOCATION_ENTRY))
#define BAUL_IS_LOCATION_ENTRY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), BAUL_TYPE_LOCATION_ENTRY))
#define BAUL_LOCATION_ENTRY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAUL_TYPE_LOCATION_ENTRY, BaulLocationEntryClass))

typedef struct BaulLocationEntryDetails BaulLocationEntryDetails;

typedef struct BaulLocationEntry
{
    BaulEntry parent;
    BaulLocationEntryDetails *details;
} BaulLocationEntry;

typedef struct
{
    BaulEntryClass parent_class;
} BaulLocationEntryClass;

typedef enum
{
    BAUL_LOCATION_ENTRY_ACTION_GOTO,
    BAUL_LOCATION_ENTRY_ACTION_CLEAR
} BaulLocationEntryAction;

GType      baul_location_entry_get_type     	(void);
GtkWidget* baul_location_entry_new          	(void);
void       baul_location_entry_set_special_text     (BaulLocationEntry *entry,
        const char            *special_text);
void       baul_location_entry_set_secondary_action (BaulLocationEntry *entry,
        BaulLocationEntryAction secondary_action);
void       baul_location_entry_update_current_location (BaulLocationEntry *entry,
        const char *path);

#endif /* BAUL_LOCATION_ENTRY_H */
