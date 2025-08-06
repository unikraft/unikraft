# `posix-vfs-fsroot`: POSIX VFS Static Root File

This sub-library implements a static singleton file intended to serve as the root of the POSIX virtual filesystem.
It is responsible for exactly three things:
- behave as a read-only empty root dir when nothing is mounted
- be a mount point for any real root
- ensure, via lookup, that `/` is always its own parent
