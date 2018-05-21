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

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>
#if defined(LONG_OPTIONS)
#include <getopt.h>
#endif

#include "buffer.h"
#include "map.h"
#include "keymap.h"
#include "screen.h"
#include "script.h"
#include "utils.h"
#include "vector.h"

/* Configuration and internationalisation elements.  */
struct constants {
  const char *MODE_TUTORIAL;
  const char *MODE_QUERY;
  const char *MODE_DRILL;
  const char *MODE_SPEED_TEST;
  const char *MODE_MENU;
  const char *MODE_EXIT;

  char QUERY_YES;
  char QUERY_NO;
  char DRILL_CHARACTER_ERROR;
  char DRILL_NEWLINE_ERROR;
  char ESCAPE;
  char CTRL_D;
  char BELL;
  char DELETE;

  const char *EXERCISE_MESSAGE;
  const char *WAIT_MESSAGE;
  const char *EXIT_MESSAGE;
  const char *REPEAT_MESSAGE;
  const char *REGRESS_MESSAGE;
  const char *FAILED_MESSAGE;
  const char *MENU_MESSAGE;
  const char *CONFIRM_MESSAGE;
  const char *SPEED_RAW;
  const char *SPEED_ADJUSTED;
  const char *SPEED_PERCENT_ERROR;

  const char *DEFAULT_SCRIPT;
  const char *PSEUDO_FUNCTION_KEYS;
};

static struct constants constants;

static void
init_constants (void)
{
  constants.MODE_TUTORIAL   = " Tutorial ";
  constants.MODE_QUERY      = "  Query   ";
  constants.MODE_DRILL      = "  Drill   ";
  constants.MODE_SPEED_TEST = "Speed test";
  constants.MODE_MENU       = "   Menu   ";
  constants.MODE_EXIT       = "   Exit   ";

  constants.QUERY_YES = 'Y';
  constants.QUERY_NO  = 'N';
  constants.DRILL_CHARACTER_ERROR = '^';
  constants.DRILL_NEWLINE_ERROR   = '<';
  constants.ESCAPE = 27;
  constants.CTRL_D = 4;
  constants.BELL   = 7;
  constants.DELETE = 127;

  constants.EXERCISE_MESSAGE = " (Pressing Escape will restart or terminate"
                               " this exercise) ";
  constants.WAIT_MESSAGE     = " Press Return or Space to continue... ";
  constants.EXIT_MESSAGE     = " Finished -"
                               " press Return or Space to exit... ";
  constants.REPEAT_MESSAGE   = " Press 'R' to repeat, Return or Space"
                               " to continue... ";
  constants.REGRESS_MESSAGE  = " Too many errors -"
                               " regressing to an earlier exercise... ";
  constants.FAILED_MESSAGE   = " Error rate of %1.0f%% is too high;"
                               " repeat (or press 'E' to leave)...";
  constants.MENU_MESSAGE     = " Up and Down to move, Return or Space"
                               " to select, or Escape ";
  constants.CONFIRM_MESSAGE  = " Leaving this menu exits the program -"
                               " press 'Q' to confirm ";

  constants.SPEED_RAW           = " Raw speed       %6.2f wpm ";
  constants.SPEED_ADJUSTED      = " Adjusted speed  %6.2f wpm ";
  constants.SPEED_PERCENT_ERROR = "           with %3.0f%% errors ";

  constants.DEFAULT_SCRIPT = "default.json";
  constants.PSEUDO_FUNCTION_KEYS = "qwertyuiopas";
}

/* Table of pseudo-function keys, to allow ^Q to double as Fkey1, etc.  */
static char pseudo_function_keys[12];

/* ASCII ctrl-X is 'X' - 0100.  */
static void
init_pseudo_function_keys (void)
{
  int i;
  const char *surrogates = constants.PSEUDO_FUNCTION_KEYS;
  for (i = 0; surrogates[i]; i++)
    pseudo_function_keys[i] = toupper (surrogates[i]) - ('A' - 1);
}

/* Command line options/values.  */
struct options {
  const char *version;        /* Version */
  const char *copyright;      /* GPL copyright */
  int no_timer;               /* No timings in drills */
  keymapper_s *keymapper;     /* Maps keyboard layouts if set */
  int terminal_cursor;        /* Don't do software cursor */
  int cursor_flash_period;    /* Cursor flash period */
  int no_sound;               /* No beep on errors */
  char *start_label;          /* Initial lesson start point */
  int colour;                 /* Set if -c given */
  int foreground_colour;      /* Foreground colour */
  int background_colour;      /* Background colour */
  int no_wordprocessor_mode;  /* Don't do wordprocessor-like stuff */
  double failure_percent;     /* Default error limit %age for exercises */
  int quiet;                  /* Suppress printing of errors/warnings */
  FILE *message_stream;       /* Use in place of stderr for errors/warnings */
  int loop_limit;             /* Action count limit for unbreakable loop */
  int depth_limit;            /* Nested execution depth limit */
  int strict_json;            /* Use a strict JSON parser */
  int parse_only;             /* Debug aid, parse script and then exit */
  int print_parse;            /* Debug aid, print script's interal repr */
};

static struct options options;

static void
init_options (const char *version, const char *copyright)
{
  memset (&options, 0, sizeof (options));

  options.version = version;
  options.copyright = copyright;

  options.cursor_flash_period = 10;
  options.failure_percent = 3.0;
  options.loop_limit = 4096;
  options.depth_limit = 32;
}

/* Runtime context.  */
struct context {
  script_s *script;                   /* Script being interpreted */
  int query_flag;                     /* Most recent Y/N query response */
  int previous_action;                /* Most recently completed action */
  map_s *labels;                      /* Map of labels to script positions */
  int num_function_keys;              /* Number of function keys */
  char *function_key_binding[12];     /* Labels bound to function keys */
  map_s *menu_memos;                  /* Saved menu selections */
  double error_limit;                 /* Non-practice error limit */
  int error_limit_persistent;         /* Indicates error limit persistence */
  char *on_failure_label;             /* Transfer if drill/speed test fail */
  int on_failure_persistent;          /* Indicates on_failure persistence */
  int execution_depth;                /* Level of execution nesting */
};
typedef struct context context_s;

static void
init_context (context_s *context,
              script_s *script, int execution_depth)
{
  memset (context, 0, sizeof (*context));

  context->script = script;

  /* Script positions from strings, and menu selections (int index) from
     positions. 127 and 31 are Mersenne primes.  */
  context->labels = map_.create (sizeof (int), 0, 127);
  context->menu_memos = map_.create (sizeof (int), sizeof (int), 31);

  context->num_function_keys
    = sizeof (context->function_key_binding)
      / sizeof (*context->function_key_binding);

  context->error_limit = -1.0;
  context->execution_depth = execution_depth;
}

static void
destroy_context (context_s *context)
{
  script_.close (context->script);
  map_.destroy (context->labels);
  map_.destroy (context->menu_memos);
  memset (context, 0, sizeof (*context));
}

/* Some screen postions.  */
static int message_line (void) { return screen_.get_lines () - 1; }
static int banner_top_line (void) { return 0; }
static int informational_top_line (void) { return banner_top_line () + 1; }
static int instruction_top_line (void) { return informational_top_line (); }
static int exercise_top_line (void) { return instruction_top_line () + 2; }
static int speed_line (void) { return screen_.get_lines () - 5; }

/* Get a typed character with invisible cursor.  */
static int
get_char_with_no_cursor (void)
{
  int y, x;
  int return_char;

  screen_.getyx (&y, &x);

  screen_.cursor_off ();
  screen_.refresh ();
  screen_.move_top_left ();
  screen_.break_ ();
  return_char = screen_.get_char ();
  if (options.keymapper)
    return_char = keymapper_.convert (options.keymapper, return_char);
  screen_.move (y, x);
  if (return_char == constants.CTRL_D)
    return constants.ESCAPE;
  return return_char;
}

