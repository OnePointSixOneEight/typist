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

include ../Makefile.inc

KEYMAP_FILES = colemak.json de.json \
               dvorak_single_lh.json dvorak_single_rh.json \
               dvorak_uk.json dvorak_us.json es.json fr.json jcuken.json \
               uk.json us.json workman.json
DO = /bin/bash -e -c
TYPIST = $(CURDIR)/../src/typist

default: all

all:

dist: Makefile $(KEYMAP_FILES)

$(KEYMAP_FILES): generate.py
	python generate.py

install: all
	$(INSTALL) -d $(libdir)/typist/keymaps
	$(DO) "for file in $(KEYMAP_FILES); do $(INSTALL_DATA) \$$file $(libdir)/typist/keymaps/\$$file; done"

install-strip: all
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install

uninstall:
	$(DO) "for file in $(KEYMAP_FILES); do rm -f $(libdir)/typist/keymaps/\$$file; done"
	-rmdir $(libdir)/typist/keymaps $(libdir)/typist

check: $(KEYMAP_FILES)
	$(DO) "tempfile=\$$(mktemp --suffix=.typ); echo 'X|' >\$$tempfile; \
	for file in $(KEYMAP_FILES); \
	  do echo ' ' | $(TYPIST) -# -S -k$(CURDIR)/\$$file,$(CURDIR)/\$$file \$$tempfile; echo PASS: \$$file; \
	done; \
	rm -f \$$tempfile"

maintainer-clean:
	rm -f $(KEYMAP_FILES)

clean:
clobber:
distclean:
