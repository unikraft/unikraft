.. _concept-static-bundles:

Static Bundles
==============

The normal and most flexible way to add a CppMicroServices bundle to
an application is to compile it into a shared library using the :any:`BundleContext::InstallBundles() <cppmicroservices::BundleContext::InstallBundles>` function at runtime.

However, bundles can be linked statically to your application or shared
library. This makes the deployment of your application less error-prone
and in the case of a complete static build, also minimizes its binary
size and start-up time. However, in order to add new functionality to
your application, you must rebuild and redistribute it.


Creating Static Bundles
-----------------------

Static bundles are written just like shared bundles - there are no
differences in the usage of the CppMicroServices API or the provided
preprocessor macros.

Using Static Bundles
--------------------

Static bundles can be used (imported) in shared or other static
libraries, or in the executable itself. For every static bundle you would
like to import, you need to add a call to
:any:`CPPMICROSERVICES_IMPORT_BUNDLE` or to
:any:`CPPMICROSERVICES_INITIALIZE_STATIC_BUNDLE` (if the bundle does not
provide an activator) in the source code of the importing library.

.. note::

   While you can link static bundles to other static bundles, you will
   still need to import *all* of the static bundles into the final
   executable to ensure proper initialization.

The two main usage scenarios- using a shared or static CppMicroServices library- 
are explained in the sections below.

Using a Shared CppMicroServices Library
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Building the CppMicroServices library as a shared library allows you to
import static bundles into other shared or static bundles, or into the
executable.

.. literalinclude:: snippets/uServices-staticbundles/main.cpp
   :caption: Example code for importing ``MyStaticBundle1`` into another library or executable
   :language: cpp
   :start-after: ImportStaticBundleIntoMain
   :end-before: ImportStaticBundleIntoMain

Using a Static CppMicroServices Library
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The CppMicroServices library can be built as a static library. In that
case, creating shared bundles is not supported. If you create shared
bundles that link a static version of the CppMicroServices library, the
runtime behavior is undefined.

In this usage scenario, every bundle will be statically built and linked
to an executable:

.. literalinclude:: snippets/uServices-staticbundles/main.cpp
   :caption: Static bundles and CppMicroServices library
   :language: cpp
   :start-after: ImportStaticBundleIntoMain2
   :end-before: ImportStaticBundleIntoMain2

Note that the first :any:`CPPMICROSERVICES_IMPORT_BUNDLE` call imports the
static CppMicroServices library. Next, the ``MyStaticBundle2`` bundle is
imported and finally, the executable itself is initialized (this is
necessary if the executable itself is a C++ Micro Services bundle).
