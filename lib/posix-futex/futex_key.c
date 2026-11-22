/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/isr/string.h>

#include "futex_key.h"

static void futex_key_private(struct futex_key *key, struct uk_vas *vas,
			      __u32 *addr, bool private)
{
	key->kind = private ? FUTEX_KEY_PRIVATE : FUTEX_KEY_ANON_SHARED;
	key->vas = uk_vas_get(vas);
	key->address = (__uptr)addr;
}

int futex_key_lookup(struct uk_vas *vas, __u32 *addr, bool private,
		     struct futex_key *out)
{
	struct futex_key key = { 0 };
	int rc;

	UK_ASSERT(vas && out);
	if (!IS_ALIGNED((__uptr)addr, sizeof(*addr)))
		return -EINVAL;
	if (!uk_paging_vaddr_range_isvalid((__vaddr_t)addr, sizeof(*addr)))
		return -EFAULT;
	if (!private) {
		rc = uk_vas_get_shared_ref(vas, (__vaddr_t)addr, sizeof(*addr),
					   &key.shared);
		if (!rc) {
			key.kind = FUTEX_KEY_BACKING;
			*out = key;
			return 0;
		}
		if (rc != -ENOTSUP)
			return rc;
	}
	futex_key_private(&key, vas, addr, private);
	*out = key;
	return 0;
}

int futex_key_read(struct uk_vas *vas, __u32 *addr, bool private,
		   __u32 *value, struct futex_key *out)
{
	struct futex_key key = { 0 };
	int rc;

	UK_ASSERT(vas && value && out);
	if (!uk_paging_vaddr_range_isvalid((__vaddr_t)addr, sizeof(*addr)))
		return -EFAULT;
	if (private)
		rc = uk_vas_load_u32_try(vas, (__vaddr_t)addr, value);
	else
		rc = uk_vas_load_u32_shared_try(vas, (__vaddr_t)addr, value,
						&key.shared);
	if (rc)
		return rc;
	if (key.shared.object)
		key.kind = FUTEX_KEY_BACKING;
	else
		futex_key_private(&key, vas, addr, private);
	*out = key;
	return 0;
}

void futex_key_copy(struct futex_key *dst, const struct futex_key *src)
{
	UK_ASSERT(src->kind != FUTEX_KEY_NONE);
	if (src->shared.object)
		uk_vma_shared_ref_get(&src->shared);
	else
		uk_vas_get(src->vas);
	*dst = *src;
}

void futex_key_put(struct futex_key *key)
{
	uk_vma_shared_ref_put(&key->shared);
	uk_vas_put(key->vas);
	memset_isr(key, 0, sizeof(*key));
}

bool futex_key_equal(const struct futex_key *a, const struct futex_key *b)
{
	if (a->kind == FUTEX_KEY_NONE || a->kind != b->kind)
		return false;
	if (a->kind == FUTEX_KEY_BACKING)
		return a->shared.object == b->shared.object &&
			a->shared.offset == b->shared.offset;
	return a->vas == b->vas && a->address == b->address;
}
