#ifndef __UK_PLAT_NATIVE_ARCH_ECTX_H__
#define __UK_PLAT_NATIVE_ARCH_ECTX_H__

#include <uk/arch/types.h>
#include <uk/config.h>
#include <uk/essentials.h>

/* RISC-V currently has no platform-managed extended register state. */
#define UK_PLAT_NATIVE_ECTX_SIZE			0
#define UK_PLAT_NATIVE_ECTX_ALIGN			16

#if CONFIG_LIBUKPLAT_NATIVE_ECTX
#define UK_PLAT_NATIVE_ECTX_LOAD_FNSYM					\
	uk_plat_native_ectx_load
#define UK_PLAT_NATIVE_ECTX_STORE_FNSYM					\
	uk_plat_native_ectx_store
#define UK_PLAT_NATIVE_ECTX_SANITIZE_FNSYM				\
	uk_plat_native_ectx_sanitize
#endif /* CONFIG_LIBUKPLAT_NATIVE_ECTX */

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_ECTX
struct uk_plat_native_ectx;

__isr void uk_plat_native_ectx_sanitize(struct uk_plat_native_ectx *state);
__isr void uk_plat_native_ectx_store(struct uk_plat_native_ectx *state);
__isr void uk_plat_native_ectx_load(struct uk_plat_native_ectx *state);
__isr void uk_plat_native_ectx_init(struct uk_plat_native_ectx *state);
__isr void uk_plat_native_ectx_assert_equal(struct uk_plat_native_ectx *state);
#endif /* CONFIG_LIBUKPLAT_NATIVE_ECTX */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_ECTX_H__ */
