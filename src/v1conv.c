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

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "_gettext.h"
#include "buffer.h"
#include "json.h"
#include "script.h"
#include "utils.h"
#include "convert.h"

static void
append (int action_code, const char *data, buffer_s *buffer)
{
  const char action = action_code, separator = '|';

  buffer_.append (buffer, &action, 1);
  buffer_.append (buffer, &separator, 1);
  buffer_.append (buffer, data, strlen (data) + 1);
}

static int
convert_action_to_code (const char *action)
{
  if (strcasecmp (action, "Label") == 0)
    return C.LABEL;
  else if (strcasecmp (action, "Tutorial") == 0)
    return C.TUTORIAL;
  else if (strcasecmp (action, "Instruction") == 0)
    return C.INSTRUCTION;
  else if (strcasecmp (action, "Clear_screen") == 0)
    return C.CLEAR_SCREEN;
  else if (strcasecmp (action, "Goto") == 0)
    return C.GOTO;
  else if (strcasecmp (action, "Exit") == 0)
    return C.EXIT;
  else if (strcasecmp (action, "Query") == 0)
    return C.QUERY;
  else if (strcasecmp (action, "If_yes_goto") == 0)
    return C.IF_YES_GOTO;
  else if (strcasecmp (action, "If_no_goto") == 0)
    return C.IF_NO_GOTO;
  else if (strcasecmp (action, "Drill") == 0)
    return C.DRILL;
  else if (strcasecmp (action, "Drill_practice") == 0)
    return C.DRILL_PRACTICE;
  else if (strcasecmp (action, "Speed_test") == 0)
    return C.SPEED_TEST;
  else if (strcasecmp (action, "Speed_test_practice") == 0)
    return C.SPEED_TEST_PRACTICE;
  else if (strcasecmp (action, "Bind_function_key") == 0)
    return C.BIND_FUNCTION_KEY;
  else if (strcasecmp (action, "Set_error_limit") == 0)
    return C.SET_ERROR_LIMIT;
  else if (strcasecmp (action, "On_failure_goto") == 0)
    return C.ON_FAILURE_GOTO;
  else if (strcasecmp (action, "Menu") == 0)
    return C.MENU;
  else if (strcasecmp (action, "Execute") == 0)
    return C.EXECUTE;
  else if (strcasecmp (action, "Persistent_set_error_limit") == 0)
    return C.PERSISTENT_SET_ERROR_LIMIT;
  else if (strcasecmp (action, "Persistent_on_failure_goto") == 0)
    return C.PERSISTENT_ON_FAILURE_GOTO;
  else
    return 0;
}

static void
process_data_array_element (const json_s *json,
                            int string, int index_,
                            void *v_buffer, void *v_action_code)
{
  buffer_s *buffer = (buffer_s *)v_buffer;
  int action_code = (int)v_action_code;

  if (json_.element_type (json, string) != json_string)
    {
      utils_.error (_("JSON non-string 'data[]'"
                      " element found, at index %d (ignored)"), index_);
      return;
    }

  if (action_code == C.INSTRUCTION)
    {
      utils_.error_if (index_ == 2,
                       _("oversized 'Instruction' found (truncated)"));
      if (index_ > 1)
        return;
    }

  if (index_ == 0
      || action_code == C.LABEL
      || action_code == C.CLEAR_SCREEN
      || action_code == C.GOTO
      || action_code == C.EXIT
      || action_code == C.QUERY
      || action_code == C.IF_YES_GOTO
      || action_code == C.IF_NO_GOTO
      || action_code == C.BIND_FUNCTION_KEY
      || action_code == C.SET_ERROR_LIMIT
      || action_code == C.ON_FAILURE_GOTO
      || action_code == C.EXECUTE
      || action_code == C.PERSISTENT_SET_ERROR_LIMIT
      || action_code == C.PERSISTENT_ON_FAILURE_GOTO)
    append (action_code, json_.element_string (json, string), buffer);
  else
    append (C.CONTINUATION, json_.element_string (json, string), buffer);
}

static void
process_statement (const json_s *json,
                   int statement, int index_, void *v_buffer, void *v_count)
{
  buffer_s *buffer = (buffer_s *)v_buffer;
  int *count = (int*)v_count;
  int action, data, action_code;

  if (json_.element_type (json, statement) != json_object)
    {
      utils_.error (_("JSON non-object 'statements[]'"
                      " element found, at index %d (ignored)"), index_);
      return;
    }

  action =
    json_.get_required_element_of_type (json, statement, "statements[]",
                                        "action", json_string);
  if (!action)
    return;

  action_code =
    convert_action_to_code (json_.element_string (json, action));
  if (!action_code)
    {
      utils_.error (_("action '%s' is invalid, at index %d (object ignored)"),
                    json_.element_string (json, action), index_);
      return;
    }

  data = json_.get_element_of_types (json, statement, "data",
                                     json_string, json_array);

  if (!data || action_code == C.EXIT)
    append (action_code, "", buffer);

  else if (json_.element_type (json, data) == json_string)
    append (action_code, json_.element_string (json, data), buffer);

  else if (json_.element_type (json, data) == json_array)
    json_.for_each_array_element (json, data,
                                  process_data_array_element,
                                  buffer, (void*)action_code);

  *count += 1;
}

static void
json_to_legacy (const json_s *json, int typist, buffer_s *buffer)
{
  int statements, count = 0;

  statements =
    json_.get_required_element_of_type (json, typist, "typist",
                                        "statements", json_array);
  if (!statements)
    return;

  json_.for_each_array_element (json, statements,
                                process_statement, buffer, &count);

  utils_.info (_("JSON processed %d 'statements[]' objects"), count);

  if (!count)
    utils_.error (_("no usable JSON 'action' objects found in 'statements[]'"));
}

__attribute__((constructor))
void
init_v1_convert (void)
{
  convert_.v1_json_to_legacy = json_to_legacy;
}
