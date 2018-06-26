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

/* Buffer is a growable basic byte array.  */

#ifndef BUFFER_H_
#define BUFFER_H_

struct buffer;
typedef struct buffer buffer_s;

struct buffer_ {
  buffer_s *(*create) (int size);
  void (*destroy) (buffer_s *buffer);
  const void *(*get) (const buffer_s *buffer);
  void *(*export) (buffer_s *buffer);
  int (*get_length) (const buffer_s *buffer);
  void (*clear) (buffer_s *buffer);
  void (*append) (buffer_s *buffer, const void *data, int size);
};

extern struct buffer_ buffer_;

#endif
