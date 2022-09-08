.. _concept-resources:

The Resource System
===================

The C++ Micro Services library provides a generic resource system that allows you to:

- Embed resources in a bundle.
- Access resources at runtime.

The features and limitations of the resource system are described in more detail in
the following sections.


Embedding Resources in a Bundle
-------------------------------

Resources are embedded into a bundle's shared or static library (or into
an executable) by using the :program:`usResourceCompiler3` executable. It will
create a ZIP archive of all input files and can append it to the bundle
file with a configurable compression level. See :any:`usResourceCompiler3(1)`
for the command line reference. 


Accessing Resources at Runtime
------------------------------

Each bundle provides individual resource lookup and access to its embedded 
resources via the :any:`Bundle <cppmicroservices::Bundle>` class which provides methods
returning :any:`BundleResource <cppmicroservices::BundleResource>`
objects. The BundleResource class provides a high-level API for accessing
resource information and traversing the resource tree.

The :any:`BundleResourceStream <cppmicroservices::BundleResourceStream>`
class provides a ``std::istream`` compatible object for the seamless usage of
embedded resource data in third-party libraries.

Resources are managed in a tree hierarchy, modeling the original parent-child
relationship on the file-system.

The following example shows how to retrieve a resource from each
currently installed bundle whose path is specified by a bundle property:

.. literalinclude:: snippets/uServices-resources/main.cpp
   :language: cpp
   :start-after: [2]
   :end-before: [2]

This example could be enhanced to dynamically react to bundles being
started and stopped, making use of the popular *extender pattern* from
OSGi.

Runtime Overhead
----------------

The resource system has the following runtime characteristics:

-  During bundle install, the bundle's ZIP archive header data (if available)
   is parsed and stored in memory.
-  Querying ``Bundle`` or ``BundleResource`` objects for resource
   information will not extract the embedded resource data and hence
   only has minimal runtime and memory overhead.
-  Creating a ``BundleResourceStream`` object will allocate memory for
   the uncompressed resource data and inflate it. The memory will be
   free'ed after the ``BundleResourceStream`` object is destroyed.
   
Conventions and Limitations
---------------------------
 
-  Resources have a size limitation of 2GB due to the use of the ZIP format.
-  Resource entries are stored with case-insensitive names. On
   case-sensitive file systems, adding resources with the same name but
   different capitalization will lead to an error.
-  Looking up resources by name at runtime *is* case sensitive.
-  The CppMicroServices library will search for a valid zip file inside
   a shared library, starting from the end of the file. If other zip
   files are embedded in the bundle as well (e.g. as an additional
   resource embedded via the Windows RC compiler or using other
   techniques), it will stop at the first valid zip file and use it as
   the resource container.
