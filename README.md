# Robin Hood hash map

[![Build Status](https://travis-ci.org/rmind/rhashmap.svg?branch=master)](https://travis-ci.org/rmind/rhashmap)

Robin Hood hash map library -- a general purpose hash table, using open
addressing with linear probing and Robin Hood hashing for the collision
resolution algorithm.  Optimal for solving the _dictionary problem_.
The library provides support for the SipHash and Murmurhash3 algorithms.
The implementation is written in C99 and distributed under the 2-clause
BSD license.

Reference:

	Pedro Celis, 1986, Robin Hood Hashing, University of Waterloo
	https://cs.uwaterloo.ca/research/tr/1986/CS-86-14.pdf

## API

* `rhashmap_t *rhashmap_create(size_t size, unsigned flags)`
  * Construct a new hash map.  If `size` is not zero, then the hash map
  will be pre-allocated with, at least, the given size as a minimum;
  otherwise, a default size will be used.  Certain hash map behaviour can
  be specified using any of the following optional `flags`:
    * `RHM_NOCOPY`: the keys on insert will not be copied and the given
    pointers to them will be expected to be valid and the values constant
    until the key is deleted; by default, the put operation will make a
    copy of the key.
    * `RHM_NONCRYPTO`: a non-cryptographic hash function will be used to
    provide higher performance.  By default, half SipHash-2-4 is used to
    defend against the hash-flooding DoS attacks.  With this flag set,
    the hash function will be switched to the MurmurHash3 algorithm.

* `void rhashmap_destroy(rhashmap_t *hmap)`
  * Destroy the hash map, freeing the memory it uses.

* `void *rhashmap_get(rhashmap_t *hmap, const void *key, size_t len)`
  * Lookup the key (of a given length) and return the value associated with it.
  Return `NULL` if the key is not found (see the caveats section).

* `void *rhashmap_put(rhashmap_t *hmap, const void *key, size_t len, void *val)`
  * Insert the key with an arbitrary value.  If the key is already present,
  return the already existing associated value without changing it.
  Otherwise, on a successful insert, return the given value.  Just compare
  the result against `val` to test whether the insert was successful.

* `void *rhashmap_del(rhashmap_t *hmap, const void *key, size_t len)`
  * Remove the given key.  If the key was present, return the associated
  value; otherwise return `NULL`.

## Caveats

* The hash table will grow when it reaches ~85% fill and will shrink when
the fill is below ~40%.

* The key sizes greater than 64 KB are not supported.  The hash map supports
up to `UINT_MAX` elements, which is, on any modern CPU architecture, more than
4 billion elements.  These limits are expected to be enough for all practical
use cases, while allowing this implementation to use less memory.

* While the `NULL` values may be inserted, `rhashmap_get` and `rhashmap_del`
cannot indicate whether the key was not found or a key with a NULL value
was found.  If the caller needs to indicate an "empty" value, it can use a
special pointer value, such as `(void *)(uintptr_t)0x1`.

## Performance

With small to medium key sizes, Robin Hood hash map scores above Judy
array (JudyHS) and Google Sparse hash map on lookup performance benchmarks.
With the very small key sizes, it demonstrates similar performance to JudyHS.

Disclaimer: benchmark results, however, depend on many aspects (workload,
hardware characteristics, methodology, etc).  Ultimately, readers are
encouraged to perform their own benchmarks.

## Example

An illustrative code fragment:
```c
#include <rhashmap.h>

rhashmap_t *kvmap;
struct obj *obj;

kvmap = rhashmap_create(0, 0);
assert(kvmap != NULL);
...
obj = obj_create();
rhashmap_put(kvmap, "test", sizeof("test") - 1, obj);
...
obj = rhashmap_get(kvmap, "test", sizeof("test") - 1);
...
rhashmap_destroy(kvmap);
```

## Packages

Just build the package, install it and link the library using the
`-lrhashmap` flag.
* RPM (tested on RHEL/CentOS 7): `cd pkg && make rpm`
* DEB (tested on Debian 9): `cd pkg && make deb`
