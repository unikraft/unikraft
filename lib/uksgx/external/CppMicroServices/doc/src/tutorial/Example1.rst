.. _example1:

Example 1 - Service Event Listener
==================================

This example creates a simple bundle that listens for service events.
This example does not do much at first, because it only prints out the
details of registering and unregistering services. In the next example
we will create a bundle that implements a service, which will cause this
bundle to actually do something. For now, we will just use this example
to help us understand the basics of creating a bundle and its activator.

A bundle gains access to the C++ Micro Services API using a unique
instance of :any:`cppmicroservices::BundleContext`.
In order for a bundle to get its unique bundle context, it must call
:any:`GetBundleContext() <cppmicroservices::GetBundleContext>` or
implement the :any:`cppmicroservices::BundleActivator` interface.
This interface has two methods, ``Start()`` and ``Stop()``, that both
receive the bundle's context and are called when the bundle is started
and stopped, respectively.

In the following source code, our bundle implements the ``BundleActivator``
interface and uses the context to add itself as a listener for service
events (in the ``eventlistener/Activator.cpp`` file):

.. literalinclude:: eventlistener/Activator.cpp
   :language: cpp
   :start-after: [Activator]
   :end-before: [Activator]

After implementing the C++ source code for the bundle activator, we must
*export* the activator as shown in the last line above. This ensures
that the C++ Micro Services library can create an instance of the activator
and call the ``Start()`` and ``Stop()`` methods.

After implementing the source code for the bundle, we must also define a
manifest file that contains meta-data needed by the C++ Micro Services
framework for manipulating the bundle. The manifest is embedded in the
shared library along with the compiled source code. We create a file
called ``manifest.json`` that contains the following:

.. literalinclude:: eventlistener/resources/manifest.json
   :language: json

Next, we need to compile the source code. This example uses CMake as the
build system and the top-level CMakeLists.txt file could look like this:

.. literalinclude:: ../CMakeLists.txt
   :language: cmake
   :end-before: [fnc-end]
   :append: add_subdirectory(eventlistener)

and the CMakeLists.txt file in the eventlistener subdirectory is:

.. literalinclude:: eventlistener/CMakeLists.txt
   :language: cmake

The call to :any:`usFunctionGenerateBundleInit` creates required callback
functions to be able to manage the bundle within the C++ Micro Services
runtime. If you are not using CMake, you have to place a macro call to
:any:`CPPMICROSERVICES_INITIALIZE_BUNDLE` yourself into the bundle's
source code, e.g. in ``Activator.cpp``. Have a look at
:any:`build-instructions` for more details about using CMake or other
build systems (e.g. Makefiles) when writing bundles.

To run the examples contained in the C++ Micro Services library, we use
a small driver program called ``usTutorialDriver``::

   CppMicroServices-build> bin/usTutorialDriver
   > h
   h                    This help text
   start <id | name>    Start the bundle with id <id> or name <name>
   stop <id | name>     Stop the bundle with id <id> or name <name>
   status               Print status information
   shutdown             Shut down the framework

Typing :kbd:`status` at the command prompt lists all installed bundles and
their current state. Note that the driver program pre-installs the
example bundles, so they will be listed initially with the ``INSTALLED``
state. To start the eventlistener bundle, type ``start eventlistener``
at the command prompt::

   > status
   Id | Symbolic Name        | State
   -----------------------------------
    0 | system_bundle        | ACTIVE
    1 | eventlistener        | INSTALLED
    2 | dictionaryservice    | INSTALLED
    3 | frenchdictionary     | INSTALLED
    4 | dictionaryclient     | INSTALLED
    5 | dictionaryclient2    | INSTALLED
    6 | dictionaryclient3    | INSTALLED
    7 | spellcheckservice    | INSTALLED
    8 | spellcheckclient     | INSTALLED
   > start eventlistener
   Starting to listen for service events.
   >

The above command started the eventlistener bundle (implicitly loading
its shared library). Keep in mind, that this bundle will not do much at
this point since it only listens for service events and we are not
registering any services. In the next example we will register a service
that will generate an event for this bundle to receive. To exit the
``usTutorialDriver``, use the :kbd:`shutdown` command.
