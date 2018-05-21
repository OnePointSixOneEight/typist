/* vi: set ts=4 shiftwidth=4 expandtab:

   Typist 3.0 - improved typing tutor program for UNIX systems
   Copyright (C) 1998-2018  Simon Baldwin (simon_baldwin@yahoo.com)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Abstraction of curses. A particular aim here is to keep the extensive
   curses macros and other curses.h verbiage away from global namespace.
   Achieved by encapsulating all curses code in our associated C file, and
   exposing just the bits of it that the program needs through this screen_
   struct.  */

#ifndef SCREEN_H
#define SCREEN_H

struct screen_ {
  void (*init) (void);
  void (*finalize) (void);
  void (*setup_colour) (int fg, int bg);
  void (*getyx) (int *y, int *x);
  void (*cursor_off) (void);
  void (*cursor_on) (void);
  void (*refresh) (void);
  void (*break_) (void);
  int (*get_char) (void);
  void (*add_char) (char c);
  void (*add_string) (const char *s);
  void (*add_char_reverse) (char c);
  void (*add_string_reverse) (const char *s);
  void (*add_char_underline) (char c);
  void (*add_string_underline) (const char *s);
  int (*get_lines) (void);
  int (*get_columns) (void);
  void (*move) (int y, int x);
  void (*move_top_left) (void);
  void (*clear) (void);
  void (*clear_to_line_end) (void);
  void (*clear_to_screen_bottom) (void);
  int (*has_colors) (void);
  void (*start_color) (void);
  void (*init_pair) (int pair, int fg, int bg);
  void (*halfdelay) (int n);
  int (*function_key) (int n);
  int ERR_;
  int KEY_BACKSPACE_;
  int KEY_DOWN_;
  int KEY_UP_;
  int num_colours;
  int colour_array[8];
};

struct screen_ screen_;

#endif
