/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __POSIX_FUTEX_KEY_H__
#define __POSIX_FUTEX_KEY_H__

#include <uk/config.h>

#if CONFIG_LIBPOSIX_FUTEX_VAS
#include <stdbool.h>
#include <uk/vmem.h>

enum futex_key_kind {
	FUTEX_KEY_NONE,
	FUTEX_KEY_PRIVATE,
	FUTEX_KEY_ANON_SHARED,
	FUTEX_KEY_BACKING
};

struct futex_key {
	enum futex_key_kind kind;
	struct uk_vas *vas;
	__uptr address;
	struct uk_vma_shared_ref shared;
};

/* The caller keeps vas live. These operations never fault or wait for a
 * mapping transition. On failure, outputs are unchanged. Key outputs must
 * be empty on entry to a successful lookup; each success owns one pin.
 */
int futex_key_lookup(struct uk_vas *vas, __u32 *addr, bool private,
		     struct futex_key *out);
int futex_key_read(struct uk_vas *vas, __u32 *addr, bool private,
		   __u32 *value, struct futex_key *out);

/* Copying only acquires a pin; releasing runs outside locks with IRQs on. */
void futex_key_copy(struct futex_key *dst, const struct futex_key *src);
void futex_key_put(struct futex_key *key);
bool futex_key_equal(const struct futex_key *a, const struct futex_key *b);
#endif

#endif /* __POSIX_FUTEX_KEY_H__ */
