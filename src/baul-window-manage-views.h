/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/*
 *  Baul
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 1999, 2000, 2001 Eazel, Inc.
 *
 *  Baul is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  Baul is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Author: Darin Adler <darin@bentspoon.com>
 *
 */

#ifndef BAUL_WINDOW_MANAGE_VIEWS_H
#define BAUL_WINDOW_MANAGE_VIEWS_H

#include "baul-window.h"
#include "baul-window-pane.h"
#include "baul-navigation-window.h"

void baul_window_manage_views_close_slot (BaulWindowPane *pane,
        BaulWindowSlot *slot);


/* BaulWindowInfo implementation: */
void baul_window_report_load_underway     (BaulWindow     *window,
        BaulView       *view);
void baul_window_report_selection_changed (BaulWindowInfo *window);
void baul_window_report_view_failed       (BaulWindow     *window,
        BaulView       *view);
void baul_window_report_load_complete     (BaulWindow     *window,
        BaulView       *view);
void baul_window_report_location_change   (BaulWindow     *window);
void baul_window_update_up_button         (BaulWindow     *window);

#endif /* BAUL_WINDOW_MANAGE_VIEWS_H */
