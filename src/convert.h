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

/* Converts a JSON representation into a legacy format.  */

#ifndef CONVERT_H
#define CONVERT_H

#include "buffer.h"
#include "json.h"

struct conversion {
  int version;
  int length;
  char *data;
};
typedef struct conversion conversion_s;

struct convert_ {
  conversion_s *(*json_to_legacy) (const char *file_data,
                                   int file_length, int strict_json);
  void (*v1_json_to_legacy) (const json_s *json, int typist, buffer_s *buffer);
  void (*v2_json_to_legacy) (const json_s *json, int typist, buffer_s *buffer);
};

struct convert_ convert_;

#endif
