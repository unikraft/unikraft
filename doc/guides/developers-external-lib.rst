****************************
External Library Development
****************************
Porting an external library (e.g., openssl) isn't too different from
porting an :doc:`application <developers-app>`: in this case, no
Makefile is needed, and Makefile.uk follows the same format described
above except that for naming ``lib`` is prefixed instead of ``app``
(``lib[name]`` instead of ``app[name]``; e.g., ``libnewlib`` for
``newlib``).

Another difference relates to Config.uk: You surround your settings with
``menuconfig`` that enables selecting and deselecting the library. The name of
this boolean setting should be ``LIB[NAME]``: ::

  menuconfig LIBNAME
  	bool "name: My lib"
  	select LIBNOLIBC if !HAVE_LIBC
  	select LIBUKALLOC
  	default n

  if LIBNAME

    ### Library settings go in here ###

  endif

Instead of defining an invisible setting for the dependencies, you define them
with ``select`` in the ``menuconfig`` option.
In general, you should provide a minimal configuration with the defaults. You
should depend options to feature flags if the option requires it.
This also means that the library should be off on default.

Since the library can be switched on and off for the build, you should use the
following registration command for libraries instead (Makefile.uk): ::

  $(eval $(call addlib_s,libname,$(CONFIG_LIBNAME)))

Among the same set of reserved of variable names, libraries most likely forward
some includes and flags to the whole build, meaning also to other libraries and
the application. ::

  CFLAGS CFLAGS-y           - Global flags for C files
  CXXFLAGS CXXFLAGS-y       - Global flags for C++ files
  ASFLAGS ASFLAGS-y         - Global flags for assembly files
  CINCLUDES CINCLUDES-y     - Global includes for C files
  CXXINCLUDES CXXINCLUDES-y - Global includes for C++ files
  ASINCLUDES ASINCLUDES-y   - Global includes for assembly files

We recommend to set up a dummy application while porting or developing a
library. This application should depends on your library so that we can drive
the build process with it. Follow the instructions in the user's guide to see
how to go about this.
