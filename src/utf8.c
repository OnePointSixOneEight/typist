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

#include <ctype.h>
#include <string.h>

#include "utils.h"
#include "utf8.h"

static const int noncharacter = -1;
static const int replacement = 0xfffd;

static int
hex_digit (int ucs, char digit, int *consumed)
{
  static const char *hex = "0123456789abcdef";
  char *v = strchr (hex, tolower (digit));

  if (ucs == noncharacter || !digit || !v)
    return noncharacter;

  *consumed += 1;
  return (ucs << 4) | (v - hex);
}

static int
ucs_escape (const char **string_)
{
  const char *string = *string_;
  int ucs = 0, consumed = 2;

  if (strlen (string) < 6 || string[0] != '\\' || string[1] != 'u')
    return noncharacter;

  ucs = hex_digit (ucs, string[2], &consumed);
  ucs = hex_digit (ucs, string[3], &consumed);
  ucs = hex_digit (ucs, string[4], &consumed);
  ucs = hex_digit (ucs, string[5], &consumed);

  utils_.error_if (ucs == noncharacter,
                   "invalid Unicode escape: '%.6s'", string);

  *string_ += consumed;
  return (!ucs || ucs == noncharacter) ? replacement : ucs;
}

static int
invalid_sequence_byte (char byte)
{
  static const char invalid[64] = "11000000000000000000000000000000"
                                  "00000000000000000000011111111111";
  unsigned char offset = byte;

  return offset > 191 && (invalid[offset - 192] - '0');
}

static int
trailing_byte (int ucs, char byte, int *consumed)
{
  static const int shift = 6, mask = 0x3f;
  static const int byte_mask = 0xc0, byte_check = 0x80;

  if (ucs == noncharacter
      || (byte & byte_mask) != byte_check || invalid_sequence_byte (byte))
    return noncharacter;

  *consumed += 1;
  return (ucs << shift) | (byte & mask);
}

static int
sequence_length (char byte)
{
  static const char lengths[32] = "11111111111111110000000022223340";
  unsigned char offset = byte;

  return lengths[offset >> 3] - '0';
}

static int
shortest_encoding (int ucs, int consumed)
{
  static const int single = 0x80, double_ = 0x800, triple = 0x10000;

  return (ucs < single) ? consumed == 1 : (ucs < double_) ? consumed == 2
       : (ucs < triple) ? consumed == 3 : consumed == 4;
}

static int
ucs_char (const char **string_)
{
  static const int single = 0x7f, sequence = 0x1f;
  const char *string = *string_;
  int ucs, length = sequence_length (string[0]), consumed = 1;
  unsigned char initial = string[0];
  int available = strlen (string);

  if (!length || invalid_sequence_byte (string[0]))
    {
      utils_.error ("invalid UTF-8 sequence start: %02X", initial);
      *string_ += consumed;
      return replacement;
    }

  if (available < length)
    {
      utils_.error ("incomplete UTF-8 sequence, starting: %02X", initial);
      *string_ += available;
      return replacement;
    }

  ucs = string[0] & (length == 1 ? single : sequence >> (length - 2));
  ucs = length > 1 ? trailing_byte (ucs, string[1], &consumed) : ucs;
  ucs = length > 2 ? trailing_byte (ucs, string[2], &consumed) : ucs;
  ucs = length > 3 ? trailing_byte (ucs, string[3], &consumed) : ucs;

  utils_.error_if (ucs == noncharacter,
                   "invalid UTF-8 sequence byte: %02X", string[consumed]);

  if (ucs != noncharacter && !shortest_encoding (ucs, consumed))
    {
      utils_.error ("overlong UTF-8 sequence, starting: %02X", initial);
      ucs = noncharacter;
    }

  *string_ += consumed;
  return (ucs == noncharacter) ? replacement : ucs;
}

static int
ucs_high_surrogate (int ucs)
{
  static const int high_start = 0xd800, high_end = 0xdbff;

  return !(ucs < high_start || ucs > high_end);
}

static int
ucs_low_surrogate (int ucs)
{
  static const int low_start = 0xdc00, low_end = 0xdfff;

  return !(ucs < low_start || ucs > low_end);
}

static int
ucs_surrogate (int high, int low)
{
  static const int high_start = 0xd800, low_start = 0xdc00;
  static const int shift = 10, plane_1 = 0x10000;

  if (high == noncharacter || !ucs_high_surrogate (high))
    return noncharacter;

  if (!ucs_low_surrogate (low))
    {
      utils_.error ("invalid UTF-16 surrogate pair: U+%04X,U+%04X", high, low);
      return replacement;
    }

  return plane_1 | ((high - high_start) << shift) | (low - low_start);
}

static int
filter_low_surrogates (int ucs)
{
  if (ucs == noncharacter)
    return noncharacter;

  if (ucs_low_surrogate (ucs))
    {
      utils_.error ("invalid isolated UTF-16 low surrogate: U+%04X", ucs);
      return replacement;
    }

  return ucs;
}

static int
filter_high_surrogates (int ucs)
{
  if (ucs == noncharacter)
    return noncharacter;

  if (ucs_high_surrogate (ucs))
    {
      utils_.error ("invalid isolated UTF-16 high surrogate: U+%04X", ucs);
      return replacement;
    }

  return ucs;
}

static int *
to_ucs (const char *string)
{
  int *result = utils_.alloc ((strlen (string) + 1) * sizeof (*result));
  int *cursor = result;
  int previous = noncharacter;

  while (*string)
    {
      int surrogate, ucs = ucs_escape (&string);

      if (ucs == noncharacter)
        ucs = ucs_char (&string);

      surrogate = ucs_surrogate (previous, ucs);
      if (surrogate != noncharacter)
        {
          cursor--;
          ucs = surrogate;
        }

      ucs = filter_low_surrogates (ucs);
      ucs = *string ? ucs : filter_high_surrogates (ucs);

      previous = ucs;
      *cursor++ = (ucs == noncharacter) ? replacement : ucs;
    }

  *cursor = 0;
  return result;
}

static int
strlen_ (const char *string)
{
  int *ucs = to_ucs (string);
  int *cursor = ucs;

  while (*cursor)
    cursor++;

  utils_.free (ucs);
  return cursor - ucs;
}

static void
free_ (void *pointer)
{
  utils_.free (pointer);
}

struct utf8_ utf8_;

__attribute__((constructor))
void
init_utf8 (void)
{
  utf8_.to_ucs = to_ucs;
  utf8_.strlen = strlen_;
  utf8_.free = free_;
}
