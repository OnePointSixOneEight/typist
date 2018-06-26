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

/* Alternate screen functions, for debugging/testing.  */

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "_gettext.h"
#include "utf8.h"
#include "utils.h"
#include "screen.h"

static const int ERR = -1;
static const int KEY_BACKSPACE = -263;
static const int KEY_F0 = -264;
static const int KEY_DOWN = -258;
static const int KEY_UP = -259;

struct display {
  int columns;
  int lines;
  int x;
  int y;
  int *buffer;
};

static struct display display;
static FILE *stream = stdout;

static const int reverse = 1 << 24;
static const int underline = 1 << 25;

static int
set_test_mode (FILE *stream_, int columns, int lines)
{
  int cells = lines * columns;

  if (display.buffer)
    {
      utils_.error (_("already in test mode (call ignored)"));
      return B.False;
    }

  display.columns = columns;
  display.lines = lines;
  display.x = display.y = 0;
  display.buffer = utils_.alloc (cells * sizeof (*display.buffer));
  stream = stream_;
  utils_.info (_("test mode display activated"));

  if (isatty (fileno (stdin)))
    printf (_("Typist test mode activated, enter '/help' for more.\n"));

  return B.True;
}

static void
intset (int *to, int i, int count)
{
  while (count--)
    *to++ = i;
}

static int *
get_display_cell (int y, int x)
{
  return display.buffer + y * display.columns + x;
}

static void
print_character (int c)
{
  const char *escaped = "\b\f\n\r\t\"\\";
  char *e = strchr (escaped, c & 0x7f);

  if (c <= '~' && c && e)
    fprintf (stream, "\\%c", "bfnrt\"\\"[e - escaped]);

  else if (c >= ' ' && c <= '~')
    fprintf (stream, "%c", c);

  else if (c <= 0xffff)
    fprintf (stream, "\\u%04x", c);

  else
    fprintf (stream, "\\u%04x\\u%04x",
                     ((c - 0x10000) >> 10) | 0xd800,
                     ((c - 0x10000) & 0x3ff)  | 0xdc00);
}

static void
print_display (void)
{
  int y, prior_empty = 0;

  fprintf (stream, "{\n  \"display\": {\n");
  fprintf (stream, "    \"geometry\":"
                   " \"%dx%d\",\n", display.columns, display.lines);
  fprintf (stream, "    \"buffer\": [\n");

  for (y = 0; y < display.lines; y++)
    {
      int end = display.columns, x, this_empty;

      while (end > 0 && *get_display_cell (y, end - 1) == ' ')
        end--;
      this_empty = !end;

      fprintf (stream, prior_empty && !this_empty ? "\n     " : "");
      fprintf (stream, prior_empty ? " \"" : "      \"");

      for (x = 0; x < end; x++)
        {
          int c = *get_display_cell (y, x);

          fprintf (stream, "%c",
                   c & reverse ? '~' : c & underline ? '_' : ' ');
          print_character (c & ~(reverse | underline));
        }

      fprintf (stream, y < display.lines - 1 ? "\"," : "\"");
      fprintf (stream, this_empty ? "" : "\n");

      prior_empty = this_empty;
    }

  fprintf (stream, prior_empty ? "\n" : "");
  fprintf (stream, "    ]\n  }\n}\n");
}

static void
init (void)
{
  intset (display.buffer, ' ', display.lines * display.columns);
  display.x = display.y = 0;
  utils_.info (_("test mode display initialized"));
}

static void
finalize (void)
{
  utils_.free (display.buffer);
  memset (&display, 0, sizeof (display));
  utils_.info (_("test mode display finalized"));
}

static void stub (void) {}
static void stub_i (int a) { a = a; }
static void stub_ii (int a, int b) { a = a; b = b; }
static void stub_iii (int a, int b, int c) { a = a; b = b; c = c; }

static void
getyx_ (int *y, int *x)
{
  *y = display.y;
  *x = display.x;
}

static int
refill_buffer (char *buffer, int length)
{
  memset (buffer, 0, length);

  if (isatty (fileno (stdin)))
    printf (_("typist> "));

  if (feof (stdin) || !fgets (buffer, length, stdin))
    return B.False;

  if (buffer[length - 1])
    {
      utils_.error (_("input buffer overflow (truncated)"));
      buffer[length - 1] = 0;
    }
  else
     buffer[strlen (buffer) - 1] = 0;

  return B.True;
}

