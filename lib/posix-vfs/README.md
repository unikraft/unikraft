# `posix-vfs`: POSIX Virtual Filesystem for Unikraft

This library implements a POSIX-compatible virtual filesystem in Unikraft on top of the ukfs API.
This includes all operations working with paths, as well as with VFS state more generally.

Filesystem operations are exported under the Unikraft-internal `uk_sys_*` API.
If filesystem syscalls are enabled, Linux-compatible syscall / libc equivalents are also exported.
This will have `posix-vfs` take over filesystem duties from `vfscore` entirely, requiring the latter to be disabled.

Consult the `uk/posix-vfs.h` header for more details.
