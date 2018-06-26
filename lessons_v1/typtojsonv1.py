#!/usr/bin/python

# Convert typist script file into an equivalent JSON representation.

import sys
import zipfile
import StringIO

class options:
  compact = True  # compact JSON representation of some actions
  coalesce_actions = True  # coalesce multiple label and keybinds into one
  avoid_single_element_arrays = True  # change ['x'] data to 'x'
  tabstop = 8  # untab to 8-space tabs

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

def make_verbose (action):
  verbose_actions = {'*': 'Label',
                     'T': 'Tutorial',
                     'I': 'Instruction',
                     'B': 'Clear_screen',
                     'G': 'Goto',
                     'Q': 'Query',
                     'N': 'If_no_goto',
                     'Y': 'If_yes_goto',
                     'D': 'Drill',
                     'd': 'Drill_practice',
                     'S': 'Speed_test',
                     's': 'Speed_test_practice',
                     'K': 'Bind_function_key',
                     'E': 'Set_error_limit',
                     'F': 'On_failure_goto',
                     'M': 'Menu',
                     'X': 'Exit'}
  return verbose_actions[action]

def format_compact (verbose, data, j, more):
  if options.avoid_single_element_arrays and len (data) == 1:
    e = j.esc (data[0])
    j.add ('{ "action": "%s", "data": "%s" }' % (verbose, e), more=more)
    return
  s = '", "'.join ([j.esc (d) for d in data])
  j.add ('{ "action": "%s", "data": [ "%s" ] }' % (verbose, s), more=more)

def format_full (verbose, data, j, more):
  j.add ('{', 1)
  j.add ('"action": "%s",' % verbose)
  if options.avoid_single_element_arrays and len (data) == 1:
    j.add ('"data": "%s"' % (j.esc (data[0])))
  else:
    j.add ('"data": [', 1)
    for datum, m in j.iter (data):
      j.add ('"%s"' % j.esc (datum), more=m)
    j.add (']', -1)
  j.add ('}', -1, more=more)

def format (action, data, j, more=True):
  verbose = make_verbose (action)
  if not data:
    j.add ('{ "action": "%s" }' % verbose, more=more)
    return
  if options.compact and action in ['*', 'G', 'Y', 'N', 'Q', 'K', 'X']:
    format_compact (verbose, data, j, more)
  else:
    format_full (verbose, data, j, more)

def validate_and_sanitize (action, data, line_number):
  if action == 'I':
    assert len (data) <= 2, 'Line %d: oversized instruction' % line_number
    assert len (data) > 0, 'Line %d: undersized instruction' % line_number
  if action in ['T', 'I', 'D', 'D', 'd', 'S', 's'
                '*', 'G', 'Y', 'N', 'Q', 'K', 'M', 'E', 'F']:
    assert data, 'Line %d, no data found' % line_number
  if action in ['G', 'Y', 'N', 'Q', 'E', 'F']:
    assert len (data) == 1, 'Line %d: oddly sized statement' % line_number
  if action == 'X':
    assert data == [""], 'Line %d, data found on X action' % line_number
    data.pop ()
  if action == 'K':
    for element in data:
      fkey, label = element.split (':')
      assert int(fkey) in range (1, 13), 'Line %d, bad bind' % line_number

def convert (stream, name):
  stats = {}

  j = json ()
  j.add ('{', 1)
  copyright (j)

  j.add ('"typist": {', 1)
  j.add ('"version": 1,')
  j.add ('"locale": "en_US.UTF-8",')
  j.add ('"name": "%s",' % name)
  j.add ('"statements": [', 1)

  action, data = None, None
  lines = [line[:-1] for line in stream]
  line_number = 0
  for line in lines:
    line_number += 1
    if not line or line[0] in ['#', '!']:
      continue
    assert line[1] == ':', 'Line %d: missing separator' % line_number

    if action is None:
      action, data = line[0].upper (), [line[2:]]
      continue

    if action not in ['*', 'T', 'I', 'B', 'G', 'Q', 'N', 'Y',
                      'D', 'd', 'S', 's', 'K', 'E', 'F', 'M', 'X']:
      assert False, 'Line %d: invalid action' % line_number
    if action not in ['T', 'I', 'D', 'd', 'S', 's', 'M']:
      assert line[0] != ' ', 'Line %d: unexpected continuation' % line_number

    if action in ['T', 'I', 'D', 'd', 'S', 's', 'M'] and line[0] == ' ':
      data += [line[2:]]
      continue

    if options.coalesce_actions:
      if action in ['*', 'G', 'Y', 'N', 'Q', 'K', 'X']:
        if line[0].upper () in [action, ' ']:
          data += [line[2:]]
          continue

    validate_and_sanitize (action, data, line_number)
    format (action, data, j, more=True)
    stats[action] = stats.get (action, 0) + 1
    action, data = line[0].upper (), [line[2:]]

  validate_and_sanitize (action, data, line_number)
  format (action, data, j, more=False)
  stats[action] = stats.get (action, 0) + 1

  j.add (']', -1)
  j.add ('}', -1)
  j.add ('}', -1)

  sys.stdout.write ('%s: %r\n' % (name, stats))
  return j.get ()

def main (argv):
  if len (argv) != 2:
    sys.stderr.write ('Usage: %s <typ_file> | <zip_file>\n' % argv[0])
    return 1

  def gen_name (string):
    if len (string) == 1:
      return 'Series %s' % string.upper ()
    else:
      return string[0].upper () + string[1:]

  if argv[1].endswith ('.zip'):
    zip = zipfile.ZipFile (argv[1])
    for entry in zip.namelist ():
      if not entry.endswith ('.typ'):
        continue
      name = gen_name (entry.replace ('.typ', ''))
      stream = StringIO.StringIO (zip.read (entry))
      conversion = convert (stream, name)
      stream.close ()

      if len (entry) == 5:
        entry = 'series_' + entry
      stream = open (entry.replace ('.typ', '.json'), 'w')
      stream.write (conversion)
      stream.close ()
    zip.close ()
  else:
    stream = open (argv[1])
    name = gen_name (argv[1].split ('/')[-1].replace ('.typ', ''))
    conversion = convert (stream, name)
    stream.close ()

    base = argv[1]
    if len (base) == 5:
      base = 'series_' + base
    stream = open (base.replace ('.typ', '.json'), 'w')
    stream.write (conversion)
    stream.close ()

  return 0

if __name__ == '__main__':
  sys.exit (main (sys.argv))
