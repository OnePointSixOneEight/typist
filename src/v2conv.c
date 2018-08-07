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

#include "_gettext.h"
#include "buffer.h"
#include "json.h"
#include "map.h"
#include "script.h"
#include "utils.h"
#include "convert.h"

struct constants {
  const char *CONTINUE_MESSAGE;
  const char *FALLTHROUGH_MESSAGE;
  const char *EXIT_MESSAGE;
};

static struct constants constants;

static void
init_constants (void)
{
  static int initialized = 0;

  if (initialized++)
    return;

  constants.CONTINUE_MESSAGE    = _(" Continue to the next lesson? [Y/N] ");
  constants.FALLTHROUGH_MESSAGE = _(" No more lessons -"
                                    " exit this lesson series? [Y/N] ");
  constants.EXIT_MESSAGE        = _(" Exit this lesson series? [Y/N] ");
}

static void
append (int action_code, const char *data, buffer_s *buffer)
{
  const char action = action_code, separator = '|';

  buffer_.append (buffer, &action, 1);
  buffer_.append (buffer, &separator, 1);
  buffer_.append (buffer, data, strlen (data) + 1);
}

static void
build_menu_entry (const json_s *json,
                  int entry, int index_, void *v_buffer, void *unused)
{
  buffer_s *buffer = (buffer_s *)v_buffer;
  int title;
  const char *title_string;
  char *string;
  unused = unused;

  if (json_.element_type (json, entry) != json_object)
    {
      utils_.error (_("JSON non-object 'entries[]'"
                      " element found, at index %d (ignored)"), index_);
      return;
    }

  title =
    json_.get_required_element_of_types (json, entry, "entries[]",
                                         "title", json_string, json_primitive);
  if (title && json_.element_is_null (json, title))
    {
      append (C.CONTINUATION, "_menu \"\"", buffer);
      return;
    }
  if (!title)
    return;

  title_string = json_.element_string (json, title);

  string = utils_.alloc (strlen (title_string) + 128);
  sprintf (string, "_menu_%d \"%s\"", index_, title_string);
  append (C.CONTINUATION, string, buffer);
  utils_.free (string);
}

static void
build_menu (const json_s *json,
            const char *seriesdescription,
            const char *title, int entries, buffer_s *buffer)
{
  char *string;

  append (C.LABEL, "_menu", buffer);
  append (C.BIND_FUNCTION_KEY, "11:_confirm_exit", buffer);
  append (C.BIND_FUNCTION_KEY, "12:_menu", buffer);
  append (C.CLEAR_SCREEN, seriesdescription, buffer);

  string = utils_.alloc (strlen (title) + 128);
  sprintf (string, " up=_confirm_exit \"%s\"", title);
  append (C.MENU, string, buffer);
  utils_.free (string);

  json_.for_each_array_element (json, entries,
                                build_menu_entry, buffer, NULL);
}

static void
build_instruction_element (const json_s *json,
                           int string,
                           int index_, void *v_buffer, void *unused)
{
  buffer_s *buffer = (buffer_s *)v_buffer;
  int action;
  unused = unused;

  if (json_.element_type (json, string) != json_string)
    {
      utils_.error (_("JSON non-string 'instructions[]'"
                      " element found, at index %d (ignored)"), index_);
      return;
    }

  utils_.error_if (index_ == 2,
                   _("oversized 'instruction' found (truncated)"));
  if (index_ > 1)
    return;

  action = index_ == 0 ? C.INSTRUCTION : C.CONTINUATION;
  append (action, json_.element_string (json, string), buffer);
}

static void
build_lesson_text_element (const json_s *json,
                           int string,
                           int index_, void *v_buffer, void *v_action_code)
{
  buffer_s *buffer = (buffer_s *)v_buffer;
  int action, action_code = (int)v_action_code;

  if (json_.element_type (json, string) != json_string)
    {
      utils_.error (_("JSON non-string 'text[]'"
                      " element found, at index %d (ignored)"), index_);
      return;
    }

  action = index_ == 0 ? action_code : C.CONTINUATION;
  append (action, json_.element_string (json, string), buffer);
}

