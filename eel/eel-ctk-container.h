/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-ctk-container.h - Functions to simplify the implementations of
  			 CtkContainer widgets.

   Copyright (C) 2001 Ramiro Estrugo.

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

#ifndef EEL_CTK_CONTAINER_H
#define EEL_CTK_CONTAINER_H

#include <ctk/ctk.h>
#include "eel-art-extensions.h"

void eel_ctk_container_child_expose_event (CtkContainer   *container,
        CtkWidget      *child,
        cairo_t        *cr);

void eel_ctk_container_child_map          (CtkContainer   *container,
        CtkWidget      *child);
void eel_ctk_container_child_unmap        (CtkContainer   *container,
        CtkWidget      *child);
void eel_ctk_container_child_add          (CtkContainer   *container,
        CtkWidget      *child);
void eel_ctk_container_child_remove       (CtkContainer   *container,
        CtkWidget      *child);
void eel_ctk_container_child_size_allocate (CtkContainer *container,
        CtkWidget *child,
        EelIRect child_geometry);

#endif /* EEL_CTK_CONTAINER_H */
