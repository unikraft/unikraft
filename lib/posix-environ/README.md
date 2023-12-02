# posix-environ

This library implements the POSIX environment variables related functions such as `clearenv()`, `setenv()`, and `unsetenv()`.

The environment variables are held into a 2D array named `__environ`, which keeps the name of the environment variable and its value.

## Configuring Applications to Use `posix-environ`

An Unikraft application that uses the `posix-environ` library will use code snippets such as the one below:

```c
#include <stdio.h>

int main () {
   printf("MY_ENV : %s\n", getenv("MY_ENV"));

   return 0;
}
```

To configure the application to use the `posix-environ` library we follow the steps below:

1.  Enter the configuration interface by running:

   ```console
   $ make menuconfig
   ```

1.  Under `Library Configuration`, select `posix-environ: Environment variables  --->`.

1.  Select `Compiled-in environment variables  --->`.

1.  Add a new entry, for example: `MY_ENV=123`.

1.  Save the configuration and exit the interface.

1.  To run the program under `qemu` use the following command:

```console
$ qemu-system-x86_64 \
-kernel build/app-helloworld_kvm-x86_64 \
-nographic
```

Building and running the program above yields the output below:

```console
    SeaBIOS (version rel-1.16.2-0-gea1b7a073390-prebuilt.qemu.org)

    iPXE (http://ipxe.org) 00:03.0 CA00 PCI2.10 PnP PMM+06FD1040+06F31040 CA00

    Booting from ROM..Powered by
    o.   .o       _ _               __ _
    Oo   Oo  ___ (_) | __ __  __ _ ' _) :_
    oO   oO ' _ `| | |/ /  _)' _` | |_|  _)
    oOo oOO| | | | |   (| | | (_) |  _) :_
    OoOoO ._, ._:_:_,\_._,  .__,_:_, \___)
            Pandora 0.15.0~92334f93-custom
    MY_ENV : 123
```

As we can see in the snippet above, the value `123` from the `MY_ENV`
environment variable is printed to the standard output.
