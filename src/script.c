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
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "_gettext.h"
#include "convert.h"
#include "utf8.h"
#include "utils.h"

#include "script.h"

static const char COMMENT             = '#';
static const char ALTERNATE_COMMENT   = '!';
static const char SEPARATOR           = ':';
static const char ALTERNATE_SEPARATOR = '|';

struct script {
  int version;
  char *locale;
  int requires_utf8;
  char *data;
  int length;
  int position;
  char *current_statement;
};

static int
requires_utf8 (const char *data, int length)
{
  static const char *hex = "0123456789abcdef";
  int i;

  for (i = 0; i < length; i++)
    {
      unsigned char c = data[i];

      if (c > '~' + 1)
        return B.True;

      else if (data[i] == '\\' && data[i + 1] == 'u'
          && data[i + 2] && strchr (hex, tolower (data[i + 2]))
          && data[i + 3] && strchr (hex, tolower (data[i + 3]))
          && data[i + 4] && strchr (hex, tolower (data[i + 4]))
          && data[i + 5] && strchr (hex, tolower (data[i + 5])))
        {
          unsigned int escape;

          if (sscanf (data + i + 2, "%04x", &escape) == 1
              && escape > '~' + 1)
            return B.True;
        }
    }
  return B.False;
}

static int
prepare_json_data (script_s *script,
                   const char *file_data, int file_length, int strict_json)
{
  conversion_s *conversion;

  conversion = convert_.json_to_legacy (file_data, file_length, strict_json);
  if (!conversion)
    return B.False;

  script->version = conversion->version;
  if (conversion->locale)
    script->locale = conversion->locale;
  else
    script->locale = utils_.strdup ("");

  script->data = utils_.realloc (conversion->data, conversion->length + 1);
  script->data[conversion->length] = 0;
  script->length = conversion->length;
  script->requires_utf8 = requires_utf8 (script->data, script->length);

  utils_.free (conversion);
  return B.True;
}

static int
prepare_legacy_data (script_s *script, const char *file_data, int file_length)
{
  int i;

  script->version = 0;
  script->locale = utils_.strdup ("");

  script->data = utils_.alloc (file_length + 1);
  memcpy (script->data, file_data, file_length);
  script->data[file_length] = 0;
  script->length = file_length;
  script->requires_utf8 = requires_utf8 (script->data, script->length);

  for (i = 0; i < script->length; i++)
    {
      /* Allow for possible DOS-format input.  */
      if (script->data[i] == '\r' && script->data[i + 1] == '\n')
        script->data[i] = script->data[i + 1] = 0;
      else if (script->data[i] == '\n')
        script->data[i] = 0;
    }
  return B.True;
}

static script_s *
open (const char *path, int strict_json)
{
  script_s *script;
  char *file_type, *file_data;
  int file_length, status = 0;
  FILE *stream;

  file_type = strrchr (path, '.');
  if (!(file_type && (strcasecmp (file_type, ".json") == 0
                   || strcasecmp (file_type, ".typ") == 0)))
    {
      utils_.error (_("unsupported file type, expected .json or .typ"));
      return NULL;
    }

  stream = fopen (path, "rb");
  if (!stream)
    return NULL;
  utils_.info ("opened file '%s' on stream p_%p", path, p_(stream));

  file_length = utils_.read_in_file (stream, &file_data);
  utils_.info ("read %d bytes from stream p_%p", file_length, p_(stream));
  fclose (stream);

  if (file_length == 0)
    {
      utils_.error (_("file '%s' contains no data"), path);
      return NULL;
    }

  script = utils_.alloc (sizeof (*script));
  memset (script, 0, sizeof (*script));

  if (strcasecmp (file_type, ".json") == 0)
    status = prepare_json_data (script, file_data, file_length, strict_json);

  else if (strcasecmp (file_type, ".typ") == 0)
    status = prepare_legacy_data (script, file_data, file_length);

  else
    utils_.fatal (_("internal error: unsupported file type"));

  utils_.info ("script preparation status is b_%d", status);
  utils_.free (file_data);

  if (!status)
    {
      utils_.free (script);
      script = NULL;
    }

  utils_.info ("open script returned p_%p", p_(script));
  return script;
}

static script_s *
load (const char *name, const char *search_path, int strict_json)
{
  char *path;
  script_s *script = NULL;

  path = utils_.locate_file (search_path, name, "");
  if (!path)
    path = utils_.locate_file (search_path, name, ".json");
  if (!path)
    path = utils_.locate_file (search_path, name, ".typ");

  if (path)
    script = open (path, strict_json);

  utils_.free (path);
  return script;
}

static void
close (script_s *script)
{
  utils_.free (script->locale);
  utils_.free (script->data);
  utils_.free (script->current_statement);

  memset (script, 0, sizeof (*script));
  utils_.free (script);
  utils_.info ("closed script p_%p", p_(script));
}

static int
requires_utf8_ (const script_s *script)
{
  return script->requires_utf8;
}

static int
get_version (const script_s *script)
{
  return script->version;
}

static const char *
get_locale (const script_s *script)
{
  return script->locale;
}

static int
get_action (const script_s *script)
{
  if (script->position > script->length)
    return -1;

  if (script->current_statement[0] == C.COMPAT_DRILL_PRACTICE)
    return C.DRILL_PRACTICE;

  else if (script->current_statement[0] == C.COMPAT_SPEED_TEST)
    return C.SPEED_TEST;

  return script->current_statement[0];
}

