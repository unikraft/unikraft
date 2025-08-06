# `posix-fdio`: POSIX File I/O, Metadata & Control for Unikraft

This library implements core POSIX file operations on top of open file descriptions.
These are:
- I/O: `read` & `write` family, `lseek`, `sendfile`
- Metadata: `fstat`, `fchmod`, `futime(n)s`, etc.
- Control: `fcntl`, `ioctl`, `ftruncate`, etc.
- Driver-agnostic behavior of file memory mappings

These operations are exported under the Unikraft-internal `uk_sys_*` API in `uk/posix-fdio.h`.
If `posix-fdtab` is enabled, the equivalent file-descriptor-based Linux-compatible syscalls / libc functions are also provided.
