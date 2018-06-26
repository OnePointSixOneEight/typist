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

/* Stub internationalization/localization.  */

#ifndef _GETTEXT_H
#define _GETTEXT_H

#if !defined(ENABLE_NLS) || !ENABLE_NLS

#if !defined(PACKAGE)
#define PACKAGE "stub_package"
#endif
#if !defined(LOCALEDIR)
#define LOCALEDIR "stub_localedir"
#endif

#endif  /* ENABLE_NLS */

#if !defined(GETTEXT_STUBS)

/* Not stubs: use the real gettext.h.  */
#define _(S) gettext (S)
#define N_(S) gettext_noop (S)

#include "gettext.h"

#else /* if defined(GETTEXT_STUBS) */

#if defined(__OPTIMIZE__)

/* Stubs+Optimized: make _() invisible for printf format checks.  */
#define _(S) S
#define N_(S) (S)

#define textdomain(D)
#define bindtextdomain(P, D)
#define bind_textdomain_codeset(P, C)

#else /* if !defined(__OPTIMIZE__) */

/* Stubs+Debug: make _() a function, checks that real gettext will work.  */
#define _(S) gettext (S)
#define N_(S) gettext_noop (S)

#define gettext_noop(S) S
static void textdomain (const char *d) { d = d; }
static void bindtextdomain (const char *p, const char *d) { p = p; d = d; }
static void
    bind_textdomain_codeset (const char *p, const char *c) { p = p; c = c; }

static const char *
gettext (const char *s)
{
  if (!s)
    {
      /* Not reached. Purely to suppress unused function warnings.  */
      textdomain (s);
      bindtextdomain (s, s);
      bind_textdomain_codeset (s, s);
    }
  return s;
}

#endif /* __OPTIMIZE__ */

#endif /* GETTEXT_STUBS */

#endif
