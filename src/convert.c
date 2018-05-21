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

#include "buffer.h"
#include "json.h"
#include "utils.h"
#include "convert.h"

static conversion_s *
json_to_legacy (const char *file_data, int file_length, int strict_json)
{
  json_s *json;
  double timer;
  int typist, version, version_;
  buffer_s *buffer;
  conversion_s *conversion;

  timer = utils_.start_timer ();

  json = json_.parse (file_data, file_length, strict_json);
  if (!json)
    return NULL;

  utils_.info ("JSON parse: %f ms", utils_.timer_interval (timer) * 1.0e3);

  typist =
    json_.get_required_element_of_type (json, 0, "top-level",
                                        "typist", json_object);
  if (!typist)
    {
      json_.destroy (json);
      return NULL;
    }

  version =
    json_.get_required_element_of_type (json, typist, "typist",
                                        "version", json_primitive);
  if (!version)
    {
      json_.destroy (json);
      return NULL;
    }
  if (!(strcmp (json_.element_string (json, version), "1") == 0
        || strcmp (json_.element_string (json, version), "2") == 0))
    {
      utils_.error ("'version' is not 1 or 2");
      json_.destroy (json);
      return NULL;
    }

  sscanf (json_.element_string (json, version), "%d", &version_);

  buffer = buffer_.create (0);
  timer = utils_.start_timer ();

  if (version_ == 1)
    convert_.v1_json_to_legacy (json, typist, buffer);

  else if (version_ == 2)
    convert_.v2_json_to_legacy (json, typist, buffer);

  else
    utils_.fatal ("internal error: unexpected 'version' encountered");

  json_.destroy (json);

  utils_.info ("JSON convert: %f ms", utils_.timer_interval (timer) * 1.0e3);

  conversion = utils_.alloc (sizeof (*conversion));
  conversion->version = version_;
  conversion->length = buffer_.get_length (buffer);
  conversion->data = buffer_.export (buffer);
  buffer_.destroy (buffer);

  utils_.info ("JSON converter generated %d bytes of"
               " data, version %d", conversion->length, conversion->version);

  return conversion;
}

__attribute__((constructor))
void
init_convert (void)
{
  convert_.json_to_legacy = json_to_legacy;
}