/* Here we want a way to get a character that always uses a flashing cursor,
   to separate out the inverse cursor from the inverse mistyping indicator.
   Some xterms seem unwilling to do curses A_BLINK, so to get the desired
   effect we have to do the work explicitly here. To complicate things
   even more, some xterms seem not make the cursor invisible either. Offers
   ^D as a non-delaying alternative to Escape.  */
static int
get_char_with_flashing_cursor (char cursor_char)
{
  int y, x;
  int return_char;

  screen_.getyx (&y, &x);

  /* Just use the terminal's cursor.  */
  if (options.terminal_cursor)
    {
      screen_.cursor_on ();
      screen_.refresh ();
      screen_.break_ ();
      return_char = screen_.get_char ();
      if (options.keymapper)
        return_char = keymapper_.convert (options.keymapper, return_char);
      screen_.cursor_off ();
      screen_.refresh ();
      if (return_char == constants.CTRL_D)
        return constants.ESCAPE;
      return return_char;
  }

  /* Produce a flashing cursor; a block where cursor_char is ' '.  */
  screen_.add_char_reverse (cursor_char);
  screen_.cursor_off ();
  screen_.refresh ();
  screen_.move_top_left ();
  if (options.cursor_flash_period / 2 > 0)
    {
      int alternate = B.False;
      screen_.halfdelay (options.cursor_flash_period / 2);
      while ((return_char = screen_.get_char ()) == screen_.ERR_)
        {
          screen_.move (y, x);
          if (alternate)
            screen_.add_char_reverse (cursor_char);
          else
            screen_.add_char (cursor_char);
          screen_.move_top_left ();
          alternate = !alternate;
        }
    }
  else
    {
      screen_.break_ ();
      return_char = screen_.get_char ();
    }
  if (options.keymapper)
    return_char = keymapper_.convert (options.keymapper, return_char);
  screen_.move (y, x);
  screen_.add_char (cursor_char);
  screen_.move (y, x);
  if (return_char == constants.CTRL_D)
    return constants.ESCAPE;
  return return_char;
}

/* Iterate script lines and add all the labels to the label map.  */
static void
map_script_labels (context_s *context)
{
  script_s *script = context->script;
  int position;

  script_.rewind (script);

  position = script_.get_position (script);
  script_.get_next_statement (script);
  while (script_.has_more_data (script))
    {
      const char *data = script_.get_data (script);
      char *label, trail;
      int scanned;

      if (script_.get_action (script) != C.LABEL)
        {
          position = script_.get_position (script);
          script_.get_next_statement (script);
          continue;
        }

      label = utils_.strdup (data);
      scanned = sscanf (data, " %s %c ", label, &trail);

      if (scanned != 1 || label[strlen (label) - 1] == '*')
        {
          utils_.error ("bad or invalid label (action ignored): '%s'", data);

          utils_.free (label);
          position = script_.get_position (script);
          script_.get_next_statement (script);
          continue;
        }

      if (map_.contains (context->labels, label))
        utils_.warning ("label re-defined (shadowed prior): '%s'", data);

      /* Subtle -- position points to the line BEFORE the label, rather than
         the statement that follows on from it. Client code that transfers
         to a label needs to ensure that there is a call to get_next_statement
         afterwards. The 'normal' case may be that there is one on the
         statement handler exit anyway...  */
      map_.set (context->labels, label, &position);
      utils_.free (label);

      position = script_.get_position (script);
      script_.get_next_statement (script);
    }
}

/* Locate a previously indexed label and reposition the script to
   its location.  */
static void
reposition_to_label (context_s *context, const char *target)
{
  char *label, trail;
  int scanned, position;

  utils_.info ("request for transfer to label '%s'", target);

  label = utils_.strdup (target);
  scanned = sscanf (target, " %s %c ", label, &trail);

  if (scanned != 1 || !map_.get (context->labels, label, &position))
    {
      utils_.error ("bad or invalid target label (action ignored): '%s'",
                    target);
      scanned = 0;
    }

  utils_.free (label);

  if (scanned)
    script_.set_position (context->script, position);
}

/* Get the label bound to a function key (or pseudo), NULL if not bound.  */
static const char *
get_bound_label (context_s *context, int keypress)
{
  const char *label = NULL;
  int fkey;

  for (fkey = 0; fkey < context->num_function_keys; fkey++)
    {
      if (keypress == pseudo_function_keys[fkey])
        {
          keypress = screen_.function_key (fkey + 1);
          break;
        }
    }

  for (fkey = 0; fkey < context->num_function_keys; fkey++)
    {
      if (context->function_key_binding[fkey]
          && keypress == screen_.function_key (fkey + 1))
        {
          label = context->function_key_binding[fkey];
          break;
        }
    }

  return label;
}

/* Convenience functions to write to the message line and clear it.  */
static void
display_status (const char *message, const char *mode)
{
  screen_.move (message_line (), 0);
  screen_.clear_to_line_end ();

  screen_.move (message_line (), 0);
  screen_.add_string_reverse (message);

  screen_.move (message_line (),
                screen_.get_columns () - strlen (mode) - 2);
  screen_.add_string_reverse (mode);
}

static void
clear_status (void)
{
  screen_.move (message_line (), 0);
  screen_.clear_to_line_end ();
}

/* Convenience function to write a banner line.  */
void
display_banner (const char *message)
{
  int i;

  screen_.move (banner_top_line (), 0);
  screen_.clear_to_line_end ();

  for (i = 0; i < screen_.get_columns (); i++)
    screen_.add_char_reverse (' ');

  screen_.move (banner_top_line (), 0);
  screen_.add_string_reverse (message);

  screen_.move (banner_top_line (),
                screen_.get_columns () - strlen (options.version) - 1);
  screen_.add_string_reverse (options.version);
}

/* Wait for a keypress from the user before continuing. Returns the key
   pressed, or -1 if repositioned due to function key.  */
static int
wait_for_keypress (context_s *context,
                   const char *message, const char *mode,
                   const char *valid_characters)
{
  int response;

  display_status (message, mode);
  do
    {
      const char *label;
      response = get_char_with_no_cursor ();

      label = get_bound_label (context, response);
      if (label)
        {
          reposition_to_label (context, label);
          script_.get_next_statement (context->script);
          response = -1;
        }
     }
  while (response != -1 && !strchr (valid_characters, response));
  clear_status ();

  return response;
}

/* Display speed and accuracy of a drill or speed test.  */
static void
display_speed (int keystrokes, int errors, double elapsed_time)
{
  double test_time_in_minutes, words_typed;
  double speed, adjusted_speed, percent_error;
  char message[128];

  /* Calculate the speeds. elapsed_time is in seconds so divide by 60
     to get minutes. Divide keystrokes by 5 to get an effective,
     albeit somewhat estimated/average, word count. 100 times errors
     divided by keystrokes is the percentage error rate.  */
  test_time_in_minutes = elapsed_time / 60.0;
  words_typed = (double)keystrokes / 5.0;
  percent_error = 100.0 * (double)errors / (double)keystrokes;

  if (elapsed_time)
    {
      speed = words_typed / test_time_in_minutes;
      speed = speed < 999.99 ? speed : 999.99;
      adjusted_speed = (words_typed - errors / 5.0) / test_time_in_minutes;
      adjusted_speed = adjusted_speed < 999.99 ? adjusted_speed : 999.99;
    }
  else
    {
      /* Unmeasurable elapsed time -- use big numbers.  */
      speed = adjusted_speed = 999.99;
    }

  /* Display the results -- no negative numbers allowed.  */
  sprintf (message, constants.SPEED_RAW, speed);
  screen_.move (speed_line (), screen_.get_columns () - strlen (message) - 1);
  screen_.add_string_reverse (message);
  sprintf (message, constants.SPEED_ADJUSTED,
           adjusted_speed >= 0.01 ? adjusted_speed : 0.0);
  screen_.move (speed_line () + 1,
                screen_.get_columns () - strlen (message) - 1);
  screen_.add_string_reverse (message);
  sprintf (message, constants.SPEED_PERCENT_ERROR, percent_error);
  screen_.move (speed_line () + 2,
                screen_.get_columns () - strlen (message) - 1);
  screen_.add_string_reverse (message);
}

