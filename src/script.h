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

/* Abstraction of a typist script. Uses a struct of function pointer as a
   simple C namespace, to avoid burdening the global program namespace.
   The script_ structure contains pointers to implementation functions and
   is 'public'. The script structure contains per-instance script data and
   is in effect 'private' by being opaque.  */

#ifndef SCRIPT_H
#define SCRIPT_H

static const struct {
  const char CONTINUATION;
  const char LABEL;
  const char TUTORIAL;
  const char INSTRUCTION;
  const char CLEAR_SCREEN;
  const char GOTO;
  const char EXIT;
  const char QUERY;
  const char IF_YES_GOTO;
  const char IF_NO_GOTO;
  const char DRILL;
  const char DRILL_PRACTICE;
  const char SPEED_TEST;
  const char SPEED_TEST_PRACTICE;
  const char BIND_FUNCTION_KEY;
  const char SET_ERROR_LIMIT;
  const char ON_FAILURE_GOTO;
  const char MENU;
  const char EXECUTE;
  /* JSON converter conveniences. These allow JSON to easily use a distinct
     action for persistent setting, rather than a less obvious trailing '*'
     on data. The trailing '*' will still work, though.  */
  const char PERSISTENT_SET_ERROR_LIMIT;
  const char PERSISTENT_ON_FAILURE_GOTO;
  /* Very old Typist releases used 'O' and 'P' for single run drills and
     speed tests. Implemented here for (very) backwards compatibility.  */
  const char COMPAT_DRILL_PRACTICE;
  const char COMPAT_SPEED_TEST;
} C = { ' ', '*', 'T', 'I', 'B', 'G', 'X',
        'Q', 'Y', 'N', 'D', 'd', 'S', 's', 'K', 'E', 'F', 'M', '+',
        'e', 'f', 'O', 'P' };

struct script;
typedef struct script script_s;

struct script_ {
  script_s *(*open) (const char *path, int strict_json);
  script_s *(*load) (const char *name,
                     const char *search_path, int strict_json);
  void (*close) (script_s *script);
  int (*requires_utf8) (const script_s *script);
  int (*get_version) (const script_s *script);
  const char *(*get_locale) (const script_s *script);
  int (*get_action) (const script_s *script);
  const char *(*get_data) (const script_s *script);
  const char *(*get_statement_buffer) (const script_s *script);
  void (*rewind) (script_s *script);
  int (*get_position) (const script_s *script);
  void (*set_position) (script_s *script, int position);
  int (*has_more_data) (const script_s *script);
  void (*get_next_statement) (script_s *script);
  void (*validate_parsed_data) (const script_s *script);
  void (*print_parsed_data) (const script_s *script);
};

extern struct script_ script_;

#endif
