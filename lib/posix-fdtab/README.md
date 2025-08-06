# `posix-fdtab`: POSIX File Descriptors & their Tables for Unikraft

This library implements POSIX file descriptor tables (fdtabs), providing management of file descriptors.

A file descriptor (fd) is a non-negative integer that is used to reference an open file description from userspace.
This contextual mapping of integer-to-file-description is managed by an fdtab, which provides:
- Kernel API:
  - `open*`: associate an open file description with a free fd
  - `get`: retrieve open file description associated with an fd
  - `[get|set]flags`: manipulate file descriptor flags (`O_CLOEXEC`)
- Userspace API:
  - `close()`: close an fd, releasing its open file description
  - `dup*()`: associate a free fd with a duplicate reference to the same open file description as another fd

Both kernel and userspace APIs are exported under the Unikraft-internal `uk_sys_*` namespace.
The userspace API is additionally exported as Linux-compatible syscalls / libc functions, depending on configuration.

Consult the main `uk/posix-fdtab.h` header for API details.
