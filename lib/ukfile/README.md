# `ukfile`: files for Unikraft

This core library contains the unikraft abstractions of a "file" as well as an "open file" (a.k.a. "open file descriptions").
These are low-level internal abstractions that do not have direct correspondents in any userspace-facing API.
Not to be confused with "file descriptors" or other similar POSIX-y concepts; please see `posix-fd*` for those.

This README discusses higher-level design considerations for (open) files.
Consult the headers `uk/file.h` and `uk/ofile.h` for specifics on implementation.

## What is a _file_?

To overuse a classical *NIX idiom, "everything is a file".
More concretely however, a file is an abstraction for any resource that offers a combination of input, output, and/or control operations.
A file in Unikraft is a combination of an _immutable identity_ coupled with _mutable state_.

Files are represented in Unikraft by the `struct uk_file` type, referenced in APIs as `const struct uk_file *` to enforce immutability.
Identity consists of:
- A volume identifier: driver-specific field, used to identify the file type as well as its originating driver instance
- A file node: reference to driver-specific data associated with the file
- Table of file operations: implementations of well-defined file operations (see below)

File state is used for bookkeeping purposes and includes:
- Reference counting (strong & weak references)
- Locks for synchronization
- Event set & queue for polling operations

### File Operations

Files allow for a defined set of operations, some of which are driver-implemented, while others are common across all files.
Driver-specific operations have a well-defined interface and are implemented by file drivers.
These are:
- I/O: manipulating an array of unstructured bytes
  - `read`: retrieve a specific contiguous block of bytes from this array
  - `write`: ensure a specific contiguous block in this array has specific bytes
- Metadata: manipulating a defined structure of metadata related to the file
  - `getstat`: get file metadata fields
  - `setstat`: set file metadata fields
- Control: requests for special operations to be performed by the file
  - `ctl`
- (internal) cleanup/destructor: what happens, if anything, when we no longer need the file

Common operations are implemented centrally for all file objects:
- Reference counting: acquire/release of regular (strong) or weak references
  - Strong references allow the full functionality of files
  - Weak references allow only common operations (polling, locking)
- Event polling & notification:
  - Driver API:
    - Set & clear what event flags are active on the file
  - User API:
    - Check whether specific events are set on a file
    - Wait and be awoken when an event becomes set on a file
- Voluntary kernel-space synchronization mechanisms:
  - Driver operations provide no synchronization or atomicity guarantees themselves in the general case
  - Drivers are free to implement these operations as efficiently as their internal data model allows
  - Higher-level APIs that want to provide atomicity guarantees (e.g. POSIX read vs. write serialization) can and should use these mechanisms to achieve their goal


## What is an _open file_?

Open files are stateful and mutable references to a file that is "in use".
The precise definition of "in use" is intentionally left vague and up to client code.
Open file state consists of:
- Reference count, allowing multiple independent shared references
- Open "mode", a client-defined bitmask of open file options
- Current position for I/O (i.e. what one sets with `lseek()`)
- Lock for synchronizing changes to the above

Open files are represented in Unikraft by the `struct uk_ofile` type.
A single `struct uk_file` may be referenced by an arbitrary number of ofiles, each of which acts independently from the others.

Open files do not expose any operations themselves, instead providing only a base data structure for higher-level abstractions, such as file descriptor tables and POSIX I/O syscalls.
