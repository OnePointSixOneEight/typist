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

#include "jsmn.h"
#include "utils.h"
#include "json.h"

/* Strict parser functions, created by compiling jsmn.c differently a
   second time, and using macros to alter the names of the entry points.  */
extern void strict_jsmn_init (jsmn_parser *parser);
extern int strict_jsmn_parse (jsmn_parser *parser,
                              const char *js, size_t len,
                              jsmntok_t *tokens, unsigned int num_tokens);

struct json {
  char *data;         /* Modifiable copy of parsed file data */
  jsmntok_t *tokens;  /* JSMN parser tokens, allocated */
};

static void
stringify_and_unescape (json_s *json, int token_count)
{
  char *data = json->data;
  int i;

  for (i = 0; i < token_count; i++)
    {
      const jsmntok_t *token = json->tokens + i;
      int j, k;

      if (token->type == JSMN_PRIMITIVE || token->type == JSMN_STRING)
        json->data[token->end] = 0;

      if (token->type != JSMN_STRING)
        continue;

      k = token->start;
      for (j = token->start; j < token->end; j++, k++)
        {
          if (data[j] == '\\' && data[j + 1])
            {
              const char *escapes = "bfnrt\"\\";
              char *e = strchr (escapes, data[++j]);
              data[k] = e ? "\b\f\n\r\t\"\\"[e - escapes] : data[j];
            }
          else
            data[k] = data[j];
        }
      data[k] = 0;
    }
}

static void
indicate_location (int position, const char *json_data)
{
  char buffer[32];

  sscanf (json_data + position, "%31[^\n]", buffer);
  utils_.error_if (strlen (buffer),
                   "JSON file parse: near '%s ...'", buffer);
}

static json_s *
parse (const char *file_data, int file_length, int strict_json)
{
  jsmn_parser parser;
  int token_count, status;
  json_s *json = utils_.alloc (sizeof (*json));

  if (!file_length)
    {
      utils_.error ("JSON file parse error: file contains no data");
      utils_.free (json);
      return NULL;
    }

  json->data = utils_.alloc (file_length);
  memcpy (json->data, file_data, file_length);

  if (strict_json)
    {
      strict_jsmn_init (&parser);
      token_count = strict_jsmn_parse (&parser,
                                       json->data, file_length, NULL, 0);
    }
  else
    {
      jsmn_init (&parser);
      token_count = jsmn_parse (&parser,
                                json->data, file_length, NULL, 0);
    }

  if (token_count == JSMN_ERROR_INVAL)
    indicate_location (parser.pos, json->data);

  utils_.error_if (token_count == JSMN_ERROR_PART,
                   "JSON file parse error: file data is incomplete");
  utils_.error_if (token_count == JSMN_ERROR_INVAL,
                   "JSON file parse error: file data is not valid JSON");

  if (token_count < 1)
    {
      utils_.error_if (token_count == 0,
                       "JSON file parse error: file contains no usable data");
      utils_.free (json->data);
      utils_.free (json);
      return NULL;
    }

  json->tokens = utils_.alloc (token_count * sizeof (jsmntok_t));

  if (strict_json)
    {
      strict_jsmn_init (&parser);
      status = strict_jsmn_parse (&parser, json->data,
                                  file_length, json->tokens, token_count);
    }
  else
    {
      jsmn_init (&parser);
      status = jsmn_parse (&parser, json->data,
                           file_length, json->tokens, token_count);
    }

  if (token_count == JSMN_ERROR_INVAL)
    indicate_location (parser.pos, json->data);

  utils_.error_if (status == JSMN_ERROR_PART,
                   "JSON file parse error: file data is incomplete");
  utils_.error_if (status == JSMN_ERROR_INVAL,
                   "JSON file parse error: file data is not valid JSON");

  if (status < 1)
    {
      utils_.fatal_if (status == JSMN_ERROR_NOMEM,
                       "internal error: not enough parser tokens allocated");
      utils_.free (json->data);
      utils_.free (json->tokens);
      utils_.free (json);
      return NULL;
    }

  stringify_and_unescape (json, token_count);
  return json;
}

static void
destroy (json_s *json)
{
  utils_.free (json->data);
  utils_.free (json->tokens);
  utils_.free (json);
  memset (json, 0, sizeof (json));
}

static const char *
type_string (jsmntype_t type)
{
  if (type == JSMN_OBJECT)
    return "object";
  else if (type == JSMN_ARRAY)
    return "array";
  else if (type == JSMN_STRING)
    return "string";
  else if (type == JSMN_PRIMITIVE)
    return "primitive";
  else
    return "<unknown>";
}

static int
extent_of (const json_s *json, int element)
{
  const jsmntok_t *token = json->tokens + element;

  if (token->type == JSMN_PRIMITIVE || token->type == JSMN_STRING)
    return 1;
  else if (token->type == JSMN_OBJECT)
    {
      int i, j = 0;
      for (i = 0; i < token->size; i++)
        {
          j += extent_of (json, element + j + 1);
          j += extent_of (json, element + j + 1);
        }
      return j + 1;
    }
  else if (token->type == JSMN_ARRAY)
    {
      int i, j = 0;
      for (i = 0; i < token->size; i++)
        j += extent_of (json, element + j + 1);
      return j + 1;
    }

  utils_.fatal ("internal error: invalid JSON token encountered");
  return 0;
}

