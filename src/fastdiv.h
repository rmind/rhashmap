/*-
 * Copyright (c) 2007, 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Joerg Sonnenberger.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_FASTDIV_H_
#define	_FASTDIV_H_

#include <inttypes.h>

#include "utils.h"

/*
 * Fast 32bit division and remainder.
 *
 * Reference:
 *
 *	Torbjörn Granlund and Peter L. Montgomery, "Division by Invariant
 *	Integers Using Multiplication", ACM SIGPLAN Notices, Issue 6, Vol 29,
 *	http://gmplib.org/~tege/divcnst-pldi94.pdf, 61-72, June 1994.
 *
 * The following example computes q = a / b and r = a % b:
 *
 *	uint64_t divinfo = fast_div32_init(b);
 *	q = fast_div32(a, b, divinfo);
 *	r = fast_rem32(a, b, divinfo);
 */

static inline uint64_t
fast_div32_init(uint32_t div)
{
	uint64_t mt;
	uint8_t s1, s2;
	int l;

	l = fls(div - 1);
	mt = (uint64_t)(0x100000000ULL * ((1ULL << l) - div));
	s1 = (l > 1) ? 1U : (uint8_t)l;
	s2 = (l == 0) ? 0 : (uint8_t)(l - 1);
	return (uint64_t)(mt / div + 1) << 32 | (uint32_t)s1 << 8 | s2;
}

static inline uint32_t
fast_div32(uint32_t v, uint32_t div, uint64_t divinfo)
{
	const uint32_t m = divinfo >> 32;
	const unsigned s1 = (divinfo & 0x0000ff00) >> 8;
	const unsigned s2 = (divinfo & 0x000000ff);
	const uint32_t t = (uint32_t)(((uint64_t)v * m) >> 32);
	(void)div; // unused
	return (t + ((v - t) >> s1)) >> s2;
}

static inline uint32_t
fast_rem32(uint32_t v, uint32_t div, uint64_t divinfo)
{
	return v - div * fast_div32(v, div, divinfo);
}

/*
 * Common divisions (compiler will inline the constants and optimise).
 *
 * x = FAST_DIV32_10(1234);
 * y = FAST_REM32_10(1234);
 *
 * Will produce x = 123 and y = 4.
 */

#define	FAST_DIV32_3(x)		fast_div32((x), 3, fast_div32_init(3))

#define	FAST_DIV32_10(x)	fast_div32((x), 10, fast_div32_init(10))
#define	FAST_REM32_10(x)	fast_rem32((x), 10, fast_div32_init(10))

#endif
