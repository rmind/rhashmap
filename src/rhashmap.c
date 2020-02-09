/*
 * Copyright (c) 2017-2020 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Use is subject to license terms, as specified in the LICENSE file.
 */

/*
 * A general purpose hash table, using the Robin Hood hashing algorithm.
 *
 * Conceptually, it is a hash table using linear probing on lookup with
 * a particular displacement strategy on inserts.  The central idea of
 * the Robin Hood hashing algorithm is to reduce the variance of the
 * probe sequence length (PSL).
 *
 * Reference:
 *
 *	Pedro Celis, 1986, Robin Hood Hashing, University of Waterloo
 *	https://cs.uwaterloo.ca/research/tr/1986/CS-86-14.pdf
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>

#include "rhashmap.h"
#include "fastdiv.h"
#include "utils.h"

#define	MAX_GROWTH_STEP		(1024U * 1024)

#define	APPROX_85_PERCENT(x)	(((x) * 870) >> 10)
#define	APPROX_40_PERCENT(x)	(((x) * 409) >> 10)

typedef struct {
	void *		key;
	void *		val;
	uint64_t	hash	: 32;
	uint64_t	psl	: 16;
	uint64_t	len	: 16;
} rh_bucket_t;

struct rhashmap {
	unsigned	size;
	unsigned	nitems;
	unsigned	flags;
	unsigned	minsize;
	uint64_t	divinfo;
	rh_bucket_t *	buckets;
	uint64_t	hashkey;

	/*
	 * Small optimisation for a single element case: allocate one
	 * bucket together with the hashmap structure -- it will generally
	 * fit within the same cache-line.
	 */
	rh_bucket_t	init_bucket;
};

static inline uint32_t __attribute__((always_inline))
compute_hash(const rhashmap_t *hmap, const void *key, const size_t len)
{
	/*
	 * Avoiding the use function pointers here; test and call relying
	 * on branch predictors provides a better performance.
	 */
	if (hmap->flags & RHM_NONCRYPTO) {
		return murmurhash3(key, len, hmap->hashkey);
	}
	return halfsiphash(key, len, hmap->hashkey);
}

static int __attribute__((__unused__))
validate_psl_p(rhashmap_t *hmap, const rh_bucket_t *bucket, unsigned i)
{
	unsigned base_i = fast_rem32(bucket->hash, hmap->size, hmap->divinfo);
	unsigned diff = (base_i > i) ? hmap->size - base_i + i : i - base_i;
	return bucket->key == NULL || diff == bucket->psl;
}

/*
 * rhashmap_get: lookup an value given the key.
 *
 * => If key is present, return its associated value; otherwise NULL.
 */
void *
rhashmap_get(rhashmap_t *hmap, const void *key, size_t len)
{
	const uint32_t hash = compute_hash(hmap, key, len);
	unsigned n = 0, i = fast_rem32(hash, hmap->size, hmap->divinfo);
	rh_bucket_t *bucket;

	ASSERT(key != NULL);
	ASSERT(len != 0);

	/*
	 * Lookup is a linear probe.
	 */
probe:
	bucket = &hmap->buckets[i];
	ASSERT(validate_psl_p(hmap, bucket, i));

	if (bucket->hash == hash && bucket->len == len &&
	    memcmp(bucket->key, key, len) == 0) {
		return bucket->val;
	}

	/*
	 * Stop probing if we hit an empty bucket; also, if we hit a
	 * bucket with PSL lower than the distance from the base location,
	 * then it means that we found the "rich" bucket which should
	 * have been captured, if the key was inserted -- see the central
	 * point of the algorithm in the insertion function.
	 */
	if (!bucket->key || n > bucket->psl) {
		return NULL;
	}
	n++;

	/* Continue to the next bucket. */
	i = fast_rem32(i + 1, hmap->size, hmap->divinfo);
	goto probe;
}

/*
 * rhashmap_insert: internal rhashmap_put(), without the resize.
 */
