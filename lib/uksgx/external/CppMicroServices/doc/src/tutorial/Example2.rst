.. _example2:

Example 2 - Dictionary Service Bundle
=====================================

This example creates a bundle that implements a service. Implementing a
service is a two-step process, first we must define the interface of the
service and then we must define an implementation of the service
interface. In this particular example, we will create a dictionary
service that we can use to check if a word exists, which indicates if
the word is spelled correctly or not. First, we will start by defining a
simple dictionary service interface in a file called
``dictionaryservice/IDictionaryService.h``:

.. literalinclude:: dictionaryservice/IDictionaryService.h
   :language: cpp
   :start-after: [service]
   :end-before: [service]

The service interface is quite simple, with only one method that needs
to be implemented. Because we provide an empty out-of-line destructor
(defined in the file ``IDictionaryService.cpp``) we must export the
service interface by using the bundle specific
``DICTIONARYSERVICE_EXPORT`` macro.

In the following source code, the bundle uses its bundle context to
register the dictionary service. We implement the dictionary service as
an inner class of the bundle activator class, but we could have also put
it in a separate file. The source code for our bundle is as follows in a
file called ``dictionaryservice/Activator.cpp``:

.. literalinclude:: dictionaryservice/Activator.cpp
   :language: cpp
   :start-after: [Activator]
   :end-before: [Activator]

Note that we do not need to unregister the service in the ``Stop()`` method,
because the C++ Micro Services library will automatically do so for us.
The dictionary service that we have implemented is very simple; its
dictionary is a set of only five words, so this solution is not optimal
and is only intended for educational purposes.

.. note::

   In this example, the service interface and
   implementation are both contained in one bundle which exports the
   interface class. However, service implementations almost never need to
   be exported and in many use cases it is beneficial to provide the
   service interface and its implementation(s) in separate bundles. In such
   a scenario, clients of a service will only have a link-time dependency
   on the shared library providing the service interface (because of the
   out-of-line destructor) but not on any bundles containing service
   implementations.

We must create a ``manifest.json`` file that contains the
meta-data for our bundle; the manifest file contains the following:

.. literalinclude:: dictionaryservice/resources/manifest.json
   :language: json

For an introduction how to compile our source code, see :any:`example1`.

After running the ``usTutorialDriver`` program we should make sure that
the bundle from Example 1 is active. We can use the :kbd:`status` shell
command to get a list of all bundles, their state, and their bundle
identifier number. If the Example 1 bundle is not active, we should
start the bundle using the :kbd:`start` command and the bundle's identifier
number or symbolic name that is displayed by the :kbd:`status` command. Now
we can start our dictionary service bundle by typing the
:kbd:`start dictionaryservice` command::

   CppMicroServices-build> bin/usTutorialDriver
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
   > start dictionaryservice
   Ex1: Service of type IDictionaryService registered.
   > status
   Id | Symbolic Name        | State
   -----------------------------------
    0 | system_bundle        | ACTIVE
    1 | eventlistener        | ACTIVE
    2 | dictionaryservice    | ACTIVE
    3 | frenchdictionary     | INSTALLED
    4 | dictionaryclient     | INSTALLED
    5 | dictionaryclient2    | INSTALLED
    6 | dictionaryclient3    | INSTALLED
    7 | spellcheckservice    | INSTALLED
    8 | spellcheckclient     | INSTALLED
   >

To stop the bundle, use the :kbd:`stop 2` command. If the bundle
from :any:`Example 1 <example1>` is still
active, then we should see it print out the details of the service event
it receives when our new bundle registers its dictionary service. Using
the ``usTutorialDriver`` commands ``stop`` and ``start`` we can stop and
start it at will, respectively. Each time we start and stop our
dictionary service bundle, we should see the details of the associated
service event printed from the bundle from Example 1. In
:any:`Example 3 <example3>`, we will create a client for our dictionary
service. To exit ``usTutorialDriver``, we use the :kbd:`shutdown` command.
