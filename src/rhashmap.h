/*
 * Copyright (c) 2017 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Use is subject to license terms, as specified in the LICENSE file.
 */

#ifndef _RHASHMAP_H_
#define _RHASHMAP_H_

__BEGIN_DECLS

struct rhashmap;
typedef struct rhashmap rhashmap_t;

#define	RHM_NOCOPY		0x01
#define	RHM_NONCRYPTO		0x02

rhashmap_t *	rhashmap_create(size_t, unsigned);
void		rhashmap_destroy(rhashmap_t *);

void *		rhashmap_get(rhashmap_t *, const void *, size_t);
void *		rhashmap_put(rhashmap_t *, const void *, size_t, void *);
void *		rhashmap_del(rhashmap_t *, const void *, size_t);

#define	RHM_WALK_BEGIN		((uintmax_t)0)

void *		rhashmap_walk(rhashmap_t *, uintmax_t *, size_t *, void **);

__END_DECLS

#endif
