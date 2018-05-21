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

#include <string.h>

#include "utils.h"
#include "buffer.h"

struct buffer {
  int allocation;  /* Number of bytes allocated for in data */
  int length;      /* Count of bytes actually used */
  void *data;      /* Allocated growable data space */
};

/* Internal implementation.  */
static void
extend (buffer_s *buffer, int extension)
{
  int new_allocation, required;
  utils_.fatal_if (extension < 0,
                   "internal error: bad buffer extension size");

  required = buffer->length + extension;

  for (new_allocation = buffer->allocation; new_allocation < required; )
    new_allocation = new_allocation == 0 ? 1 : new_allocation << 1;

  if (new_allocation > buffer->allocation)
    {
      buffer->allocation = new_allocation;
      buffer->data = utils_.realloc (buffer->data, new_allocation);
    }
}

/* Public interface.  */
static buffer_s *
create (int size)
{
  buffer_s *buffer;
  utils_.fatal_if (size < 0, "internal error: bad buffer initial size");

  buffer = utils_.alloc (sizeof (*buffer));
  buffer->allocation = buffer->length = 0;
  buffer->data = NULL;

  if (size)
    extend (buffer, size);

  return buffer;
}

static void
destroy (buffer_s *buffer)
{
  utils_.free (buffer->data);

  memset (buffer, 0, sizeof (*buffer));
  utils_.free (buffer);
}

static const void *
get (const buffer_s *buffer)
{
  return buffer->data;
}

static void *
export (buffer_s *buffer)
{
  void *data = buffer->data;
  buffer->allocation = buffer->length = 0;
  buffer->data = NULL;
  return data;
}

static int
get_length (const buffer_s *buffer)
{
  return buffer->length;
}

static void
clear (buffer_s *buffer)
{
  buffer->length = 0;
}

static void
append (buffer_s *buffer, const void* const data, int size)
{
  if (size > 0)
    {
      extend (buffer, size);
      memcpy ((unsigned char*) buffer->data + buffer->length, data, size);
      buffer->length += size;
    }
}

__attribute__((constructor))
void
init_buffer (void)
{
  buffer_.create = create;
  buffer_.destroy = destroy;
  buffer_.get = get;
  buffer_.export = export;
  buffer_.get_length = get_length;
  buffer_.clear = clear;
  buffer_.append = append;
}
