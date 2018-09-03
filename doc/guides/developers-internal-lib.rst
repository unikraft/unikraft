****************************
Internal Library Development
****************************
Unikraft libraries are no different than :doc:`external ones
<developers-external-lib>`, except for the fact that

1. they are part of the main Unikraft repo and live under the
   ``lib/name/`` directories.
2. they do not use any external source files, i.e., all source code is
   within that directory.
3. they must not have dependencies on external libraries, i.e., the
   Unikraft repo must be able to be built on its own. Remember that
   for such builds, ``libnolibc`` has to be sufficient ``libc`` replacement
   to compile, link, and execute internal libraries. This means that nolibc
   has to be extended from time to time.

Other than that the process of creating and adding a Unikraft lib is
the same as for external libraries.

============================
Unikraft APIs
============================
One important thing to point out regarding Unikraft internal libraries is
that for each "category" of library (e.g., memory allocators, schedulers,
device buses, network drivers, etc.) Unikraft defines (or will define)
an API that each library under that category must comply with. This is
so that it's possible to easily plug and play different libraries of a
certain type (e.g., using a co-operative scheduler or a pre-emptive one).

An API consists of a header file defining the actual API as well as an
implementation of some generic/compatibility functions, if any, that are
common to all libraries under a specific category. This functionality is
all wrapped in a Unikraft library and placed under ``lib/uk[category]``
(e.g., ``lib/ukalloc`` for the memory allocator API, ``lib/uknetdev``
for the network driver API). The directory structure for an API generally
looks as follows: ::

  ├── [category].c
  ├── Config.uk
  ├── include
  │   └── uk
  │       └── [category].h
  ├── Makefile.uk
  └── exportsyms.uk


The ``Config.uk``, ``Makefile.uk`` and ``exportsyms.uk`` files are
fairly straightforward. You can refer to an existing API in the repo
to see what they look like. Also check the corresponding sections
:ref:`Application Development and Porting <lib-essential-files>`

The header file contains the API itself
and follows a generic structure that all APIs should follow; please
refer to existing API header files if you'd like to see what these
look like (e.g., ``libukalloc``, ``libuksched``).

To implement a library that complies with an API, we begin by creating
a similar directory structure as above, using ``uk[category][name]``
for the library name (e.g., ``ukschedcoop`` for a co-operative
scheduler). In the ``Config.uk`` file, state that your library depends
on the library API. For instance, for a new scheduler you would write:
::

  config LIBUKSCHEDMYSCHED
  	bool "ukschemysched: my scheduler"
  	default n
  	select LIBUKALLOC
  	select LIBUKSCHED

Like other Unikraft libraries, you'll need a simple ``Makefile.uk``: ::

  $(eval $(call addlib_s,libucschedmysched,$(CONFIG_LIBUKSCHEDMYSCHED)))
  CINCLUDES-$(LIBUKSCHEDMYSCHED)     += -I$(LIBUKSCHEDMYSCHED_BASE)/include
  CXXINCLUDES-$(LIBUKSCHEDMYSCHED)   += -I$(LIBUKSCHEDMYSCHED_BASE)/include
  LIBUKSCHEDMYSCHED_SRCS-y += $(LIBUKSCHEDMYSCHED_BASE)/mysched.c

The header file does little else than include the API header file and
define an init function: ::

  #include <uk/sched.h>
  #include <uk/alloc.h>
  struct uk_sched *uc_schedmysched_init(struct uk_alloc *a);

Finally the C file implements this init function and any other
functions required by the API. The init function uses the
``uk_sched_init`` macro defined in the API header file to register its
functions, e.g.,: ::

  struct uk_sched *uc_schedmysched_init(struct uk_alloc *a)
  {
      struct uk_sched *a;
      ...
      uk_sched_init(a, uc_schedmysched_schedule, ..., uc_schedmysched_get_idle);
      ...
      return a;
  }

And that's it, you should now be able to build your new library just
like any other in Unikraft (i.e., you'll need to create an application,
to select the library in the menu, and to type ``make``).