/* Convenience tracer for handler functions.  */
static void
trace_handler_entry (context_s *context, const char *function_name)
{
  script_s *script = context->script;

  utils_.info ("call to %s, script p_%p, position %d",
               function_name, p_(script), script_.get_position (script));
}

/* Print the given text onto the screen.  */
static void
handle_tutorial (context_s *context)
{
  script_s *script = context->script;
  int line = informational_top_line ();
  trace_handler_entry (context, "handle_tutorial");

  screen_.move (line, 0);
  screen_.clear_to_screen_bottom ();

  do
    {
      screen_.move (line++, 0);
      screen_.add_string (script_.get_data (script));
      script_.get_next_statement (script);
    }
  while (script_.has_more_data (script)
         && script_.get_action (script) == C.CONTINUATION);

  if (script_.get_action (script) != C.QUERY)
     wait_for_keypress (context,
                        constants.WAIT_MESSAGE, constants.MODE_TUTORIAL, " \n");
}

/* Print one or two lines, usually followed by a drill or a speed test.  */
static void
handle_instruction (context_s *context)
{
  script_s *script = context->script;
  trace_handler_entry (context, "handle_instruction");

  screen_.move (instruction_top_line (), 0);
  screen_.clear_to_screen_bottom ();
  screen_.add_string (script_.get_data (script));
  script_.get_next_statement (script);

  if (script_.has_more_data (script)
      && script_.get_action (script) == C.CONTINUATION)
    {
      screen_.move (instruction_top_line () + 1, 0);
      screen_.add_string (script_.get_data (script));
      script_.get_next_statement (script);
    }

  if (script_.has_more_data (script)
      && script_.get_action (script) == C.CONTINUATION)
    {
      utils_.error ("instruction longer than two lines (truncated)");
      do
        {
          script_.get_next_statement (script);
        }
      while (script_.has_more_data (script)
             && script_.get_action (script) == C.CONTINUATION);
    }
}

/* Execute a typing drill. Returns TRUE if the user exited early.  */
static int
handle_drill (context_s *context, const char *data,
              int *return_keystrokes, int *return_errors)
{
  int errors, line, keystrokes;
  const char *cursor;
  int key = 0;
  double timer = 0.0;
  trace_handler_entry (context, "handle_drill");

  /* If the last action was a tutorial or a menu, ensure we have the
     complete screen.  */
  if (context->previous_action == C.TUTORIAL
      || context->previous_action == C.MENU)
    {
      screen_.move (informational_top_line (), 0);
      screen_.clear_to_screen_bottom ();
    }

  /* Display the drill pattern.  */
  line = exercise_top_line ();
  screen_.move (line, 0);
  screen_.clear_to_screen_bottom ();
  for (cursor = data; *cursor; cursor++)
    {
      if (*cursor != '\n')
        screen_.add_char (*cursor);
      else
        {
          line += 2;  /* Alternate lines */
          screen_.move (line, 0);
        }
    }

  display_status (constants.EXERCISE_MESSAGE, constants.MODE_DRILL);

  errors = keystrokes = 0;
  line = exercise_top_line () + 1;

  /* Execute the exercise.  */
  screen_.move (line, 0);
  for (cursor = data; *cursor == ' '; cursor++)
    screen_.add_char (' ');
  for (; *cursor; cursor++)
    {
      const char screen_cursor = *cursor == '\t' ? '\t' : ' ';
      int correct;

      key = get_char_with_flashing_cursor (screen_cursor);
      if (key == constants.ESCAPE)
        break;

      /* Ignore delete or backspace in drills.  */
      if (key == screen_.KEY_BACKSPACE_
          || key == '\b' || key == constants.DELETE)
        {
          cursor--;  /* Defeat cursor++ at loop head */
          continue;
        }

      /* Start the timer on the first keystroke.  */
      if (!keystrokes)
        timer = utils_.start_timer ();
      keystrokes++;

      /* Check that the character was correct.  */
      correct = key == *cursor || (!options.no_wordprocessor_mode
                                   && key == ' ' && *cursor == '\n');
      if (correct)
        screen_.add_char_underline (*cursor);
      else
        {
          char error_indicator;
          if (*cursor == '\t')
            error_indicator = '\t';
          else if (*cursor == '\n')
            error_indicator = constants.DRILL_NEWLINE_ERROR;
          else
            error_indicator = isprint (key) ? key
                              : constants.DRILL_CHARACTER_ERROR;
          screen_.add_char_reverse (error_indicator);
          if (!options.no_sound)
            {
              putchar (constants.BELL);
              fflush (stdout);
            }
          errors++;
        }

      /* Move screen location if newline.  */
      if (*cursor == '\n')
        {
          line += 2;
          screen_.move (line, 0);
        }

      /* Perform any other wordprocessor-like adjustments.  */
      if (correct && !options.no_wordprocessor_mode)
        {
          if (key == ' ')
            {
              while (cursor[1] == ' ')
                {
                  cursor++;
                  screen_.add_char_underline (*cursor);
                }
            }
          else if (key == '\n')
            {
              while (cursor[1] == ' ' || cursor[1] == '\n')
                {
                  cursor++;
                  screen_.add_char_underline (*cursor);
                  if (*cursor == '\n')
                    {
                      line += 2;
                      screen_.move (line, 0);
                    }
                }
            }
          else if (cursor[0] != '-'
                   && cursor[1] == '-' && cursor[2] == '\n')
            {
              cursor += 2;
              screen_.add_string_underline ("-\n");
              line += 2;
              screen_.move (line, 0);
            }
        }
    }

  if (timer && key != constants.ESCAPE && !options.no_timer)
    display_speed (keystrokes, errors, utils_.timer_interval (timer));

  *return_keystrokes = keystrokes;
  *return_errors = errors;

  utils_.info ("handle_drill"
               " recorded %d keystrokes, %d errors, early exit is b_%d",
               keystrokes, errors, key == constants.ESCAPE);

  return key == constants.ESCAPE;
}

