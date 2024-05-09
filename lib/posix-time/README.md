# posix-time

`posix-time` is an internal library of Unikraft that enables the system calls that are dependent of the system clock.

Note: timer operations are not yet supported.

## Configuring applications to use `posix-time`

You can select `posix-time` under the `Library Configuration` screen of the `make menuconfig` command.
After that, you can use the functions exposed by the internal library just like you would for a Linux system,

An example of a simple application that uses the `posix-time` library is the following:

```c
#include <stdio.h>
#include <time.h>

int main(void) {
    time_t before, after;

    struct timespec sleep_time;
    sleep_time.tv_sec = 2; /* 2 seconds */
    sleep_time.tv_nsec = 0; /* 0 nanoseconds */

    before = time(NULL);
    nanosleep(&sleep_time, NULL);
    after = time(NULL);
    printf("The sleep lasted for %ld seconds.\n", after - before);
    return 0;
}
```

To configure, build and run the application, follow the steps below:

1. Enter the configuration interface:

```console
make menuconfig
```

1. Enter the `Library Configuration` menu and select `posix-time`
1. Build the application using `make`
1. Run the application using `qemu-system`

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
                Pandora 0.15.0~4244c95c
The sleep lasted for 2 seconds.
```