static void
unescape_buffer (char *buffer)
{
  int i, j = 0;

  for (i = 0; buffer[i]; i++, j++)
    {
      if (buffer[i] == '\\' && buffer[i + 1] && buffer[i + 1] != 'u')
        {
          const char *escapes = "bfnrt\"\\";
          char *e = strchr (escapes, buffer[++i]);
          buffer[j] = e ? "\b\f\n\r\t\"\\"[e - escapes] : buffer[i];
        }
      else
        buffer[j] = buffer[i];
    }
  buffer[j] = 0;
}

static void
print_help (void)
{
  printf (_("Enter characters as JSON-escaped UTF-8 (may include Unicode).\n"));
  printf (_("The following special commands are also recognised:\n"));
  printf (_("  /show  - print the current display contents as JSON\n"));
  printf (_("  /help  - this message        /quit  - exit the program\n"));
  printf (_("  /down  - emulate Down-arrow  /up    - emulate Up-arrow\n"));
  printf (_("  /esc   - emulate Esc key     /fN    - emulate Fkey N\n"));
  printf (_("The '/' command indicator must be the first input character.\n"));
  printf (_("To pass Typist a string starting with '/', begin with '//'.\n"));
}

struct char_queue {
  int ucs[256];
  int cursor;
};

static struct char_queue queue;
static const int queue_bytes = sizeof (queue.ucs) / sizeof (queue.ucs[0]);

static void
queue_set (int *ucs)
{
  int i = 0;

  while (*ucs)
    queue.ucs[i++] = *ucs++;
  queue.ucs[i] = queue.cursor = 0;
}

static void
queue_set_single (int c)
{
  queue.ucs[0] = c;
  queue.ucs[1] = queue.cursor = 0;
}

static int
queue_empty ()
{
  return !queue.ucs[queue.cursor];
}

static int
queue_dequeue ()
{
  if (!queue_empty ())
    return queue.ucs[queue.cursor++];
  else
    return 0;
}

static void
queue_back_up (int c)
{
  if (queue.cursor > 0)
    queue.ucs[--queue.cursor] = c;
}

enum { none_, break_, continue_ };

static int
handle_command (char *buffer)
{
  int n;

  if (buffer[0] != '/')
    return none_;

  if (strcmp (buffer, "/quit") == 0)
    return break_;

  if (strcmp (buffer, "/help") == 0)
    {
      print_help ();
      return continue_;
    }

  if (strcmp (buffer, "/show") == 0)
    {
      print_display ();
      return continue_;
    }

  if (strcmp (buffer, "/down") == 0)
    {
      queue_set_single (KEY_DOWN);
      return break_;
    }

  if (strcmp (buffer, "/up") == 0)
    {
      queue_set_single (KEY_UP);
      return break_;
    }

  if (strcmp (buffer, "/esc") == 0)
    {
      queue_set_single (27);
      return break_;
    }

  if (sscanf (buffer, "/f%d", &n) == 1 && n > 0 && n < 13)
    {
      queue_set_single (KEY_F0 - n);
      return break_;
    }

  if (buffer[1] == '/')
    {
      memmove (buffer, buffer + 1, strlen (buffer));
      return none_;
    }

  if (isatty (fileno (stdin)))
    printf (_("typist: '%s': invalid command (try '/help')\n"), buffer);
  else
    utils_.error (_("invalid command '%s'"), buffer);
  return continue_;
}

static int
get_char (void)
{
  char *buffer = NULL;

  while (queue_empty () && !feof (stdin))
    {
      int status, *ucs;

      if (!buffer)
        buffer = utils_.alloc (queue_bytes);

      if (!refill_buffer (buffer, queue_bytes))
        break;

      status = handle_command (buffer);
      if (status == break_)
        break;
      else if (status == continue_)
        continue;

      unescape_buffer (buffer);

      ucs = utf8_.to_ucs (buffer);
      queue_set (ucs);
      utf8_.free (ucs);

      if (isatty (fileno (stdin)))
        {
          int length = utf8_.strlen (buffer);

          if (length == 1)
            printf (_("typist: registered 1 character\n"));
          else
            printf (_("typist: registered %d characters\n"), length);
        }
    }

  utils_.free (buffer);

  if (!queue_empty ())
    return queue_dequeue ();

  kill (getpid (), SIGHUP);
  return 0;
}

static void
push_back_char (int c)
{
  queue_back_up (c);
}