static int
get_element (const json_s *json, int object, const char *target)
{
  const jsmntok_t *token = json->tokens + object;
  int length = strlen (target), i, j;

  if (token->type != JSMN_OBJECT)
    {
      utils_.error ("attempt to get an"
                    " element from a JSON '%s'", type_string (token->type));
      return 0;
    }

  j = object;
  for (i = 0; i < token->size; i++)
    {
      const jsmntok_t *name = json->tokens + j + 1;
      const char *string = json->data + name->start;

      if (!(name->type == JSMN_STRING || name->type == JSMN_PRIMITIVE))
        {
          utils_.error ("JSON object names must be string (or"
                        " primitive), found '%s'", type_string (name->type));

          return 0;
        }

      if (name->end - name->start == length
          && memcmp (string, target, length) == 0)
        return j + 2;

      j += extent_of (json, j + 1);
      j += extent_of (json, j + 1);
    }

  return 0;
}

static int
element_type (const json_s *json, int element)
{
  return json->tokens[element].type;
}

static const char *
element_string (const json_s *json, int element)
{
  const jsmntok_t *token = json->tokens + element;

  if (!(token->type == JSMN_PRIMITIVE || token->type == JSMN_STRING))
    {
      utils_.error ("attempt to get a string from a"
                    " '%s' JSON element", type_string (token->type));
      return "<invalid>";
    }

  return json->data + token->start;
}

static int
element_is_null (const json_s *json, int element)
{
  const jsmntok_t *token = json->tokens + element;

  return token->type == JSMN_PRIMITIVE
         && strcasecmp (json->data + token->start, "null") == 0;
}

static int
get_element_of_type (const json_s *json,
                     int object, const char *target, jsmntype_t type)
{
  int result = get_element (json, object, target);
  const jsmntok_t *token = json->tokens + result;

  if (result && token->type != type)
    {
      utils_.error ("JSON element '%s'"
                    " is not of type '%s'", target, type_string (type));
      result = 0;
    }
  return result;
}

static int
get_element_of_types (const json_s *json,
                      int object, const char *target,
                      jsmntype_t type1, jsmntype_t type2)
{
  int result = get_element (json, object, target);
  const jsmntok_t *token = json->tokens + result;

  if (result && !(token->type == type1 || token->type == type2))
    {
      utils_.error ("JSON element '%s' is not of type '%s' or type '%s'",
                    target, type_string (type1), type_string (type2));
      result = 0;
    }
  return result;
}

static int
get_required_element (const json_s *json,
                      int object, const char *from, const char *target)
{
  int result = get_element (json, object, target);

  if (!result)
    utils_.error ("JSON object '%s' is missing a '%s' element", from, target);
  return result;
}

static int
get_required_element_of_type (const json_s *json, int object,
                              const char *from, const char *target,
                              jsmntype_t type)
{
  int result = get_required_element (json, object, from, target);
  const jsmntok_t *token = json->tokens + result;

  if (result && token->type != type)
    {
      utils_.error ("JSON element '%s' in '%s' is not"
                    " of type '%s'", target, from, type_string (type));
      result = 0;
    }
  return result;
}

static int
get_required_element_of_types (const json_s *json,
                               int object,
                               const char *from, const char *target,
                               jsmntype_t type1, jsmntype_t type2)
{
  int result = get_required_element (json, object, from, target);
  const jsmntok_t *token = json->tokens + result;

  if (result && !(token->type == type1 || token->type == type2))
    {
      utils_.error ("JSON element '%s' in '%s' is not"
                    " of type '%s' or type '%s'",
                    target, from, type_string (type1), type_string (type2));
      result = 0;
    }
  return result;
}

static void
foreach_array_element (const json_s *json, int array,
                       void (*handler) (const json_s *json,
                                        int element,
                                        int index,
                                        void *opaque1, void *opaque2),
                       void *opaque1, void *opaque2)
{
  const jsmntok_t *token = json->tokens + array;
  int i, j;

  if (token->type != JSMN_ARRAY)
    {
      utils_.error ("attempt to iterate a '%s'"
                    " JSON element as an array", type_string (token->type));
      return;
    }

  j = array;
  for (i = 0; i < token->size; i++)
    {
      handler (json, j + 1, i, opaque1, opaque2);
      j += extent_of (json, j + 1);
    }
}

static void
foreach_object_element (const json_s *json, int object,
                        void (*handler) (const json_s *json,
                                         int name, int value, void *opaque),
                        void *opaque)
{
  const jsmntok_t *token = json->tokens + object;
  int i, j;

  if (token->type != JSMN_OBJECT)
    {
      utils_.error ("attempt to iterate a '%s'"
                    " JSON element as an object", type_string (token->type));
      return;
    }

  j = object;
  for (i = 0; i < token->size; i++)
    {
      handler (json, j + 1, j + 2, opaque);
      j += extent_of (json, j + 1);
      j += extent_of (json, j + 1);
    }
}

__attribute__((constructor))
void
init_json (void)
{
  json_.parse = parse;
  json_.destroy = destroy;
  json_.get_element = get_element;
  json_.element_type = element_type;
  json_.element_string = element_string;
  json_.element_is_null = element_is_null;
  json_.get_element_of_type = get_element_of_type;
  json_.get_element_of_types = get_element_of_types;
  json_.get_required_element = get_required_element;
  json_.get_required_element_of_type = get_required_element_of_type;
  json_.get_required_element_of_types = get_required_element_of_types;
  json_.foreach_array_element = foreach_array_element;
  json_.foreach_object_element = foreach_object_element;
}
