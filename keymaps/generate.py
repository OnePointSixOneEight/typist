#!/usr/bin/python
# -*- coding: UTF-8 -*-

# Generate JSON keymaps from dense data.

# Approximates: http://unicode.org/reports/tr35/tr35-keyboards.html#Keyboards
# Keyboard rows are B (lowest) to E (highest, usually numbers). A is ignored.
# Keyboard columns are 00..NN numbered from the left. Eg, for standard QWERTY,
# E01 glyph is '1', E10 is '0', D01 is 'Q', C01 is 'A', etc.

import sys

# [(filename, name,
#   unshifted-E00..E15,D00..D15,C00..C15,B00..B15, shifted-...), ...]
# A space character indicates a key that is either absent or unmapped. Space
# itself is never remapped, nor is Return.
KEYMAPS = [
[ 'us', 'US',
    r"`1234567890-=    qwertyuiop[]\   asdfghjkl;'      zxcvbnm,./    ",
    r'~!@#$%^&*()_+    QWERTYUIOP{}|   ASDFGHJKL:"      ZXCVBNM<>?    '],
#     |Row E          |Row D          |Row C          |Row B          |
[ 'uk', 'UK',
    r"`1234567890-=    qwertyuiop[]    asdfghjkl;'#    \zxcvbnm,./    ",
    r'¬!"£$%^&*()_+    QWERTYUIOP{}    ASDFGHJKL:@~    |ZXCVBNM<>?    '],
#     |Row E          |Row D          |Row C          |Row B          |
[ 'dvorak_us', 'US Dvorak',
    r"`1234567890[]    ',.pyfgcrl/=\   aoeuidhtns-      ;qjkxbmwvz    ",
    r'~!@#$%^&*(){}    "<>PYFGCRL?+|   AOEUIDHTNS_      :QJKXBMWVZ    '],
#     |Row E          |Row D          |Row C          |Row B          |
[ 'dvorak_uk', 'UK Dvorak',
    r"`1234567890[]    ',.pyfgcrl/=    aoeuidhtns-#    \;qjkxbmwvz    ",
    r'¬!"£$%^&*(){}    @<>PYFGCRL?+    AOEUIDHTNS_~    |:QJKXBMWVZ    '],
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
[ 'fr', 'French',
    "²&é\"'(-è_çá)=    azertyuiop $    qsdfghjklmù*    <wxcvbn,;:!    ",
    r' 1234567890 +    AZERTYUIOP £    QSDFGHJKLM%µ    >WXCVBN?./§    '],
#     |Row E          |Row D          |Row C          |Row B          |
[ 'de', 'German',
    r" 1234567890ß     qwertzuiopü+    asdfghjklöä#    <yxcvbnm,.-    ",
    "°!\"§$%&/()=?     QWERTZUIOPÜ*    ASDFGHJKLÖÄ'    >YXCVBNM;:_    "],
#     |Row E          |Row D          |Row C          |Row B          |
[ 'es', 'Spanish',
    r"º1234567890'¡    qwertyuiop`+    asdfghjklñ ç    <zxcvbnm,.-    ",
    r'ª!"#$%&/()=?¿    QWERTYUIOP^*    ASDFGHJKLÑ Ç    >ZXCVBNM;:_    '],
#     |Row E          |Row D          |Row C          |Row B          |
[ 'jcuken', 'Russian',
    r"ё1234567890-=\   йцукенгшщзхъ    фывапролджэ      ячсмитьбю.    ",
    r'Ё!"№;%:?*()_+/   ЙЦУКЕНГШЩЗХЪ    ФЫВАПРОЛДЖЭ      ЯЧСМИТЬБЮ,    '],
#     |Row E          |Row D          |Row C          |Row B          |
]

ISO_RANGE = range (0, 16)
ISO_POSITIONS = (
    ['E%02d' % i for i in ISO_RANGE] + ['D%02d' % i for i in ISO_RANGE]
  + ['C%02d' % i for i in ISO_RANGE] + ['B%02d' % i for i in ISO_RANGE])

def json_escape (string):
  escaped = string.replace ('\\', '\\\\')
  return escaped.replace ('"', '\\"')

def generate (name, unshifted, shifted):
  lines = [
    '{',
    '  "_comment_1" : [',
    '    "# Typist 3.0 - improved typing tutor program for UNIX systems",',
    '    "# Copyright (C) 1998-2018  Simon Baldwin (simon_baldwin@yahoo.com)"',
    '  ],',
    '  "keyboard": {',
    '    "version": 1,',
    '    "name": "%s",' % name,
    '    "keyMap": ['
    ]

  for keymap in [unshifted, shifted]:
    lines.append ('      {')
    if keymap == shifted:
      lines.append ('        "modifiers": "shift",')
    lines.append ('        "map": [')
    maps = []
    for i in xrange (0, len (keymap)):
      if keymap[i] != ' ':
        iso = ISO_POSITIONS[i]
        to = json_escape (keymap[i])
        maps.append ('{ "iso": "%s", "to": "%s" }' % (iso, to))
    if len (maps) % 2:
      maps.append (None)
    paired = [[maps[i], maps[i + 1]] for i in xrange (0, len (maps), 2)]
    for pair in paired[:-1]:
      lines.append ('          %s, %s,' % (pair[0], pair[1]))
    pair = paired[-1]
    if pair[1]:
      lines.append ('          %s, %s' % (pair[0], pair[1]))
    else:
      lines.append ('          %s' % pair[0])
    lines.append ('        ]')
    if keymap == unshifted:
      lines.append ('      },')
    else:
      lines.append ('      }')

  lines += [
    '    ]',
    '  }',
    '}'
  ]

  return '\n'.join (lines) + '\n'

def validate (name, unshifted, shifted):
  expected = len (ISO_POSITIONS)

  for keymap in [unshifted, shifted]:
    count = len (keymap)
    assert count == expected, (
        '%s: keymap error (want %d, got %d)' % (name, expected, count))

  found = {}
  for glyph in (unshifted + shifted).replace (' ', ''):
    assert glyph not in found, (
        '%s: duplicate entry for "%s"' % (name, glyph))
    found[glyph] = True

def main (argv):
  for entry in KEYMAPS:
    filename, name, unshifted, shifted = entry
    unshifted = unicode (unshifted, 'utf8')
    shifted = unicode (shifted, 'utf8')
    validate (name, unshifted, shifted)
    json = generate (name, unshifted, shifted)

    stream = open (filename + '.json', 'w')
    stream.write (json.encode ('utf8'))
    stream.close ()

    sys.stdout.write ('Wrote %s ("%s")\n' % (filename, name))

if __name__ == '__main__':
  main (sys.argv)