static void
build_lesson_of_type (const json_s *json,
                      int action_code, const char *title,
                      int instructions, int text, buffer_s *buffer)
{
  append (C.CLEAR_SCREEN, title, buffer);

  if (instructions)
    {
      if (action_code == C.DRILL || action_code == C.SPEED_TEST)
        {
          if (json_.element_type (json, instructions) == json_string)
            append (C.INSTRUCTION,
                    json_.element_string (json, instructions), buffer);
          else
            json_.for_each_array_element (json, instructions,
                                          build_instruction_element,
                                          buffer, NULL);
        }
      else if (action_code == C.TUTORIAL)
        utils_.error (_("'instructions' is invalid on Tutorials (ignored)"));
    }

  if (json_.element_type (json, text) == json_string)
    append (action_code, json_.element_string (json, text), buffer);
  else
    json_.for_each_array_element (json, text,
                                  build_lesson_text_element,
                                  buffer, (void*)action_code);
}

static void
build_lesson (const json_s *json,
              int lesson, const char *name,  buffer_s *buffer)
{
  int type, title, instructions, text;
  const char *type_string, *title_string;

  type =
    json_.get_required_element_of_types (json, lesson, name,
                                         "type", json_string, json_primitive);
  if (!type)
    return;

  type_string = json_.element_string (json, type);

  if (strcasecmp (type_string, "Execute") == 0)
    {
      int script =
        json_.get_required_element_of_types (json, lesson, name,
                                             "script",
                                             json_string, json_primitive);
      if (script)
        append (C.EXECUTE, json_.element_string (json, script), buffer);
      return;
    }

  text =
    json_.get_required_element_of_types (json, lesson, name,
                                         "text", json_string, json_array);
  title =
    json_.get_required_element_of_types (json, lesson, name,
                                         "title", json_string, json_primitive);
  if (!text || !title)
    return;

  title_string = json_.element_string (json, title);

  instructions =
    json_.get_element_of_types (json, lesson,
                                "instructions", json_string, json_array);

  if (strcasecmp (type_string, "Tutorial") == 0)
    build_lesson_of_type (json, C.TUTORIAL,
                          title_string, instructions, text, buffer);

  else if (strcasecmp (type_string, "Drill") == 0)
    build_lesson_of_type (json, C.DRILL,
                          title_string, instructions, text, buffer);

  else if (strcasecmp (type_string, "Paragraph") == 0)
    build_lesson_of_type (json, C.SPEED_TEST,
                          title_string, instructions, text, buffer);

  else
    utils_.error (_("invalid lesson type"
                    " '%s' found in '%s' (ignored)"), type_string, name);
}

static void
build_lesson_entry (const json_s *json,
                    int lesson_name,
                    int index_, void *v_buffer, void *v_typist_map)
{
  buffer_s *buffer = (buffer_s *)v_buffer;
  map_s *typist_map = (map_s *)v_typist_map;
  int lesson;

  if (!(json_.element_type (json, lesson_name) == json_string
        || json_.element_type (json, lesson_name) == json_primitive))
    {
      utils_.error (_("JSON non-string (and non-primitive) 'lessons[]'"
                      " element found, at index %d (ignored)"), index_);
      return;
    }

  if (!map_.get (typist_map,
                 json_.element_string (json, lesson_name), &lesson))
    {
      utils_.error (_("JSON object 'typist' is missing a '%s'"
                      " element"), json_.element_string (json, lesson_name));
      return;
    }
  if (json_.element_type (json, lesson) != json_object)
    {
      utils_.error (_("JSON element '%s' is not of type"
                      " 'object'"), json_.element_string (json, lesson_name));
      return;
    }

  build_lesson (json, lesson,
                json_.element_string (json, lesson_name), buffer);
}

