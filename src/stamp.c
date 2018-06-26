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

#define gv1 __GNUC__
#define gv2 __GNUC_MINOR__
#define gv3 __GNUC_PATCHLEVEL__

#define cv1 __clang_major__
#define cv2 __clang_minor__
#define cv3 __clang_patchlevel__

#define s(x) #x
#define ss(x) s(x)

#if defined(__GNUC__)
const char *const COMPILER = "gcc" ss(gv1) "." ss(gv2) "." ss(gv3);
#elif defined(__clang__)
const char *const COMPILER = "clang" ss(cv1) "." ss(cv2) "." ss(cv3);
#else
const char *const COMPILER = "<unknown>";
#endif

const char *const BUILD_DATE =
#include "_timestamp"
;

const char *const SOURCE_MD5 =
#include "stamp"
;