static void *
rhashmap_insert(rhashmap_t *hmap, const void *key, size_t len, void *val)
{
	const uint32_t hash = compute_hash(hmap, key, len);
	rh_bucket_t *bucket, entry;
	unsigned i;

	ASSERT(key != NULL);
	ASSERT(len != 0);

	/*
	 * Setup the bucket entry.
	 */
	if ((hmap->flags & RHM_NOCOPY) == 0) {
		if ((entry.key = malloc(len)) == NULL) {
			return NULL;
		}
		memcpy(entry.key, key, len);
	} else {
		entry.key = (void *)(uintptr_t)key;
	}
	entry.hash = hash;
	entry.len = len;
	entry.val = val;
	entry.psl = 0;

	/*
	 * From the paper: "when inserting, if a record probes a location
	 * that is already occupied, the record that has traveled longer
	 * in its probe sequence keeps the location, and the other one
	 * continues on its probe sequence" (page 12).
	 *
	 * Basically: if the probe sequence length (PSL) of the element
	 * being inserted is greater than PSL of the element in the bucket,
	 * then swap them and continue.
	 */
	i = fast_rem32(hash, hmap->size, hmap->divinfo);
probe:
	bucket = &hmap->buckets[i];
	if (bucket->key) {
		ASSERT(validate_psl_p(hmap, bucket, i));

		/*
		 * There is a key in the bucket.
		 */
		if (bucket->hash == hash && bucket->len == len &&
		    memcmp(bucket->key, key, len) == 0) {
			/* Duplicate key: return the current value. */
			if ((hmap->flags & RHM_NOCOPY) == 0) {
				free(entry.key);
			}
			return bucket->val;
		}

		/*
		 * We found a "rich" bucket.  Capture its location.
		 */
		if (entry.psl > bucket->psl) {
			rh_bucket_t tmp;

			/*
			 * Place our key-value pair by swapping the "rich"
			 * bucket with our entry.  Copy the structures.
			 */
			tmp = entry;
			entry = *bucket;
			*bucket = tmp;
		}
		entry.psl++;

		/* Continue to the next bucket. */
		ASSERT(validate_psl_p(hmap, bucket, i));
		i = fast_rem32(i + 1, hmap->size, hmap->divinfo);
		goto probe;
	}

	/*
	 * Found a free bucket: insert the entry.
	 */
	*bucket = entry; // copy
	hmap->nitems++;

	ASSERT(validate_psl_p(hmap, bucket, i));
	return val;
}

static int
rhashmap_resize(rhashmap_t *hmap, size_t newsize)
{
	const size_t len = newsize * sizeof(rh_bucket_t);
	rh_bucket_t *oldbuckets = hmap->buckets;
	const size_t oldsize = hmap->size;
	rh_bucket_t *newbuckets;

	ASSERT(newsize > 0);
	ASSERT(newsize > hmap->nitems);

	/*
	 * Check for an overflow and allocate buckets.  Also, generate
	 * a new hash key/seed every time we resize the hash table.
	 */
	if (newsize == 1) {
		memset(&hmap->init_bucket, 0, sizeof(rh_bucket_t));
		newbuckets = &hmap->init_bucket;
	} else if (newsize > UINT_MAX) {
		return -1;
	} else if ((newbuckets = calloc(1, len)) == NULL) {
		return -1;
	}
	hmap->buckets = newbuckets;
	hmap->size = newsize;
	hmap->nitems = 0;

	hmap->divinfo = fast_div32_init(newsize);
	hmap->hashkey ^= random() | (random() << 32);

	for (unsigned i = 0; i < oldsize; i++) {
		const rh_bucket_t *bucket = &oldbuckets[i];

		/* Skip the empty buckets. */
		if (!bucket->key) {
			continue;
		}
		rhashmap_insert(hmap, bucket->key, bucket->len, bucket->val);
		if ((hmap->flags & RHM_NOCOPY) == 0) {
			free(bucket->key);
		}
	}
	if (oldbuckets && oldbuckets != &hmap->init_bucket) {
		free(oldbuckets);
	}
	return 0;
}

/*
 * rhashmap_put: insert a value given the key.
 *
 * => If the key is already present, return its associated value.
 * => Otherwise, on successful insert, return the given value.
 */
void *
rhashmap_put(rhashmap_t *hmap, const void *key, size_t len, void *val)
{
	const size_t threshold = APPROX_85_PERCENT(hmap->size);

	/*
	 * If the load factor is more than the threshold, then resize.
	 */
	if (__predict_false(hmap->nitems > threshold)) {
		/*
		 * Grow the hash table by doubling its size, but with
		 * a limit of MAX_GROWTH_STEP.
		 */
		const size_t grow_limit = hmap->size + MAX_GROWTH_STEP;
		const size_t newsize = MIN(hmap->size << 1, grow_limit);
		if (rhashmap_resize(hmap, newsize) != 0) {
			return NULL;
		}
	}

	return rhashmap_insert(hmap, key, len, val);
}