/* Execute a timed speed test. Returns TRUE if the user exited early.  */
static int
handle_speed_test (context_s *context, const char *data,
                   int *return_keystrokes, int *return_errors)
{
  int errors, line, keystrokes;
  const char *cursor;
  int key = 0;
  double timer = 0.0;
  trace_handler_entry (context, "handle_speed_test");

  /* If the last action was a tutorial or a menu, ensure we have the
     complete screen.  */
  if (context->previous_action == C.TUTORIAL
      || context->previous_action == C.MENU)
    {
      screen_.move (informational_top_line (), 0);
      screen_.clear_to_screen_bottom ();
    }

  /* Display the speed test pattern.  */
  line = exercise_top_line ();
  screen_.move (line, 0);
  screen_.clear_to_screen_bottom ();
  for (cursor = data; *cursor; cursor++)
    {
      if (*cursor != '\n')
        screen_.add_char (*cursor);
      else
        {
          line++;
          screen_.move (line, 0);
        }
    }

  display_status (constants.EXERCISE_MESSAGE, constants.MODE_SPEED_TEST);

  errors = keystrokes = 0;
  line = exercise_top_line ();

  /* Execute the exercise.  */
  screen_.move (line, 0);
  for (cursor = data; *cursor == ' '; cursor++)
    screen_.add_char (' ');
  for (; *cursor; cursor++)
    {
      const char screen_cursor = *cursor == '\n' ? ' ' : *cursor;
      int correct;

      key = get_char_with_flashing_cursor (screen_cursor);
      if (key == constants.ESCAPE)
        break;

      /* Start timer on the first keystroke.  */
      if (!keystrokes)
        timer = utils_.start_timer ();
      keystrokes++;

      /* Check for delete keys if not at line start or speed test start.  */
      if (key == screen_.KEY_BACKSPACE_
          || key == '\b' || key == constants.DELETE)
        {
          /* Just ignore deletes where handling is hard or impossible.  */
          if (cursor > data && !(cursor[-1] == '\n' || cursor[-1] == '\t'))
            {
              /* Back up one character.  */
              screen_.add_char ('\b');
              cursor--;
            }
          cursor--;  /* Defeat cursor++ at loop head */
          continue;
        }

      /* Check that the character was correct.  */
      correct = key == *cursor || (!options.no_wordprocessor_mode
                                   && key == ' ' && *cursor == '\n');
      if (correct)
        screen_.add_char_underline (*cursor);
      else
        {
          char error_indicator;
          if (*cursor == '\n')
            error_indicator = constants.DRILL_NEWLINE_ERROR;
          else
            error_indicator = *cursor;
          screen_.add_char_reverse (error_indicator);
          if (!options.no_sound)
            {
              putchar (constants.BELL);
              fflush (stdout);
            }
          errors++;
        }

      /* Move screen location if newline.  */
      if (*cursor == '\n')
        {
          line++;
          screen_.move (line, 0);
        }

      /* Perform any other wordprocessor-like adjustments.  */
      if (correct && !options.no_wordprocessor_mode)
        {
          if (key == ' ')
            {
              while (cursor[1] == ' ')
                {
                  cursor++;
                  screen_.add_char_underline (*cursor);
                }
            }
          else if (key == '\n')
            {
              while (cursor[1] == ' ' || cursor[1] == '\n')
                {
                  cursor++;
                  screen_.add_char_underline (*cursor);
                  if (*cursor == '\n')
                    screen_.move (++line, 0);
                }
            }
          else if (cursor[0] != '-'
                   && cursor[1] == '-' && cursor[2] == '\n')
            {
              cursor += 2;
              screen_.add_string_underline ("-\n");
              screen_.move (++line, 0);
            }
        }
    }

  if (timer && key != constants.ESCAPE)
    display_speed (keystrokes, errors, utils_.timer_interval (timer));

  *return_keystrokes = keystrokes;
  *return_errors = errors;

  utils_.info ("handle_speed_test"
               " recorded %d keystrokes, %d errors, early exit is b_%d",
               keystrokes, errors, key == constants.ESCAPE);

  return key == constants.ESCAPE;
}

/* Buffer the complete data for a script element that has continuations. */
static void
get_full_element_data (script_s *script, buffer_s *buffer)
{
  do
    {
      const char *data = script_.get_data (script);

      buffer_.append (buffer, data, strlen (data));
      buffer_.append (buffer, "\n", 1);

      script_.get_next_statement (script);
    }
  while (script_.has_more_data (script)
         && script_.get_action (script) == C.CONTINUATION);
  buffer_.append (buffer, "\0", 1);
}

/* Handle repeated invocations of a practice drill or speed test. */
static void
handle_practice_exercise (context_s *context, int action)
{
  buffer_s *buffer = buffer_.create (0);
  const char *data;
  trace_handler_entry (context, "handle_practice_exercises");

  get_full_element_data (context->script, buffer);
  data = buffer_.get (buffer);

  /* Run a practice while the user requests repeat, ignore typing errors.  */
  while (B.True)
    {
      const char *mode = NULL;
      int terminated = 0, response, keystrokes, errors;

      if (action == C.DRILL_PRACTICE)
        {
          mode = constants.MODE_DRILL;
          terminated = handle_drill (context, data, &keystrokes, &errors);
        }
      else if (action == C.SPEED_TEST_PRACTICE)
        {
          mode = constants.MODE_SPEED_TEST;
          terminated = handle_speed_test (context, data, &keystrokes, &errors);
        }
      else
        utils_.fatal ("internal error: bad call to handle_practice_exercise");

      /* If terminated in mid-exercise, restart it. */
      if (terminated && keystrokes > 0)
        continue;

      /* 'R' to repeat, Return to continue.  */
      response = wait_for_keypress (context,
                                    constants.REPEAT_MESSAGE, mode, "Rr \n");
      if (toupper (response) == 'R')
        continue;
      else
        break;
    }

  buffer_.destroy (buffer);
}

/* Handle repeated invocations of a drill or speed test. */
static void
handle_exercise (context_s *context, int action)
{
  buffer_s *buffer = buffer_.create (0);
  const char *data;
  trace_handler_entry (context, "handle_exercise");

  get_full_element_data (context->script, buffer);
  data = buffer_.get (buffer);

  /* Run an exercise until the user achieves an acceptably low error rate.  */
  while (B.True)
    {
      const char *mode = NULL;
      int terminated = 0, response, keystrokes, errors, failed;
      double error_rate;

      if (action == C.DRILL)
        {
          mode = constants.MODE_DRILL;
          terminated = handle_drill (context, data, &keystrokes, &errors);
        }
      else if (action == C.SPEED_TEST)
        {
          mode = constants.MODE_SPEED_TEST;
          terminated = handle_speed_test (context, data, &keystrokes, &errors);
        }
      else
        utils_.fatal ("internal error: bad call to handle_exercise");

      /* If terminated in mid-exercise, restart it. */
      if (terminated && keystrokes > 0)
        continue;

      if (terminated)
        {
          /* 'R' to repeat, Return to continue.  */
          response = wait_for_keypress (context,
                                        constants.REPEAT_MESSAGE,
                                        mode, "Rr \n");
          if (toupper (response) == 'R')
            continue;
          else
            break;
        }

      /* Compute the error rate from values returned by the exercise,
         and compare to either the per-exercise error limit or the
         one set by general options.  */
      error_rate = 100.0 * (double)errors / (double)keystrokes;
      if (context->error_limit >= 0.0)
        failed = error_rate > context->error_limit;
      else
        failed = error_rate > options.failure_percent;

      if (failed && context->on_failure_label)
        {
          /* Return to regress and leave the loop.  */
           wait_for_keypress (context,
                              constants.REGRESS_MESSAGE, mode, " \n");
           reposition_to_label (context, context->on_failure_label);
           script_.get_next_statement (context->script);

           if (!context->on_failure_persistent)
             {
               utils_.free (context->on_failure_label);
               context->on_failure_label = NULL;
             }
           break;
        }

      if (failed)
        {
          char message[128];
          sprintf (message, constants.FAILED_MESSAGE, error_rate);

          /* 'E' to leave the loop, Return or Space to repeat.  */
          response = wait_for_keypress (context, message, mode, "Ee \n");
          if (toupper (response) == 'E')
            break;
          else
            continue;
        }
      else
        {
          /* Return or Space to leave the loop.  */
          wait_for_keypress (context, constants.WAIT_MESSAGE, mode, " \n");
          break;
        }
    }

  if (!context->error_limit_persistent)
    context->error_limit = -1.0;

  buffer_.destroy (buffer);
}

