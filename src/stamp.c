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

#define v1 __GNUC__
#define v2 __GNUC_MINOR__
#define v3 __GNUC_PATCHLEVEL__

#define s(x) #x
#define ss(x) s(x)
const char *const COMPILER = ss(v1) "." ss(v2) "." ss(v3);
const char *const BUILD_DATE = __DATE__ " " __TIME__;
const char *const SOURCE_MD5 =
#include "stamp"
;
