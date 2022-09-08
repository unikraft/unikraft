.. _example5:

Example 5 - Service Tracker Dictionary Client Bundle
====================================================

In :any:`Example 4 <example4>`, we created a
more robust client bundle for our dictionary service. Due to the
complexity of dealing with dynamic service availability, even that
client may not sufficiently address all situations. To deal with this
complexity the C++ Micro Services library provides the
:any:`cppmicroservices::ServiceTracker` utility class. In this example
we create a client for the dictionary service that uses the
``ServiceTracker`` class to monitor the dynamic availability of the
dictionary service, resulting in an even more robust client.

The functionality of the new dictionary client is essentially the same
as the one from Example 4. Our bundle uses its bundle context to create
a ``ServiceTracker`` instance to track the dynamic availability of the
dictionary service on our behalf. Our client uses the dictionary service
returned by the ``ServiceTracker``, which is selected based on a ranking
algorithm defined by the C++ Micro Services library. The source code for
our bundles is as follows in a file called
``dictionaryclient3/Activator.cpp``:

.. literalinclude:: dictionaryclient3/Activator.cpp
   :language: cpp
   :start-after: [Activator]
   :end-before: [Activator]

Since this client uses the ``ServiceTracker`` utility class, it will
automatically monitor the dynamic availability of the dictionary
service. Like normal, we must create a ``manifest.json`` file that
contains the meta-data for our bundle:

.. literalinclude:: dictionaryclient3/resources/manifest.json
   :language: json

Again, we must link our bundle to the ``dictionaryservice`` bundle:

.. literalinclude:: dictionaryclient3/CMakeLists.txt
   :language: cmake

After running the ``usTutorialDriver`` executable, and starting the
event listener bundle, we can use the :kbd:`start dictionaryclient3`
command to start our robust dictionary client bundle::

   CppMicroServices-debug> bin/usTutorialDriver
   > start eventlistener
   Starting to listen for service events.
   > start dictionaryclient3
   Ex1: Service of type IDictionaryService registered.
   Enter a blank line to exit.
   Enter word:

The above command starts the bundle and it will use the main thread to
prompt us for words. Enter one word at a time to check the words and
enter a blank line to stop checking words. To re-start the bundle, we
must first use the :kbd:`stop dictionaryclient3` command to stop the bundle, then
the :kbd:`start dictionaryclient3` command to re-start it. To test the dictionary
service, enter any of the words in the dictionary (e.g., "welcome",
"to", "the", "micro", "services", "tutorial") or any word not in the
dictionary.

Since this client monitors the dynamic availability of the dictionary
service, it is robust in the face of sudden departures of the the
dictionary service. Further, when a dictionary service arrives, it
automatically gets the service if it needs it and continues to function.
These capabilities are a little difficult to demonstrate since we are
using a simple single-threaded approach, but in a multi-threaded or
GUI-oriented application this robustness is very useful.
