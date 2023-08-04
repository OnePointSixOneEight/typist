/* vi: set ts=4 shiftwidth=4 expandtab:

   Typist 3.0 - improved typing tutor program for UNIX systems
   Copyright (C) 1998-2018  Simon Baldwin (simon_baldwin@yahoo.com)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>

#include "_gettext.h"
#include "json.h"
#include "utf8.h"
#include "utils.h"
#include "keymap.h"

struct keymap {
  union {
    struct {
      int normal_keys[64];   /* ISO key ids [B..E][00..15] unshifted.  */
      int shifted_keys[64];  /* ISO key ids [B..E][00..15] shifted.  */
    } pair;
    int keys[64 << 1];       /* Keys as a single array.  */
  } u;
};

static const int keymap_length = sizeof (keymap_s) / sizeof (int);

static void
process_iso_to (keymap_s *keymap,
                const char *iso, const char *to, int shifted)
{
  char row;
  int *ucs_to, ucs, *keys, column, offset;

  if (strlen (iso) != 3 || sscanf (iso, "%c%02d", &row, &column) != 2
      || row < 'B' || row > 'E' || column < 0 || column > 15)
    {
      utils_.error (_("invalid ISO key identifier '%s' (entry ignored)"), iso);
      return;
    }

  ucs_to = utf8_.to_ucs (to);

  if (!ucs_to || !ucs_to[0] || ucs_to[1])
    {
      utils_.error (_("invalid ISO 'to' mapping '%s' (entry ignored)"), to);
      utils_.free (ucs_to);
      return;
    }

  ucs = *ucs_to;
  utf8_.free (ucs_to);

  offset = (int) (row - 'B') * 16 + column;
  keys = shifted ? keymap->u.pair.shifted_keys : keymap->u.pair.normal_keys;

  if (keys[offset])
    utils_.warning (_("duplicate ISO mapping '%s' (shadows prior)"), iso);

  keys[offset] = ucs;
}

static void
process_map (const json_s *json,
             int value, int index_, void *v_keymap, void *v_shifted)
{
  keymap_s *keymap = (keymap_s*)v_keymap;
  int shifted = (int)v_shifted;
  int iso, to;

  if (json_.element_type (json, value) != json_object)
    {
      utils_.error (_("Non-object JSON"
                      " element found in 'map[]', at index %d"), index_);
      return;
    }

  iso = json_.get_element_of_type (json, value, "iso", json_string);
  if (!iso)
    return;

  to = json_.get_element_of_type (json, value, "to", json_string);
  if (!to)
    return;

  process_iso_to (keymap,
                  json_.element_string (json, iso),
                  json_.element_string (json, to), shifted);
}

static int
validate_keymap_core (const keymap_s *keymap)
{
  static const char* const iso_core
    = "`1234567890-=" "qwertyuiop[]\\" "asdfghjkl;'"  "zxcvbnm,./"
      "~!@#$%^&*()_+" "QWERTYUIOP{}|"  "ASDFGHJKL:\"" "ZXCVBNM<>?";
  const char *core;
  int missing = 0;

  for (core = iso_core; *core; core++)
    {
      const int *cursor;
      const int seeking = *core;

      for (cursor = keymap->u.keys;
           cursor < keymap->u.keys + keymap_length; cursor++)
        {
          if (*cursor == seeking)
            break;
        }
      if (cursor == keymap->u.keys + keymap_length)
        {
          utils_.error (_("missing ISO core key map for '%c'"), seeking);
          missing++;
        }
    }

  if (missing)
    utils_.error (_("missing %d core ISO key map(s)"), missing);

  return missing == 0;
}

static void
process_keymap (const json_s *json,
                int value, int index_, void *v_keymap, void *unused)
{
  keymap_s *keymap = (keymap_s*)v_keymap;
  int modifiers, map, shifted;
  unused = unused;

  if (json_.element_type (json, value) != json_object)
    {
      utils_.error (_("Non-object JSON"
                      " element found in 'keyMap[]', at index %d"), index_);
      return;
    }

  modifiers = json_.get_element_of_type (json,
                                         value, "modifiers", json_string);
  if (modifiers)
    {
      if (strcmp (json_.element_string (json, modifiers), "shift") != 0)
        {
          utils_.error (_("only the 'shift'"
                          " modifier is supported (modifier ignored)"));
          return;
        }
      shifted = B.True;
    }
  else
    shifted = B.False;

  map = json_.get_element_of_type (json, value, "map", json_array);
  if (!map)
    return;

  json_.for_each_array_element (json, map,
                                process_map, keymap, (void*)shifted);
}

