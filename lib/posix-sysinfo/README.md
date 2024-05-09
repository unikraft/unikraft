# `posix-sysinfo`: Unikraft's System Information Internal Library

`posix-sysinfo` is an internal library of Unikraft that provides a similar interface to the Linux system information related syscalls (`sysinfo`, `uname`, etc.).

The [`struct sysinfo`](https://github.com/unikraft/unikraft/blob/staging/lib/posix-sysinfo/include/sys/sysinfo.h#L36) and [`struct utsname`](https://github.com/unikraft/unikraft/blob/staging/lib/posix-sysinfo/include/sys/utsname.h#L43) structures follow the Linux conventions.
The Unikraft `sysinfo` library will not fill all the items in the `struct sysinfo` structure, some of them will be set to 0 (such as `uptime`, `loads`, `*swap`, `*high`), as support for them is not yet implemented.
For memory-related fields to be populated (`*ram`, `mem_unit`), the `Virtual memory API` config option must be selected from the `Platform Configuration -> Platform Interface Options` configuration screen.

## Configuring applications to use `posix-sysinfo`

You can select `posix-sysinfo` under the `Library Configuration` screen of the `make menuconfig` command.
After that, you can use the functions exposed by the internal library just like you would do for a Linux system,

An example of a simple application that uses the `posix-sysinfo` library is the following:

```c
#include <stdio.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

int main(void)
{
        struct sysinfo info;
        struct utsname utsn;

        sysinfo(&info);
        uname(&utsn);

        printf("Nr proc: %hu\n", info.procs);
        printf("Kernel release: %s\n", utsn.release);

        return 0;
}
```

To configure, build and run the application, follow the steps below:

1. Enter the configuration interface:

```console
make menuconfig
```

1. Enter the `Library Configuration` menu and select `posix-sysinfo`
1. Build the application using `make`
1. Run the application using `qemu-system`.
   If the application was built for `x86_64`, run using the following command:

```console
qemu-system-x86_64 -kernel <path-to-application-image> -nographic
```

This should print an output similar to:

```text
Booting from ROM..Powered by
o.   .o       _ _               __ _
Oo   Oo  ___ (_) | __ __  __ _ ' _) :_
oO   oO ' _ `| | |/ /  _)' _` | |_|  _)
oOo oOO| | | | |   (| | | (_) |  _) :_
 OoOoO ._, ._:_:_,\_._,  .__,_:_, \___)
                Pandora 0.15.0~423d8933
Nr proc: 1
Kernel release: 5-Pandora
```
