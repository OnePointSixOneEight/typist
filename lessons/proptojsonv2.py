#!/usr/bin/python

# Convert Jtypist properties files into equivalent JSON representations.

import sys
import zipfile
import StringIO

class options:
  tabstop = 8  # untab to 8-space tabs

MENU_ITEM_TITLES = {
  'Lesson Q1':  r'Lesson Q1     a s d f j k l ; e i',
  'Lesson Q2':  r'Lesson Q2     h g o u n . t',
  'Lesson Q3':  r'Lesson Q3     y r c , ? : p',
  'Lesson Q4':  r'Lesson Q4     m w v z x b q',
  'Lesson Q5':  r"Lesson Q5     ' -",
  'Lesson R1':  r'Lesson R1     Practise',
  'Lesson R2':  r'Lesson R2     Practise',
  'Lesson R3':  r'Lesson R3     Practise',
  'Lesson R4':  r'Lesson R4     Practise',
  'Lesson R5':  r'Lesson R5     Practise',
  'Lesson R6':  r'Lesson R6     Practise',
  'Lesson R7':  r'Lesson R7     Practise',
  'Lesson R8':  r'Lesson R8     Practise',
  'Lesson R9':  r'Lesson R9     Practise',
  'Lesson R10': r'Lesson R10    Practise',
  'Lesson R11': r'Lesson R11    Practise',
  'Lesson R12': r'Lesson R12    Practise',
  'Lesson R13': r'Lesson R13    Practise',
  'Lesson R14': r'Lesson R14    Practise',
  'Lesson T1':  r'Lesson T1     a s d f g h j k l ;',
  'Lesson T2':  r'Lesson T2     e g h ,',
  'Lesson T3':  r'Lesson T3     i r . shift',
  'Lesson T4':  r'Lesson T4     o p shift ?',
  'Lesson T5':  r'Lesson T5     t n :',
  'Lesson T6':  r'Lesson T6     b u /',
  'Lesson T7':  r'Lesson T7     c m -',
  'Lesson T8':  r'Lesson T8     w y v backspace',
  'Lesson T9':  r'Lesson T9     q x z',
  'Lesson T10': r'Lesson T10    1 2 3 4',
  'Lesson T11': r'Lesson T11    7 8 9 0',
  'Lesson T12': r'Lesson T12    5 6 ( )',
  'Lesson T13': r'Lesson T13',
  'Lesson T14':  "Lesson T14    ' \" !",
  'Lesson T15': r'Lesson T15    = * + > < ^',
  'Lesson T16': r'Lesson T16    @ # $ % &',
  'Lesson V1':  r'Lesson V1     R F U J',
  'Lesson V2':  r'Lesson V2     D K E I',
  'Lesson V3':  r'Lesson V3     T Y G H',
  'Lesson V4':  r'Lesson V4     S L W O',
  'Lesson V5':  r'Lesson V5     A ; Q P',
  'Lesson V6':  r'Lesson V6     Shift Keys for Capitalization',
  'Lesson V7':  r'Lesson V7     Shift Lock and :',
  'Lesson V8':  r'Lesson V8     Introducing the Period',
  'Lesson V9':  r'Lesson V9     V and M',
  'Lesson V10': r'Lesson V10    B and N',
  'Lesson V11': r'Lesson V11    C and Comma',
  'Lesson V12': r'Lesson V12    X and .',
  'Lesson V13': r'Lesson V13    Z and /',
  'Lesson V14': r'Lesson V14    The Question Mark',
  'Lesson V15': r'Lesson V15    1, 4, 5, 6, 7',
  'Lesson V16': r'Lesson V16    3 and 8',
  'Lesson V17': r'Lesson V17    2 and 9',
  'Lesson V18': r'Lesson V18    0 and the Hyphen',
  'Lesson V19': r'Lesson V19    Practise',
  'Lesson U1':  r'Lesson U1     Home row',
  'Lesson U2':  r'Lesson U2     Other letters',
  'Lesson U3':  r'Lesson U3     Shift numerals figs',
  'Lesson U4':  r'Lesson U4     Practise',
  'Lesson U5':  r'Lesson U5     Drill on S Combinations',
  'Lesson U6':  r'Lesson U6     Drill on R Combinations',
  'Lesson U7':  r'Lesson U7     Drill on L Combinations',
  'Lesson U8':  r'Lesson U8     Drill on D-T Combinations',
  'Lesson U9':  r'Lesson U9     Drill on M-N Combinations',
  'Lesson U10': r'Lesson U10    Drill on com-con Combinations',
  'Lesson U11': r'Lesson U11    Drill on sion-tion Combinations',
  'Lesson U12': r'Lesson U12    Drill on ter, ther, tor, ture, ster, der',
  'Lesson U13': r'Lesson U13    Drill on qu, ch, wh, dw, sw, tw, de, des, dis, ex,',
  'Lesson U13': r'Lesson U13    self, tran, cial, cious, ology, ship, tive',
  'Lesson D1':  r'Lesson D1     The home row',
  'Lesson D2':  r'Lesson D2     Learning the Shift Key',
  'Lesson D3':  r'Lesson D3     Home Row and the Period',
  'Lesson D4':  r'Lesson D4     Upper Row and Essential Punctuation',
  'Lesson D5':  r'Lesson D5     Review',
  'Lesson D6':  r'Lesson D6     Learning the Lower Row',
  'Lesson D7':  r'Lesson D7     Practise',
  'Lesson D8':  r'Lesson D8     Practise',
  'Lesson D9':  r'Lesson D9     Practise',
  'Lesson D10': r'Lesson D10    Practise',
  'Lesson D11': r'Lesson D11    Practise',
  'Lesson D12': r'Lesson D12    Practise',
  'Lesson D13': r'Lesson D13    Frequent words',
  'Lesson D14': r'Lesson D14    Alphabetic sentences',
  'Lesson C1':  r'Lesson C1     S, T, N and E',
  'Lesson C2':  r'Lesson C2     R and I',
  'Lesson C3':  r'Lesson C2     A and O',
  'Lesson C4':  r'Lesson C2     D and H',
  'Lesson C5':  r'Lesson C2     HOME row exercises',
  'Lesson C6':  r'Lesson C2     P and L',
  'Lesson C7':  r'Lesson C2     F and U',
  'Lesson C8':  r'Lesson C2     W and Y',
  'Lesson C9':  r'Lesson C2     G and J',
  'Lesson C10': r'Lesson C2     M and V',
  'Lesson C11': r'Lesson C2     B and K',
  'Lesson C12': r'Lesson C2     Q and C',
  'Lesson C13': r'Lesson C2     Z and X',
  'Lesson C14': r'Lesson C2     Learning the SHIFT key',
  'Lesson C15': r'Lesson C2     Learning punctuation',
  'Lesson C16': r'Lesson C2     Frequent words',
  'Lesson C17': r'Lesson C2     Alphabetic sentences',
  'Lesson M1':  r'Lesson M1     Practise',
  'Lesson M2':  r'Lesson M2     Practise',
  'Lesson M3':  r'Lesson M3     Practise',
  'Lesson M4':  r'Lesson M4     Practise',
  'Lesson M5':  r'Lesson M5     Practise',
  'Lesson M6':  r'Lesson M6     Practise',
  'Lesson M7':  r'Lesson M7     Practise',
  'Lesson M8':  r'Lesson M8     Practise',
  'Lesson M9':  r'Lesson M9     Practise',
  'Lesson M10': r'Lesson M10    Practise',
  'Lesson M11': r'Lesson M11    Practise',
  'Lesson S1':  r'Lesson S1     Speed tests',
  'Lesson S2':  r'Lesson S2     Speed tests',
  'Lesson S3':  r'Lesson S3     Speed tests',
  'Lesson S4':  r'Lesson S4     Speed tests',
  'Lesson N1':  r'Lesson N1     4 5 . 6 1',
  'Lesson N2':  r'Lesson N2     2 3 0 8 9 7',
  'Lesson N3':  r'Lesson N3     Practise',
  'Lesson N4':  r'Lesson N4     Other numeric keypad keys',
}

class json:
  def __init__ (self):
    self.__buf = []
    self.__indent = 0

  def esc (s):
    def untab (s):
      r = ''
      for c in s:
        if c == '\t':
          if len (r) % options.tabstop == 0: r += ' '
          while (len (r) % options.tabstop): r += ' '
        else:
          r += c
      return r
    return (untab (s).replace('\\', '\\\\')
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

def extract_properties (stream):
  property, properties = None, []
  lines = [line[:-1].strip () for line in stream]
  line_number, continues = 0, False
  for line in lines:
    line_number += 1
    if not line or (line[0] == '#' and not continues):
      continue
    if not property:
      if '=' in line:
        property = line.rstrip ('\\')
      else:
        assert False, 'Line %d: non-definition data' % line_number
    else:
      property += line.rstrip ('\\')
    if not line.endswith ('\\'):
      name, value = property.split ('=', 1)
      # Handle lines that begin with \+whitespace>.
      value = value.replace ('\\ ', '').replace ('\\\t', '')
      properties += [[name, value]]
      property = None
      continues = False
    else:
      continues = True

  return properties

def extract_elements (properties):
  valid = ['title', 'type', 'instructions', 'text']

  stats = {}
  lesson_index, series_id, description = None, None, None
  elements = {}
  for name, value in properties:
    if name == 'series_id':
      series_id = value
      continue
    if name == 'description':
      description = value
      continue
    if name == 'lesson_index':
      lesson_index = value.split ()
      continue

    # Property names look like "Lesson_N4.2.type". Make use of this.
    lesson, attr = name.rsplit ('.', 1)
    assert attr in valid, '%s: invalid attribute' % attr

    # Nest elements as needed for numbers extracted above.
    if lesson not in elements:
      elements[lesson] = {}
    if attr == 'type':
      value = value[0].upper () + value[1:].lower ()
      stats[value] = stats.get (value, 0) + 1
    elements[lesson][attr] = value

  assert lesson_index and series_id and description and elements
  sys.stdout.write ('%s ("%s"): %r\n' % (series_id, description, stats))

  meta = {'lesson_index': lesson_index,
          'series_id': series_id, 'description': description}
  return (meta, elements)

def format_menu (meta, j):
  groups = []
  for lesson in meta['lesson_index']:
    # Indexed lessons look like "Lesson_N4.2"; we want "Lesson_N4".
    group = lesson.split ('.')[0]
    if group not in groups:
      groups += [group]

  title = 'The %s series contains the following ' % meta['series_id']
  if len (groups) == 1:
    title += '1 lesson'
  else:
    title += '%d lessons' % len (groups)

  j.add ('"seriesMenu": {', 1)
  j.add ('"title": "%s",' % title)
  j.add ('"entries": [', 1)

  for group, m1 in j.iter (groups):
    item = group[0].upper () + group[1:].replace ('_', ' ')
    item = j.esc (MENU_ITEM_TITLES.get (item, item))
    j.add ('{', 1)
    j.add ('"title": "%s",' % item)
    j.add ('"lessons": [', 1)

    members = [l for l in meta['lesson_index'] if l.startswith (group + '.')]
    for lesson, m2 in j.iter (members):
      j.add ('"%s"' % lesson, more=m2)
    j.add (']', -1)
    j.add ('}', -1, more=m1)

  j.add (']', -1)
  j.add ('},', -1)

def format_elements (meta, elements):
  valid = ['title', 'type', 'instructions', 'text']

  j = json ()
  j.add ('{', 1)
  copyright (j)

  j.add ('"typist": {', 1)
  j.add ('"version": 2,')
  j.add ('"seriesName": "%s",' % meta['series_id'])
  j.add ('"seriesDescription": "%s",' % meta['description'])

  format_menu (meta, j)

  for lesson, m1 in j.iter (meta['lesson_index']):
    element = elements[lesson]
    assert 'text' in element, 'Missing "text": %r' % element
    j.add ('"%s": {' % lesson, 1)

    # Order attributes explicitly.
    attrs = [a for a in valid if a in element.keys ()]
    for attribute, m2 in j.iter (attrs):
      value = element[attribute]
      # Split on '\\n', as value contains the literal '\n' for newline
      payload = [j.esc (v) for v in (value.split (r'\n'))]
      if payload[-1] == '':
        payload = payload[:-1]
      assert payload, 'Empty payload: %r' % element

      if len (payload) == 1:
        j.add ('"%s": "%s"' % (attribute, payload[0]), more=m2)
      else:
        j.add ('"%s": [' % attribute, 1)
        for value, m3 in j.iter (payload):
          j.add ('"%s"' % value, more=m3)
        j.add (']', -1, more=m2)
    j.add ('}', -1, more=m1)

  j.add ('}', -1)
  j.add ('}', -1)

  return j.get ()

def convert (stream):
  properties = extract_properties (stream)
  meta, elements = extract_elements (properties)
  conversion = format_elements (meta, elements)
  return conversion

def main (argv):
  if len (argv) != 2:
    sys.stderr.write ('Usage: %s <properties_file> | <zip_file>\n' % argv[0])
    return 1

  if argv[1].endswith ('.zip'):
    zip = zipfile.ZipFile (argv[1])
    for entry in zip.namelist ():
      if not entry.endswith ('.properties'):
        continue
      stream = StringIO.StringIO (zip.read (entry))
      conversion = convert (stream)
      stream.close ()

      stream = open (entry.replace ('.properties', '.json'), 'w')
      stream.write (conversion)
      stream.close ()
    zip.close ()
  else:
    stream = open (argv[1])
    conversion = convert (stream)
    stream.close ()

    stream = open (argv[1].replace ('.properties', '.json'), 'w')
    stream.write (conversion)
    stream.close ()

  return 0

if __name__ == '__main__':
  sys.exit (main (sys.argv))