/* Implement a simple selection menu.  */
static void
handle_menu (context_s *context)
{
  script_s *script = context->script;
  const char *data;
  char *up, *up_label, *title, *target;
  int position, scanned, extent = -1;
  struct { char *label; char *descr; } pair;
  vector_s *menu_pairs;
  int menu_entries, item, pad_to, selected;
  trace_handler_entry (context, "handle_menu");

  /* Find the memo for this menu, or create one if not yet visited.  */
  position = script_.get_position (script);
  if (!map_.get (context->menu_memos, &position, &selected))
    {
      selected = 0;
      map_.set (context->menu_memos, &position, &selected);
    }

  data = script_.get_data (script);

  /* Extract the any UP=... setting and the menu title from the first line.  */
  up = utils_.strdup (data);
  up_label = utils_.strdup (data);
  scanned = sscanf (data, " %[^=\"]=%s %n", up, up_label, &extent);
  if (extent < 0)
    sscanf (data, " %n", &extent);

  /* If the UP=... is not valid, store "" in the up_label. This ensures
     that it is not NULL, and so will terminate the later selection loop.  */
  if (scanned == 2 && strcasecmp (up, "up") == 0)
    {
      if (strcasecmp (up_label, "_exit") != 0
          && !map_.contains (context->labels, up_label))
        {
          utils_.error ("invalid menu up target (UP=... ignored): '%s'", data);
          strcpy (up_label, "");
        }
    }
  else
    strcpy (up_label, "");
  utils_.free (up);

  /* The rest of the line is the menu title, stripped of enclosing quotes.  */
  extent += data[extent] == '"' ? 1 : 0;
  title = utils_.strdup (data + extent);
  if (title[strlen (title) - 1] == '"')
    title[strlen (title) - 1] = 0;

  /* Growable arrays for menu item labels and descriptions.  */
  menu_pairs = vector_.create (sizeof (pair));

  pad_to = 0;

  /* Extract the label and menu string for each menu item.  */
  script_.get_next_statement (script);
  while (script_.has_more_data (script)
         && script_.get_action (script) == C.CONTINUATION)
    {
      const char *item_data = script_.get_data (script);
      int descr_length;

      pair.label = utils_.strdup (item_data);
      scanned = sscanf (item_data, " %s %n", pair.label, &extent);

      if (scanned != 1 || !map_.contains (context->labels, pair.label))
        {
          utils_.error ("bad or invalid menu entry (entry ignored): '%s'",
                        item_data);

          script_.get_next_statement (script);
          continue;
        }

      extent += item_data[extent] == '"' ? 1 : 0;
      pair.descr = utils_.strdup (item_data + extent);
      if (pair.descr[strlen (pair.descr) - 1] == '"')
        pair.descr[strlen (pair.descr) - 1] = 0;

      /* Find the longest item description, for later display padding.  */
      descr_length = strlen (pair.descr);
      if (descr_length > pad_to)
        pad_to = descr_length;

      vector_.push_back (menu_pairs, &pair);
      script_.get_next_statement (script);
    }

  /* Start at the top of the screen, and clear it.  */
  screen_.move (informational_top_line (), 0);
  screen_.clear_to_screen_bottom ();

  /* Write the menu title, centred, and free it.  */
  screen_.move (informational_top_line (),
                (screen_.get_columns () - strlen (title)) / 2);
  screen_.add_string (title);
  utils_.free (title);

  /* Repeat menu display until user selection. Starting selection is
     anything previously selected for this menu, or 0 if first visit.  */
  menu_entries = vector_.get_length (menu_pairs);
  target = NULL;
  while (!target && menu_entries)
    {
      int control, line;

      /* Menu items display two lines below the menu title.  */
      line = informational_top_line () + 2;
      screen_.move (line, 1);
      screen_.clear_to_screen_bottom ();
      for (item = 0; item < menu_entries; item++)
        {
          vector_.get (menu_pairs, item, &pair);
          if (item == selected)
            {
              int padding;
              screen_.add_string_reverse (pair.descr);
              for (padding = strlen (pair.descr);
                   padding < pad_to; padding++)
                screen_.add_char_reverse (' ');
            }
          else
            screen_.add_string (pair.descr);
          line++;
          screen_.move (line, 1);
        }

      display_status (constants.MENU_MESSAGE, constants.MODE_MENU);

      control = get_char_with_no_cursor ();
      if (control == constants.ESCAPE || toupper (control) == 'Q')
        target = up_label;

      else if (control == ' ' || control == '\n')
        {
          vector_.get (menu_pairs, selected, &pair);
          target = pair.label;
        }

      else if (control == screen_.KEY_DOWN_ || toupper (control) == 'J')
        {
          if (++selected > menu_entries - 1)
            selected = menu_entries - 1;
        }
      else if (control == screen_.KEY_UP_ || toupper (control) == 'K')
        {
          if (--selected < 0)
            selected = 0;
        }

      /* If target would trigger program exit, confirm first.  */
      if (target == up_label)
        {
          if (strcasecmp (target, "_exit") == 0
              && !map_.contains (context->labels, target))
            {
              int response;
              response = wait_for_keypress (context,
                                            constants.CONFIRM_MESSAGE,
                                            constants.MODE_MENU, "Qq \n");
              if (toupper (response) != 'Q')
                target = NULL;
            }
        }
    }

  /* Retain this selection in the memo for this menu.  */
  map_.set (context->menu_memos, &position, &selected);

  /* If no UP=... and escape pressed, target will be "". In this case we
     drop through the menu to the next action (if any).  */
  if (target && strlen (target) > 0)
    {
      if (strcasecmp (target, "_exit") == 0
          && !map_.contains (context->labels, target))
        {
          /* _EXIT is a special case. If not defined as a label, treat as
             program exit. Effect by reading forwards to the script end.  */
          while (script_.has_more_data (script))
            script_.get_next_statement (script);
        }
      else
        {
          reposition_to_label (context, target);
          script_.get_next_statement (script);
        }
    }
  else
    /* Note: dropping through does not match gtypist. It treats lack of
       an UP=... as go back to the prior menu. We can't readily do this
       here as maps are not ordered.  */
    wait_for_keypress (context,
                       constants.WAIT_MESSAGE, constants.MODE_MENU, " \n");

  utils_.free (up_label);
  for (item = 0; item < vector_.get_length (menu_pairs); item++)
    {
      vector_.get (menu_pairs, item, &pair);
      utils_.free (pair.label);
      utils_.free (pair.descr);
    }
  vector_.destroy (menu_pairs);
}

/* Clear the complete screen and put a banner on the top line.  */
static void
handle_clear_screen (context_s *context)
{
  trace_handler_entry (context, "handle_clear_screen");

  screen_.clear ();
  display_banner (script_.get_data (context->script));
  script_.get_next_statement (context->script);
}

/* Transfer a label. The flag is used to implement conditional goto's.  */
static void
handle_goto (context_s *context, int flag)
{
  trace_handler_entry (context, flag ? "handle_goto[flag=b_1]"
                                     : "handle_goto[flag=b_0]");
  if (flag)
    reposition_to_label (context, script_.get_data (context->script));
  script_.get_next_statement (context->script);
}

/* Exit from the program, implied at the end of the script.  */
static void
handle_exit (context_s *context)
{
  script_s *script = context->script;
  trace_handler_entry (context, "handle_exit");

  /* If more script data this call is from a script exit. If no more
     script data this call is the one that is implied at the end of
     the script.  */
  if (script_.has_more_data (script))
    {
      /* Advance script to its end -- this causes the interpreter main
         loop to terminate.  */
      while (script_.has_more_data (script))
        script_.get_next_statement (script);
    }
  else if (context->execution_depth == 0)
    /* No user message unless this is the outer-level script.  */
    wait_for_keypress (context,
                       constants.EXIT_MESSAGE, constants.MODE_EXIT, " \n");
}

/* Obtain a Y/N response from the user.  */
static void
handle_query (context_s *context)
{
  trace_handler_entry (context, "handle_query");

  display_status (script_.get_data (context->script), constants.MODE_QUERY);
  while (B.True)
    {
      const char *label;
      int response;
      response = get_char_with_no_cursor ();

      label = get_bound_label (context, response);
      if (label)
        {
          reposition_to_label (context, label);
          break;
        }

      if (toupper (response) == constants.QUERY_YES)
        {
          context->query_flag = B.True;
          break;
        }
      if (toupper (response) == constants.QUERY_NO)
        {
          context->query_flag = B.False;
          break;
        }
    }
  clear_status ();

  script_.get_next_statement (context->script);
}

