#!/usr/bin/python

# Generate JSON keymaps from dense data.

# Approximates: http://unicode.org/reports/tr35/tr35-keyboards.html#Keyboards
# Keyboard rows are B (lowest) to E (highest, usually numbers). A is ignored.
# Keyboard columns are 00..NN numbered from the left. Eg, for standard QWERTY,
# E01 glyph is '1', E10 is '0', D01 is 'Q', C01 is 'A', etc.

import sys

# [(filename, name,
#   unshifted-E00..E15,D00..D15,C00..C15,B00..B15, shifted-...), ...]
# A space character indicates a key that is either absent or unmapped. Space
# itself is never remapped, nor is Return. Pound is currently not implemented
# on UK keyboard variants.
KEYMAPS = [
[ 'us', 'US',
    r"`1234567890-=    qwertyuiop[]\   asdfghjkl;'      zxcvbnm,./    ",
    r'~!@#$%^&*()_+    QWERTYUIOP{}|   ASDFGHJKL:"      ZXCVBNM<>?    '],
#     |Row E          |Row D          |Row C          |Row B          |
[ 'uk', 'UK',
    r"`1234567890-=    qwertyuiop[]    asdfghjkl;'#    \zxcvbnm,./    ",
    r' !" $%^&*()_+    QWERTYUIOP{}    ASDFGHJKL:@~    |ZXCVBNM<>?    '],
#     |Row E          |Row D          |Row C          |Row B          |
[ 'dvorak_us', 'US Dvorak',
    r"`1234567890[]    ',.pyfgcrl/=\   aoeuidhtns-      ;qjkxbmwvz    ",
    r'~!@#$%^&*(){}    "<>PYFGCRL?+|   AOEUIDHTNS_      :QJKXBMWVZ    '],
#     |Row E          |Row D          |Row C          |Row B          |
[ 'dvorak_uk', 'UK Dvorak',
    r"`1234567890[]    ',.pyfgcrl/=    aoeuidhtns-#    \;qjkxbmwvz    ",
    r' !" $%^&*(){}    @<>PYFGCRL?+    AOEUIDHTNS_~    |:QJKXBMWVZ    '],
#     |Row E          |Row D          |Row C          |Row B          |
[ 'colemak', 'Colemak',
    r"`1234567890-=    qwfpgjluy;[]\   arstdhneio'      zxcvbkm,./    ",
    r'~!@#$%^&*()_+    QWFPGJLUY:{}|   ARSTDHNEIO"      ZXCVBKM<>?    '],
#     |Row E          |Row D          |Row C          |Row B          |
[ 'dvorak_single_lh', 'Dvorak Single Hand Left-handed',
    r"`[]/pfmlj4321    ;qbyurso.65=\   -kctdheaz87      'xgvwni,09    ",
    r'~{}?PFMLJ$#@!    :QBYURSO>^%+|   _KCTDHEAZ*&      "XGVWNI<)(    '],
#     |Row E          |Row D          |Row C          |Row B          |
[ 'dvorak_single_rh', 'Dvorak Single Hand Right-handed',
    r"`1234jlmfp/[]    56q.orsuyb;=\   78zaehtdck-      90x,inwvg'    ",
    r'~!@#$JLMFP?{}    %^Q>ORSUYB:+|   &*ZAEHTDCK_      ()X<INWVG"    '],
#     |Row E          |Row D          |Row C          |Row B          |
[ 'workman', 'Workman',
    r"`1234567890-=    qdrwbjfup;[]\   ashtgyneoi'      zxmcvkl,./    ",
    r'~!@#$%^&*()_+    QDRWBJFUP:{}|   ASHTGYNEOI"      ZXMCVKL<>?    '],
#     |Row E          |Row D          |Row C          |Row B          |
]

ISO_RANGE = range (0, 16)
ISO_POSITIONS = (
    ['E%02d' % i for i in ISO_RANGE] + ['D%02d' % i for i in ISO_RANGE]
  + ['C%02d' % i for i in ISO_RANGE] + ['B%02d' % i for i in ISO_RANGE])