static keymap_s *
process_layout (const json_s *json, int keymap_obj, int strict_iso_core)
{
  keymap_s *keymap;

  keymap = utils_.alloc (sizeof *keymap);
  memset (keymap, 0, sizeof (*keymap));

  json_.for_each_array_element (json, keymap_obj,
                                process_keymap, keymap, NULL);

  if (strict_iso_core && !validate_keymap_core (keymap))
    {
      utils_.free (keymap);
      keymap = NULL;
    }

  return keymap;
}

static int
requires_utf8 (const keymap_s *keymap)
{
  const int *cursor;

  for (cursor = keymap->u.keys;
       cursor < keymap->u.keys + keymap_length; cursor++)
    {
      if (*cursor > '~' + 1)
        return B.True;
    }

  return B.False;
}

static char *
to_ascii (const keymap_s *keymap)
{
  char *string = utils_.alloc (keymap_length * 10 + 1);
  const int *cursor;
  int i = 0;

  for (cursor = keymap->u.keys;
       cursor < keymap->u.keys + keymap_length; cursor++)
    {
      int c = *cursor ? *cursor : ' ';

      if (c >= ' ' && c <= '~')
        i += sprintf (string + i, "%c", c);
      else
        i += sprintf (string + i, "U+%04X", c);
    }
  string[i] = 0;

  return string;
}

static keymap_s *
create (const char *file_data,
        int file_length, int strict_json, int strict_iso_core)
{
  json_s *json;
  int keyboard, version, keymap_obj;
  keymap_s *keymap;

  json = json_.parse (file_data, file_length, strict_json);
  if (!json)
    return NULL;

  keyboard =
    json_.get_required_element_of_type (json, 0, "top-level",
                                        "keyboard", json_object);
  if (!keyboard)
    {
      json_.destroy (json);
      return NULL;
    }

  version =
    json_.get_required_element_of_type (json, keyboard, "keyboard",
                                        "version", json_primitive);
  if (!version)
    {
      json_.destroy (json);
      return NULL;
    }
  if (strcmp (json_.element_string (json, version), "1") != 0)
    {
      utils_.error (_("'version' is not 1"));
      json_.destroy (json);
      return NULL;
    }

  keymap_obj =
    json_.get_required_element_of_type (json, keyboard, "keyboard",
                                        "keyMap", json_array);
  if (!keymap_obj)
    {
      json_.destroy (json);
      return NULL;
    }

  keymap = process_layout (json, keymap_obj, strict_iso_core);
  json_.destroy (json);

  if (!keymap)
    utils_.error (_("unable to load JSON keymap"));

  if (keymap)
    {
      char *info = to_ascii (keymap);
      utils_.info (_("loaded JSON keymap: %s"), info);
      utils_.free (info);
    }

  return keymap;
}

static keymap_s *
open (const char *path, int strict_json, int strict_iso_core)
{
  keymap_s *keymap;
  char *file_data;
  int file_length;
  FILE *stream;

  stream = fopen (path, "rb");
  if (!stream)
    return NULL;
  utils_.info (_("opened file '%s' on stream p_%p"), path, p_(stream));

  file_length = utils_.read_in_file (stream, &file_data);
  utils_.info (_("read %d bytes from stream p_%p"), file_length, p_(stream));
  fclose (stream);

  if (file_length == 0)
    {
      utils_.error (_("file '%s' contains no data"), path);
      return NULL;
    }

  keymap = create (file_data, file_length, strict_json, strict_iso_core);
  utils_.free (file_data);

  utils_.info (_("open keymap returned p_%p"), p_(keymap));
  return keymap;
}

static keymap_s *
load (const char *name,
      const char *search_path, int strict_json, int strict_iso_core)
{
  char *path;
  keymap_s *keymap = NULL;

  path = utils_.locate_file (search_path, name, "");
  if (!path)
    path = utils_.locate_file (search_path, name, ".json");

  if (path)
    keymap = open (path, strict_json, strict_iso_core);

  utils_.free (path);
  return keymap;
}

static void
close (keymap_s *keymap)
{
  memset (keymap, 0, sizeof (*keymap));
  utils_.free (keymap);
}

struct keymap_ keymap_;