/* Bind a function key to a label.  */
static void
handle_bind_function_key (context_s *context)
{
  const char *data = script_.get_data (context->script);
  int fkey, scanned;
  char *label, trail;
  trace_handler_entry (context, "handle_bind_function_key");

  /* Extract the fkey number and label string, and check the syntax and
     correctness of the mappings.  */
  label = utils_.alloc (strlen (data) + 1);
  scanned = sscanf (data, " %d : %s %c ", &fkey, label, &trail);

  if (scanned != 2)
    {
      utils_.error ("invalid key binding (want <number>:<label>): '%s'", data);
      scanned = 0;
    }
  else if (fkey < 1 || fkey > context->num_function_keys)
    {
      utils_.error ("invalid function key number (range is 1 to %d): '%s'",
                    context->num_function_keys, data);
      scanned = 0;
    }

  if (scanned)
    {
      utils_.info ("binding fkey %d to label '%s'", fkey, label);

      /* Free any previous binding and allocated data.  */
      fkey--;
      utils_.free (context->function_key_binding[fkey]);
      context->function_key_binding[fkey] = NULL;

      /* Add the association to the array, or unbind the association
         if the target is the special label "null".  */
      if (strcasecmp (label, "null") != 0)
        context->function_key_binding[fkey] = label;
      else
        utils_.free (label);
    }

  script_.get_next_statement (context->script);
}

/* Set the error limit for (non-practice) drills and speed tests.  */
static void
handle_set_error_limit (context_s *context, int force_persistent)
{
  const char *data = script_.get_data (context->script);
  char *special, trail;
  int scanned, default_;
  double error_limit;
  char persistent = ' ';
  trace_handler_entry (context, force_persistent
                                ? "handle_set_error_limit[force=b_1]"
                                : "handle_set_error_limit[force=b_0]");

  special = utils_.strdup (data);
  if (sscanf (data, " %s %c ", special, &trail) == 1)
    default_ = strcasecmp (special, "default") == 0;
  else
    default_ = B.False;
  utils_.free (special);

  if (default_)
    {
      utils_.info ("setting error limit to <default>");
      context->error_limit = -1.0;
      context->error_limit_persistent = B.False;

      script_.get_next_statement (context->script);
      return;
    }

  scanned = sscanf (data, " %lf %% %1[*] %c ",
                    &error_limit, &persistent, &trail);

  if (!(scanned == 1 || (scanned == 2 && persistent == '*')))
    {
      utils_.error ("bad error limit,"
                    " not <float>%%[*] (action ignored): '%s'", data);
      scanned = 0;
    }
  else if (error_limit < 0.0 || error_limit > 100.00)
    {
      utils_.error ("bad error limit,"
                    " outside 0-100%% (action ignored): '%s'", data);
      scanned = 0;
    }

  if (scanned)
    {
      utils_.info ("setting error limit to f_%.6f, persistent is b_%d",
                   error_limit, persistent == '*');
      context->error_limit = error_limit;
      context->error_limit_persistent = persistent == '*';
      context->error_limit_persistent |= force_persistent;
    }

  script_.get_next_statement (context->script);
}

/* Set the drill failure transfer label.  */
static void
handle_on_failure_goto (context_s *context, int force_persistent)
{
  const char *data = script_.get_data (context->script);
  char *special, *label, trail;
  int scanned, null_;
  char persistent = ' ';
  trace_handler_entry (context, force_persistent
                                ? "handle_on_failure_goto[force=b_1]"
                                : "handle_on_failure_goto[force=b_0]");

  special = utils_.strdup (data);
  if (sscanf (data, " %s %c ", special, &trail) == 1)
    null_ = strcasecmp (special, "null") == 0;
  else
    null_ = B.False;
  utils_.free (special);

  if (null_)
    {
      utils_.info ("setting error limit to <null>");
      utils_.free (context->on_failure_label);
      context->on_failure_label = NULL;
      context->on_failure_persistent = B.False;

      script_.get_next_statement (context->script);
      return;
    }

  label = utils_.strdup (data);
  scanned = sscanf (data, " %s %1[*] %c ", label, &persistent, &trail);

  if (scanned == 1 && strlen (label) > 1 && label[strlen (label) - 1] == '*')
    {
      /* Pretend that the %s above did not capture the '*'.  */
      label[strlen (label) - 1] = 0;
      persistent = '*';
      scanned = 2;
    }

  if (!(scanned == 1 || (scanned == 2 && persistent == '*')))
    {
      utils_.error ("bad failure label,"
                    " not <label>[*] (action ignored): '%s'", data);
      scanned = 0;
    }
  else if (!map_.contains (context->labels, label))
    {
      utils_.error ("bad failure label,"
                    " does not exist (action ignored): '%s'", data);
      scanned = 0;
    }

  if (scanned)
    {
      utils_.info ("setting on_failure label to '%s', persistent is b_%d",
                   label, persistent == '*');
      utils_.free (context->on_failure_label);
      context->on_failure_label = label;
      context->on_failure_persistent = persistent == '*';
      context->on_failure_persistent |= force_persistent;
    }
  else
    utils_.free (label);

  script_.get_next_statement (context->script);
}

/* Forward declaration of reentrant execution entry point.  */
void execute_script (context_s *context);

/* Execute another script.  */
static void
handle_execute (context_s *context)
{
  const char *search_path = getenv ("TYPIST_PATH");
  char *name, *path;
  script_s *script;
  context_s other;
  trace_handler_entry (context, "handle_execute");

  if (context->execution_depth > options.depth_limit - 1)
    utils_.fatal ("execution nesting"
                  " depth limit %d exceeded", options.depth_limit);

  path = utils_.strdup (script_.get_data (context->script));
  name = path + strspn (path, " \t");
  name[strcspn (name, " \t")] = 0;
  script = script_.load (name, search_path, options.strict_json);
  utils_.free (path);

  if (!script)
    {
      utils_.error ("failed to load '%s' for execution", name);
      script_.get_next_statement (context->script);
      return;
    }

  /* Preserves a couple of items of state from the executed script.  */
  init_context (&other, script, context->execution_depth + 1);
  execute_script (&other);
  context->query_flag = other.query_flag;
  context->previous_action = other.previous_action;
  destroy_context (&other);

  script_.get_next_statement (context->script);

  /* Ensure a clean screen repaint on return from execution in a version 2
     JSON conversion when not followed immediately by another execution.  */
  if (script_.get_version (context->script) == 2)
    {
      if (script_.get_action (context->script) != C.EXECUTE)
        {
           reposition_to_label (context, "_menu");
           script_.get_next_statement (context->script);
        }
    }
}

/* Execute a single action found in the script.  */
static void
interpret_action (context_s *context, int action)
{
  if (action == C.TUTORIAL)
    handle_tutorial (context);

  else if (action == C.INSTRUCTION)
    handle_instruction (context);

  else if (action == C.CLEAR_SCREEN)
    handle_clear_screen (context);

  else if (action == C.GOTO)
    handle_goto (context, B.True);

  else if (action == C.EXIT)
    handle_exit (context);

  else if (action == C.QUERY)
    handle_query (context);

  else if (action == C.IF_YES_GOTO)
    handle_goto (context, context->query_flag);

  else if (action == C.IF_NO_GOTO)
    handle_goto (context, !context->query_flag);

  else if (action == C.DRILL_PRACTICE || action == C.SPEED_TEST_PRACTICE)
    handle_practice_exercise (context, action);

  else if (action == C.DRILL || action == C.SPEED_TEST)
    handle_exercise (context, action);

  else if (action == C.BIND_FUNCTION_KEY)
    handle_bind_function_key (context);

  else if (action == C.SET_ERROR_LIMIT
           || action == C.PERSISTENT_SET_ERROR_LIMIT)
    handle_set_error_limit (context,
                            action == C.PERSISTENT_SET_ERROR_LIMIT);

  else if (action == C.ON_FAILURE_GOTO
           || action == C.PERSISTENT_ON_FAILURE_GOTO)
    handle_on_failure_goto (context,
                            action == C.PERSISTENT_ON_FAILURE_GOTO);

  else if (action == C.MENU)
    handle_menu (context);

  else if (action == C.EXECUTE)
    handle_execute (context);

  else if (action == C.LABEL)
    script_.get_next_statement (context->script);

  else
    {
      utils_.error ("unknown action '%c' (discarded): '%s'",
                    action,
                    script_.get_statement_buffer (context->script));
      script_.get_next_statement (context->script);
    }
}

