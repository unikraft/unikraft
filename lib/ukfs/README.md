# `ukfs`: Unikraft Filesystem Interface

This core library provides the Unikraft abstraction of filesystem-backed files.
Also known as "filesystem nodes", these are ukfiles that provide an extra set of filesystem operations in addition to the base `ukfile` API.

Note that while all filesystem nodes are ukfiles, not all ukfiles are filesystem nodes, with pipes, network sockets, and pseudo-files (epollfd, timerfd, etc.) being perfect examples of the latter.
It is undefined to pass a non-filesystem file to the `ukfs` API.

Not to be confused with _the_ filesystem -- aka VFS, the single filesystem namespace anchored at `/` common to any *nix OS.
The VFS is a higher-level concept and implemented by `posix-vfs`.

This README provides a high-level overview, consult the public header files for more details.

Consumer API:
- `uk/fs.h`: filesystem API
- `uk/fs/driver.h`: driver registration & lookup

Driver utils:
- `uk/fs/pathutil.h`: utilities for manipulating paths
- `uk/fs/dirent.h`: utilities for outputting `struct dirent64`
- `uk/fs/prio.h`: priority conventions for initializing filesystems
- `uk/fs/common-ops.h`: common boilerplate filesystem operations

Driver templates:
- `uk/fs/template/live.h`: live reference driver template
- `uk/fs/template/pseudo.h`: pseudo-fs driver template

## Filesystem Operations

In brief, the ukfs API provides operations for:
- volume-wide stats & sync
- lookup on all nodes
- readlink of symbolic links
- directory ops: listing, creation, deletion, renaming
- runtime state management: `mount`, `graft`, `rebind`

Detailed description of each operation can be found in `uk/fs.h`.

## Peculiarities

To someone familiar with userspace VFS APIs or legacy vfscore, some design choices of ukfs may seem peculiar.
This is mainly because ukfs focuses on defining _mechanism_, leaving higher abstraction layers to build upon it to implement _policy_.
Here we aim to clarify and argue some of these design choices.

#### Locality

All ukfs operations are _relative_ to their target node, and any changes they enact are _local_ to that node.
This has several notable implications:
1. each node provides an authoritative reference to the ops it can perform
2. all paths are relative, there's no concept of "absolute path"
3. all lookups are relative and must make progress with local information only
4. changes to filesystem state are local
5. there is no global view of in-use filesystems whatsoever

Point (1) differs from the legacy vfscore stack as there is no single public "driver ops table" to speak of, giving drivers the freedom to tailor ops tables to different file types.

Point (2) highlights ukfs's focus on mechanism, as "`/` -- the VFS root" along with absolute paths are a higher-level concept and fall outside its scope.

Continuing from this, point (3) informs the interface of `lookup` and the separation of responsibilities it enforces between the caller and driver.
Most notably this affects what happens when lookup encounters something other than a regular filesystem node, such as a symlink, mount point, etc., where the driver notifies the caller about the event, but makes no decision on what action to take.
Indeed, concepts like traversing symlinks or mount points are alien to a ukfs driver and belong in a higher-level library that calls into ukfs.

Point (4) removes the need for complex or global synchronization of filesystem ops in calling code, improving parallelism.
Drivers are of course free to synchronize internally as needed.

Finally point (5) means that concepts like "mount table" also fall outside the scope of ukfs.
However, this in conjunction with point (3) means that ukfs drivers must be made locally aware of larger-scale structures like mounts, at least to a very minimal extent.
This leads nicely into the next section.

#### Mount, Graft & Rebind

Known collectively as "ops managing runtime-volatile state", `mount`, `graft`, and `rebind` are probably the most peculiar of ukfs operations.
When broken down however, their logic is very simple and boils down to 3 things:
1. `mount` manages what happens on lookup of self (or `.`)
   - every file implicitly looks up itself on any lookup, which is a no-op by default
   - `mount` allows callers to hook this step in order to return a special case at lookup -- the mounted node
   - the driver remains oblivious to what "mounting" means in context; it just knows to return this one special case on lookup
   - caller logic can decide whether to traverse this mount point or handle it some other way
2. `graft` manages what happens on lookup of parent (`..`)
   - making traversal of a mount point work in reverse requires `..` in the mounted directory to reference the parent of its mount point
   - `graft` allows for this to happen, causing a lookup of `..` to report a mount point with the appropriate target
   - drivers are as oblivious to the larger context as for `mount`; to them it is merely a lookup artifice
   - this is separate from and orthogonal to `mount` -- callers may use a combination of either to "weave" together any structure they wish, not just a single VFS tree
3. `rebind` creates a new instance of an open volume
   - similar to a "clone" but for filesystem nodes, creating an independent instance that shares state with the original
   - intended to facilitate bind mounts, where a sub-tree of an existing filesystem is re-instanced and mounted somewhere else
   - different to opening the same volume twice (although depending on driver implementation these might behave the same)

#### Lifetime

The ukfs API provides exactly three ways to obtain new filesystem node references: `lookup`, `create`, and `rebind`.
This poses somewhat of a chicken-and-egg problem, as all of these are relative, and thus require an existing node to operate on.
In practice this means that at any point in time all live filesystem nodes can be traced, through a combination of these three operations, back to a set of seed nodes.

Producing these seed nodes is the responsibility of `vopen`, the single part of a ukfs driver that can be called from a global context.
`vopen` takes a (driver-specific) volume, along with flags and options, and returns the root node of a new filesystem instance.

There is no corresponding `vclose` operation.
Drivers are responsible for cleaning up volume instances when the last node referencing it has been released, or, at the very latest, at system shutdown.

## Filesystem Driver Templates

The ukfs/ukfile interface gives drivers complete control over all aspects of their files:
- volume-wide state
- volume lifetime management
- driver-internal node representation
- public ukfile mutable state
- lifetime management (refcounting semantics)
- ukfs runtime volatile state (mounts, rebinds, etc.)

Oftentimes however, a particular driver may not need this level of control, nor want to be burdened with the corresponding responsibility.
For these cases, ukfs provides so-called driver "templates" -- header files specialized in generating ukfs-compatible operations from driver code that natively operates at a lower abstraction layer.

### Live Reference

The "live reference" driver template is tailored for drivers that can naturally handle all lifetime management and public file state, but want a boilerplate implementation of ukfs runtime state.
The name "live reference" refers to the node objects of these drivers which, just like ukfiles, are "live" -- references are counted, there are no explicit open/close operations, and a reference simply existing implies it being "open".

Consult the `uk/fs/template/live.h` header for more details.

### Pseudo-FS

In addition to "true" filesystems (ones whose purpose is the organization of data) there are many use-cases for so-called pseudo-filesystems -- hierarchies of special filesystem nodes that provide a file-like interface to various kernel functionalities.
Their implementation usually follows a simple pattern:
1. Initialize at boot time a backend root piggy-backing on top of another (usually volatile) filesystem
2. Populate said root with desired contents
3. Export a rebind of the backend root under a custom filesystem name

The pseudo-fs driver template generates boilerplate code handling steps (1) and (3) above, leaving drivers to focus on implementing their bespoke functionality.

Consult the `uk/fs/template/pseudo.h` header for more details.
