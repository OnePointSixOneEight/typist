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

/* Implements a hash map of strings to opaque objects.  */

#ifndef MAP_H_
#define MAP_H_

struct map;
typedef struct map map_s;

struct map_ {
  map_s *(*create) (int span, int key_length, int buckets);
  void (*destroy) (map_s *map);
  int (*contains) (const map_s *map, const void *key);
  int (*get) (const map_s *map, const void *key, void *payload);
  void (*set) (map_s *map, const void *key, const void *payload);
  void (*delete_) (map_s *map, const void *key);
  int (*get_length) (const map_s *map);
  void (*clear) (map_s *map);
};

extern struct map_ map_;

#endif