/* Script interpreter main loop.  */
static void
interpret_script (context_s *context, char *label)
{
  int action_count = 0, loop_detect = 0;
  int deprecation_warning = 0;
  utils_.info ("interpreter entry, depth %d, context p_%p, script p_%p",
               context->execution_depth, p_(context), p_(context->script));

  if (label)
    reposition_to_label (context, label);
  else
    script_.rewind (context->script);
  script_.get_next_statement (context->script);

  while (script_.has_more_data (context->script))
    {
      const int action = script_.get_action (context->script);

      interpret_action (context, action);
      action_count++;

      if (action == C.TUTORIAL
          || action == C.INSTRUCTION || action == C.CLEAR_SCREEN
          || action == C.DRILL || action == C.DRILL_PRACTICE
          || action == C.SPEED_TEST || action == C.SPEED_TEST_PRACTICE
          || action == C.MENU)
        context->previous_action = action;

      /* Many actions with no input looks like an unbreakable loop.  */
      if (action == C.TUTORIAL
          || action == C.DRILL || action == C.DRILL_PRACTICE
          || action == C.SPEED_TEST || action == C.SPEED_TEST_PRACTICE
          || action == C.QUERY || action == C.MENU)
        loop_detect = 0;
      else if (options.loop_limit && ++loop_detect > options.loop_limit)
        utils_.fatal ("unbreakable script loop detected after %d actions",
                      loop_detect);

      /* Warn once if we find a deprecated function key binding and this
         is not a JSON version 2 conversion.  */
      if (script_.get_version (context->script) != 2)
        {
          if (action == C.BIND_FUNCTION_KEY)
            utils_.warning_if (!deprecation_warning++,
                               "function key binding is now deprecated");
        }
    }
  utils_.info ("interpreter exit,"
               " context p_%p, script p_%p, %d actions handled",
               p_(context), p_(context->script), action_count);
}

/* Script execution entry point, reentrant.  */
void
execute_script (context_s *context)
{
  screen_.clear ();
  display_banner ("");

  script_.validate_parsed_data (context->script);
  map_script_labels (context);
  interpret_script (context, options.start_label);
  handle_exit (context);
}

/* Print help information.  */
static void
print_help (void)
{
  const char *help[] =
#if defined(LONG_OPTIONS)
{"Usage: typist [ options... ] [ script_file ]\n",
 "       -n, --notimer           turn off timer in drills",
 "       -t, --term-cursor       use the terminal's hardware cursor",
 "       -f, --curs-flash=P      s/w cursor flash period P*.1 sec (default 10)",
 "                               valid values are between 0 and 512,",
 "                               ignored if -t, --term_cursor",
 "       -c, --colours=F,B       set initial display colours where available",
 "       -k, --mapkeys=F,T       map between keyboard layouts",
 "       -s, --no-sound          do not beep on errors",
 "       -l, --start-label=L     start the lesson at label 'L'",
 "       -w, --no-wordprocessor  do not try to mimic word processors",
 "       -e, --failure-percent=E general exercise fail percent (default 3%)",
 "       -q, --quiet             do not print errors and warnings to stderr",
 "       -j, --log=J             send errors and warnings to file J",
 "       -m, --loop-limit=M      consider more than M actions without user",
 "                               input as an unbreakable loop (default 4096)",
 "       -d, --depth-limit=D     limit execute depth to D levels (default 32)",
 "       -S, --strict-json       use a strict JSON parser (normally relaxed)",
 "       -T, --trace             debug aid, trace interpreter on stderr",
 "       -D, --parse-only        debug aid, stop after parsing the script",
 "       -P, --print-parse       debug aid, print the result of parsing",
 "       -B, --print-build-id    debug aid, print build id and source checksum",
 "       -h, --help              print this message",
 "       -v, --version           output version information and exit\n",
 "If not supplied, script_file defaults to 'default.json'.",
 "The path $TYPIST_PATH is searched for script files.",
 "The path $TYPIST_KEYMAPS_PATH is searched for keymap files.\n", NULL};
#else
{"Usage: typist [ options... ] [ script_file ]\n",
 "       -n      turn off timer in drills",
 "       -t      use the terminal's hardware cursor",
 "       -f P    s/w cursor flash period P*.1 sec (default 10)",
 "               valid values are between 0 and 512, ignored if -t",
 "       -c F,B  set initial display colours where available",
 "       -k F,T  map between keyboard layouts",
 "       -s      don't beep on errors",
 "       -l L    start the lesson at label 'L'",
 "       -w      don't try to mimic word processors",
 "       -e      general exercise fail percent (default 3%)",
 "       -q      do not print errors and warnings to stderr",
 "       -j J    send errors and warnings to file J",
 "       -m M    consider more than M actions without user",
 "               input as an unbreakable loop (default 4096)",
 "       -d D    limit execute depth to D levels (default 32)",
 "       -S      use a strict JSON parser (normally relaxed)",
 "       -T      debug aid, trace interpreter on stderr",
 "       -D      debug aid, stop after parsing the script",
 "       -P      debug aid, print the result of parsing",
 "       -B      debug aid, print build id and source checksum",
 "       -h      print this message",
 "       -v      output version information and exit\n",
 "If not supplied, script_file defaults to 'default.json'.",
 "The path $TYPIST_PATH is searched for script files.",
 "The path $TYPIST_KEYMAPS_PATH is searched for keymap files.\n", NULL};
#endif

  const char **string;
  for (string = help; *string; string++)
    printf ("%s\n", *string);
}

/* From stamp.c.  */
extern const char *const COMPILER;
extern const char *const BUILD_DATE;
extern const char *const SOURCE_MD5;

/* Parse command line options for initial values. Values obtained here are
   written into the the options struct.  */
