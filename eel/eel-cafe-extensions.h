/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-cafe-extensions.h - interface for new functions that operate on
                                 cafe classes. Perhaps some of these should be
  			         rolled into cafe someday.

   Copyright (C) 1999, 2000, 2001 Eazel, Inc.

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

   Authors: Darin Adler <darin@eazel.com>
*/

#ifndef EEL_CAFE_EXTENSIONS_H
#define EEL_CAFE_EXTENSIONS_H

#include <ctk/ctk.h>

/* Return a command string containing the path to a terminal on this system. */
char *        eel_cafe_make_terminal_command                         (const char               *command);

/* Open up a new terminal, optionally passing in a command to execute */
void          eel_cafe_open_terminal_on_screen                       (const char               *command,
        CdkScreen                *screen);

#endif /* EEL_CAFE_EXTENSIONS_H */
