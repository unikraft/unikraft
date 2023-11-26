# nolibc, Unikraft's Lightweight Subset of the C Standard Library

The `nolibc` library provides the basic core functionalities of libc in a lighter and more compact package, fulfilling the needs of most simple applications without occupying a large amount of memory, while also remaining POSIX-compliant.
Complex applications that require a more comprehensive set of standard C functions are better suited with Unikraft's port of [Musl](https://github.com/unikraft/lib-musl).

## Configuring Applications to Use `nolibc`

The `nolibc` library is an internal library of Unikraft and is enabled by default when configuring a new project.
Opting for a different implementation of the C standard library will automatically override `nolibc` and utilize the one provided by the user.
This behavior can be observed in the configuration menu under the `Library Configuration` section.

An example of a simple application that uses the `nolibc` library is the following:

```c
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    char str[1024], *p;

    strcpy(str, "There's the way it ought to be");
    printf("%s\n", str);

    strcat(str, "and there's the way it is");
    p = strchr(str, 'b');
    p += 2;
    fprintf(stderr, "%s\n", p);

    return 0;
}
```

The steps for configuring the application to use `nolibc` are as follows:

1. Enter the configuration interface with:

```console
$ make menuconfig
```

1. Enter the `Library Configuration` submenu
1. Select `nolibc: Only necessary subset of libc functionality`
1. This submenu allows you to choose between using [Unikraft-specific assertions](https://github.com/unikraft/unikraft/blob/staging/lib/ukdebug/include/uk/assert.h) or [`nolibc` assertions](https://github.com/unikraft/unikraft/blob/staging/lib/nolibc/include/assert.h) by toggling the `Implement assertions with libukdebug` option

Building and running the program above results in output similar to the following:

```console
SeaBIOS (version 1.13.0-1ubuntu1.1)
Booting from ROM...
Powered by
o.   .o       _ _               __ _
Oo   Oo  ___ (_) | __ __  __ _ ' _) :_
oO   oO ' _ `| | |/ /  _)' _` | |_|  _)
oOo oOO| | | | |   (| | | (_) |  _) :_
 OoOoO ._, ._:_:_,\_._,  .__,_:_, \___)
             Epimetheus 0.12.0~4c7352c0
There's the way it ought to be
and there's the way it is
Console terminated, terminating guest (PID: 10490)...
```

## Limitations and Caveats

The small size and simple interface of `nolibc` came at the cost of reducing the scope of some functions.
Notable limitations include:

-   `fprintf()` being suitable only for `stdout` and `stderr`, not other files or devices
-   No internal buffering for `stdio` functions
-   No implementation of `fscanf` and its derivatives (input should be received only from `stdin`)