static void
parse_command_line (int argc, char *argv[])
{
  int c;
#if defined(LONG_OPTIONS)
  static const struct option long_options[] = {
    { "notimer",          no_argument,       0, 'n' },
    { "term-cursor",      no_argument,       0, 't' },
    { "curs-flash",       required_argument, 0, 'f' },
    { "colours",          required_argument, 0, 'c' },
    { "colors",           required_argument, 0, 'c' },
    { "mapkeys",          required_argument, 0, 'k' },
    { "no-sound",         no_argument,       0, 's' },
    { "start-label",      required_argument, 0, 'l' },
    { "no-wordprocessor", no_argument,       0, 'w' },
    { "failure-percent",  required_argument, 0, 'e' },
    { "quiet",            no_argument,       0, 'q' },
    { "log",              required_argument, 0, 'j' },
    { "loop-limit",       required_argument, 0, 'm' },
    { "depth-limit",      required_argument, 0, 'd' },
    { "strict-json",      no_argument,       0, 'S' },
    { "trace",            no_argument,       0, 'T' },
    { "parse-only",       no_argument,       0, 'D' },
    { "print-parse",      no_argument,       0, 'P' },
    { "print-build-id",   no_argument,       0, 'B' },
    { "help",             no_argument,       0, 'h' },
    { "version",          no_argument,       0, 'v' },
    { NULL,               no_argument,       0, 0 }
  };
  int option_index;
  static const char *help = "Try 'typist --help' for more information.\n";
#else
  static const char *help = "Try 'typist -h' for more information.\n";
#endif
  static const char *optstring = "ntf:c:k:sl:we:qj:m:d:STDPBhv";
  char *map_keyboard_from = NULL, *map_keyboard_to = NULL;

#if defined(LONG_OPTIONS)
  while ((c = getopt_long (argc, argv, optstring,
                           long_options, &option_index)) != -1)
#else
  while ((c = getopt (argc, argv, optstring)) != -1)
#endif
    {
      if (c == 'n')
        options.no_timer = B.True;

      else if (c == 't')
        options.terminal_cursor = B.True;

      else if (c == 'f')
        {
          if (sscanf (optarg, "%d", &options.cursor_flash_period) != 1
              || options.cursor_flash_period < 0
              || options.cursor_flash_period > 512)
            {
              utils_.error ("invalid curs_flash value (range is 0 to 512)");
              exit (1);
            }
        }

      else if (c == 'c')
        {
          if (sscanf (optarg, "%d,%d",
                      &options.foreground_colour,
                      &options.background_colour) != 2
              || options.foreground_colour < 0
              || options.foreground_colour >= screen_.num_colours
              || options.background_colour < 0
              || options.background_colour >= screen_.num_colours
              || options.foreground_colour == options.background_colour)
            {
              utils_.error ("invalid colours value (range is 0 to 7)");
              exit (1);
            }
          options.colour = B.True;
      }

      else if (c == 'k')
        {
          map_keyboard_from = utils_.strdup (optarg);
          map_keyboard_to = utils_.strdup (optarg);

          if (sscanf (optarg, "%[^,],%[^,]",
                      map_keyboard_from, map_keyboard_to) != 2)
            {
              utils_.error ("invalid mapkeys value (want <string>,<string>)");
              exit (1);
            }
        }

      else if (c == 's')
        options.no_sound = B.True;

      else if (c == 'l')
        options.start_label = utils_.strdup (optarg);

      else if (c == 'w')
        options.no_wordprocessor_mode = B.True;

      else if (c == 'e')
        {
          if (sscanf (optarg, "%lf", &options.failure_percent) != 1
              || options.failure_percent < 0.0
              || options.failure_percent > 100.0)
            {
              utils_.error ("invalid failure_percent value"
                            " (range is 0 to 100)");
              exit (1);
            }
        }

      else if (c == 'q')
        options.quiet = B.True;

      else if (c == 'j')
        {
          options.message_stream = fopen (optarg, "w");
          if (!options.message_stream)
            {
              utils_.error ("error opening log file '%s'", optarg);
              exit (1);
            }
          if (utils_.info_stream)
            utils_.info_stream = options.message_stream;
        }

      else if (c == 'm')
        {
          if (sscanf (optarg, "%d", &options.loop_limit) != 1
              || options.loop_limit < 0
              || (options.loop_limit > 0 && options.loop_limit < 256))
            {
              utils_.error ("invalid loop_limit value (range is 0 or >255)");
              exit (1);
            }
        }

      else if (c == 'd')
        {
          if (sscanf (optarg, "%d", &options.depth_limit) != 1
              || options.depth_limit < 0)
            {
              utils_.error ("invalid depth_limit value (range is 0 or >0)");
              exit (1);
            }
        }

      else if (c == 'S')
        options.strict_json = B.True;

      else if (c == 'T')
        utils_.info_stream = options.message_stream
                           ? options.message_stream : stderr;

      else if (c == 'D')
        options.parse_only = B.True;

      else if (c == 'P')
        options.print_parse = B.True;

      else if (c == 'B')
        {
          struct tm tm;
          struct utsname ut;
          char timestamp[128];

          strptime (BUILD_DATE, "%b %d %Y %H:%M:%S", &tm);
          strftime (timestamp, sizeof (timestamp), "%Y%m%d%H%M%S", &tm);
          uname (&ut);
          printf ("%s\n", options.version);
          printf ("Build id : %s/gcc%s/%s\n", ut.machine, COMPILER, timestamp);
          printf ("Source id: md5/%s\n\n", SOURCE_MD5);
          exit (0);
        }

      else if (c == 'h')
        {
          print_help ();
          exit (0);
        }

      else if (c == 'v')
        {
          printf ("%s, %s\n\n", options.version, options.copyright);
          exit (0);
        }

      else if (c == '?')
        {
          fprintf (stderr, help);
          exit (1);
        }

      else
        {
          utils_.error ("getopt returned unknown '%c' value", c);
          exit (1);
        }
    }
  if (argc - optind > 1)
    {
      fprintf (stderr, help);
      exit (1);
    }

  if (map_keyboard_from && map_keyboard_to)
    {
      const char *search_path = getenv("TYPIST_KEYMAPS_PATH");
      keymap_s *from = keymap_.load (map_keyboard_from,
                                     search_path, options.strict_json, B.True);
      keymap_s *to = keymap_.load (map_keyboard_to,
                                   search_path, options.strict_json, B.True);

      utils_.error_if (!from,
                       "unable to load the requested keymap '%s'",
                       map_keyboard_from);
      utils_.error_if (!to,
                       "unable to load the requested keymap '%s'",
                       map_keyboard_to);
      if (!from || !to)
        exit (1);

      options.keymapper = keymapper_.create (from, to, B.False);
    }

  utils_.free (map_keyboard_from);
  utils_.free (map_keyboard_to);
}

/* Signal handler, required to tidy up curses on interrupt.  */
static void
finalize_and_exit (int sig)
{
  utils_.info ("interrupted by signal %d, exiting", sig);
  screen_.finalize ();
  exit (1);
}

/* Main routine.  */
int
main (int argc, char *argv[])
{
  static const char *version = "Typist 3.0";
  static const char *copyright =
      "Copyright (C) 1998-2018  Simon Baldwin\n\n"
      "This program comes with ABSOLUTELY NO WARRANTY; for details please "
      "see the\nfile 'COPYING' supplied with the source code. This is free "
      "software, and\nyou are welcome to redistribute it under certain "
      "conditions; again, see\n'COPYING' for details. This program is "
      "released under the GNU General Public\nLicense.\n\n"
      "JSON parser:\nCopyright (C) 2012-2017 Serge Zaitsev."
      "  All rights reserved.\nhttps://github.com/zserge/jsmn";
  const char *search_path = getenv ("TYPIST_PATH");
  char *file;
  script_s *script = NULL;
  context_s context;

  /* Run static initializers. These must run first, and in this order.  */
  init_constants ();
  init_options (version, copyright);
  init_pseudo_function_keys ();

  /* Check usage, parsed command line options, and open input file.  */
  parse_command_line (argc, argv);
  if (argc - optind == 1)
    file = utils_.strdup (argv[optind]);
  else
    file = utils_.strdup (constants.DEFAULT_SCRIPT);

  script = script_.load (file, search_path, options.strict_json);
  if (!script)
    {
      utils_.error ("unable to open script file '%s'", file);
      exit (1);
    }

  utils_.free (file);

  /* If not executing the script, parse and maybe print and we are done.  */
  if (options.parse_only || options.print_parse)
    {
      init_context (&context, script, 0);

      script_.validate_parsed_data (script);
      map_script_labels (&context);
      if (options.print_parse)
        script_.print_parsed_data (script);

      destroy_context (&context);
      exit (0);
    }

  if (options.message_stream)
    utils_.error_stream = options.message_stream;
  if (options.quiet)
    utils_.error_stream = NULL;

  /* Register signal handlers to clean up curses on interrupt.  */
  signal (SIGHUP, finalize_and_exit);
  signal (SIGINT, finalize_and_exit);
  signal (SIGQUIT, finalize_and_exit);
  signal (SIGCHLD, finalize_and_exit);
  signal (SIGPIPE, finalize_and_exit);
  signal (SIGTERM, finalize_and_exit);

  /* Initialize curses, and add colour if possible.  */
  screen_.init ();
  if (options.colour && screen_.has_colors ())
    screen_.setup_colour (options.foreground_colour,
                          options.background_colour);
  utils_.error_if (screen_.get_columns () < 80
                   || screen_.get_lines () < 25,
                   "minimum practical screen size is 80x25 (trying anyway)");

  /* Execute the script, then clean up and exit.  */
  init_context (&context, script, 0);
  execute_script (&context);
  destroy_context (&context);

  if (options.keymapper)
    keymapper_.destroy (options.keymapper);
  if (options.message_stream)
    fclose (options.message_stream);
  screen_.finalize ();

  return 0;
}
