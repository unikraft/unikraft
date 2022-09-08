.. _example6:

Example 6 - Spell Checker Service Bundle
========================================

In this example, we complicate things further by defining a new service
that uses an arbitrary number of dictionary services to perform its
function. More precisely, we define a spell checker service which will
aggregate all dictionary services and provide another service that
allows us to spell check passages using our underlying dictionary
services to verify the spelling of words. Our bundle will only provide
the spell checker service if there are at least two dictionary services
available. First, we will start by defining the spell checker service
interface in a file called ``spellcheckservice/ISpellCheckService.h``:

.. literalinclude:: spellcheckservice/ISpellCheckService.h
   :language: cpp
   :start-after: [service]
   :end-before: [service]

The service interface is quite simple, with only one method that needs
to be implemented. Because we provide an empty out-of-line destructor
(defined in the file ``ISpellCheckService.cpp``) we must export the
service interface by using the bundle specific
``SPELLCHECKSERVICE_EXPORT`` macro.

In the following source code, the bundle needs to create a complete list
of all dictionary services; this is somewhat tricky and must be done
carefully if done manually via service event listners. Our bundle makes
use of the :any:`cppmicroservices::ServiceTracker` and
:any:`cppmicroservices::ServiceTrackerCustomizer` classes
to robustly react to service events related to dictionary services. The
bundle activator of our bundle now additionally implements the
``ServiceTrackerCustomizer`` class to be automatically notified of
arriving, departing, or modified dictionary services. In case of a newly
added dictionary service, our
``ServiceTrackerCustomizer::AddingService()`` implementation checks if a
spell checker service was already registered and if not registers a new
``ISpellCheckService`` instance if at lead two dictionary services are
available. If the number of dictionary services drops below two, our
``ServiceTrackerCustomizer`` implementation un-registers the previously
registered spell checker service instance. These actions must be
performed in a synchronized manner to avoid interference from service
events originating from different threads. The implementation of our
bundle activator is done in a file called
``spellcheckservice/Activator.cpp``:

.. literalinclude:: spellcheckservice/Activator.cpp
   :language: cpp
   :start-after: [Activator]
   :end-before: [Activator]

Note that we do not need to unregister the service in ``Stop()`` method,
because the C++ Micro Services library will automatically do so for us.
The spell checker service that we have implemented is very simple; it
simply parses a given passage into words and then loops through all
available dictionary services for each word until it determines that the
word is correct. Any incorrect words are added to an error list that
will be returned to the caller. This solution is not optimal and is only
intended for educational purposes. Next, we create a ``manifest.json``
file that contains the meta-data for our bundle:

.. literalinclude:: spellcheckservice/resources/manifest.json
   :language: json

.. note::

   In this example, the service interface and
   implementation are both contained in one bundle which exports the
   interface class. However, service implementations almost never need to
   be exported and in many use cases it is beneficial to provide the
   service interface and its implementation(s) in separate bundles. In such
   a scenario, clients of a service will only have a link-time dependency
   on the shared library providing the service interface (because of the
   out-of-line destructor) but not on any bundles containing service
   implementations. This often leads to bundles which do not export any
   symbols at all.

For an introduction how to compile our source code, see
:any:`example1`.

After running the ``usTutorialDriver`` program we should make sure that
the bundle from Example 1 is active. We can use the :kbd:`status` shell
command to get a list of all bundles, their state, and their bundle
identifier number. If the Example 1 bundle is not active, we should
start the bundle using the start command and the bundle's identifier
number or symbolic name that is displayed by the :kbd:`status` command. Now
we can start the spell checker service bundle by entering the
:kbd:`start spellcheckservice` command which will also trigger the starting
of the dictionaryservice bundle containing the english dictionary::

   CppMicroServices-build> bin/usTutorialDriver
   > start eventlistener
   Starting to listen for service events.
   > start spellcheckservice
   > status
   Id | Symbolic Name        | State
   -----------------------------------
    0 | system_bundle        | ACTIVE
    1 | eventlistener        | ACTIVE
    2 | dictionaryservice    | INSTALLED
    3 | frenchdictionary     | INSTALLED
    4 | dictionaryclient     | INSTALLED
    5 | dictionaryclient2    | INSTALLED
    6 | dictionaryclient3    | INSTALLED
    7 | spellcheckservice    | ACTIVE
    8 | spellcheckclient     | INSTALLED
   >

To trigger the registration of the spell checker service from our
bundle, we start the frenchdictionary using the
:kbd:`start frenchdictionary` command. If the bundle from
:any:`Example 1 <example1>` is still active,
then we should see it print out the details of the service event it
receives when our new bundle registers its spell checker service::

   CppMicroServices-build> bin/usTutorialDriver
   > start frenchdictionary
   Ex1: Service of type IDictionaryService registered.
   Ex1: Service of type ISpellCheckService registered.
   >

We can experiment with our spell checker service's dynamic availability
by stopping the french dictionary service; when the service is stopped,
the eventlistener bundle will print that our bundle is no longer
offering its spell checker service. Likewise, when the french dictionary
service comes back, so will our spell checker service. We create a
client for our spell checker service in :any:`Example 7 <example7>`. To
exit the ``usTutorialDriver`` program, we use the :kbd:`shutdown` command.
