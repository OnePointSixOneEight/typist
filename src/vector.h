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

/* Vector is a growable array, holds opaque objects.  */

#ifndef VECTOR_H_
#define VECTOR_H_

struct vector;
typedef struct vector vector_s;

struct vector_ {
  vector_s *(*create) (int span);
  void (*destroy) (vector_s *vector);
  void (*get) (const vector_s *vector, int index_, void *payload);
  void (*set) (vector_s *vector, int index_, const void *payload);
  void (*delete_) (vector_s *vector, int index_);
  int (*get_length) (const vector_s *vector);
  void (*clear) (vector_s *vector);
  int (*push_back) (vector_s *vector, const void *payload);
};

extern struct vector_ vector_;

#endif
