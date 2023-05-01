# 9pfs: Unikraft's 9p Virtual Filesystem

With `9pfs`, you can create virtual filesystem devices (`uk_9pdev` in the context of Unikraft) and make them directly accessible between a guest OS and a host as a pass-through filesystem.
This filesystem uses the [9P network protocol](https://en.wikipedia.org/wiki/9P_(protocol)) for communication between the two instances.

This protocol has a few operations that are all initiated by the clients.
Each request is satisfied by a single associated response from the server.

## Key Data Structures

### Mount Data Structure

```c
struct uk_9pfs_mount_data {
 /* 9P device, used to interact with a `virtio` device;
  * the initialization for this field is done in
  * `uk_9p_dev_connect()`
  */
 struct uk_9pdev       *dev;
 /* Wanted transport */
 struct uk_9pdev_trans *trans;
 /* Protocol version used */
 enum uk_9pfs_proto    proto;
 /* User name to attempt to mount as on the remote server */
 char                  *uname;
 /*
  * File system name to access when the server is
  * offering several exported file systems.
  */
 char                  *aname;
};
```

The `uk_9pfs_mount_data` structure is used for the `mount` operation, and it contains:

* The `dev` field, which refers to a 9P device and is initialized in [`uk_9p_dev_connect`](https://github.com/unikraft/unikraft/blob/staging/lib/uk9p/9pdev.c#L237).
* The `trans` field describing the wanted transport to use (such as `virtio`, `unix`, `tcp` etc.).
* The `proto` field, which refers to the protocol we want to use.
  It can be `UK_9P_PROTO_2000U`, `UK_9P_PROTO_2000L` or `UK_9P_PROTO_MAX`.
* The `uname` field, which refers to the user name attempting the connection.
* The `aname` specifying the file system name to mount.

### File Data Structure

```c
struct uk_9pfs_file_data {
 /* File id of the 9pfs file */
 struct uk_9pfid        *fid;
 /*
  * Buffer for persisting results from a 9P read operation across
  * `readdir()` calls
  */
 char                   *readdir_buf;
 /*
  * Offset within the buffer where the `uk_9p_stat`
  * stat structure of the next child can be found
  */
 int                    readdir_off;
 /* Total size of the data in the `readdir` buf */
 int                    readdir_sz;
};
```

The `uk_9pfs_mount_data` structure is used for operations that can be done on the files in the filesystem (e.g. `open`, `close`, `readdir`).
It contains:

* The `fid`, a unique file identifier which identifies the file handle.
* The `readdir_buf`, which contains the information read during the `readdir()` calls.
  In the `uk_9pfs_mount_data` we can also find the `readdir_sz` which is the size of data in the `readdir_buf`.
* The `readdir_off`, which refers to the offset from within the `readdir_buf` where we can find information about the next child.

### Note Data Structure

```c
struct uk_9pfs_node_data {
 /* File id of the vfs node */
 struct uk_9pfid        *fid;
 /* Number of files opened from the vfs node */
 int                    nb_open_files;
 /* Is a 9P remove call required when `nb_open_files` reaches 0? */
 bool                   removed;
};
```

The `uk_9pfs_node_data` structure is used for operations that involve allocating or freeing `vnode` data.

## Configuring Applications to Use `9pfs`

An Unikraft application that uses the `9pfs` filesystem will use code snippets such as the one below:

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
   char buf[1024];
   int fd;

   fd = open("/westworld.txt", O_RDWR | O_CREAT, 0777);
   write(fd, "These violent delights have violent ends.", 41);
   close(fd);

   fd = open("/westworld.txt", O_RDWR | O_CREAT, 0777);
   read(fd, buf, 41);
   printf("%s\n", buf);
   close(fd);

   return 0;
}
```

To configure the application to run under `9pfs` we follow the steps below:

1. Enter the configuration interface by running:

   ```console
   $ make menuconfig
   ```

1. Under `Library Configuration`, select `vfscore:` `VFS Core Interface`.
1. Under `vfscore: Configuration  --->` select `Automatically mount a root filesystem (/)`.
1. Select `Default root filesystem -> 9pfs`.
1. Set the `Default root device` option to `rootfs` (which is the default value).
1. We want to run Unikraft with QEMU / KVM, so we must select KVM guest in the `Platform Configuration` menu.
For `9pfs` we also need to enable, in the KVM guest options menu, `Virtio --->`, then `Virtio PCI device support`.
1. Save the configuration and exit the interface.
1. Create a new folder in the current directory by running:

   ```console
   $ mkdir rootfs
   ```

The complete command for running this program with `qemu` is:

```console
$ qemu-system-x86_64 \
-fsdev local,id=myid,path=./rootfs/,security_model=none \
-device virtio-9p-pci,fsdev=myid,mount_tag=rootfs \
-kernel build/app-helloworld_kvm-x86_64 \
-nographic
```

Breaking it down we can see:

* `-fsdev local,id=myid,path=./rootfs/,security_model=none` - assign an id (`myid`) to the `./rootfs/` local folder.
* `-device virtio-9p-pci,fsdev=myid,mount_tag=rootfs` - create a device with the 9pfs type, assign the `myid` for the `-fsdev` option and also assign the mount tag that we configured above (`rootfs`).
  Unikraft will look after that mount tag when trying to mount the filesystem, so it is important that the mount tag from the configuration is the same as the one given as argument to qemu.
* `-kernel build/app-helloworld_kvm-x86_64` - tells QEMU that it will run a kernel image; if this parameter is omitted, QEMU will think it runs a raw file.
* `-nographic` - prints the output of QEMU to the standard output, it doesn’t open a graphical window.

Building and running the program above yields the output below:

```console
SeaBIOS (version 1.15.0-1)

iPXE (https://ipxe.org) 00:03.0 CA00 PCI2.10 PnP PMM+07F8B1F0+07ECB1F0 CA00

Booting from ROM..Powered by
o.   .o       _ _               __ _
Oo   Oo  ___ (_) | __ __  __ _ ' _) :_
oO   oO ' _ `| | |/ /  _)' _` | |_|  _)
oOo oOO| | | | |   (| | | (_) |  _) :_
 OoOoO ._, ._:_:_,\_._,  .__,_:_, \___)
             Epimetheus 0.12.0~4c7352c0
These violent delights have violent ends.
Arguments:  "build/app-helloworld_kvm-x86_64"
```

As we can see in the snippet above, the phrase `These violent delights have violent ends.` from the `westworld.txt` file is printed to the standard output.
