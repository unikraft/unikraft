#ifndef __BOOTINFO_H__
#define __BOOTINFO_H__

#include <uk/arch/types.h>
#include <stddef.h>
#include <stdint.h>

struct uk_bootinfo {
	/* Command line information */
	__u64 u64_cmdline;

	/* Memmory mapping information */
	size_t max_addr;

	/* Initrd information */
	__u8 has_initrd;
	uintptr_t initrd_start;
	uintptr_t initrd_end;
	size_t initrd_length;
};

#endif /* __BOOTINFO_H__ */
