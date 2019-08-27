*****************************
Internal Platform Development
*****************************
Platforms (e.g., Xen, KVM, Linux user-space, etc.) are also
treated as libraries in Unikraft but there are a few differences:

1. You need to namespace any variables in Makefile.uk with
   ``LIB[PLATNAME]PLAT``, e.g., ``LIBKVMPLAT``.

2. You need to register the platform **and** the library with Unikraft
   using the following commands: ::

     $(eval $(call addplat_s,platname,$(CONFIG_PLAT_PLATNAME)))
     $(eval $(call addplatlib,platname,libplatnameplat))

3. You need to provide a linker script and name the file ``Linker.uk``.

4. The default linker script for a platform is provided using the
   ``UK_PLAT_PLATNAME_DEF_LDS`` variable in the Makefile.uk of the platform
   library. The default linker script is also added as a source file to the
   platform library to be built. If the default linker script is not provided,
   then Unikraft would rely on the default linker script provided by the
   compiler tool chain.

5. You need to place all platform files in the Unikraft repo under
   ``plat/platname/``.

6. A platform have to implement interfaces defined in ``include/uk/plat``
   (this is analogue to architectures that have to implement interfaces in
   ``include/uk/arch``)

7. They do not use any external source files, i.e., all source code is
   within the Unikraft tree.

8. They must not have dependencies on external libraries, i.e., the
   Unikraft repo must be able to be built on its own. Remember that
   for such builds, ``libnolibc`` has to be sufficient ``libc`` replacement
   to compile, link, and execute internal libraries. This means that nolibc
   has to be extended from time to time.

9. All changes/additions to ``include/uk/plat`` and ``include/uk/arch``
   have to be completely independent of any library (internal and external).
   They do not include any header provided by any library and never conflict
   with any library. Most of the times this is challenging for defining data
   types and structs. We use the same style as Linux kernel uses for kernel-
   internal types: double-underscore in front of the type. You can base
   your data types and function prototypes on ``include/arch/types.h``

Please refer to the existing platforms to have a fuller idea of how
platform libraries are implemented, and in particular what the syntax
of the linker script should be.
