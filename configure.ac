dnl vi: set ts=8 shiftwidth=8 noexpandtab:
dnl
dnl Typist 3.0 - improved typing tutor program for UNIX systems
dnl Copyright (C) 1998-2018  Simon Baldwin (simonb@sco.com)
dnl
dnl This program is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU General Public License
dnl as published by the Free Software Foundation; either version 2
dnl of the License, or (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
dnl

AC_INIT

AC_PROG_CC
AC_PROG_INSTALL

AC_CHECK_LIB(ncursesw,setcchar)
test "$ac_cv_lib_ncursesw_setcchar" = "no" \
    && AC_MSG_ERROR([Unable to locate a usable ncurses library])

AC_CONFIG_FILES([Makefile.inc Makefile])
AC_OUTPUT
