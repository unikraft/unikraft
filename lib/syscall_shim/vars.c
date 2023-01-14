#include <uk/syscall.h>

__uk_tls __uptr _uk_syscall_return_addr = 0x0;
#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
__uk_tls __uptr _uk_syscall_ultlsp = 0x0;
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */
