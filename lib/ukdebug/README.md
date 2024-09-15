# `ukdebug`: Unikraft's debugging features

This library contains features to debug Unikraft.

## GDB stub

The GDB stub allows remotely debugging Unikraft with GDB using the [GDB Remote Serial Protocol (RSP)](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Protocol.html).
The debugging process involves two machines.
The development machine (the GDB host) runs GDB and implements the client side of the RSP.
The other machine runs Unikraft (the target), which implements the GDB stub and serves requests issued by the host.
The two sides communicate over a serial connection.
With this setup, the machine running Unikraft can be a local VM or a physically remote machine.

```plain
   Development machine                     Unikraft
  +-------------+                         +-------------+
  |             |    Serial connection    |             |
  |  GDB Host <-+-------------------------+-> GDB Stub  |
  |             |                         |             |
  +-------------+                         +-------------+
```

The stub runs after every debug trap, meaning the stub executes in an exception context.
A loop inside the stub waits to receive packets from the host and to act upon them.
Once the stub has finished handling a packet, it will send the response to the host and wait for the next packet.
Some packets instruct the stub to leave the loop and resume execution of the kernel.

The GDB stub uses a serial console device to communicate with the host.
This can be either the same console that is used for standard output, or a separate console dedicated for debugging.
If the GDB stub and standard output use the same console, Unikraft's standard output is printed inline with GDB's output in the GDB CLI.

### Kernel parameters

Two kernel parameters control the GDB stub.

- `debug.gdbcon=<id>`: ID of the console device that is used to communicate with the GDB host.
  If there's only a single console on the system, its ID is `0`, so `debug.gdbcon=0` should be used to select it.
  The GDB stub is only enabled at runtime if `debug.gdbcon` is set.
- `debug.gdbmode=[0, 1, -1]`: A tristate value controlling the mode of the GDB stub.
  The three states are "shared" (`1`), "not shared" (`0`) and "auto" (`-1`).
  The default it "auto".
  In "shared" mode, Unikraft's standard output is printed inline with GDB's output in the GDB CLI on the host.
  In "not shared" mode, Unikraft's standard output is never printed inline with GDB's output.
  If the console chosen to communicate with the GDB host is used for standard output, "auto" will enable the "shared" mode.

### QEMU & GDB example

What follows is a concrete example of using the GDB stub with QEMU and GDB.
For starters, configure your project to enable the GDB stub and a single console device.
Note that the GDB stub depends on `uklibparam`, so you need to select that before the GDB stub shows up in the `ukdebug` menu in kconfig.
Let's assume we are using x86.
A possible QEMU command to start a VM that the GDB host can connect to is:

```bash
qemu-system-x86_64 -kernel build/app-helloworld_qemu-x86_64 \
                   -cpu max \
                   -display none \
                   -serial tcp::1235,server,nowait \
                   -append "build/app-helloworld_qemu-x86_64 debug.gdbcon=0 --"
```

The only two interesting options here are `-serial` and `-append`.
`-serial tcp::1235,server,nowait` makes QEMU create a TCP socket on `localhost:1235` that's connected to COM1 inside the VM.
This TCP socket will be used to connect the GDB host to the serial console in the VM.
`-append` specifies the kernel command line that's passed to the kernel at runtime.
Passing the name of the image as the first parameter is an Unikraft convention.
If the first parameter is a kernel parameter, this kernel parameter will be dropped, so be careful.
With `debug.gdbcon=0`, we tell the GBD stub to use `con0` to communicate with the GDB host.
On x86 this is COM1, and it's connected to `localhost:1235`.

You can use `-serial stdio` instead of `-serial tcp::1235,server,nowait` to see if the GDB stub is enabled and set up correctly.
If you can run the above command with `-serial stdio`, you should see some output that ends with something like the below (assuming verbosity is set to >= `info`):

```plain
[    0.104746] Info: [libukdebug] <gdbstub.c @ 1237> Waiting for debugger connection on con0...
[    0.104944] Info: [libukconsole] <console.c @  176> Registered con1: GDB virtual console, flags: --
$S05#b8
```

