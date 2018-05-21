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

# For gcc 4 on MacOS, should also work for Linux.
CC = gcc
DEBUG = # set to '-g -O0' for a debug build
WARNINGS = -Wall -Wextra -Wshadow -pedantic

CURSES_FLAGS = -DNCURSES -I/usr/include/ncurses
OPTIONS_FLAGS = -DLONG_OPTIONS
CFLAGS = -ansi -O2 $(OPTIONS_FLAGS) $(CURSES_FLAGS) $(WARNINGS) $(DEBUG)
LDFLAGS = $(DEBUG)
LIBS = -lncurses -lm

JSMN_STRICT_WRAP = -Djsmn_init=strict_jsmn_init -Djsmn_parse=strict_jsmn_parse
JSMN_STRICT = -DJSMN_STRICT $(JSMN_STRICT_WRAP)

OBJECTS = stamp.o buffer.o convert.o v1conv.o v2conv.o jsmn.o jsmn_s.o \
          json.o keymap.o map.o screen.o script.o typist.o utils.o vector.o

default: all
all: Makefile typist_bin

stamp: *.h *.c Makefile
	test -f stamp || echo "<unknown>" > stamp
	echo "s($$(cat *.h *.c Makefile | md5sum))" > /tmp/stamp
	cmp -s /tmp/stamp stamp || cp /tmp/stamp stamp
	rm -f /tmp/stamp

typist_bin: $(OBJECTS)
	$(CC) $(LDFLAGS) -o typist_bin $(OBJECTS) $(LIBS)

jsmn_s.o: jsmn.c jsmn.h
	$(CC) $(CFLAGS) $(JSMN_STRICT) -c jsmn.c -o jsmn_s.o

buffer.o: buffer.c utils.h buffer.h
convert.o: convert.c buffer.h json.h jsmn.h utils.h convert.h
jsmn.o: jsmn.c jsmn.h
json.o: json.c jsmn.h utils.h json.h
keymap.o: keymap.c json.h jsmn.h utils.h keymap.h
map.o: map.c utils.h map.h
screen.o: screen.c screen.h
script.o: script.c convert.h buffer.h json.h jsmn.h utils.h script.h
stamp.o: stamp.c stamp
typist.o: typist.c buffer.h map.h keymap.h screen.h script.h utils.h vector.h
utils.o: utils.c screen.h utils.h
v1conv.o: v1conv.c buffer.h json.h jsmn.h script.h utils.h convert.h
v2conv.o: v2conv.c buffer.h json.h jsmn.h map.h script.h utils.h convert.h
vector.o: vector.c utils.h vector.h

dist:
clobber: clean
	rm -f .gdb_history
distclean: clobber
maintainer-clean: clobber
	rm -f stamp
clean:	Makefile
	rm -f *.o typist_bin
