#include <uk/config.h>

.globl vfscore_einitrd_start
.globl vfscore_einitrd_end

.section .rodata

.balign 16
vfscore_einitrd_start:
.incbin CONFIG_LIBVFSCORE_AUTOMOUNT_EINITRD_PATH
.balign 1
vfscore_einitrd_end:
.byte 0