/*
 * rhashmap_del: remove the given key and return its value.
 *
 * => If key was present, return its associated value; otherwise NULL.
 */
void *
rhashmap_del(rhashmap_t *hmap, const void *key, size_t len)
{
	const size_t threshold = APPROX_40_PERCENT(hmap->size);
	const uint32_t hash = compute_hash(hmap, key, len);
	unsigned n = 0, i = fast_rem32(hash, hmap->size, hmap->divinfo);
	rh_bucket_t *bucket;
	void *val;

	ASSERT(key != NULL);
	ASSERT(len != 0);
probe:
	/*
	 * The same probing logic as in the lookup function.
	 */
	bucket = &hmap->buckets[i];
	if (!bucket->key || n > bucket->psl) {
		return NULL;
	}
	ASSERT(validate_psl_p(hmap, bucket, i));

	if (bucket->hash != hash || bucket->len != len ||
	    memcmp(bucket->key, key, len) != 0) {
		/* Continue to the next bucket. */
		i = fast_rem32(i + 1, hmap->size, hmap->divinfo);
		n++;
		goto probe;
	}

	/*
	 * Free the bucket.
	 */
	if ((hmap->flags & RHM_NOCOPY) == 0) {
		free(bucket->key);
	}
	val = bucket->val;
	hmap->nitems--;

	/*
	 * The probe sequence must be preserved in the deletion case.
	 * Use the backwards-shifting method to maintain low variance.
	 */
	for (;;) {
		rh_bucket_t *nbucket;

		bucket->key = NULL;
		bucket->len = 0;

		i = fast_rem32(i + 1, hmap->size, hmap->divinfo);
		nbucket = &hmap->buckets[i];
		ASSERT(validate_psl_p(hmap, nbucket, i));

		/*
		 * Stop if we reach an empty bucket or hit a key which
		 * is in its base (original) location.
		 */
		if (!nbucket->key || nbucket->psl == 0) {
			break;
		}

		nbucket->psl--;
		*bucket = *nbucket;
		bucket = nbucket;
	}

	/*
	 * If the load factor is less than threshold, then shrink by
	 * halving the size, but not more than the minimum size.
	 */
	if (hmap->nitems > hmap->minsize && hmap->nitems < threshold) {
		size_t newsize = MAX(hmap->size >> 1, hmap->minsize);
		(void)rhashmap_resize(hmap, newsize);
	}
	return val;
}

void *
rhashmap_walk(rhashmap_t *hmap, uintmax_t *iter, size_t *lenp, void **valp)
{
	const unsigned hmap_size = hmap->size;
	unsigned i = *iter;

	while (i < hmap_size) {
		rh_bucket_t *bucket = &hmap->buckets[i];

		i++; // next
		if (!bucket->key) {
			continue;
		}
		*iter = i;
		if (lenp) {
			*lenp = bucket->len;
		}
		if (valp) {
			*valp = bucket->val;
		}
		return bucket->key;
	}
	return NULL;
}

/*
 * rhashmap_create: construct a new hash table.
 *
 * => If size is non-zero, then pre-allocate the given number of buckets;
 * => If size is zero, then a default minimum is used.
 */
rhashmap_t *
rhashmap_create(size_t size, unsigned flags)
{
	rhashmap_t *hmap;

	hmap = calloc(1, sizeof(rhashmap_t));
	if (!hmap) {
		return NULL;
	}
	hmap->flags = flags;
	hmap->minsize = MAX(size, 1);
	if (rhashmap_resize(hmap, hmap->minsize) != 0) {
		free(hmap);
		return NULL;
	}
	ASSERT(hmap->buckets);
	ASSERT(hmap->size);
	return hmap;
}

/*
 * rhashmap_destroy: free the memory used by the hash table.
 *
 * => It is the responsibility of the caller to remove elements if needed.
 */
void
rhashmap_destroy(rhashmap_t *hmap)
{
	if ((hmap->flags & RHM_NOCOPY) == 0) {
		for (unsigned i = 0; i < hmap->size; i++) {
			const rh_bucket_t *bucket = &hmap->buckets[i];

			if (bucket->key) {
				free(bucket->key);
			}
		}
	}
	if (hmap->buckets != &hmap->init_bucket) {
		free(hmap->buckets);
	}
	free(hmap);
}
