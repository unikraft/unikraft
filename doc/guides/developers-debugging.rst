****************************
Debugging in Unikraft
****************************
Debugging in Unikraft isn't much more complicated than debugging a
standard application with gdb. A couple of hints that should help:

1. In the menu, under "Build Options" make sure that "Drop unused
   functions and data" is **unselected**. This prevents Unikraft from
   removing unused symbols from the final image and, if enabled, might
   hide missing dependencies during development.

2. Use ``make V=1`` to see verbose output for all of the commands being
   executed during the build. If the compilation for a particular file is
   breaking and you would like to understand why (e.g., perhaps the
   include paths are wrong), you can debug things by adding the ``-E``
   flag to the command, removing the ``-o [objname]``, and redirecting
   the output to a file which you can then inspect.

3. Check out the targets under "Miscellaneous" when typing ``make
   help``, these may come in handy. For instance, ``make print-vars``
   enables inspecting at the value of a particular variable in
   ``Makefile.uk``.

4. Use the individual ``make clean-[libname]`` targets to ensure that you're
   cleaning only the part of Unikraft you're working on and not all the
   libraries that it may depend on; this will speed up the build
   and thus the development process.

5. Use the Linux user-space platform target for quicker and easier
   development and debugging.

============================
Using GDB
============================

For gdb debugging, first go to the menu and under "Build Options" make
sure you select/deselect the following options as shown: ::

  [*] Debugging information
  [*]   Create a debugging information file
  [*]   Create a symbols file
  [ ] Strip final image

Then save the configuration and build your image. For Linux user-space
simply point gdb to the resulting image, for example: ::

  gdb build/helloworld_linuxu-x86_64

For KVM, use the command: ::

  qemu-system-x86_64 -s -S -cpu host -enable-kvm -m 128 -nodefaults -no-acpi -display none -serial stdio -device isa-debug-exit -kernel build/helloworld_kvm-x86_64 -append verbose

For Xen the process is slightly more complicated and depends on Xen's
gdbsx tool. First you'll need to make sure you have the tool on your
system. Here are sample instructions to do that: ::

  [get Xen sources]
  ./configure
  cd tools/debugger/gdbsx/ && make

The gdbsx tool will then be under ``tools/debugger``. For the actual
debugging, you first need to create the guest (we recommend paused state:
``xl create -p``), note its domain ID (``xl list``) and execute the
debugger backend: ::

  gdbsx -a [DOMAIN ID] 64 [PORT]

You can then connect gdb within a separate console and you're ready to debug: ::

  gdb --eval-command="target remote :[PORT]" build/helloworld_xen-x86_64

You should be also able to use the debugging file
(``build/helloworld_linuxu-x86_64.dbg``) for gdb instead passing the kernel
image.
