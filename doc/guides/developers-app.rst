************************************
Application Development and Porting
************************************
Porting an application to Unikraft should be for the most part
relatively painless given that the available Unikraft libraries
provide all of the application's dependencies. In most cases, the
porting effort requires no changes (or in the worst case small
patches) to the actual application code. At a high level, most of the
work consists of creating a Unikraft makefile called **Makefile.uk**
that Unikraft uses to compile the application's source code (i.e.,
generally we avoid using an application's native Makefile(s), if any,
and rely on Unikraft's build system to build the necessary objects with
correct and compatible compiler and linker flags).

In greater detail, in order for an application to work with Unikraft
you need to provide at least the following four files:

 * **Makefile**: Used to specify where the main Unikraft repo is with
   respect to the application's repo, as well as repos for any external
   libraries the application needs to use.

 * **Makefile.uk**: The main Makefile used to specify which sources to
   build (and optionally where to fetch them from), include paths, flags
   and any application-specific targets.

 * **Config.uk**: A Kconfig-like snippet used to populate Unikraft's
   menu with application-specific options.

 * **exportsyms.uk**: A text file where each line contains the name
   of one symbol that should be exported to other libraries. This file
   usually contains only `main` for an application that is developed/ported
   as a single library to Unikraft.

 * **extra.ld**: Optional. Contains an amendment to the main linker
   script

The Makefile is generally short and simple and might remind you to
Linux kernel modules that are built off-tree. For most applications
the Makefile should contain no more than the following: ::

  UK_ROOT  ?= $(PWD)/../../unikraft
  UK_LIBS  ?= $(PWD)/../../libs
  UK_PLATS ?= $(PWD)/../../plats
  LIBS  := $(UK_LIBS)/lib1:$(UK_LIBS)/lib2:$(UK_LIBS)/libN
  PLATS ?=

  all:
          @make -C $(UK_ROOT) A=$(PWD) L=$(LIBS) P=$(PLATS)

  $(MAKECMDGOALS):
	  @make -C $(UK_ROOT) A=$(PWD) L=$(LIBS) P=$(PLATS) $(MAKECMDGOALS)

We cover the format of the other two files in turn next, followed by
an explanation of the build process.

.. _lib-essential-files:

============================
Config.uk
============================
Unikraft's configuration system is based on Linux's KConfig system. With
Config.uk you define possible configurations for your application and define
dependencies to other libraries. This file is included by Unikraft to render the
menu-based configuration, and to load and store configurations (aka ``.config``).
Each setting that is defined in this file will be globally populated as set
variable for all ``Makefile.uk`` file, as defined macro in your source code when
you use ``"#include <uk/config.h>"``. Please ensure that all settings are
properly name spaced. They should begin with ``[APPNAME]_`` (e.g.,
``APPHELLOWORLD_``). Please also not that some variable names are predefined for
each application or library name space (see Makefile.uk).

We recommend as current best practice to begin the file with defining
dependencies with an invisible boolean option that is set to ``y``: ::

  ### Invisible option for dependencies
  config APPHELLOWORLD_DEPENDENCIES
  	bool
  	default y
  	# dependencies with `select`:
  	select LIBNOLIBC if !HAVE_LIBC
  	select LIB1
  	select LIB2

In this example, ``LIB1`` and ``LIB2`` would been enabled (the user can't
unselect them). Additionally, if the user did not provide and select any libc
the Unikraft internal replacement ``libnolibc`` would be selected. Of course
you could also directly define a dependency on a particular libc
(e.g., ``libnewlib``), instead.
You can also depend on feature flags (like ``HAVE_LIBC``) to provide or hide
options. The feature flag ``HAVE_LIBC`` in this example is set as soon as a
proper and full-fledged libc was selected by the user. You can get an overview
of available feature flags in ``libs/Config.uk``.

Any other setting of your application can be a type of boolean, int, or string.
You are even able to define dropdown-selections. You can find a documentation of
the syntax at the Linux kernel tree:
https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/plain/Documentation/kbuild/kconfig-language.txt
Please note that Unikraft does not have tristates (the ``m`` option is not
available). ::

  ### App configuration
  config APPHELLOWORLD_OPTION
  	bool "Boolean Option 1"
  	default y
  	help
  	  Help text of Option 1

============================
Makefile.uk
============================
The Unikraft build system provides a number of functions and macros to
make it easier to write the Makefile.uk file. Also, please ensure that
all variables that you define begin with ``APP[NAME]_`` (e.g.,
``APPHELLOWORLD_``) so that they are properly namespaced.

The first thing to do is to call the Unikraft ``addlib`` function to
register the application with the system (note the letters "lib":
everything in Unikraft is ultimately a library). This function call
will also populate ``APPNAME_BASE`` (containing the directory path to
the application sources) and ``APPNAME_BUILD`` (containing the directory path to
the application's build directory): ::

  $(eval $(call addlib,appname))

where ``name`` would be replaced by your application's name. In case your
main application code should be downloaded as archive from a remote server, the
next step is to set up a variable to point to a URL with the
application sources (or objects, or pre-linked libraries, see further
below) - if required. and to call Unikraft's ``fetch`` command to download and
uncompress those sources (``APPNAME_ORIGIN`` is populated containing the
directory path to the extracted files) ::

  APPNAME_ZIPNAME=myapp-v0.0.1
  APPNAME_URL=http://app.org/$(APPNAME_ZIPNAME).zip
  $(eval $(call fetch,appname,$(APPNAME_URL)))

Next we set up a call to use Unikraft's patch functionality. Even if
you don't have any patches yet, it's good to have this set up in case
you need it later::

  APPNAME_PATCHDIR=$(APPNAME_BASE)/patches
  $(eval $(call patch,appname,$(APPNAME_PATCHDIR),$(APPNAME_ZIPNAME)))

With all of this in place, you can already start testing things out: ::

  make menuconfig
  [choose appropriate options and save configuration, see user's guide]
  make

You should see Unikraft downloading your sources, uncompressing them
and doing the same plus building for any libraries you might have
specified in the ``Makefile`` or through the menu (there'll be nothing
to build for your application yet as we haven't yet specified any
sources to build). When building, Unikraft creates a ``build``
directory and places all temporary and object files under it; the
application's sources are placed under
``build/origin/[tarballname]/``.

To tell Unikraft which source files to build we add files to the
``APPNAME_SRCS-y`` variable: ::

  APPNAME_SRCS-y += $(APPNAME_BASE)/path_to_src/myfile.c

For source files, Unikraft so far supports C (``.c``), C++ (``.cc``) and
assembly files (``.S``).

In case you have pre-compiled object files, you could add them with
(but due to possible incompatible compilation flags of your pre-compiled object
files, you should handle this with care): ::

  APPNAME_OBJS-y += $(APPNAME_BASE)/path_to_src/myobj.o

You can also use ``APPNAME_OBJS-y`` to add pre-built libraries (as .o or .a): ::

  APPNAME_OBJS-y += $(APPNAME_BASE)/path_to_lib/mylib.a

Once you have specified all of your source files (and optionally binary files)
it is generally also necessary to specify include paths and compile flags: ::

  # Include paths
  APPNAME_ASINCLUDES  += -I$(APPNAME_BASE)/path_to_include/include [for assembly files]
  APPNAME_CINCLUDES   += -I$(APPNAME_BASE)/path_to_include/include [for C files]
  APPNAME_CXXINCLUDES += -I$(APPNAME_BASE)/path_to_include/include [for C++ files]

  # Flags for application sources
  APPNAME_ASFLAGS-y   += -DFLAG_NAME1 ... -DFLAG_NAMEN [for assembly files]
  APPNAME_CFLAGS-y    += -DFLAG_NAME1 ... -DFLAG_NAMEN [for C files]
  APPNAME_CXXFLAGS-y  += -DFLAG_NAME1 ... -DFLAG_NAMEN [for C++ files]

With all of this in place, you can save ``Makefile.uk``, and type
``make``. Assuming that the chosen Unikraft libraries provide all of
the support that your application needs, Unikraft should compile and
link everything together, and output one image per target platform
specified in the menu.

In addition to all the functionality mentioned, applications might need to
perform a number of additional tasks after the sources are downloaded and
decompressed but *before* the compilation takes place (e.g., run a configure
script or a custom script that generates source code from source files).
To support this, Unikraft provides a "prepare" variable which you can set to a
temporary marker file and from there to a target in your Makefile.uk file.
For example: ::

  $(APPNAME_BUILD)/.prepared: [dependencies to further targets]
         cmd && $(TOUCH) $@

  UK_PREPARE += $(APPNAME_BUILD)/.prepared

In this way, you ensure that ``cmd`` is run before any compilation
takes place. If you use ``fetch``, add ``$(APPNAME_BUILD)/.origin``
as dependency. If you used ``patch`` then add ``$(APPNAME_BUILD)/.patched``
instead.

Further, you may find it necessary to specify compile flags or includes only
for a *specific* source file. Unikraft supports this through the following
syntax: ::

  APPNAME_SRCS-y += $(APPNAME_BASE)/filename.c
  APPNAME_FILENAME_FLAGS-y += -DFLAG
  APPNAME_FILENAME_INCLUDES-y += -Iextra/include

It is also be possible compile a single source files multiple times with
different flags. For this case, Unikraft supports variants: ::

  APPNAME_SRCS-y += $(APPNAME_BASE)/filename.c|variantname
  APPNAME_FILENAME_VARIANTNAME_FLAGS-y += -DFLAG2
  APPNAME_FILENAME_VARIANTNAME_INCLUDES-y += -Iextra/include

.. note:: The build system treats the reserved ``isr`` variant specially:
	  This variant is intended for build units that contain code that can
	  also be called from interrupt context. Separate global
	  architecture flags are used to generate interrupt-safe code
	  (``ISR_ARCHFLAGS-y`` instead of ``ARCHFLAGS-y``). Generally, these
	  flags avoid the use of extended machine units which aren't saved by the
	  processor before entering interrupt context (e.g., floating point
	  units, vector units).

Finally, you may also need to provide "glue" code, for instance to
implement the ``main()`` function that Unikraft expects you to
implement by calling your application's main or init routines. As a
rule of thumb, we suggest to place any such files in the application's
main directory (``APPNAME_BASE``), and any includes they may depend
on under ``APPNAME_BASE/include``. And of course don't forget to
add the source files and include path to Makefile.uk.

To see full examples of Makefile.uk files you can browse the available
external applications or library repos.

Reserved variable names in the name scope are so far: ::

  APPNAME_BASE                              - Path to source base
  APPNAME_BUILD                             - Path to target build dir
  APPNAME_EXPORTS                           - Path to the list of exported symbols
                                              (default is '$(APPNAME_BASE)/exportsyms.uk')
  APPNAME_ORIGIN                            - Path to extracted archive
                                              (when fetch or unarchive was used)
  APPNAME_CLEAN APPNAME_CLEAN-y             - List of files to clean additional
                                              on make clean
  APPNAME_SRCS APPNAME_SRCS-y               - List of source files to be
                                              compiled
  APPNAME_OBJS APPNAME_OBJS-y               - List of object files to be linked
                                              for the library
  APPNAME_OBJCFLAGS APPNAME_OBJCFLAGS-y     - link flags (e.g., define symbols
                                              as internal)
  APPNAME_CFLAGS APPNAME_CFLAGS-y           - Flags for C files of the library
  APPNAME_CXXFLAGS APPNAME_CXXFLAGS-y       - Flags for C++ files of the library
  APPNAME_ASFLAGS APPNAME_ASFLAGS-y         - Flags for assembly files of the
                                              library
  APPNAME_CINCLUDES APPNAME_CINCLUDES-y     - Includes for C files of the
                                              library
  APPNAME_CXXINCLUDES APPNAME_CXXINCLUDES-y - Includes for C++ files of the
                                              library
  APPNAME_ASINCLUDES APPNAME_ASINCLUDES-y   - Includes for assembly files of
                                              the library
  APPNAME_FILENAME_FLAGS                    - Flags for a *specific* source file
  APPNAME_FILENAME_FLAGS-y                    of the library (not exposed to its
                                              variants)
  APPNAME_FILENAME_INCLUDES                 - Includes for a *specific* source
  APPNAME_FILENAME_INCLUDES-y                 file of the library (not exposed
                                              to its variants)
  APPNAME_FILENAME_VARIANT_FLAGS            - Flags for a *specific* source file
  APPNAME_FILENAME_VARIANT_FLAGS-y            and variant of the library
  APPNAME_FILENAME_VARIANT_INCLUDES         - Includes for a *specific* source
  APPNAME_FILENAME_VARIANT_INCLUDES-y         file and variant of the library


============================
exportsyms.uk
============================
Unikraft provides separate namespaces for each library. This means that
every function and variable will only be visible and linkable internally.

To make a symbol visible for other libraries, add it to this
``exportsyms.uk`` file. It is simply a flat file, with one symbol name per
line. Line comments may be introduced by the hash character ('#'). This
option may be given more than once.

If you are writing an application, you need to add your program entry point
to this file (this is ``main`` if you use ``libukboot``). Most likely nothing
else should be there. For a library, all external API functions must be listed.

For the sake of file structure consistency, it is not recommended to
change the default path of this symbols file, unless it is really necessary
(e.g., multiple libraries are sharing the same base folder, this symbols file
is part of a remotely fetched archive). You can override it by defining the
``APPNAME_EXPORTS`` variable. The path must be either absolute (you can refer
with ``$(APPNAME_BASE)`` to the base directory of your application sources) or
relative to the Unikraft sources directory.

============================
extra.ld
============================
If your library/application needs a section in the final elf, edit
your Makefile.uk to add ::

    LIBYOURAPPNAME_SRCS-$(CONFIG_LIBYOURAPPNAME) += $(LIBYOURAPPNAME_BASE)/extra.ld

An example context of extra.ld: ::

    SECTIONS
    {
        .uk_fs_list : {
             PROVIDE(uk_fslist_start = .);
             KEEP (*(.uk_fs_list))
             PROVIDE(uk_fslist_end = .);
        }
    }
    INSERT AFTER .text;

This will add the section .uk_fs_list after the .text

============================
Syscall shim layer
============================

The system call shim layer (``lib/syscall_shim``) provides Linux-style mappings
of system call numbers to actual system call handler functions. You can
implement a system call handler by using one of the definition macros
(``UK_SYSCALL_DEFINE``, ``UK_SYSCALL_R_DEFINE``) and register the system
call by adding it to ``UK_PROVIDED_SYSCALLS-y`` within your ``Makefile.uk``.

The shim layer library supports two implementation variants for system call
handlers:

(1) libc-style: The function implementation returns ``-1`` and sets ``errno``
    in case of errors

(2) and raw: The function implementation returns a negative error value in case
    of errors. ``errno`` is not used at all.

Because of library internals, each system call implementation needs to be
provided with both variants. The build option `Drop unused functions and data`
is making sure that only the variants are compiled-in that are actually in use.

You can use helper macros in order to implement the call just once. The first
variant can be implemented with the ``UK_SYSCALL_DEFINE`` macro:

.. code-block:: c

    UK_SYSCALL_DEFINE(return_type, syscall_name, arg1_type, arg1_name,
                                                 arg2_type, arg2_name, ..)
    {
        /* ... */
    }

Example:

.. code-block:: c

    #include <uk/syscall.h>

    UK_SYSCALL_DEFINE(ssize_t, write, int, fd, const void *, buf, size_t, count)
    {
        ssize_t ret;

        ret = vfs_do_write(fd, buf, count);
        if (ret < 0) {
            errno = EFAULT;
            return -1;
        }
        return ret;
    }


Raw implementations should use the ``UK_SYSCALL_R_DEFINE`` macro:

.. code-block:: c

    UK_SYSCALL_R_DEFINE(return_type, syscall_name, arg1_type, arg1_name,
                                                   arg2_type, arg2_name, ..)
    {
        /* ... */
    }

Example:

.. code-block:: c

    #include <uk/syscall.h>

    UK_SYSCALL_R_DEFINE(ssize_t, write, int, fd, const void *, buf, size_t, count)
    {
        ssize_t ret;

        ret = vfs_do_write(fd, buf, count);
        if (ret < 0) {
            return -EFAULT;
        }
        return ret;
    }

Please note that in the raw case (``UK_SYSCALL_R_DEFINE``), errors are always
returned as negative value. Whenever the return type is a pointer value, the
helpers defined in `<uk/errptr.h>` can be used to forward error codes.

Both macros create the following three symbols:

.. code-block:: c

    /* libc-style system call that returns -1 and sets errno on errors */
    long uk_syscall_e_<syscall_name>(long <arg1_name>, long <arg2_name>, ...);

    /* Raw system call that returns negative error codes on errors */
    long uk_syscall_r_<syscall_name>(long <arg1_name>, long <arg2_name>, ...);

    /* libc-style wrapper (the same as uk_syscall_e_<syscall_name> but with actual types) */
    <return_type> <syscall_name>(<arg1_type> <arg1_name>,
                                 <arg2_type> <arg2_name>, ...);

For the case that the libc-style wrapper does not match the signature and return
type of the underlying system call, a so called low-level variant of these two
macros are available: ``UK_LLSYSCALL_DEFINE``, ``UK_LLSYSCALL_R_DEFINE``.
These macros only generate the ``uk_syscall_e_<syscall_name>`` and
``uk_syscall_r_<syscall_name>`` symbols. You can then provide the custom
libc-style wrapper on top:

.. code-block:: c

    #include <uk/syscall.h>

    UK_LLSYSCALL_R_DEFINE(ssize_t, write, int, fd, const void *, buf, size_t, count)
    {
        ssize_t ret;

        ret = vfs_do_write(fd, buf, count);
        if (ret < 0) {
            return -EFAULT;
        }
        return ret;
    }

    #if UK_LIBC_SYSCALL
    ssize_t write(int fd, const void *buf, size_t count)
    {
        return (ssize_t) uk_syscall_e_write((long) fd,
                                            (long) buf, (long) count);
    }
    #endif /* UK_LIBC_SYSCALL */

Note: Please note that the implementation of custom libc-style wrappers have to
be guarded with ``#if UK_LIBC_SYSCALL``. This macro is provided by the
``<uk/syscall.h>`` header. Some libC ports (e.g., musl) deactivate this option
whenever their provide own wrapper functions. For such cases, the syscall_shim
library will only provide the ``uk_syscall_e_<syscall_name>`` and
``uk_syscall_r_<syscall_name>`` symbols.

Note: When `syscall_shim` library is not enabled, the original design idea was
that the macros provide the libc-style wrapper only. However, all the
described macros are still available and populate the symbols as documented
here. This is done to support the case that a system call is implemented by
calling another.

If your library uses an ``exportsyms.uk`` file, you need to add the three
symbols for making them public available: ::

   uk_syscall_e_<syscallname>
   uk_syscall_r_<syscallname>
   <syscallname>

In our example: ::

   uk_syscall_e_write
   uk_syscall_r_write
   write

In order to register the system call to `syscall_shim`, add it to
``UK_PROVIDED_SYSCALLS-y`` with the library ``Makefile.uk``: ::

   UK_PROVIDED_SYSCALLS-$(CONFIG_<YOURLIB>) += <syscall_name>-<number_of_arguments>

The ``Makefile.uk`` snippet for our example: ::

   UK_PROVIDED_SYSCALLS-$(CONFIG_LIBWRITESYS) += write-3

==================================
Command line arguments in Unikraft
==================================
Both a Unikraft application or library within Unikraft may need to be
configured at instantiation time. Unikraft supports the ability to do
so via command line paramters: the arguments for a Unikraft library
come first, followed by the separator ``--``, followed by the
application arguments.

Type of parameters in a library
--------------------------------
Unikraft provides support to pass arguments of the following data
types:

========  ========================
Type      Description
========  ========================
char      Single character value; an alias for __s8.
__s8      Same as char
__u8      Single byte value
__s16     Short signed integer
__u16     Short unsigned integer
int       Integer; aan alias for __s32.
__s32     Signed integer
__u32     Unsigned integer
__s64     Signed long integer
__u64     Unsigned long integer
charp     C strings.
========  ========================

Register a library parameter with Unikraft
--------------------------------------------
In order for a library to configure options at execution time, it needs
to select the `uklibparam` library while configuring the Unikraft build.
The library should also be registered  with the `uklibparam` library using 
`addlib_paramprefix` in the Makefile.uk of your library.

There are three interfaces through which a library registers a variable as a
parameter; these are:

* UK_LIB_PARAM     - Pass a scalar value of the above type to a variable.
* UK_LIB_PARAM_STR - Pass a null terminated string to a variable.
* UK_LIB_PARAM_ARR - Pass a space-separated list of values of the above type.

Each library parameter is identified by the following format ::

 [library name].[variable name]

 where
 
     library name is the name registered with the Unikraft build system.
     variable name is the name of the global or static variable in the program.

Examples
--------
If the library needs to configure a variable at execution time, it needs some
configuration to be performed while building the library. A Unikraft library can
be specific to a particular platform or common across all platforms.
For the common library, one has to edit the Makefile.uk with

.. code-block:: bash

 $(eval $(call addlib_paramprefix,libukalloc,alloc))
 
 where
 
      libukalloc is the name of the library
      alloc is the alias for the library name.

As the next step, we define a variable and register it with the `uk_libparam`
library. The example below shows a simple code snippet.

.. code-block:: c

    static __u32 heap_size = CONFIG_LINUXU_DEFAULT_HEAPMB;
    UK_LIB_PARAM(heap_size, __u32);

We can override the default value using the following command line:

.. code-block:: bash

  ./unikraft_linuxu-x86_64 linuxu.heap_size=10 --

As a further example, we now show how to provide a parameter defined
as a string. We define a char pointer pointing to a default value and
register it with the `uk_libparam` library using the UK_LIB_PARAM_STR
helper function:

.. code-block:: c

    static const char *test_string = "Hello World";
    UK_LIB_PARAM_STR(test_string);

We can override the default value using the following command:

.. code-block:: bash

  ./unikraft_linuxu-x86_64 linuxu.test_string="Hello Unikraft!" --

The example below demonstrates how to pass a list of scalar datatype
as a parameter to a library. As in the previous example, we define an
array variable and register it with the `uk_libparam` library using
the UK_LIB_PARAM_ARR helper function.

.. code-block:: c

    static int test_array[5] = {0};
    UK_LIB_PARAM_ARR(test_array, int);

The elements in an array are delimited by ' ' :

.. code-block:: bash

  ./unikraft_linuxu-x86_64 linuxu.test_array="1 2 3 4 5" --

============================
Make Targets
============================
Unikraft provides a number of make targets to help you in porting and
developing applications and libraries. You can see a listing of them
by typing ``make help``; for convenience they're also listed here
below: ::

  Cleaning:
  clean-[LIBNAME]        - delete all files created by build for a single library
                           (e.g., clean-libfdt)
  clean                  - delete all files created by build for all libraries
                           but keep fetched files
  properclean            - delete build directory
  distclean              - delete build directory and configurations (including .config)

  Building:
  * all                  - build everything (default target)
  images                 - build kernel images for selected platforms
  libs                   - build libraries and objects
  [LIBNAME]              - build a single library
  objs                   - build objects only
  prepare                - run preparation steps
  fetch                  - fetch, extract, and patch remote code

  Configuration:
  * menuconfig           - interactive curses-based configurator
                           (default target when no config exists)
  nconfig                - interactive ncurses-based configurator
  xconfig                - interactive Qt-based configurator
  gconfig                - interactive GTK-based configurator
  oldconfig              - resolve any unresolved symbols in .config
  silentoldconfig        - Same as oldconfig, but quietly, additionally update deps
  olddefconfig           - Same as silentoldconfig but sets new symbols to their default value
  randconfig             - New config with random answer to all options
  defconfig              - New config with default answer to all options
                             UK_DEFCONFIG, if set, is used as input
  savedefconfig          - Save current config to UK_DEFCONFIG (minimal config)
  allyesconfig           - New config where all options are accepted with yes
  allnoconfig            - New config where all options are answered with no

  Miscellaneous:
  print-version          - print Unikraft version
  print-libs             - print library names enabled for build
  print-lds              - print linker script enabled for the build
  print-objs             - print object file names enabled for build
  print-srcs             - print source file names enabled for build
  print-vars             - prints all the variables currently defined in Makefile
  make V=0|1             - 0 => quiet build (default), 1 => verbose build


============================
Patch Creation
============================

Go to the directory containing sources of the application you are
porting (e.g. ``build/libnewlibc/origin``). Copy over the folder with
unmodified sources::

  cp -r newlib-2.5.0.20170922 newlib.orig

Do necessary modifications, test it and run ``diff`` tool::

  diff -urNp newlib.orig newlib-2.5.0.20170922 >
          LIBLIBNAME_BASE/patches/[nnnn]-[description].patch

Open the generated patch in your favorite editor and add a short
header to the patch. Start it with a ``From:`` field, and put your
name in it. On the next line add a one-liner description of the patch
in the ``Subject:`` filed. Optionally, write a little longer
description after an empty line. And, finally, add ``---`` line at the
end of the header.

This should help people to get an idea why does this patch
exist, and whom they should address questions. Header example::

  From: Zaphod Beeblebrox <z.beeblebrox@gmail.com>
  Subject: subject of an example patch

  This is an example patch description
  ---
  diff -urNp newlib.orig/ChangeLog newlib-2.5.0.20170922/ChangeLog

Or just use git to generate patches for you.
