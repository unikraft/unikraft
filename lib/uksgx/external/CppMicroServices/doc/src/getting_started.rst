.. _getting-started:

Getting Started
===============

This section gives you a quick tour of the most important concepts in
order to get you started with developing bundles and publishing and
consuming services.

We will define a service interface, service implementation, and service consumer. A
simple executable then shows how to install and start these bundles.

The complete source code for this example can be found at
*/doc/src/examples/getting_started*. It works for both shared and
static builds, but only the more commonly used shared build mode
is discussed in detail below.

The Build System
----------------

These examples come with a complete ``CMakeLists.txt`` file showing
the main usage scenarios of the provided :any:`CMake helper functions
<cmake-support>`. The script requires the *CppMicroServices*
package:

.. literalinclude:: examples/getting_started/CMakeLists.txt
   :language: cmake
   :start-after: [proj-begin]
   :end-before: [proj-end]

A Simple Service Interface
--------------------------

Services implement one or more *service interfaces*. An interface can
be any C++ class, but typically contains only pure virtual
functions. For our example, we create a separate library containing
a service interface that allows us to retrieve the number of elapsed
milliseconds since the POSIX epoch:

.. literalinclude:: examples/getting_started/service_time/ServiceTime.h
   :language: cpp

The CMake code does not require C++ Micro Services specific additions:

.. literalinclude:: examples/getting_started/CMakeLists.txt
   :language: cmake
   :start-after: [interface-begin]
   :end-before: [interface-end]

Bundle and BundleContext
------------------------

A *bundle* is the logical set of C++ Micro Services specific initialization
code, metadata stored in a *manifest.json*
:any:`resource <concept-resources>` file, and other resources and code.
Multiple bundles can be part of the same or different (shared
or static) library or executable.
To create the bundle initialization code, you can either use the
:any:`usFunctionGenerateBundleInit` CMake function or the
:any:`CPPMICROSERVICES_INITIALIZE_BUNDLE` macro directly.

In order to publish and consume a service, we need a :any:`BundleContext
<cppmicroservices::BundleContext>` instance, through which a bundle
accesses the C++ Micro Services API. Each bundle is associated with
a distinct bundle context that is accessible from anywhere in the bundle
via the :any:`GetBundleContext() <cppmicroservices::GetBundleContext>`
function:

.. code-block:: cpp

   #include <cppmicroservices/GetBundleContext.h>

   void Dummy()
   {
     auto context = cppmicroservices::GetBundleContext();
   }

Please note that trying to use ``GetBundleContext()``
without proper initialization code in the using library will lead to compile or runtime errors.

Publishing a Service
--------------------

Publishing a service is done by calling the :any:`BundleContext::RegisterService
<cppmicroservices::BundleContext::RegisterService>` function. The
following code for the *service_time_systemclock*
bundle implements the ``ServiceTime`` interface as a service:

.. literalinclude:: examples/getting_started/service_time_systemclock/ServiceTimeImpl.cpp
   :language: cpp
   :end-before: [no-cmake]

A ``std::shared_ptr`` holding the service object is passed as the
an argument to the ``RegisterService<>()``
function within a :any:`bundle activator <cppmicroservices::BundleActivator>`. The
service is registered as long as it is explicitly unregistered or the bundle is stopped.
The bundle activator is optional, but if it is declared, its
:any:`BundleActivator::Start(BundleContext) <cppmicroservices::BundleActivator::Start>`
and
:any:`BundleActivator::Stop(BundleContext) <cppmicroservices::BundleActivator::Stop>`
functions are called when the bundle is :any:`started <cppmicroservices::Bundle::Start>`
or :any:`stopped <cppmicroservices::Bundle::Stop>`, respectively. 

The CMake code for creating our bundle looks like this:

.. literalinclude:: examples/getting_started/CMakeLists.txt
   :language: cmake
   :start-after: [publisher-begin]
   :end-before: [publisher-end]

In addition to the generated bundle initialization code, we need to specify a unique
bundle name by using the ``US_BUNDLE_NAME`` compile definition as shown above.

We also need to provide the ``manifest.json`` file, which is added as a resource and
contains the following JSON data:

.. literalinclude:: examples/getting_started/service_time_systemclock/manifest.json
   :language: json

Because our bundle provides an activator, we also need to state its existence by
setting the ``bundle.activator`` key to  ``true``. The last two elements are purely
informational and not used directly.

Consuming a Service
-------------------

The process to consume a service is very similar to the process for publishing a service, except that consumers
need to handle some additional error cases. 

Again, we use a bundle activator to execute code on bundle start that retrieves and consumes a *ServiceTime* service:

.. literalinclude:: examples/getting_started/service_time_consumer/ServiceTimeConsumer.cpp
   :language: cpp
   :end-before: [no-cmake]

Because the C++ Micro Services is a dynamic environment, a particular service might
not be available yet. Therefore, we first need to check the validity of some returned
objects.

The above code would be sufficient only in the simplest use cases. To avoid bundle
start ordering problems (e.g. one bundle assuming the existence of a service
published by another bundle), a :any:`ServiceTracker <cppmicroservices::ServiceTracker>`
should be used instead. Such a tracker allows bundles to react on service events and in turn be more robust.

The CMake code for creating a library containing the bundle is very similar to
the code for the publishing bundle and thus not included here.

Installing and Starting Bundles
-------------------------------

The two bundles above are embedded in separate libraries and need to be
installed into a :any:`Framework <cppmicroservices::Framework>` and started.
This is done by a small example program:

.. literalinclude:: examples/getting_started/main.cpp
   :language: cpp

The program expects a list of file system paths pointing to installable libraries.
It will first construct a new ``Framework`` instance and then
:any:`install <cppmicroservices::BundleContext::InstallBundles>` the given
libraries. Next, it will start all available bundles.

When the ``Framework`` instance is destroyed, it will automatically shut itself down, essentially stopping all active bundles.

.. seealso::

   A more detailed :any:`tutorial <tutorial>` demonstrating some more advanced
   features is also available.
