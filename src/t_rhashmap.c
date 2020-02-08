/*
 * Copyright (c) 2017 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Use is subject to license terms, as specified in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "rhashmap.h"

#define	NUM2PTR(x)	((void *)(uintptr_t)(x))

static void
test_basic(void)
{
	rhashmap_t *hmap;
	void *ret;

	hmap = rhashmap_create(0, 0);
	assert(hmap != NULL);

	ret = rhashmap_get(hmap, "test", 4);
	assert(ret == NULL);

	ret = rhashmap_put(hmap, "test", 4, NUM2PTR(0x55));
	assert(ret == NUM2PTR(0x55));

	ret = rhashmap_get(hmap, "test", 4);
	assert(ret == NUM2PTR(0x55));

	ret = rhashmap_del(hmap, "test", 4);
	assert(ret == NUM2PTR(0x55));

	ret = rhashmap_get(hmap, "test", 4);
	assert(ret == NULL);

	rhashmap_destroy(hmap);
}

static void
test_large(void)
{
	const unsigned nitems = 1024 * 1024;
	rhashmap_t *hmap;
	void *ret;

	hmap = rhashmap_create(0, 0);
	assert(hmap != NULL);

	for (unsigned i = 0; i < nitems; i++) {
		ret = rhashmap_put(hmap, &i, sizeof(int), NUM2PTR(i));
		assert(ret == NUM2PTR(i));

		ret = rhashmap_get(hmap, &i, sizeof(int));
		assert(ret == NUM2PTR(i));
	}

	for (unsigned i = 0; i < nitems; i++) {
		ret = rhashmap_get(hmap, &i, sizeof(int));
		assert(ret == NUM2PTR(i));
	}

	for (unsigned i = 0; i < nitems; i++) {
		ret = rhashmap_del(hmap, &i, sizeof(int));
		assert(ret == NUM2PTR(i));

		ret = rhashmap_get(hmap, &i, sizeof(int));
		assert(ret == NULL);
	}

	rhashmap_destroy(hmap);
}

static void
test_delete(void)
{
	const unsigned nitems = 300;
	rhashmap_t *hmap;
	uint64_t *keys;
	void *ret;

	hmap = rhashmap_create(0, 0);
	assert(hmap != NULL);

	keys = calloc(nitems, sizeof(uint64_t));
	assert(keys != NULL);

	for (unsigned i = 0; i < nitems; i++) {
		keys[i] = random() + 1;
		ret = rhashmap_put(hmap, &keys[i], sizeof(uint64_t), NUM2PTR(i));
		assert(ret == NUM2PTR(i));
	}

	for (unsigned i = 0; i < nitems; i++) {
		/* Delete a key. */
		ret = rhashmap_del(hmap, &keys[i], sizeof(uint64_t));
		assert(ret == NUM2PTR(i));

		/* Check the remaining keys. */
		for (unsigned j = i + 1; j < nitems; j++) {
			ret = rhashmap_get(hmap, &keys[j], sizeof(uint64_t));
			assert(ret == NUM2PTR(j));
		}
	}
	rhashmap_destroy(hmap);
	free(keys);
}

static void *
generate_unique_key(unsigned idx, int *rlen)
{
	const unsigned rndlen = random() % 32;
	const unsigned len = rndlen + sizeof(idx);
	unsigned char *key = malloc(len);

	for (unsigned i = 0; i < rndlen; i++) {
		key[i] = random() % 0xff;
	}
	memcpy(&key[rndlen], &idx, sizeof(idx));
	*rlen = len;
	return key;
}

#define	KEY_MAGIC_VAL(k)	NUM2PTR(((unsigned char *)(k))[0] ^ 0x55)

static void
test_random(void)
{
	const unsigned nitems = 300;
	unsigned n = 10 * 1000 * 1000;
	rhashmap_t *hmap;
	unsigned char **keys;
	int *lens;

	hmap = rhashmap_create(0, 0);
	assert(hmap != NULL);

	keys = calloc(nitems, sizeof(unsigned char *));
	assert(keys != NULL);

	lens = calloc(nitems, sizeof(int));
	assert(lens != NULL);

	while (n--) {
		const unsigned i = random() % nitems;
		void *val = keys[i] ? KEY_MAGIC_VAL(keys[i]) : NULL;
		void *ret;

		switch (random() % 3) {
		case 0:
			/* Create a unique random key. */
			if (keys[i] == NULL) {
				keys[i] = generate_unique_key(i, &lens[i]);
				val = KEY_MAGIC_VAL(keys[i]);
				ret = rhashmap_put(hmap, keys[i], lens[i], val);
				assert(ret == val);
			}
			break;
		case 1:
			/* Lookup a key. */
			if (keys[i]) {
				ret = rhashmap_get(hmap, keys[i], lens[i]);
				assert(ret == val);
			}
			break;
		case 2:
			/* Delete a key. */
			if (keys[i]) {
				ret = rhashmap_del(hmap, keys[i], lens[i]);
				assert(ret == val);
				free(keys[i]);
				keys[i] = NULL;
			}
			break;
		}
	}

	for (unsigned i = 0; i < nitems; i++) {
		if (keys[i]) {
			void *ret = rhashmap_del(hmap, keys[i], lens[i]);
			assert(KEY_MAGIC_VAL(keys[i]) == ret);
			free(keys[i]);
		}
	}

	rhashmap_destroy(hmap);
	free(keys);
	free(lens);
}

static void
test_walk(void)
{
	const unsigned nitems = 17; // prime
	rhashmap_t *hmap;
	void *ret, *key, *val;
	uint32_t bitmap = 0;
	uintmax_t iter;
	size_t klen;

	hmap = rhashmap_create(0, 0);
	assert(hmap != NULL);

	iter = RHM_WALK_BEGIN;
	key = rhashmap_walk(hmap, &iter, &klen, &val);
	assert(key == NULL);

	for (unsigned i = 0; i < nitems; i++) {
		ret = rhashmap_put(hmap, &i, sizeof(int), NUM2PTR(i));
		assert(ret == NUM2PTR(i));
	}
	iter = RHM_WALK_BEGIN;
	while ((key = rhashmap_walk(hmap, &iter, &klen, &val)) != NULL) {
		bitmap |= 1U << (uintptr_t)val;
	}
	assert(bitmap == 0x1ffff);

	rhashmap_destroy(hmap);
}

int
main(void)
{
	test_basic();
	test_large();
	test_delete();
	test_random();
	test_walk();
	puts("ok");
	return 0;
}