class json:
  def __init__ (self):
    self.__buf = []
    self.__indent = 0

  def esc (s):
    return (s.replace('\\', '\\\\')
            .replace('"', '\\"').replace('\t', '\\t'))
  esc = staticmethod (esc)

  def iter (l):
    return [(l[i], i < len (l) - 1) for i in xrange (0, len (l))]
  iter = staticmethod (iter)

  def add (self, s, indent=0, more=False):
    if indent < 0:
      self.__indent += indent
    e = '  ' * self.__indent + s
    if more:
      e += ','
    self.__buf += [e]
    if indent > 0:
      self.__indent += indent

  def get (self):
    return '\n'.join (self.__buf) + '\n'

def copyright (j):
  copyright = [
    'Typist 3.0 - improved typing tutor program for UNIX systems',
    'Copyright (C) 1998-2018  Simon Baldwin (simon_baldwin@yahoo.com)',
    '',
    'This program is free software; you can redistribute it and/or',
    'modify it under the terms of the GNU General Public License',
    'as published by the Free Software Foundation; either version 2',
    'of the License, or (at your option) any later version.',
    '',
    'This program is distributed in the hope that it will be useful,',
    'but WITHOUT ANY WARRANTY; without even the implied warranty of',
    'MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the',
    'GNU General Public License for more details.',
    '',
    'You should have received a copy of the GNU General Public License',
    'along with this program; if not, write to the Free Software',
    'Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.'
  ]
  j.add ('"copyright": [', 1)
  for l, m in j.iter (copyright):
    j.add ('"%s"' % j.esc (l), more=m)
  j.add ('],', -1)

def generate (name, unshifted, shifted):
  j = json ()
  j.add ('{', 1)
  copyright (j)

  j.add ('"keyboard": {', 1)
  j.add ('"version": 1,')
  j.add ('"name": "%s",' % name)
  j.add ('"keyMap": [', 1)

  for keymap in [unshifted, shifted]:
    j.add ('{', 1)
    if keymap == shifted:
      j.add ('"modifiers": "shift",')
    j.add ('"map": [', 1)
    maps = []
    for i in xrange (0, len (keymap)):
      if keymap[i] != ' ':
        iso = ISO_POSITIONS[i]
        to = j.esc (keymap[i])
        maps += ['{ "iso": "%s", "to": "%s" }' % (iso, to)]
    if len (maps) % 2:
      maps += [None]
    paired = [[maps[i], maps[i + 1]] for i in xrange (0, len (maps), 2)]
    for pair, m in j.iter (paired):
      if pair[1]:
        j.add ('%s, %s' % (pair[0], pair[1]), more=m)
      else:
        j.add ('%s' % pair[0])
    j.add (']', -1)
    j.add ('}', -1, more=(keymap==unshifted))

  j.add (']', -1)
  j.add ('}', -1)
  j.add ('}', -1)

  return j.get ()

def validate (name, unshifted, shifted):
  expected = len (ISO_POSITIONS)
  lu, ls = len (unshifted), len (shifted)
  assert lu == expected, (
    '%s: unshifted error (want %d, got %d)' % (name, expected, lu))
  assert ls == expected, (
    '%s: shifted error (want %d, got %d)' % (name, expected, ls))

  glyphs = sorted ((unshifted + shifted).replace (' ', ''))
  for i in xrange (0, len (glyphs) - 1):
    assert glyphs[i] != glyphs[i + 1], (
      '%s: duplicate entry for "%s"' % (name, glyphs[i]))

def main (argv):
  for entry in KEYMAPS:
    filename, name, unshifted, shifted = entry
    validate (name, unshifted, shifted)
    json = generate (name, unshifted, shifted)

    stream = open (filename + '.json', 'w')
    stream.write (json)
    stream.close ()

    sys.stdout.write ('Wrote %s ("%s")\n' % (filename, name))

if __name__ == '__main__':
  main (sys.argv)