The last line is the first packet that the GDB stub sends to the GDB host.
Switch back to `-serial tcp::1235,server,nowait` and you'll stop seeing any output.
Using `-serial stdio` can be useful when you're unsure if your configured kernel even boots.

Note that the `-serial` flag is position-sensitive on x86.
COM1 is connected to whatever the first occurrence (from left to right) of the `-serial` flag specifies.
COM2 is configured based on the second occurrence of `-serial`, and so forth.
E.g., as an alternative to the above, you could configure COM1 and COM2 in kconfig and then connect the GDB stub to COM2 and keep kernel output on COM1 (`debug.gdbcon` is `1` this time):

```bash
qemu-system-x86_64 -kernel build/app-helloworld_qemu-x86_64 \
                   -cpu max \
                   -display none \
                   -serial stdio \
                   -serial tcp::1235,server,nowait \
                   -append "build/app-helloworld_qemu-x86_64 debug.gdbcon=1 --"
```

But let's stick to the initial example and assume that only COM1 is configured and that `debug.gdbcon=0`.
Start the VM and in another terminal run GDB like:

```bash
gdb build/app-helloworld_qemu-x86_64.dbg
```

There are two things to note here.
First, make sure to use the `.dbg` image, as it contains symbol and debug information.
GDB won't find any symbols without it.
Second, make sure you're using a  GDB binary compiled for the target architecture.
If you're cross-compiling, you must use the GDB binary provided by the cross-compiling toolchain.

Run the command `target remote localhost:1235` in the GDB CLI to connect to the GDB stub on TCP socket `localhost:1235`.
GDB should manage to connect to the VM and you should see where execution was stopped before entering the stub.
E.g., it might look like this:

```plain
(gdb) tar remote :1235
Remote debugging using :1235
0x0000000000120e02 in gdb_entry (ctx=<optimized out>) at /home/thasso/Work/oss-unikraft/lib/ukdebug/gdbstub.c:1247
1247    }
```

You are now connected to the GDB stub that runs inside Unikraft.
All the usual GDB commands like `break`, `backtrace`, or `continue` should work.
Since we only configured a single console and didn't set `debug.gdbmode`, standard output should be printed inline in the GDB CLI.
Run `break main` and `continue` to see standard output show up in the GDB CLI.

### Debugging the GDB stub

This section provides information for developers who need to debug Unikraft's GDB stub.
First, note that GDB stub is not reentrant and should not be used to debug itself.
The GDB command `set debug remote 1` comes in handy when debugging any problem with the GDB stub.
This command will print all packets that the GDB host sends the stub along with all responses that the host receives.

QEMU has a built-in GDB stub as part of the VMM, which can be used to debug the Unikraft-native GDB stub.
The `-s` flag enables the QEMU-internal GDB stub, and the `-S` makes QEMU wait for a GDB host to connect to this stub before properly starting the VM.
`-s` creates a TCP socket on `localhost:1234` that exposes the internal GDB stub.
If we wanted to use the QEMU-internal stub in addition to the Unikraft-native stub in the example above, the QEMU command would look like this:

```bash
qemu-system-x86_64 -s -S \
                   -kernel build/app-helloworld_qemu-x86_64 \
                   -cpu max \
                   -display none \
                   -serial tcp::1235,server,nowait \
                   -append "build/app-helloworld_qemu-x86_64 debug.gdbcon=0 --"
```

First, connect one instance of GDB to the QEMU-internal stub, then set some breakpoints and resume execution.
Now the VM will start and enter the Unikraft-native GDB stub.
You can connect a second instance of GDB to the Unikraft-native stub at this point.
With the QEMU-internal GDB stub, you can safely set breakpoints on functions in the Unikraft-native stub.

The connection between the Unikraft-native GDB stub and its GDB host can time out if you set a breakpoint at an unfortunate point in the GDB stub and take too long to do your debugging.
For this reason, it's recommended that you use the command `set remotetimeout 1000000` to make the GDB host that's connected to the Unikraft-native stub wait for a very long time before timing out (here `1000000` is the number of seconds that GDB waits before it times out, any high value works).
