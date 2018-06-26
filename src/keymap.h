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

/* Keyboard layouts and conversions between them.  */

#ifndef KEYMAP_H_
#define KEYMAP_H_

struct keymap;
typedef struct keymap keymap_s;

struct keymap_ {
  keymap_s *(*open) (const char *path, int strict_json, int strict_iso_core);
  keymap_s *(*load) (const char *name, const char *search_path,
                     int strict_json, int strict_iso_core);
  void (*close) (keymap_s *keymap);
};

struct keymapper;
typedef struct keymapper keymapper_s;

struct keymapper_ {
  keymapper_s *(*create) (keymap_s *from,
                          keymap_s *to, int attempt_ctrl_keys);
  int (*requires_utf8) (const keymapper_s *keymapper);
  int (*convert) (const keymapper_s *keymapper, int key);
  int (*unconvert) (const keymapper_s *keymapper, int key);
  void (*destroy) (keymapper_s *keymapper);
};

extern struct keymap_ keymap_;
extern struct keymapper_ keymapper_;

#endif