static const char *
get_data (const script_s *script)
{
  if (script->position > script->length)
    return NULL;

  if (strlen (script->current_statement) > 2)
    return script->current_statement + 2;
  else
    return "";
}

static const char *
get_statement_buffer (const script_s *script)
{
  if (script->position > script->length)
    return NULL;

  return script->current_statement;
}

static void
rewind_ (script_s *script)
{
  script->position = 0;
  utils_.free (script->current_statement);
  script->current_statement = NULL;
}

static int
get_position (const script_s *script)
{
  if (script->position > script->length)
    return -1;

  return script->position;
}

static void
set_position (script_s *script, int position)
{
  script->position = position;
}

static int
has_more_data (const script_s *script)
{
  return script->position <= script->length;
}

static void
get_next_statement (script_s *script)
{
  do
    {
      int bytes = strlen (script->data + script->position) + 1;

      /* Copy the next script statement into current_statement.  */
      script->current_statement
          = utils_.realloc (script->current_statement, bytes);
      strcpy (script->current_statement, script->data + script->position);

      /* If not already beyond the end of the script on function entry,
         advance position, allowing for possible DOS-format input.  */
      if (has_more_data (script))
        {
          script->position += bytes;
          script->position += !script->position ? 1 : 0;
        }

      /* Convert statements consisting of whitespace into empty lines.  */
      if (strlen (script->current_statement)
          == strspn (script->current_statement, " \t"))
        script->current_statement[0] = 0;
    }
  while (has_more_data (script)
         && (!script->current_statement[0] ||
              script->current_statement[0] == COMMENT ||
              script->current_statement[0] == ALTERNATE_COMMENT));
}

static void
validate_statement (const char *statement)
{
  static int deprecation_warning = 0;
  const int action = statement[0];

  if (statement[0] == COMMENT || statement[0] == ALTERNATE_COMMENT
      || strspn (statement, " \t") == strlen (statement))
    return;

  if (action &&
      (action == C.COMPAT_DRILL_PRACTICE || action == C.COMPAT_SPEED_TEST))
    {
      utils_.warning_if (!deprecation_warning++,
                         _("compatibility action"
                           " '%c' (will work, but deprecated): '%s'"),
                         action, statement);
      utils_.warning_if (deprecation_warning == 1,
                         _("note: only the first occurrence is reported"));
    }

  if (action &&
      ! (action == C.CONTINUATION || action == C.LABEL
      || action == C.TUTORIAL || action == C.INSTRUCTION
      || action == C.CLEAR_SCREEN || action == C.GOTO
      || action == C.EXIT || action == C.QUERY
      || action == C.IF_YES_GOTO || action == C.IF_NO_GOTO
      || action == C.DRILL || action == C.DRILL_PRACTICE
      || action == C.SPEED_TEST || action == C.SPEED_TEST_PRACTICE
      || action == C.BIND_FUNCTION_KEY || action == C.SET_ERROR_LIMIT
      || action == C.ON_FAILURE_GOTO || action == C.MENU
      || action == C.EXECUTE || action == C.PERSISTENT_SET_ERROR_LIMIT
      || action == C.PERSISTENT_ON_FAILURE_GOTO
      || action == C.COMPAT_DRILL_PRACTICE || action == C.COMPAT_SPEED_TEST))
    {
      if (action >= ' ' && action <= '~')
        utils_.error (_("invalid action"
                        " '%c' (statement not discarded): '%s'"),
                      action, statement);
      else
        utils_.error (_("invalid action"
                        " U+%04X (statement not discarded): '%s'"),
                      action, statement);
    }

  if (action)
    {
      const int separator = statement[1];

      if (separator && !(separator == SEPARATOR
                      || separator == ALTERNATE_SEPARATOR))
        utils_.error (_("missing ':' or '|' separator"
                        " (statement not discarded): '%s'"), statement);
    }
}

static void
print_statement (const char *statement)
{
  int *ucs;

  printf ("%s\n", statement);
  ucs = utf8_.to_ucs (statement);
  utf8_.free (ucs);
}

static void
for_each_statement (const script_s *script, void (*handler) (const char *))
{
  int cursor = 0;

  while (cursor <= script->length)
    {
      const char *statement = script->data + cursor;

      handler (statement);
      cursor += strlen (statement) + 1;
      /* Allow for possible DOS-format input.  */
      cursor += !script->data[cursor] ? 1 : 0;
    }
}

static void
validate_parsed_data (const script_s *script)
{
  for_each_statement (script, validate_statement);
}

void
print_parsed_data (const script_s *script)
{
  printf ("# Script internal representation (%d bytes):\n", script->length);
  for_each_statement (script, print_statement);
}

struct script_ script_;

__attribute__((constructor))
void
init_script (void)
{
  script_.open = open;
  script_.load = load;
  script_.close = close;
  script_.requires_utf8 = requires_utf8_;
  script_.get_version = get_version;
  script_.get_locale = get_locale;
  script_.get_action = get_action;
  script_.get_data = get_data;
  script_.get_statement_buffer = get_statement_buffer;
  script_.rewind = rewind_;
  script_.get_position = get_position;
  script_.set_position = set_position;
  script_.has_more_data = has_more_data;
  script_.get_next_statement = get_next_statement;
  script_.validate_parsed_data = validate_parsed_data;
  script_.print_parsed_data = print_parsed_data;
}
