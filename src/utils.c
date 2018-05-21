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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "screen.h"
#include "utils.h"

/* Message handling.  */
static void
info (const char *format, ...)
{
  FILE* stream = (FILE*)utils_.info_stream;
  va_list ap; va_start (ap, format);
  if (stream)
    {
      fprintf (stream, "typist: Info: ");
      vfprintf (stream, format, ap);
      fprintf (stream, "\n");
      fflush (stream);
    }
  va_end (ap);
}

static void
emit (const char *severity, const char *format, va_list ap)
{
  FILE* stream = (FILE*)utils_.error_stream;
  if (!stream && strcasecmp (severity, "Fatal") == 0)
    stream = stderr;
  if (stream)
    {
      fprintf (stream, "typist: %s: ", severity);
      vfprintf (stream, format, ap);
      fprintf (stream, "\n");
      fflush (stream);
    }
}

static void
emit_if (int test,
         const char *severity, const char *format, va_list ap)
{
  if (test)
    emit (severity, format, ap);
}

static void
warning (const char *format, ...)
{
  va_list ap; va_start (ap, format);
  emit ("Warning", format, ap);
  va_end (ap);
}

static void
warning_if (int test, const char *format, ...)
{
  va_list ap; va_start (ap, format);
  emit_if (test, "Warning", format, ap);
  va_end (ap);
}

static void
error (const char *format, ...)
{
  va_list ap; va_start (ap, format);
  emit ("Error", format, ap);
  va_end (ap);
}

static void
error_if (int test, const char *format, ...)
{
  va_list ap; va_start (ap, format);
  emit_if (test, "Error", format, ap);
  va_end (ap);
}

static void
fatal (const char *format, ...)
{
  va_list ap; va_start (ap, format);
  if (screen_.finalize)
    screen_.finalize ();
  emit ("Fatal", format, ap);
  exit (1);
}

static void
fatal_if (int test, const char *format, ...)
{
  va_list ap; va_start (ap, format);
  if (test)
    {
      if (screen_.finalize)
        screen_.finalize ();
      emit ("Fatal", format, ap);
      exit (1);
    }
  va_end (ap);
}

/* Memory allocation (wrappers around malloc and friends).  */
static void *
alloc_ (int bytes)
{
  void *allocated;
  fatal_if (!(bytes > 0), "internal error: attempt to alloc 0 bytes or fewer");

  allocated = malloc (bytes);
  fatal_if (!allocated, "internal error: malloc returned a null pointer");
  return allocated;
}

static void *
realloc_ (void *pointer, int bytes)
{
  void *allocated;
  fatal_if (!(bytes > 0),
            "internal error: attempt to realloc 0 bytes or fewer");

  allocated = realloc (pointer, bytes);
  fatal_if (!allocated, "internal error: realloc returned a null pointer");
  return allocated;
}

static void *
strdup_ (const char *string)
{
  void *allocated;
  fatal_if (!string, "internal error: attempt to strdup a null string");

  allocated = malloc (strlen (string) + 1);
  fatal_if (!allocated, "internal error: malloc returned a null pointer");
  strcpy (allocated, string);
  return allocated;
}

static void
free_ (void *pointer)
{
  free (pointer);
}

/* Timing.  */
static double
start_timer (void)
{
  struct timeval timer;
  gettimeofday (&timer, NULL);
  return timer.tv_sec + (timer.tv_usec / 1.0e6);
}

static double
timer_interval (double started)
{
  fatal_if (!(started > 0.0),
            "internal error: attempt to use an uninitialized timer");
  return start_timer () - started;
}

/* File paths.  */
static char *
locate_file (const char *search_path,
             const char *name, const char *extension)
{
  char *path = NULL, *tokenized, *token;
  FILE *stream;

  if (strchr (name, '/') || !search_path)
    {
      path = utils_.strdup (name);
      stream = fopen (path, "r");
      if (stream)
        {
          fclose (stream);
          return path;
        }

      path = utils_.realloc (path, strlen (name) + strlen (extension) + 1);
      sprintf (path, "%s%s", name, extension);
      stream = fopen (path, "r");
      if (stream)
        {
          fclose (stream);
          return path;
        }

      utils_.free (path);
      return NULL;
    }

  tokenized = utils_.strdup (search_path);
  for (token = strtok (tokenized, ":"); token; token = strtok (NULL, ":"))
    {
      path = utils_.alloc (strlen (token) + strlen (name) + 2);
      sprintf (path, "%s/%s", token, name);
      stream = fopen (path, "r");
      if (stream)
        {
          fclose (stream);
          utils_.free (tokenized);
          return path;
        }

      path = utils_.realloc (path,
                             strlen (token)
                             + strlen (name) + strlen (extension) + 2);
      sprintf (path, "%s/%s%s", token, name, extension);
      stream = fopen (path, "r");
      if (stream)
        {
          fclose (stream);
          utils_.free (tokenized);
          return path;
        }
    }
  utils_.free (tokenized);
  utils_.free (path);
  return NULL;
}

__attribute__((constructor))
void
init_utils (void)
{
  utils_.info_stream = NULL;
  utils_.error_stream = stderr;
  utils_.info = info;
  utils_.warning = warning;
  utils_.warning_if = warning_if;
  utils_.error = error;
  utils_.error_if = error_if;
  utils_.fatal = fatal;
  utils_.fatal_if = fatal_if;

  utils_.alloc = alloc_;
  utils_.realloc = realloc_;
  utils_.strdup = strdup_;
  utils_.free = free_;

  utils_.start_timer = start_timer;
  utils_.timer_interval = timer_interval;

  utils_.locate_file = locate_file;
}
