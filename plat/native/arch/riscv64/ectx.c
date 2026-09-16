#include <uk/assert.h>
#include <uk/essentials.h>

#include <uk/plat/native/arch/ectx.h>

static inline void uk_plat_native_ectx_assert_valid(
	struct uk_plat_native_ectx *state)
{
	UK_ASSERT(state);
	UK_ASSERT(IS_ALIGNED((__uptr)state, UK_PLAT_NATIVE_ECTX_ALIGN));
}

__isr void uk_plat_native_ectx_sanitize(struct uk_plat_native_ectx *state)
{
	uk_plat_native_ectx_assert_valid(state);
}

__isr void uk_plat_native_ectx_store(struct uk_plat_native_ectx *state)
{
	uk_plat_native_ectx_assert_valid(state);
}

__isr void uk_plat_native_ectx_load(struct uk_plat_native_ectx *state)
{
	uk_plat_native_ectx_assert_valid(state);
}

__isr void uk_plat_native_ectx_init(struct uk_plat_native_ectx *state)
{
	uk_plat_native_ectx_assert_valid(state);
}

__isr void uk_plat_native_ectx_assert_equal(struct uk_plat_native_ectx *state)
{
	uk_plat_native_ectx_assert_valid(state);
}