static void
put_ucs_char (int c, int standout)
{
  int *cell;

  if (c == '\b')
    {
      if (display.x-- < 0)
        {
          display.x = 0;
          if (display.y-- < 0)
            display.y = 0;
        }
      return;
    }

  cell = get_display_cell (display.y, display.x);
  *cell = c | standout;

  if (display.x++ >= display.columns)
    {
      if (display.y++ >= display.lines)
        display.y = display.lines - 1;
      display.x = 0;
    }
}

static void
add_ucs_char (int c)
{
  put_ucs_char (c, 0);
}

static void
add_ucs_char_reverse (int c)
{
  put_ucs_char (c, reverse);
}

static void
add_ucs_char_underline (int c)
{
  put_ucs_char (c, underline);
}

static void
put_utf8_string (const char *s, int standout)
{
  int *cursor, *ucs = utf8_.to_ucs (s);

  for (cursor = ucs; *cursor; cursor++)
    put_ucs_char (*cursor, standout);
  utf8_.free (ucs);
}

static void
add_utf8_string (const char *s)
{
  put_utf8_string (s, 0);
}

static void
add_utf8_string_reverse (const char *s)
{
  put_utf8_string (s, reverse);
}

static void
add_utf8_string_underline (const char *s)
{
  put_utf8_string (s, underline);
}

static int get_lines (void) { return display.lines; }
static int get_columns (void) { return display.columns; }

static void move_ (int y, int x) { display.y = y; display.x = x; }
static void move_top_left (void) { display.y = display.x = 0; }

static void
clear_ (void)
{
  intset (display.buffer, ' ', display.lines * display.columns);
}

static void clear_to_line_end (void)
{
  int *cell = get_display_cell (display.y, display.x);

  intset (cell, ' ', display.columns - display.x);
}

static void clear_to_screen_bottom (void)
{
  int *cell = get_display_cell (display.y, display.x);
  int count = display.lines * display.columns - (cell - display.buffer);

  intset (cell, ' ', count);
}

static int has_colours_ (void) { return B.False; }
static int function_key (int n) { return KEY_F0 - n; }

static struct screen_ uncurse_;

static void
_test_mode (void *v_stream, int columns, int lines)
{
  if (set_test_mode ((FILE*)v_stream, columns, lines))
    memcpy (&screen_, &uncurse_, sizeof (screen_));
}

__attribute__((constructor))
void
init_uncurse (void)
{
  screen_._test_mode = _test_mode;
  uncurse_._test_mode = _test_mode;
  uncurse_.init = init;
  uncurse_.finalize = finalize;
  uncurse_.setup_colour = stub_ii;
  uncurse_.getyx = getyx_;
  uncurse_.cursor_off = stub;
  uncurse_.cursor_on = stub;
  uncurse_.refresh = stub;
  uncurse_.break_ = stub;
  uncurse_.get_char = get_char;
  uncurse_.push_back_char = push_back_char;
  uncurse_.add_ucs_char = add_ucs_char;
  uncurse_.add_ucs_char_reverse = add_ucs_char_reverse;
  uncurse_.add_ucs_char_underline = add_ucs_char_underline;
  uncurse_.add_utf8_string = add_utf8_string;
  uncurse_.add_utf8_string_reverse = add_utf8_string_reverse;
  uncurse_.add_utf8_string_underline = add_utf8_string_underline;
  uncurse_.get_lines = get_lines;
  uncurse_.get_columns = get_columns;
  uncurse_.move = move_;
  uncurse_.move_top_left = move_top_left;
  uncurse_.clear = clear_;
  uncurse_.clear_to_line_end = clear_to_line_end;
  uncurse_.clear_to_screen_bottom = clear_to_screen_bottom;
  uncurse_.has_colours = has_colours_;
  uncurse_.start_colour = stub;
  uncurse_.init_pair = stub_iii;
  uncurse_.halfdelay = stub_i;
  uncurse_.function_key = function_key;
  uncurse_.beep = stub;
  uncurse_.ERR_ = ERR;
  uncurse_.KEY_BACKSPACE_ = KEY_BACKSPACE;
  uncurse_.KEY_DOWN_ = KEY_DOWN;
  uncurse_.KEY_UP_ = KEY_UP;
  uncurse_.num_colours
    = sizeof (uncurse_.colour_array) / sizeof (*uncurse_.colour_array);
  memset (uncurse_.colour_array, 0, sizeof (uncurse_.colour_array));
}
