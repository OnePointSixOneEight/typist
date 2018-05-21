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

#include <curses.h>
#include <stdio.h>
#include <string.h>

#include "screen.h"

/* This is unpleasant. We cannot include utils.h because the curses
   macro definitions render it into invalid C. So to gain access to
   the info() tracing function we have to duplicate a small part of
   struct utils_ here. If changing one, be sure to make corresponding
   changes to the other.  */
struct utils_ {
  void *opaque_1;
  void *opaque_2;
  void (*info) (const char *format, ...);
};
extern struct utils_ utils_;

static int initialized = 0;

static void
init (void)
{
  if (initialized)
    return;
  initscr ();
  clear ();
  refresh ();
  typeahead (-1);
  keypad (stdscr, TRUE);
  noecho ();
  curs_set (0);
  raw ();
#if defined(NCURSES)
  ESCDELAY = 1;
#endif
  initialized = 1;
  utils_.info ("screen initialized");
}

static void
finalize (void)
{
  if (!initialized)
    return;
  if (has_colors ())
    wbkgdset (stdscr, 0);
  clear ();
  refresh ();
  endwin ();
  printf ("\n");
  initialized = 0;
  utils_.info ("screen finalized");
}

static void
setup_colour (int fg, int bg)
{
  start_color ();
  init_pair (1, screen_.colour_array[fg], screen_.colour_array[bg]);
#if defined(NCURSES)
  wbkgdset (stdscr, COLOR_PAIR (1));
#else
  wbkgdset (stdscr, COLOR_PAIR (1) | ' ');
#endif
}

static void
getyx_ (int *y, int *x)
{
  int y_, x_;
  getyx (stdscr, y_, x_);
  *y = y_;
  *x = x_;
}

static void cursor_off (void) { curs_set (0); }
static void cursor_on (void) { curs_set (1); }
static void refresh_ (void) { refresh (); }
static void break_ (void) { cbreak (); }
static int get_char (void) { return getch (); }
static void add_char (char c) { addch (c); }
static void add_string (const char *s) { addstr (s); }

static void
add_char_reverse (char c)
{
  attron (A_REVERSE);
  addch (c);
  attroff (A_REVERSE);
}

static void
add_string_reverse (const char *s)
{
  attron (A_REVERSE);
  addstr (s);
  attroff (A_REVERSE);
}

static void
add_char_underline (char c)
{
  attron (A_UNDERLINE);
  addch (c);
  attroff (A_UNDERLINE);
}

static void
add_string_underline (const char *s)
{
  attron (A_UNDERLINE);
  addstr (s);
  attroff (A_UNDERLINE);
}

static int get_lines (void) { return LINES; }
static int get_columns (void) { return COLS; }
static void move_ (int y, int x) { move (y, x); }
static void move_top_left (void) { move (LINES - 1, COLS - 1); }
static void clear_ (void) { clear (); }
static void clear_to_line_end (void) { clrtoeol (); }
static void clear_to_screen_bottom (void) { clrtobot (); }
static int has_colors_ (void) { return has_colors (); }
static void start_color_ (void) { start_color (); }

static void
init_pair_ (int pair, int fg, int bg) {
  init_pair (pair, fg, bg);
}

static void halfdelay_ (int n) { halfdelay (n); }
static int function_key (int n) { return KEY_F (n); }

__attribute__((constructor))
void
init_screen (void)
{
  screen_.init = init;
  screen_.finalize = finalize;
  screen_.setup_colour = setup_colour;
  screen_.getyx = getyx_;
  screen_.cursor_off = cursor_off;
  screen_.cursor_on = cursor_on;
  screen_.refresh = refresh_;
  screen_.break_ = break_;
  screen_.get_char = get_char;
  screen_.add_char = add_char;
  screen_.add_string = add_string;
  screen_.add_char_reverse = add_char_reverse;
  screen_.add_string_reverse = add_string_reverse;
  screen_.add_char_underline = add_char_underline;
  screen_.add_string_underline = add_string_underline;
  screen_.get_lines = get_lines;
  screen_.get_columns = get_columns;
  screen_.move = move_;
  screen_.move_top_left = move_top_left;
  screen_.clear = clear_;
  screen_.clear_to_line_end = clear_to_line_end;
  screen_.clear_to_screen_bottom = clear_to_screen_bottom;
  screen_.has_colors = has_colors_;
  screen_.start_color = start_color_;
  screen_.init_pair = init_pair_;
  screen_.halfdelay = halfdelay_;
  screen_.function_key = function_key;
  screen_.num_colours
    = sizeof (screen_.colour_array) / sizeof (*screen_.colour_array);
  screen_.colour_array[0] = COLOR_BLACK;
  screen_.colour_array[1] = COLOR_RED;
  screen_.colour_array[2] = COLOR_GREEN;
  screen_.colour_array[3] = COLOR_YELLOW;
  screen_.colour_array[4] = COLOR_BLUE;
  screen_.colour_array[5] = COLOR_MAGENTA;
  screen_.colour_array[6] = COLOR_CYAN;
  screen_.colour_array[7] = COLOR_WHITE;
  screen_.ERR_ = ERR;
  screen_.KEY_BACKSPACE_ = KEY_BACKSPACE;
  screen_.KEY_DOWN_ = KEY_DOWN;
  screen_.KEY_UP_ = KEY_UP;
}
