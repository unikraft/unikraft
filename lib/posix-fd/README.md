# `posix-fd`: POSIX Open File Descriptions for Unikraft

This core library provides the Unikraft abstraction for POSIX open file descriptions -- references to a file that is "in use", along with any state required to correctly implement POSIX file operations.

Not to be confused with POSIX file descrip**tor**s, which are handled by `posix-fdtab`.

Consult the main `uk/posix-fd.h` header for API & implementation details.

## What is an Open File Description?

An open file description consists of:
- A counted reference to the open file
- An optional name for the open file
- Mutable state:
  - Reference count, allowing multiple independent shared references
  - Open file "mode", a bitmask of options the file is opened with, that affects the behavior of POSIX file operations
  - Current position for I/O (i.e. what one sets with `lseek()`)
  - Lock for synchronizing changes to the above

Open files are represented in Unikraft by `struct uk_ofile`, with fields storing the above.
A single `ukfile` may be referenced by an arbitrary number of open file descriptions, each of which acts independently.

Open file descriptions do not expose any operations themselves beyond lifetime management, with the following libraries responsible for typical POSIX file operations:
- `posix-fdio`: I/O, metadata, control
- `posix-vfs`: VFS operations
- `posix-mmap`: memory mapping
- `posix-socket`: socket operations
