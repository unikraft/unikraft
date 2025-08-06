# `ukfile`: Files for Unikraft

This core library provides the Unikraft abstraction of a "file".
`ukfile` is a low-level kernel API that decouples file _drivers_ -- implementers of the API -- from file _consumers_ -- callers of the API.

This README provides a high-level overview, consult the public header files for more details:
- `uk/file.h` -- main file API
- `uk/file/iovutil.h` -- utilities for working with buffers in `struct iovec`s
- `uk/file/nops.h` -- no-op file operations, provided as convenience for drivers

## What is a File?

To overuse a classic *NIX idiom, "everything is a file".
More concretely however, a file is an abstraction for any resource that offers a combination of I/O, and/or control operations.

### What a File is not

Although tightly related to files, the following fall outside the scope of `ukfile` and are handled by their own core libraries:
- `posix-fd`: open file descriptions -- state linked to open instances of files
- `posix-fdtab`: file descriptors (`int fd`) & their management
- `posix-fdio`: POSIX file operations -- `read`, `write`, `fcntl`, etc.
- `ukfs`: filesystem API -- additional operations for filesystem-backed files
- `posix-vfs`: POSIX virtual filesystem (VFS) layer

### File Identity & State

Files are represented in Unikraft by `struct uk_file`, whose fields, once initialized, form the file's identity and must never be changed over the lifetime of the file.

**`ukfile` drivers and consumers should always return and accept `const struct uk_file *` to enforce immutability.**

File identity consists of:
- Driver-private fields for volume and node identifier
- File operation table(s): implementations of well-defined operations (see below)
- References to mutable public file state

Public file state is used for bookkeeping purposes and includes:
- Reference counting (strong & weak references)
- Locks for synchronization
- Polling mechanism & queue

### File Operations

Files allow for a defined set of operations, some of which are driver-implemented, while others are common across all files.

Driver-specific operations have a well-defined interface and are implemented by file drivers.
These are:
- Traditional I/O: manipulating file contents as an array of unstructured bytes
  - `read`: retrieve a specific contiguous block of bytes from this array
  - `write`: ensure a specific contiguous block in this array has specific bytes
- Direct Memory I/O: unmediated access to a file's backing memory
  - `mem`: reserve, retrieve, and manage the memory backing a file's contents
- Metadata: manipulating a defined structure of metadata related to the file
  - `getstat`
  - `setstat`
- Control: requests for special operations to be performed by the file
  - `ctl`
- (internal) cleanup/destructor: what happens, if anything, when we no longer need the file
- (optional) Filesystem Operations: see `ukfs` for details

Common operations are implemented centrally for all file objects:
- Reference counting: acquire/release of regular (strong) or weak references
  - Strong references allow the full functionality of files
  - Weak references allow only common operations (polling, locking)
- Event polling & notification:
  - Driver API:
    - Managed Events: Drivers set & clear what events are active on the file in-band with I/O
    - Polled Events: Drivers only notify rising edges of events and provide a callback for polling
  - User API:
    - Check whether specific events are set on a file
    - Wait and be awoken when an event becomes set on a file
- Voluntary kernel-space synchronization mechanisms:
  - Driver operations provide no synchronization or atomicity guarantees themselves in the general case
  - Drivers are free to implement these operations as efficiently as their internal data model allows
  - Higher-level APIs that want to provide atomicity guarantees (e.g. POSIX read vs. write serialization) can and should use these mechanisms to achieve their goal

Note that there are no "open" or "close" operations; a ukfile is not "open" or "closed", it simply _is_.
A ukfile is a "live" object -- an existing reference gives full access to all of its capabilities.
All resource management is done at creation or in-band with operations, and released during destruction.

## File Lifetime

While every file's journey is different, there are some common points defining its lifetime.

### Creation

The `ukfile` API does not specify any way of "looking up" file drivers, nor any common interface for creating files.
This is a deliberate choice, as these "file sources"[^1] are extremely varied and a one-size-fits-all API could not work.
Instead, file sources are free to use bespoke APIs, provided they output files as a strongly refcounted `const struct uk_file *` reference.

**NOTE**: Although refcounted, ukfiles need not be heap-allocated or even runtime-initialized, as long as the static allocation is included in the reference count.

[^1]: _sources_ are different from and orthogonal to _drivers_: drivers implement file functionality, sources create and publish file objects.

### Use

With a ukfile reference in hand, there are really only a few things one can do:
1. Perform file operations
2. Store the reference for later
3. Acquire a new (weak) reference
4. Release a (weak) reference

Unless explicitly (and very loudly) documented otherwise, all functions operating on ukfiles "borrow" the reference for the duration of the call.
It is undefined to call any such function without a refcount of appropriate strength held.

### Destruction

When all references to a file have been released, it is cleaned up in two stages:
1. Release driver resources
   - done on releasing last strong reference
   - driver can release all private resources related to the file
   - (optional) run external file destruction hooks ("finalizers")
   - most file operations are undefined past this point (see "File Operations" above)
   - file struct & public state is still available
2. Release file object
   - done on releasing last reference of any kind
   - driver releases all resources, including the memory of file's struct
   - file is completely destroyed, reference is now freed memory and any further access is undefined
