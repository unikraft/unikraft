#ifndef __PLAT_KVM_X86_SMP_H__
#define __PLAT_KVM_X86_SMP_H__

#include <uk/arch/types.h>

__u8 smp_init(void);
void _lcpu_start16(void);

#endif /* __PLAT_KVM_X86_SMP_H__ */
