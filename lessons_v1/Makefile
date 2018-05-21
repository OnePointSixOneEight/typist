# vi: set ts=8 shiftwidth=8 noexpandtab:
#
# Typist 3.0 - improved typing tutor program for UNIX systems
# Copyright (C) 1998-2018  Simon Baldwin (simonb@sco.com)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

LESSON_FILES = default.json demo.json \
               series_c.json series_d.json series_m.json series_n.json \
               series_q.json series_r.json series_s.json series_t.json \
               series_u.json series_v.json

default: all

all: Makefile dist

dist: $(LESSON_FILES)

$(LESSON_FILES): typtojsonv1.py gtypist.zip
	python typtojsonv1.py gtypist.zip

maintainer-clean:
	rm -f $(LESSON_FILES)

clean:
clobber:
distclean:
