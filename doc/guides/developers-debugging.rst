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
The build system always creates two image files for each selected
platform: one that includes debugging information and symbols (`.dbg`
file extension) and one that does not. Before using gdb, go to the
menu under "Build Options" and select a "Debug information level" that
is bigger than 0.  We recommend 3, the highest level: ::

  ( ) Level 0 (-g0), None
  ( ) Level 1 (-g1)
  ( ) Level 2 (-g2)
  (X) Level 3 (-g3)

Once set, save the configuration and build your images. For the Linux
user-space target simply point gdb to the resulting debug image, for
example: ::

  gdb build/helloworld_linuxu-x86_64.gdb

For KVM, you can start the guest with the kernel image that includes debugging
information, or the one that does not. We recommend creating the guest
in a paused state (`-S` parameter): ::

  qemu-system-x86_64 -s -S -cpu host -enable-kvm -m 128 -nodefaults -no-acpi -display none -serial stdio -device isa-debug-exit -kernel build/helloworld_kvm-x86_64 -append verbose

and connect gdb by using the debug image with: ::

  gdb --eval-command="target remote :1234" build/helloworld_kvm-x86_64.dbg

You can initiate qemu to start the guest's execution by typing `c` within gdb.

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

  gdb --eval-command="target remote :[PORT]" build/helloworld_xen-x86_64.dbg

You should be also able to use the debugging file
(``build/helloworld_linuxu-x86_64.dbg``) for gdb instead passing the kernel
image.

============================
Trace points
============================

----------------------------
Dependencies
----------------------------
The ``support/scripts/uk_trace/trace.py`` depends on python modules
click and tabulate. You can install them by running: ::

  sudo apt-get install python3-click python3-tabulate

Or, you can install trace.py into a local virtual environment: ::

  python3 -m venv env
  . env/bin/activate
  cd support/scripts/uk_trace
  pip install --upgrade pip setuptools wheel
  pip install --editable .
  deactivate
  cd -

All the dependencies will be installed in the 'env' folder, not to
your machine. You do not have to enter your virtual environment, you
can call the installed script directly: ::

  env/bin/uk-trace --help

Because of ``--editable`` flag, any modifications made to
``support/scripts/uk_trace/trace.py`` will be reflected in the
installed file.

----------------------------
Reading tracepoints
----------------------------

Tracepoints are provided by ``libukdebug``. To make Unikraft collect
tracing data enable the option ``CONFIG_LIBUKDEBUG_TRACEPOINTS`` in your
config (via ``make menuconfig``).

Because tracepoints can noticeably affect performance, selective
enabling is implemented. Option ``CONFIG_LIBUKDEBUG_TRACEPOINTS`` just
enables the functionality, but all the tracepoints are compiled into
nothing by default (have no effect). If you would like one library to
collect tracing data, add to it's Makefile.uk

.. code-block:: make

   LIBAPPNAME_CFLAGS += -DUK_DEBUG_TRACE

If you need just the information about tracepoints in one file, define
``UK_DEBUG_TRACE`` **before** ``#include <uk/trace.h>``.

If you wish to enable **ALL** existing tracepoints, enable
``CONFIG_LIBUKDEBUG_ALL_TRACEPOINTS`` in menuconfig.

When tracing is enabled, unikraft will write samples into internal
trace buffer. Currently it is not a circular buffer, as soon as it
overflows, unikraft will stop collecting data.

To read the collected data you have 2 options:

1. Inside gdb

2. Using trace.py

For the first option, you need the 'uk-gdb.py' helper loaded into the
gdb session. To make this happen all you need to do is to add the
following line into ~/.gdbinit: ::

  add-auto-load-safe-path /path/to/your/build/directory

With this, gdb will load helper automatically, each time you start gdb
with a *.dbg image. For example ::

  gdb helloworld/build/helloworld_kvm-x86_64.dbg

Now you can print tracing log by issuing command ``uk
trace``. Alternatively, you can save all trace data into a binary file
with ``uk trace save <filename>``. This tracefile can be processed
later offline using the trace.py script: ::

  support/scripts/uk_trace/trace.py list <filename>

Which brings us to the second option. Trace.py can run gdb and fetch
the tracefile for you. Just run: ::

  support/scripts/uk_trace/trace.py fetch  <your_unikraft_image>.dbg

.. note:: The *.dbg image is required, as it have offline data needed
          for parsing the trace buffer.

----------------------------
Adding your tracepoints
----------------------------
Bellow is a snippet for using tracepoints:

.. code-block:: c

  UK_TRACEPOINT(trace_vfs_open, "\"%s\" 0x%x 0%0o", const char*, int, mode_t);
  int open(const char *pathname, int flags, ...)
  {
  	trace_vfs_open(pathname, flags, mode);

  	/* lots of cool stuff */

  	return 0;
  }

Macro ``UK_TRACEPOINT(trace_name, fmt, type1, type2, ... typeN)``
generates a static function `trace_name()`, accepting N parameters, of
types **type1**, **type2** and so on. Up to 7 parameters supported. The
**fmt** is a printf-style format which will be used to form a message
corresponding to the trace sample.

The **fmt** is static and stored offline. Only parameters values are
saved on the trace buffer. It is the job of the offline parser to
match them together and print out resulting messages.

Now you can call the generated function from the point of
interest. You are expected to call one tracepoint from exactly one
place in your code.

----------------------------
Troubleshooting
----------------------------
If you are getting a message::

  Error getting the trace buffer. Is tracing enabled?

This might be because:

1. Because you indeed need to enable tracing

2. Not a single tracepoint has been called, and dead-code elimination
   removed (rightfully) the tracing functionality
