# VFSCore: Unikraft's Virtual File System API

`vfscore` (*Virtual File System Core*) is an internal library of Unikraft that provides a generic interface of system calls related to filesystem management.
There are multiple types of filesystems available in Unikraft (e.g: `ramfs`, `9pfs`).

## File Organization

The `vfscore` library consists of multiple files:

```console
$ tree -L 2 lib/vfscore/
[...]
|-- vnode.c
|-- fops.c
|-- lookup.c
|-- stdio.c
|-- rootfs.c
|-- task.c
|-- syscalls.c
|-- vfs.h
`-- main.c
```

- [`vnode.c`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/vnode.c) defines functions that interact with a [`vnode structure`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/include/vfscore/vnode.h#L76).

- [`fops.c`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/fops.c) implements a subset of functions that are defined in [`vfs.h`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/vfs.h#L802), that are related with file operations.

- [`lookup.c`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/lookup.c) implements the subset of functions that are defined in [`vfs.h`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/vfs.h#L685), that are related to searching for a specific path in a filesystem.

- [`stdio.c`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/stdio.c) exposes the functions that are used to interact with standard IO. 
You can notice that this is done very similar to defining a new filesystem, as it will be detailed later.

- [`rootfs.c`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/rootfs.c) implements the function that is used to mount the root filesystem.

- [`task.c`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/task.c) implements some helper functions that are used to convert between different path conventions.

- [`syscalls.c`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/syscalls.c) does the actual generic implementation of the `sys_*` function defined in [`vfs.h`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/vfs.h#L121).

- [`main.c`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/main.c) defines the specific Unikraft system calls for interacting with a filesystem, which are based on the files described above. 
You can read more about adding a new system call for Unikraft [here](https://unikraft.org/docs/develop/syscall-shim/).

## Key Functions and Data Structures

The `vfscore` library provides 2 structures to define operations on a filesystem (defined in [`lib/vfscore/include/vfscore/mount.h`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/include/vfscore/mount.h#L151)):

```c
struct vfsops {
        int (*vfs_mount)        (struct mount *, const char *, int, const void*);
        ...
        struct vnops    *vfs_vnops;
};
```

```c
struct vnops {
        vnop_open_t             vop_open;
        vnop_close_t            vop_close;
        vnop_read_t             vop_read;
        vnop_write_t            vop_write;
        vnop_seek_t             vop_seek;
        vnop_ioctl_t            vop_ioctl;
        ...
};
```

The first structure mainly defines the operation of mounting the filesystem, while the second defines the operations that can be executed on files (regular files, directories, etc.).
The `vnops` structure can be seen as the `file_operation` structure in the Linux Kernel (more as an idea).
More about this structure [here](https://tldp.org/LDP/lkmpg/2.4/html/c577.htm).

The filesystem library will define two such structures through which it will provide the specified operations.
To understand how these operations end up being used, let's examine the [`open` system call](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/syscalls.c#L111)

```c
int
sys_open(char *path, int flags, mode_t mode, struct vfscore_file **fpp)
{
        struct vfscore_file *fp;
        struct vnode *vp;
        ...
        error = VOP_OPEN(vp, fp);
}
```

[`VOP_OPEN()`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/include/vfscore/vnode.h#L219) is a macro that is defined as follows:

```c
#define VOP_OPEN(VP, FP)           ((VP)->v_op->vop_open)(FP)
```

## Register a New Filesystem

To map the desired function for your filesystem with the `vfscore` application, you need to declare a `struct vnops` structure.
The example below show how this is done for [ramfs](https://github.com/unikraft/unikraft/blob/staging/lib/ramfs/ramfs_vnops.c#L651):

```c
struct vnops ramfs_vnops = {
    ramfs_open,             /* open */
    ramfs_close,            /* close */
    ramfs_read,             /* read */
    [...]
};
```

A filesytem will create its own structure and implement its own functions.
If some of the functions are not used or not yet defined, you can use the `vfscore_vop_nullop` and cast it to the appropriate type:

```c
#define ramfs_open      ((vnop_open_t)vfscore_vop_nullop)
#define ramfs_close     ((vnop_close_t)vfscore_vop_nullop)
#define ramfs_seek      ((vnop_seek_t)vfscore_vop_nullop)
```

Let's see now how to link the "file operations" of a filesystem to the `vfscore` library.
For this, the library exposes a specific structure named `vfscore_fs_type`:

```c
struct vfscore_fs_type {
        /* name of file system */
        const char *vs_name;
        /* initialize routine */
        int (*vs_init)(void);
        /* pointer to vfs operation */
        struct vfsops *vs_op;
};
```

Notice that this structure contains a pointer to the `vfsops` structure, which in turn contains the `vnops` structure.
To register a filesystem, the `vfscore` library uses an additional section in the ELF.
You can inspect the [`extra.ld`](https://github.com/unikraft/unikraft/blob/staging/lib/vfscore/extra.ld) file to see it.

As it was mentioned before, these sections come with helper macros, so this time is no exception either.
The macro that registers a filesystem is:

```c
 UK_FS_REGISTER(fssw)
```

Where the `fssw` argument is a `vfscore_fs_type` structure.

Notice that the system call will eventually call the registered operation.
So, when adding support for a new filesystem in Unikraft, you only need to implement a subset (or all) functions in the `vnops` structure. You can take a look at how this is done for [9pfs](https://github.com/unikraft/unikraft/blob/staging/lib/9pfs/9pfs_vnops.c#L977) filesystem.

## Configuring Applications to Use `vfscore`

To use the `vfscore` library, you need to have a filesystem that actually implements the file operations.
You can follow the same approach as in [`lib/ramfs/README.md`](https://github.com/unikraft/unikraft/tree/staging/lib/ramfs#configuring-applications-to-use-ramfs)
