#include <uk/assert.h>

#include <uk/plat/native/arch/sysctx.h>

__isr void uk_plat_native_sysctx_store(struct uk_plat_native_sysctx *sysctx)
{
	UK_ASSERT(sysctx);
	sysctx->tp = uk_plat_native_tlsp_get();
}

__isr void uk_plat_native_sysctx_load(struct uk_plat_native_sysctx *sysctx)
{
	UK_ASSERT(sysctx);
	uk_plat_native_tlsp_set(sysctx->tp);
}
