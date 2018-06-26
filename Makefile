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

DIST = typist-3.0

ZIPFILE = $(DIST).zip
DISTFILE = $(DIST)_$$(date +%Y%m%d%H%M%S).zip

TOPDIR = /usr/local # Override to install elsewhere (eg $HOME)
BINDIR = $(TOPDIR)/bin
LIBDIR = $(TOPDIR)/lib/typist
OWNER = root:root
MODE = 755
DMODE = 644
LESSON_FILES = default.json \
               LessonSeriesC.json LessonSeriesD.json LessonSeriesM.json \
               LessonSeriesN.json LessonSeriesQ.json LessonSeriesR.json \
               LessonSeriesS.json LessonSeriesT.json LessonSeriesU.json \
               LessonSeriesV.json
KEYMAP_FILES = colemak.json de.json \
               dvorak_single_lh.json dvorak_single_rh.json \
               dvorak_uk.json dvorak_us.json es.json fr.json jcuken.json \
               uk.json us.json workman.json

SUBDIRS = src keymaps lessons lessons_v1
DO = /bin/bash -e -c

default: all

all: Makefile
	$(DO) "for dir in $(SUBDIRS); do $(MAKE) -C \$$dir all; done"

clean:	Makefile
	$(DO) "for dir in $(SUBDIRS); do $(MAKE) -C \$$dir clean; done"
	rm -f $(ZIPFILE) $(DIST)_*.zip

clobber: clean
	$(DO) "for dir in $(SUBDIRS); do $(MAKE) -C \$$dir clobber; done"

distclean: clean
	$(DO) "for dir in $(SUBDIRS); do $(MAKE) -C \$$dir distclean; done"

maintainer-clean: clean
	$(DO) "for dir in $(SUBDIRS); do $(MAKE) -C \$$dir maintainer-clean; done"

install: all
	test -x src/typist
	-mkdir -p $(BINDIR)
	-mkdir -p $(LIBDIR)
	cp src/typist $(BINDIR)
	-chown $(OWNER) $(BINDIR)/typist
	chmod $(MODE) $(BINDIR)/typist
	( cd lessons; cp $(LESSON_FILES) $(LIBDIR) )
	-chown $(OWNER) $(LIBDIR)/*
	chmod $(DMODE) $(LIBDIR)/*
	-mkdir -p $(LIBDIR)/keymaps
	( cd keymaps; cp $(KEYMAP_FILES) $(LIBDIR)/keymaps )
	-chown $(OWNER) $(LIBDIR)/keymaps/*
	chmod $(DMODE) $(LIBDIR)/keymaps/*

uninstall:
	-rm $(BINDIR)/typist
	-rmdir $(BINDIR)
	-rm $(LIBDIR)/keymaps/*
	-rmdir $(LIBDIR)/keymaps
	-rm $(LIBDIR)/*
	-rmdir $(LIBDIR)

dist: distclean
	$(DO) "$(MAKE) -C src stamp"
	$(DO) "for dir in $(SUBDIRS); do $(MAKE) -C \$$dir dist; done"
	rm -f $(DIST); ln -s . $(DIST)
	zip -r $(ZIPFILE) $$(ls -1d $(DIST)/* | grep -v '^$(DIST)/$(DIST)$$')
	rm $(DIST)
	@echo "Distribution size: $$(expr $$(wc -c <$(ZIPFILE)) / 1024 + 1)kb"
	@ln $(ZIPFILE) $(DISTFILE)
