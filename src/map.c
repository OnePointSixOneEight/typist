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
#include "map.h"

struct mapelement {
  void *key;                     /* Allocated element key copy */
  struct mapelement *next;       /* Forwards list pointer */
                                 /* Per-element allocated data follows... */
};
typedef struct mapelement mapelement_s;

struct map {
  int span;                      /* Size of contained items, in bytes */
  int key_length;                /* Fixed key length, or 0 for string keys */
  int buckets;                   /* Count of hash buckets */
  mapelement_s **list;           /* Growable array of element list heads */
  int length;                    /* Count of elements in the map */
};

/* Internal implementation.  */
static void *
get_data (const mapelement_s *element)
{
  return (unsigned char*) element + sizeof (*element);
}

static int
keycmp (const map_s *map, const void *key, const void *compare)
{
  if (!map->key_length)
    return strcmp (key, compare);
  else
    return memcmp (key, compare, map->key_length);
}

static mapelement_s *
find (const map_s *map,
      int bucket, const void *key, mapelement_s **previous)
{
  mapelement_s *element, *prev = NULL;
  utils_.fatal_if (bucket < 0 || bucket >= map->buckets,
                   _("internal error: invalid hash bucket"));

  for (element = map->list[bucket];
       element && keycmp (map, element->key, key) != 0;
       element = element->next)
    prev = element;

  if (previous)
    *previous = prev;
  return element;
}

static int
hash_ (const map_s *map, const void *key)
{
  unsigned long hash = 0;
  const char *cursor, *extent, *const char_key = key;

  if (!map->key_length)
    extent = char_key + strlen (key);
  else
    extent = char_key + map->key_length;

  for (cursor = char_key; cursor < extent; cursor++)
    {
      unsigned long g;

      hash = (hash << 4) + *cursor;
      g = hash & 0xf0000000;
      if (g != 0)
        {
          hash = hash ^ (g >> 24);
          hash = hash ^ g;
        }
    }
  return hash % map->buckets;
}

static mapelement_s *
extend (map_s *map, const void *key)
{
  int bucket;
  mapelement_s *element;

  bucket = hash_ (map, key);

  element = find (map, bucket, key, NULL);
  if (!element)
    {
      int length;
      element = utils_.alloc (sizeof (*element) + map->span);

      if (!map->key_length)
        length = strlen (key) + 1;
      else
        length = map->key_length;

      element->key = utils_.alloc (length);
      memcpy (element->key, key, length);

      element->next = map->list[bucket];
      map->list[bucket] = element;

      map->length++;
    }

  return element;
}

static void
expunge (map_s *map, const void *key)
{
  int bucket;
  mapelement_s *element, *previous;

  bucket = hash_ (map, key);

  element = find (map, bucket, key, &previous);
  if (element)
    {
      utils_.free (element->key);

      if (previous)
        previous->next = element->next;
      else
        map->list[bucket] = element->next;

      memset (element, 0, sizeof (*element) + map->span);
      utils_.free (element);

      map->length--;
    }
}

static void
empty (map_s *map)
{
  int bucket;

  for (bucket = 0; bucket < map->buckets; bucket++)
    {
      while (map->list[bucket])
        expunge (map, map->list[bucket][0].key);
    }
}

/* Public interface.  */
static map_s *
create (int span, int key_length, int buckets)
{
  map_s *map;
  int bytes;
  utils_.fatal_if (span < 1 || key_length < 0 || buckets < 1,
                   _("internal error: bad span, key length, or hash buckets"));

  map = utils_.alloc (sizeof (*map));
  map->span = span;
  map->key_length = key_length;
  bytes = sizeof (*map->list) * buckets;
  map->list = utils_.alloc (bytes);
  memset (map->list, 0, bytes);
  map->buckets = buckets;
  map->length = 0;

  return map;
}

static void
destroy (map_s *map)
{
  empty (map);
  utils_.free (map->list);

  memset (map, 0, sizeof (*map));
  utils_.free (map);
}

static int
contains (const map_s *map, const void *key)
{
  return find (map, hash_ (map, key), key, NULL) != NULL;
}

static int
get (const map_s *map, const void *key, void *payload)
{
  mapelement_s *element;

  element = find (map, hash_ (map, key), key, NULL);
  if (element && payload)
    memcpy (payload, get_data (element), map->span);
  return element != NULL;
}

static void
set (map_s *map, const void *key, const void *payload)
{
  mapelement_s *element;

  element = extend (map, key);
  if (payload)
    memcpy (get_data (element), payload, map->span);
}

static void
delete (map_s *map, const void *key)
{
  expunge (map, key);
}

static int
get_length (const map_s *map)
{
  return map->length;
}

static void
clear (map_s *map)
{
  empty (map);
}

struct map_ map_;

__attribute__((constructor))
void
init_map (void)
{
  map_.create = create;
  map_.destroy = destroy;
  map_.contains = contains;
  map_.get = get;
  map_.set = set;
  map_.delete_ = delete;
  map_.get_length = get_length;
  map_.clear = clear;
}
