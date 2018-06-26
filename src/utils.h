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

/* Encapsulation of a few general utility functions. utils_.error_stream
   is a FILE* stream, default stderr, and can be overridden to redirect
   messages, or set to NULL to entirely suppress them. utils_.info_stream
   is similar, default NULL, for tracing interpreter actions. */

#ifndef UTILS_H
#define UTILS_H

#define _pf1 __attribute__ ((format (printf, 1, 2)))
#define _pf2 __attribute__ ((format (printf, 2, 3)))
#define p_(x) ((void*)(x))

static const struct {
  const int False;
  const int True;
} B = { 0, !0 };

struct utils_ {
  void *info_stream;
  void *error_stream;
  void (*info) (const char *format, ...)_pf1;
  void (*warning) (const char *format, ...)_pf1;
  void (*warning_if) (int test, const char *format, ...)_pf2;
  void (*error) (const char *format, ...)_pf1;
  void (*error_if) (int test, const char *format, ...)_pf2;
  void (*fatal) (const char *format, ...)_pf1;
  void (*fatal_if) (int test, const char *format, ...)_pf2;

  void *(*alloc) (int bytes);
  void *(*realloc) (void *pointer, int bytes);
  void *(*strdup) (const char *string);
  void (*free) (void *pointer);

  double (*start_timer) (void);
  double (*timer_interval) (double started);

  void (*set_locale) (const char *locale);
  int (*is_utf8_locale) (void);
  void (*bind_text_domain) (const char *package, const char *localedir);
  void (*set_text_domain) (const char *package);

  char *(*locate_file) (const char *search_path,
                        const char *name, const char *suffix);
  int (*read_in_file) (void *v_stream, char **file_data);
};

#undef _pf1
#undef _pf2

extern struct utils_ utils_;

#endif
