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

#include "_gettext.h"
#include "utils.h"
#include "vector.h"

struct vector {
  int span;        /* Size of each contained item, in bytes */
  int allocation;  /* Number of elements allocated for in data */
  int length;      /* Count of elements actually used */
  void *data;      /* Allocated growable data space */
};

/* Internal implementation.  */
static void *
get_data (const vector_s *vector, int index_)
{
  return (unsigned char*) vector->data + (index_ * vector->span);
}

static void
extend (vector_s *vector, int elements)
{
  int new_allocation;
  utils_.fatal_if (elements < 1,
                   _("internal error: bad vector elements count"));

  if (elements <= vector->length)
    return;

  for (new_allocation = vector->allocation; new_allocation < elements; )
    new_allocation = new_allocation == 0 ? 1 : new_allocation << 1;

  if (new_allocation > vector->allocation)
    {
      vector->allocation = new_allocation;
      vector->data = utils_.realloc (vector->data,
                                     new_allocation * vector->span);
    }

  vector->length = elements;
}

static void
expunge (vector_s *vector, int index_)
{
  int new_length;
  utils_.fatal_if (index_ > vector->length - 1,
                   _("internal error: bad vector index to expunge"));

  new_length = vector->length - 1;

  if (index_ < new_length)
    {
      unsigned char *addr = get_data (vector, index_);
      memmove (addr, addr + vector->span, (new_length - index_) * vector->span);
    }

  vector->length = new_length;
}

/* Public interface.  */
static vector_s *
create (int span)
{
  vector_s *vector;
  utils_.fatal_if (span < 1, _("internal error: bad vector span"));

  vector = utils_.alloc (sizeof (*vector));
  vector->span = span;
  vector->allocation = vector->length = 0;
  vector->data = NULL;

  return vector;
}

static void
destroy (vector_s *vector)
{
  utils_.free (vector->data);

  memset (vector, 0, sizeof (*vector));
  utils_.free (vector);
}

static void
get (const vector_s *vector, int index_, void *payload)
{
  if (payload)
    memcpy (payload, get_data (vector, index_), vector->span);
}

static void
set (vector_s *vector, int index_, const void *payload)
{
  extend (vector, index_ + 1);
  if (payload)
    memcpy (get_data (vector, index_), payload, vector->span);
}

static void
delete_ (vector_s *vector, int index_)
{
  expunge (vector, index_);
}

static int
get_length (const vector_s *vector)
{
  return vector->length;
}

static void
clear (vector_s *vector)
{
  vector->length = 0;
}

static int
push_back (vector_s *vector, const void *payload)
{
  set (vector, vector->length, payload);
  return vector->length - 1;
}

struct vector_ vector_;

__attribute__((constructor))
void
init_vector (void)
{
  vector_.create = create;
  vector_.destroy = destroy;
  vector_.get = get;
  vector_.set = set;
  vector_.delete_ = delete_;
  vector_.get_length = get_length;
  vector_.clear = clear;
  vector_.push_back = push_back;
}
