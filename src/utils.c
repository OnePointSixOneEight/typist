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

#include <langinfo.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <strings.h>

#include "_gettext.h"
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
      fprintf (stream, _("typist: Info: "));
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
  if (stream)
    {
      fprintf (stream, _("typist: %s: "), severity);
      vfprintf (stream, format, ap);
      fprintf (stream, "\n");
      fflush (stream);
    }
  if (stream != stderr && strcasecmp (severity, _("Fatal")) == 0)
    {
      fprintf (stderr, _("typist: %s: "), severity);
      vfprintf (stderr, format, ap);
      fprintf (stderr, "\n");
      fflush (stderr);
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
  emit (_("Warning"), format, ap);
  va_end (ap);
}

static void
warning_if (int test, const char *format, ...)
{
  va_list ap; va_start (ap, format);
  emit_if (test, _("Warning"), format, ap);
  va_end (ap);
}

static void
error (const char *format, ...)
{
  va_list ap; va_start (ap, format);
  emit (_("Error"), format, ap);
  va_end (ap);
}

static void
error_if (int test, const char *format, ...)
{
  va_list ap; va_start (ap, format);
  emit_if (test, _("Error"), format, ap);
  va_end (ap);
}

static void
fatal (const char *format, ...)
{
  va_list ap; va_start (ap, format);
  if (screen_.finalize)
    screen_.finalize ();
  emit (_("Fatal"), format, ap);
  kill (getpid (), SIGTRAP);
}

static void
fatal_if (int test, const char *format, ...)
{
  va_list ap; va_start (ap, format);
  if (test)
    {
      if (screen_.finalize)
        screen_.finalize ();
      emit (_("Fatal"), format, ap);
      kill (getpid (), SIGTRAP);
    }
  va_end (ap);
}

static void
exit_ (const char *format, ...)
{
  va_list ap; va_start (ap, format);
  if (screen_.finalize)
    screen_.finalize ();
  emit (_("Exit"), format, ap);
  exit (EXIT_FAILURE);
}

static void
exit_if (int test, const char *format, ...)
{
  va_list ap; va_start (ap, format);
  if (test)
    {
      if (screen_.finalize)
        screen_.finalize ();
      emit (_("Exit"), format, ap);
      exit (EXIT_FAILURE);
    }
}

/* Memory allocation (wrappers around malloc and friends).  */
static void *
alloc_ (int bytes)
{
  void *allocated;
  fatal_if (!(bytes > 0),
            _("internal error: attempt to alloc 0 bytes or fewer"));

  allocated = malloc (bytes);
  fatal_if (!allocated, _("internal error: malloc returned a null pointer"));
  return allocated;
}

static void *
realloc_ (void *pointer, int bytes)
{
  void *allocated;
  fatal_if (!(bytes > 0),
            _("internal error: attempt to realloc 0 bytes or fewer"));

  allocated = realloc (pointer, bytes);
  fatal_if (!allocated, _("internal error: realloc returned a null pointer"));
  return allocated;
}

static void *
strdup_ (const char *string)
{
  void *allocated;
  fatal_if (!string, _("internal error: attempt to strdup a null string"));

  allocated = malloc (strlen (string) + 1);
  fatal_if (!allocated, _("internal error: malloc returned a null pointer"));
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
            _("internal error: attempt to use an uninitialized timer"));
  return start_timer () - started;
}

/* Locales.  */
static void
set_locale (const char *locale)
{
  const char *current = setlocale (LC_ALL, NULL);
  fatal_if (!locale, _("internal error: attempt to set NULL locale"));

  if (strcmp (locale, current) != 0)
    {
      char *updated = setlocale (LC_ALL, locale);
      if (updated)
        info (_("locale set to '%s'"), updated);
      else
        error (_("error setting locale to '%s'"), locale);
    }
}

static int
is_utf8_locale (void)
{
  const char *codeset = nl_langinfo (CODESET);

  return strcasecmp (codeset, "UTF-8") == 0
         || strcasecmp (codeset, "UTF8") == 0;
}

static void
bind_text_domain (const char *package, const char *localedir)
{
  package = package;
  localedir = localedir;
  bindtextdomain (package, localedir);
  bind_textdomain_codeset (package, "UTF-8");
}

static void
set_text_domain (const char *package)
{
  package = package;
  textdomain (package);
}

/* File paths and file data.  */
static int
probe_file (const char *path)
{
  FILE *stream = fopen (path, "rb");

  if (stream)
    {
      fclose (stream);
      return B.True;
    }
  else
    return B.False;
}

static char *
locate_without_extension (const char *directory, const char *name)
{
  char *path;

  if (*directory)
    {
      path = alloc_ (strlen (directory) + strlen (name) + 2);
      sprintf (path, "%s/%s", directory, name);
    }
  else
    path = strdup_ (name);
  if (probe_file (path))
    return path;

  free_ (path);
  return NULL;
}

static char *
locate_with_extension (const char *directory,
                       const char *name, const char *extension)
{
  char *path;

  if (*directory)
    {
      path = alloc_ (strlen (directory)
                     + strlen (name) + strlen (extension) + 2);
      sprintf (path, "%s/%s%s", directory, name, extension);
    }
  else
    {
      path = alloc_ (strlen (name) + strlen (extension) + 1);
      sprintf (path, "%s%s", name, extension);
    }
  if (probe_file (path))
    return path;

  free_ (path);
  return NULL;
}

static char *
locate_file_directly (const char *directory,
                      const char *name, const char *extension)
{
  char *path;

  path = locate_without_extension (directory, name);
  if (path)
    return path;

  path = locate_with_extension (directory, name, extension);
  if (path)
    return path;

  return NULL;
}

static char *
locate_file_on_search_path (const char *search_path,
                            const char *name, const char *extension)
{
  char *token, *tokenized = strdup_ (search_path);
  for (token = strtok (tokenized, ":"); token; token = strtok (NULL, ":"))
    {
      char *path = locate_file_directly (token, name, extension);
      if (path)
        {
          free_ (tokenized);
          return path;
        }

      free_ (path);
    }

  free_ (tokenized);
  return NULL;
}

static char *
locate_file (const char *search_path,
             const char *name, const char *extension)
{
  if (strchr (name, '/') || !search_path)
    return locate_file_directly ("", name, extension);
  else
    return locate_file_on_search_path (search_path, name, extension);
}

static int
read_in_file (void *v_stream, char **file_data)
{
  FILE *stream = (FILE*)v_stream;
  char *data;
  int bytes, status;

  status = fseek (stream, 0, SEEK_END);
  fatal_if (status == -1, _("internal error: fseek returned -1"));
  bytes = ftell (stream);

  if (bytes > 0)
    {
      data = utils_.alloc (bytes);

      rewind (stream);
      status = fread (data, bytes, 1, stream);
      fatal_if (status != 1, _("internal error: fread did not return 1"));

      *file_data = data;
    }
  else
    *file_data = NULL;

  return bytes;
}

struct utils_ utils_;

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
  utils_.exit = exit_;
  utils_.exit_if = exit_if;

  utils_.alloc = alloc_;
  utils_.realloc = realloc_;
  utils_.strdup = strdup_;
  utils_.free = free_;

  utils_.start_timer = start_timer;
  utils_.timer_interval = timer_interval;

  utils_.set_locale = set_locale;
  utils_.is_utf8_locale = is_utf8_locale;
  utils_.bind_text_domain = bind_text_domain;
  utils_.set_text_domain = set_text_domain;

  utils_.locate_file = locate_file;
  utils_.read_in_file = read_in_file;
}
