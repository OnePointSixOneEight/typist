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

LESSON_FILES = LessonSeriesC.json LessonSeriesD.json LessonSeriesM.json \
               LessonSeriesN.json LessonSeriesQ.json LessonSeriesR.json \
               LessonSeriesS.json LessonSeriesT.json LessonSeriesU.json \
               LessonSeriesV.json

default: all

all: Makefile dist

dist: $(LESSON_FILES)

$(LESSON_FILES): proptojsonv2.py LessonSeries.zip
	python proptojsonv2.py LessonSeries.zip

maintainer-clean:
	rm -f $(LESSON_FILES)

clean:
clobber:
distclean:
