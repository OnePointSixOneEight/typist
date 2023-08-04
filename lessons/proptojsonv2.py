#!/usr/bin/python

# Convert Jtypist properties files into equivalent JSON representations.

import sys
import zipfile
import StringIO

class options:
  tabstop = 8     # untab to 8-space tabs
  condense = True # somewhat reduce the size of the generated JSON

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
  'Lesson V6':  r'Lesson V6     Shift keys for capitalization',
  'Lesson V7':  r'Lesson V7     Shift lock and :',
  'Lesson V8':  r'Lesson V8     Introducing the period',
  'Lesson V9':  r'Lesson V9     V and M',
  'Lesson V10': r'Lesson V10    B and N',
  'Lesson V11': r'Lesson V11    C and comma',
  'Lesson V12': r'Lesson V12    X and .',
  'Lesson V13': r'Lesson V13    Z and /',
  'Lesson V14': r'Lesson V14    The question mark',
  'Lesson V15': r'Lesson V15    1, 4, 5, 6, 7',
  'Lesson V16': r'Lesson V16    3 and 8',
  'Lesson V17': r'Lesson V17    2 and 9',
  'Lesson V18': r'Lesson V18    0 and the hyphen',
  'Lesson V19': r'Lesson V19    Practise',
  'Lesson U1':  r'Lesson U1     Home row',
  'Lesson U2':  r'Lesson U2     Other letters',
  'Lesson U3':  r'Lesson U3     Shift numerals figs',
  'Lesson U4':  r'Lesson U4     Practise',
  'Lesson U5':  r'Lesson U5     Drill on S combinations',
  'Lesson U6':  r'Lesson U6     Drill on R combinations',
  'Lesson U7':  r'Lesson U7     Drill on L combinations',
  'Lesson U8':  r'Lesson U8     Drill on D-T combinations',
  'Lesson U9':  r'Lesson U9     Drill on M-N combinations',
  'Lesson U10': r'Lesson U10    Drill on com-con combinations',
  'Lesson U11': r'Lesson U11    Drill on sion-tion combinations',
  'Lesson U12': r'Lesson U12    Drill on ter, ther, tor, ture, ster, der',
  'Lesson U13': r'Lesson U13    Drill on qu, ch, wh, dw, sw, tw, de, ...',
  'Lesson D1':  r'Lesson D1     The home row',
  'Lesson D2':  r'Lesson D2     Learning the shift key',
  'Lesson D3':  r'Lesson D3     Home row and the period',
  'Lesson D4':  r'Lesson D4     Upper row and essential punctuation',
  'Lesson D5':  r'Lesson D5     Review',
  'Lesson D6':  r'Lesson D6     Learning the lower row',
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
  'Lesson C3':  r'Lesson C3     A and O',
  'Lesson C4':  r'Lesson C4     D and H',
  'Lesson C5':  r'Lesson C5     Home row exercises',
  'Lesson C6':  r'Lesson C6     P and L',
  'Lesson C7':  r'Lesson C7     F and U',
  'Lesson C8':  r'Lesson C8     W and Y',
  'Lesson C9':  r'Lesson C9     G and J',
  'Lesson C10': r'Lesson C10    M and V',
  'Lesson C11': r'Lesson C11    B and K',
  'Lesson C12': r'Lesson C12    Q and C',
  'Lesson C13': r'Lesson C13    Z and X',
  'Lesson C14': r'Lesson C14    Learning the shift key',
  'Lesson C15': r'Lesson C15    Learning punctuation',
  'Lesson C16': r'Lesson C16    Frequent words',
  'Lesson C17': r'Lesson C17    Alphabetic sentences',
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

def json_escape (string):
  untabbed = string.expandtabs (options.tabstop)
  return untabbed.replace('"', '\\"')

def parse_properties (stream):
  definition, continues, properties = None, False, []

  for line in stream:
    line = line.strip ()
    if not line or (line[0] == '#' and not continues):
      continue

    if line.startswith ('\\ ') or line.startswith ('\\\t'):
      line = line.lstrip ('\\')

    if definition:
      definition += line.rstrip ('\\')
    else:
      if '=' in line:
        definition = line.rstrip ('\\')
      else:
        assert False, '%s: not a definition: ' % line

    if line.endswith ('\\'):
      continues = True
    else:
      name, value = definition.split ('=', 1)
      properties.append ([name, value])
      definition, continues = None, False

  return properties

def filter_properties (properties):
  valid = ['title', 'type', 'instructions', 'text']

  counters, context = {}, {}
  lesson_index, series_id, description = None, None, None
  for name, value in properties:
    if name == 'series_id':
      context['series_id'] = value

    elif name == 'description':
      context['description'] = value

    elif name == 'lesson_index':
      context['lesson_index'] = value.split ()

    else:
      # Property names look like "Lesson_N4.2.type". Make use of this.
      lesson, attribute = name.rsplit ('.', 1)
      assert attribute in valid, '%s: invalid attribute' % attribute

      if attribute == 'type':
        value = value[0].upper () + value[1:].lower ()
        counters[value] = counters.get (value, 0) + 1

      context[lesson] = context.get (lesson, {})
      context[lesson][attribute] = value

  assert 'series_id' in context, 'Missing "series_id" property'
  assert 'description' in context, 'Missing "description" property'
  assert 'lesson_index' in context, 'Missing "lesson_index" property'

  sys.stdout.write ('%s ("%s"): %r\n'
                    % (context['series_id'], context['description'], counters))

  return context

def format_menu (context):
  groups = []
  for lesson in context['lesson_index']:
    # Indexed lessons look like "Lesson_N4.2"; we want "Lesson_N4".
    group = lesson.split ('.')[0]
    if group not in groups:
      groups += [group]

  title = 'This series contains the following '
  if len (groups) == 1:
    title += 'lesson'
  else:
    title += 'lessons'

  lines = [
    '    "seriesMenu": {',
    '      "title": "%s",' % title,
    '      "entries": ['
    ]

  for group in groups:
    lines.append ('        {')
    item = group[0].upper () + group[1:].replace ('_', ' ')
    title = json_escape (MENU_ITEM_TITLES.get (item, item))
    lines.append ('          "title": "%s",' % title)

    members = [lesson for lesson in context['lesson_index']
               if lesson.startswith (group + '.')]
    if options.condense:
      lines.append ('          "lessons": ["%s"]' % '", "'.join (members))
    else:
      lines.append ('          "lessons": [')
      for member in members[:-1]:
        lines.append ('            "%s",' % member)
      lines.append ('            "%s"' % members[-1])
      lines.append ('          ]')
    lines.append ('        },')

  lines += [
    '        {',
    '          "title": null',
    '        },',
    '        {',
    '          "title": "Finished      Leave this series",',
    '          "lessons": null',
    '        }',
    '      ]',
    '    },',
    ''
    ]

  return lines

def format_lessons (context):
  valid = ['title', 'type', 'instructions', 'text']
  lines = []

  for lesson in context['lesson_index']:
    element = context[lesson]
    assert 'text' in element, '%r: missing "text"' % element

    lines.append ('    "%s": {' % lesson)

    # Order attributes explicitly.
    attributes = [attribute for attribute in valid
                  if attribute in element.keys ()]
    for attribute in attributes:
      value = element[attribute]
      # Split on '\\n', as value contains the literal '\n' for newline
      payload = [json_escape (line) for line in (value.split (r'\n'))]
      if payload[-1] == '':
        payload = payload[:-1]
      assert payload, '%r: empty payload' % element

      if len (payload) == 1:
        if attribute == attributes[-1]:
          lines.append ('      "%s": "%s"' % (attribute, payload[0]))
        else:
          lines.append ('      "%s": "%s",' % (attribute, payload[0]))
      else:
        lines.append ('      "%s": [' % attribute)
        for value in payload[:-1]:
          lines.append ('        "%s",' % value)
        lines.append ('        "%s"' % payload[-1])
        if attribute == attributes[-1]:
          lines.append ('      ]')
        else:
          lines.append ('      ],')

    lines.append ('    },')
    lines.append ('')

  return lines

def format_json (context):
  lines = [
    '{',
    '  "_comment_1": [',
    '    "# Typist 3.0 - improved typing tutor program for UNIX systems",',
    '    "# Copyright (C) 1998-2018  Simon Baldwin (simon_baldwin@yahoo.com)"',
    '  ],',
    '  "typist": {',
    '    "version": 2,',
    '    "locale": "en_US.UTF-8",',
    '',
    '    "seriesName": "Lesson series %s",' % context['series_id'],
    '    "seriesDescription": "%s",' % context['description'],
    '']

  lines += [
    '    "_comment_2": "# BEGIN MENU DEFINITIONS SECTION",',
    '']
  lines += format_menu (context)
  lines += [
    '    "_comment_3": "# END MENU DEFINITIONS SECTION",',
    '',
    '    "_comment_4": "# BEGIN LESSONS DATA SECTION",',
    '']
  lines += format_lessons (context)
  lines += [
    '    "_comment_5": "# END LESSONS DATA SECTION"'
    ]

  lines += [
    '  }',
    '}']
  return lines

def convert (stream):
  context = filter_properties (parse_properties (stream))
  conversion = format_json (context)
  joined = '\n'.join (conversion) + '\n'

  if options.condense:
    return joined.replace ('lesson_', '')
  else:
    return joined

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
