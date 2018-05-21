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

/* JSON helper functions. The JSON parse done by JSMN is generally
   both "minimal" and "relaxed" -- there is no distinction between the
   null, boolean, and number 'primitives, and single-word 'primitives'
   can be used as strings and/or object element names.

   In addition, this module's unescaping is lazy (it omits unicode escapes).

   The result of both of these is that a lot of invalid JSON will parse,
   and while the hope is that it will produce usable results in Typist,
   the reality is that it may well sometimes produce odd behaviours.  */

#ifndef JSON_H
#define JSON_H

#include "jsmn.h"

static const int json_object = JSMN_OBJECT;
static const int json_array = JSMN_ARRAY;
static const int json_string = JSMN_STRING;
static const int json_primitive = JSMN_PRIMITIVE;

struct json;
typedef struct json json_s;

struct json_ {
  json_s *(*parse) (const char *file_data, int file_length, int strict_json);
  void (*destroy) (json_s *json);
  int (*get_element) (const json_s *json, int object, const char *target);
  int (*element_type) (const json_s *json, int element);
  const char * (*element_string) (const json_s *json, int element);
  int (*element_is_null) (const json_s *json, int element);
  int (*get_element_of_type) (const json_s *json,
                              int object, const char *target, jsmntype_t type);
  int (*get_element_of_types) (const json_s *json,
                               int object, const char *target,
                               jsmntype_t type1, jsmntype_t type2);
  int (*get_required_element) (const json_s *json, int object,
                               const char *from, const char *target);
  int (*get_required_element_of_type) (const json_s *json, int object,
                                       const char *from, const char *target,
                                       jsmntype_t type);
  int (*get_required_element_of_types) (const json_s *json, int object,
                                        const char *from, const char *target,
                                        jsmntype_t type1, jsmntype_t type2);
  void (*foreach_array_element) (const json_s *json, int array,
      void (*handler) (const json_s *json,
                       int element, int index_, void *opaque1, void *opaque2),
                                 void *opaque1, void *opaque2);
  void (*foreach_object_element) (const json_s *json, int object,
      void (*handler) (const json_s *json,
                       int name, int value, void *opaque),
                                  void *opaque);
};

struct json_ json_;

#endif