__attribute__((constructor))
void
init_keymap (void)
{
  keymap_.open = open;
  keymap_.load = load;
  keymap_.close = close;
}

struct keymapper {
  keymap_s *from;
  keymap_s *to;
  int attempt_ctrl_keys;
};

static const int *
from_keys (const keymapper_s *keymapper)
{
  return keymapper->from->u.keys;
}

static const int *
to_keys (const keymapper_s *keymapper)
{
  return keymapper->to->u.keys;
}

static void
detect_ambiguity (const keymapper_s *keymapper, int forwards)
{
  const int *from = from_keys (keymapper), *to = to_keys (keymapper);
  const int *cursor, *compare;

  if (!forwards)
    {
      from = to_keys (keymapper);
      to = from_keys (keymapper);
    }

  for (cursor = from; cursor < from + keymap_length; cursor++)
    {
      int conversions = 0;

      if (!*cursor)
        continue;

      for (compare = to; compare < to + keymap_length; compare++)
        {
          if (*compare == *cursor)
            conversions++;
        }

      if (conversions > 1)
        {
          if (forwards)
            utils_.error (_("ambiguous conversion for U+%04X"
                            " (%d entries)"), *cursor, conversions);
          else
            utils_.error (_("ambiguous back-conversion for U+%04X"
                            " (%d entries)"), *cursor, conversions);
        }
    }
}

static keymapper_s *
create_keymapper (keymap_s *from, keymap_s *to, int attempt_ctrl_keys)
{
  keymapper_s *keymapper = utils_.alloc (sizeof (*keymapper));

  keymapper->from = from;
  keymapper->to = to;
  keymapper->attempt_ctrl_keys = attempt_ctrl_keys;

  detect_ambiguity (keymapper, B.True);
  detect_ambiguity (keymapper, B.False);

  utils_.info (_("keymapper p_%p"
                 " maps from p_%p to p_%p"), p_(keymapper), p_(from), p_(to));

  return keymapper;
}

static int
requires_utf8_ (const keymapper_s *keymapper)
{
  return requires_utf8 (keymapper->from) || requires_utf8 (keymapper->to);
}

static int
map_key (const keymapper_s *keymapper, int key, int forwards)
{
  const int *cursor;
  const int *from = from_keys (keymapper), *to = to_keys (keymapper);
  int mapped = 0, represented = B.False;

  if (!forwards)
    {
      from = to_keys (keymapper);
      to = from_keys (keymapper);
    }

  for (cursor = from; cursor < from + keymap_length; cursor++)
    {
      if (!*cursor)
        continue;

      if (*cursor == key)
        {
          const int *mapping = to + (cursor - from);
          if (*mapping)
            mapped = *mapping;
          represented = B.True;
        }
    }

  if (represented && mapped != key)
    {
      if (forwards)
        utils_.info (_("mapped key code U+%04X to U+%04X"), key, mapped);
      else
        utils_.info (_("mapped key code U+%04X back to U+%04X"), key, mapped);
    }

  return represented ? mapped : key;
}

static int
convert_key (const keymapper_s *keymapper, int key, int forwards)
{
  if (!keymapper->from || !keymapper->to)
    return key;

  if (key < 1 || key == ' ' || key == '\n')
    return key;

  if (key < 'z' - 'a' + 1)
    {
      if (keymapper->attempt_ctrl_keys)
        {
          int mapped = map_key (keymapper, key + 'a' - 1, forwards) - 'a' + 1;
          return (mapped > 0 && mapped < 'z' - 'a' + 1) ? mapped : 0;
        }
    }

  if (key < ' ' - 1)
    return key;

  return map_key (keymapper, key, forwards);
}

static int
convert (const keymapper_s *keymapper, int key)
{
  return convert_key (keymapper, key, B.True);
}

static int
unconvert (const keymapper_s *keymapper, int key)
{
  return convert_key (keymapper, key, B.False);
}

static void
destroy_keymapper (keymapper_s *keymapper)
{
  keymap_.close (keymapper->from);
  keymap_.close (keymapper->to);

  memset (keymapper, 0, sizeof (*keymapper));
  utils_.free (keymapper);
}

struct keymapper_ keymapper_;

__attribute__((constructor))
void
init_keymapper (void)
{
  keymapper_.create = create_keymapper;
  keymapper_.requires_utf8 = requires_utf8_;
  keymapper_.convert = convert;
  keymapper_.unconvert = unconvert;
  keymapper_.destroy = destroy_keymapper;
}
