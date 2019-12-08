/*
 * SipHash reference C implementation
 *
 * Copyright (c) 2016 Jean-Philippe Aumasson <jeanphilippe.aumasson@gmail.com>
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#include <stdlib.h>
#include <inttypes.h>
#include "utils.h"

#define	cROUNDS		2
#define	dROUNDS		4

#define ROTL(x, b)	(uint32_t)(((x) << (b)) | ((x) >> (32 - (b))))

#define U32TO8_LE(p, v)			\
	(p)[0] = (uint8_t)((v));	\
	(p)[1] = (uint8_t)((v) >> 8);	\
	(p)[2] = (uint8_t)((v) >> 16);	\
	(p)[3] = (uint8_t)((v) >> 24);

#define U8TO32_LE(p)							\
	(((uint32_t)((p)[0])) | ((uint32_t)((p)[1]) << 8) |		\
	 ((uint32_t)((p)[2]) << 16) | ((uint32_t)((p)[3]) << 24))

#define SIPROUND			\
	do {				\
		v0 += v1;		\
		v1 = ROTL(v1, 5);	\
		v1 ^= v0;		\
		v0 = ROTL(v0, 16);	\
		v2 += v3;		\
		v3 = ROTL(v3, 8);	\
		v3 ^= v2;		\
		v0 += v3;		\
		v3 = ROTL(v3, 7);	\
		v3 ^= v0;		\
		v2 += v1;		\
		v1 = ROTL(v1, 13);	\
		v1 ^= v2;		\
		v2 = ROTL(v2, 16);	\
	} while (0)

uint32_t
halfsiphash(const uint8_t *in, const size_t inlen, const uint64_t k)
{
	const uint8_t *end = in + inlen - (inlen % sizeof(uint32_t));
	const unsigned left = inlen & 3;

	uint32_t v0 = 0;
	uint32_t v1 = 0;
	uint32_t v2 = 0x6c796765;
	uint32_t v3 = 0x74656462;
	uint32_t k0 = (uint32_t)k;
	uint32_t k1 = (k >> 32);
	uint32_t m;

	uint32_t b = ((uint32_t)inlen) << 24;

	v3 ^= k1;
	v2 ^= k0;
	v1 ^= k1;
	v0 ^= k0;

	for (; in != end; in += 4) {
		m = U8TO32_LE(in);
		v3 ^= m;
		for (unsigned i = 0; i < cROUNDS; ++i) {
			SIPROUND;
		}
		v0 ^= m;
	}

	switch (left) {
	case 3:
		b |= ((uint32_t)in[2]) << 16;
		// fallthrough
	case 2:
		b |= ((uint32_t)in[1]) << 8;
		// fallthrough
	case 1:
		b |= ((uint32_t)in[0]);
		break;
	case 0:
		break;
	}

	v3 ^= b;

	for (unsigned i = 0; i < cROUNDS; ++i) {
		SIPROUND;
	}

	v0 ^= b;
	v2 ^= 0xff;

	for (unsigned i = 0; i < dROUNDS; ++i) {
		SIPROUND;
	}

	b = v1 ^ v3;

	U32TO8_LE((uint8_t *)&m, b);
	return m;
}
