/*
 * Copyright (c) 2017 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Use is subject to license terms, as specified in the LICENSE file.
 */

#ifndef	_UTILS_H_
#define	_UTILS_H_

#include <stddef.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>

/*
 * A regular assert (debug/diagnostic only).
 */

#if defined(DEBUG)
#define	ASSERT		assert
#else
#define	ASSERT(x)
#endif

/*
 * Branch prediction macros.
 */

#ifndef __predict_true
#define	__predict_true(x)	__builtin_expect((x) != 0, 1)
#define	__predict_false(x)	__builtin_expect((x) != 0, 0)
#endif

/*
 * Minimum, maximum and rounding macros.
 */

#ifndef MIN
#define	MIN(x, y)	((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define	MAX(x, y)	((x) > (y) ? (x) : (y))
#endif

/*
 * DSO visibility attributes (for ELF targets).
 */

#if defined(__GNUC__) && __GNUC__ >= (4)
#define	__dso_hidden	__attribute__((__visibility__("hidden")))
#else
#define	__dso_hidden
#endif

/*
 * Find first bit.
 */

#ifndef fls
static inline int
fls(int x)
{
	return x ? (sizeof(int) * CHAR_BIT) - __builtin_clz(x) : 0;
}
#endif

uint32_t	murmurhash3(const void *, size_t, uint32_t) __dso_hidden;
uint32_t	halfsiphash(const uint8_t *, const size_t, const uint64_t) __dso_hidden;

#endif
