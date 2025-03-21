# `posix-vfs-fsroot`: Static filesystem root file

This library implements a static singleton file intended to serve as the root of the virtual filesystem.
It is responsible for exactly three things:
- behave as a read-only empty dir when nothing is mounted
- be a mount point for any real root
- ensure, via lookup, that '/' is always its own parent
