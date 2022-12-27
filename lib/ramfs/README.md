# RamFS: Unikraft's RAM-based Virtual Filesystem

The `RamFS` filesystem is a virtual filesystem that uses memory for storage, as the name implies.
Its advantage is speed.
From a simplified perspective, we can look at a file in `RamFS` as a buffer in memory.
Since this is how `RamFS` represents data, there are two downsides:

1. Data to write is limited by the total memory size.
1. Data is not persistent: once the unikernel is shutdown, the memory is freed and the data is lost.

## Key Functions and Data Structures

The key structure used is `ramfs_node` defined as:

```c
struct ramfs_node {
   /* Next node in the same directory */
   struct ramfs_node *rn_next;
   /*
   * First child node if the current node is a directory,
   * else NULL
   */
   struct ramfs_node *rn_child;
   /*
   * Entry type: regular file - VREG, symbolic link - VLNK,
   * or directory - VDIR
   */
   int rn_type;
   /* Name of the file/directory (NULL-terminated) */
   char *rn_name;
   /* Length of the name not including the terminator */
   size_t rn_namelen;
   /* Size of the file */
   size_t rn_size;
   /* Buffer to the file data */
   char *rn_buf;
   /* Size of the allocated buffer */
   size_t rn_bufsize;
   /* Last change time */
   struct timespec rn_ctime;
   /* Last access time */
   struct timespec rn_atime;
   /* Last modification time */
   struct timespec rn_mtime;
   /* Node access mode */
   int rn_mode;
   /* Whether the rn_buf was allocated in this ramfs_node */
   bool rn_owns_buf;
};
```

This structure contains:

* The `rn_type` field, which refers to the entry type.
It can be a regular file - `VREG`, a symbolic link - `VLNK`, or a directory - `VDIR`.
* The `rn_buf` field, which is the buffer in which the data will be stored
* The buffer size, `rn_size`

Typically, an `inode-like` structure (such as `ramfs_node`) doesn't store the filename;
the filename is typically stored in a `dentry-like` structure, allowing for the creation of hard links.
For simplicity, the `RamFS` library stores the filename as the `rn_name` and `rn_namelen` fields in the `ramfs_node` structure;
consequently, this means that hard links are not available on `RamFS`.

## Configuring Applications to Use `RamFS`

An Unikraft application that uses the `RamFS` filesystem will use code snippets such as the one below:

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
   char buf[1024];
   int fd;

   open("westworld.txt", O_RDWR | O_CREAT, 0777);
   write(fd, "These violent delights have violent ends.", 41);
   close(fd);
   fd = open("westworld.txt", O_RDONLY);

   read(fd, buf, 41);
   close(fd);
   printf("%s\n", buf);

   return 0;
}
```

All the files for `RamFS` are created in the memory of the application, therefore they exist only for the duration of the program.
Moreover, this means that we cannot use existing files from outside the application (e.g. files from the host filesystem).
This is why we have to create the files we want to use inside the application.
In the snippet above, we gave the `O_CREAT` value to the flag parameter of the `open()` function to create a file named `westworld.txt`.

To configure the application to run under `RamFS` we follow the steps below:

1. Enter the configuration interface by running:

   ```console
   $ make menuconfig
   ```

1. Select `vfscore:` `VFS Core Interface`.
1. Select `vfscore: Configuration  --->`.
1. Select `Automatically mount a root filesystem (/)`.
1. Select `Default root filesystem`.
1. Select `RamFS`.

Building and running the program above yields the output below:

```console
SeaBIOS (version 1.15.0-1)
Booting from ROM...
Powered by
o.   .o       _ _               __ _
Oo   Oo  ___ (_) | __ __  __ _ ' _) :_
oO   oO ' _ `| | |/ /  _)' _` | |_|  _)
oOo oOO| | | | |   (| | | (_) |  _) :_
 OoOoO ._, ._:_:_,\_._,  .__,_:_, \___)
          Phoebe 0.10.0~9bf6e633-custom
These violent delights have violent ends.
Arguments:  "build/app-helloworld_kvm-x86_64" "console=ttyS0"
Console terminated, terminating guest (PID: 9898)...
```

As we can see in the snippet above, the phrase `These violent delights have violent ends.` from the `westworld.txt` file is printed to the standard output.