static void
build_lesson_group (const json_s *json,
                    int entry,
                    int index_, void *v_buffer, void *v_typist_map)
{
  buffer_s *buffer = (buffer_s *)v_buffer;
  int title, lessons;
  char string[128];

  if (json_.element_type (json, entry) != json_object)
    {
      utils_.error (_("JSON non-object 'entries[]'"
                      " element found, at index %d (ignored)"), index_);
      return;
    }

  title =
    json_.get_required_element_of_types (json, entry, "entries[]",
                                         "title", json_string, json_primitive);
  if (!title || json_.element_is_null (json, title))
    return;

  sprintf (string, "_menu_%d", index_);

  lessons =
    json_.get_required_element_of_types (json, entry, "entries[]",
                                         "lessons",
                                         json_array, json_primitive);
  if (!lessons)
    {
      append (C.LABEL, string, buffer);
      return;
    }

  if (json_.element_is_null (json, lessons))
    {
      append (C.GOTO, "_fallthrough", buffer);
      append (C.LABEL, string, buffer);
      append (C.GOTO, "_confirm_exit", buffer);
      return;
    }

  append (C.LABEL, string, buffer);

  if (json_.element_type (json, lessons) == json_array)
    json_.for_each_array_element (json, lessons,
                                  build_lesson_entry, buffer, v_typist_map);

  else if (json_.element_type (json, lessons) == json_primitive)
    build_lesson_entry (json, lessons, 0, buffer, v_typist_map);

  append (C.QUERY, constants.CONTINUE_MESSAGE, buffer);
  append (C.IF_NO_GOTO, "_menu", buffer);
}

static void
map_typist_object (const json_s *json,
                   int name_, int value, void *v_typist_map)
{
  map_s *typist_map = (map_s *)v_typist_map;
  const char *name = json_.element_string (json, name_);

  if (strcmp (name, "version") == 0
      || strcmp (name, "seriesName") == 0
      || strcmp (name, "seriesDescription") == 0
      || strcmp (name, "seriesMenu") == 0)
    return;

  utils_.error_if (map_.contains (typist_map, name),
                   _("lesson '%s' re-defined (shadowed prior)"), name);
  map_.set (typist_map, name, &value);
}

static void
build_body (const json_s *json, int typist, int entries, buffer_s *buffer)
{
  map_s *typist_map = map_.create (sizeof (int), 0, 127);

  json_.for_each_object_element (json, typist,
                                 map_typist_object, typist_map);
  json_.for_each_array_element (json, entries,
                                build_lesson_group, buffer, typist_map);
  map_.destroy (typist_map);
}

static void
json_to_legacy (const json_s *json, int typist, buffer_s *buffer)
{
  int seriesdescription, seriesmenu, title, entries;

  init_constants ();

  seriesdescription =
    json_.get_required_element_of_type (json, typist, "typist",
                                        "seriesDescription", json_string);
  seriesmenu =
    json_.get_required_element_of_type (json, typist, "typist",
                                        "seriesMenu", json_object);
  if (!seriesdescription || !seriesmenu)
    return;

  title =
    json_.get_required_element_of_type (json, seriesmenu, "seriesMenu",
                                        "title", json_string);
  entries =
    json_.get_required_element_of_type (json, seriesmenu, "seriesMenu",
                                        "entries", json_array);
  if (!entries || !title)
    return;

  append (C.LABEL, "_entry", buffer);

  build_menu (json,
              json_.element_string (json, seriesdescription),
              json_.element_string (json, title),
              entries, buffer);
  build_body (json, typist, entries, buffer);

  append (C.LABEL, "_fallthrough", buffer);
  append (C.QUERY, constants.FALLTHROUGH_MESSAGE, buffer);
  append (C.IF_NO_GOTO, "_entry", buffer);
  append (C.IF_YES_GOTO, "_exit", buffer);

  append (C.LABEL, "_confirm_exit", buffer);
  append (C.QUERY, constants.EXIT_MESSAGE, buffer);
  append (C.IF_NO_GOTO, "_entry", buffer);
  append (C.IF_YES_GOTO, "_exit", buffer);

  append (C.LABEL, "_exit", buffer);
}

__attribute__((constructor))
void
init_v2_convert (void)
{
  convert_.v2_json_to_legacy = json_to_legacy;
}
