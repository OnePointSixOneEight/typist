# vi: set ts=8 shiftwidth=8 noexpandtab:
#
# Typist 3.0 - improved typing tutor program for UNIX systems
# Copyright (C) 1998-2018  Simon Baldwin (simon_baldwin@yahoo.com)
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

DO	= /bin/bash -e -c

LDFLAGS	= $(LDEXTRA) $(DEBUG)
CFLAGS	= -ansi -D_POSIX_C_SOURCE=200112L
CFLAGS += -DNCURSES -DLONG_OPTIONS -DJSMN_PARENT_LINKS -DGETTEXT_STUBS \
	  -Wall -Wextra -Wshadow -pedantic \
	  $(CEXTRA) $(DEBUG)

JSMN_STRICT_WRAP = -Djsmn_init=strict_jsmn_init -Djsmn_parse=strict_jsmn_parse
JSMN_STRICT = -DJSMN_STRICT $(JSMN_STRICT_WRAP)

OBJECTS = stamp.o buffer.o convert.o v1conv.o v2conv.o jsmn.o jsmn_s.o \
	  json.o keymap.o map.o screen.o script.o typist.o uncurse.o utils.o \
	  utf8.o vector.o

TYPIST = $(CURDIR)/typist

default: all
all: Makefile typist

stamp: *.h *.c Makefile
	test -f stamp || echo "<unknown>" >| stamp
	echo "s($$(cat *.h *.c Makefile | md5sum))" >| /tmp/stamp
	cmp -s /tmp/stamp stamp || cp /tmp/stamp stamp
	rm -f /tmp/stamp

typist: $(OBJECTS)
	$(CC) $(LDFLAGS) -o typist $(OBJECTS) $(LIBS)

stamp.o: stamp.c stamp
	@echo "s($$(date '+%Y%m%d%H%M%S'))" >| _timestamp
	$(CC) $(CFLAGS) -c stamp.c -o stamp.o
	@rm -f _timestamp

jsmn_s.o: jsmn.c jsmn.h
	$(CC) $(CFLAGS) $(JSMN_STRICT) -c jsmn.c -o jsmn_s.o

buffer.o: buffer.c _gettext.h utils.h buffer.h
convert.o: convert.c _gettext.h buffer.h json.h jsmn.h utils.h convert.h
jsmn.o: jsmn.c jsmn.h
json.o: json.c _gettext.h jsmn.h utils.h json.h
keymap.o: keymap.c _gettext.h json.h jsmn.h utf8.h utils.h keymap.h
map.o: map.c _gettext.h utils.h map.h
screen.o: screen.c _gettext.h utf8.h screen.h
script.o: script.c _gettext.h convert.h buffer.h json.h jsmn.h utf8.h \
          utils.h script.h
typist.o: typist.c _gettext.h buffer.h map.h keymap.h screen.h script.h \
          utf8.h utils.h vector.h
uncurse.o: uncurse.c _gettext.h utf8.h utils.h screen.h
utf8.o: utf8.c utils.h utf8.h
utils.o: utils.c _gettext.h screen.h utils.h
v1conv.o: v1conv.c _gettext.h buffer.h json.h jsmn.h script.h utils.h \
          convert.h
v2conv.o: v2conv.c _gettext.h buffer.h json.h jsmn.h map.h script.h \
          utils.h convert.h
vector.o: vector.c _gettext.h utils.h vector.h

install: all
	$(INSTALL) -d $(bindir)
	$(INSTALL_PROGRAM) typist $(bindir)/typist

install-strip: all
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install

uninstall:
	rm -rf $(bindir)/typist

clean:
	rm -rf *.o typist
	rm -rf core gmon.out

check: all
	$(DO) "$(TYPIST) -v; $(TYPIST) -B"

dist:
clobber: clean
distclean: clean
maintainer-clean: clean
	rm -f stamp
