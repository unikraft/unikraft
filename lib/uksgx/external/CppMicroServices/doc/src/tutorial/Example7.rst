.. _example7:

Example 7 - Spell Checker Client Bundle
=======================================

In this example we create a client for the spell checker service we
implemented in :any:`Example 6 <example6>`.
This client monitors the dynamic availability of the spell checker
service using the Service Tracker and is very similar in structure to
the dictionary client we implemented in
:any:`Example 5 <example5>`. The functionality
of the spell checker client reads passages from standard input and spell
checks them using the spell checker service. Our bundle uses its bundle
context to create a ``ServiceTracker`` object to monitor spell checker
services. The source code for our bundle is as follows in a file called
``spellcheckclient/Activator.cpp``:

.. literalinclude:: spellcheckclient/Activator.cpp
   :language: cpp
   :start-after: [Activator]
   :end-before: [Activator]

After running the ``usTutorialDriver`` program use the :kbd:`status` command to
make sure that only the bundles from Example 2, Example 2b, and Example
6 are started; use the start (``start <id | name>``) and stop
(``stop <id | name>``) commands as appropriate to start and stop the
various tutorial bundles, respectively. Now we can start our spell
checker client bundle by entering :kbd:`start spellcheckclient`::

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
:any:`Example 1 <example1>` is still active, then we should see it
print out the details of the service event it receives when our new
bundle registers its spell checker service::

   CppMicroServices-build> bin/usTutorialDriver
   > start spellcheckservice
   > start frenchdictionary
   > start spellcheckclient
   Enter a blank line to exit.
   Enter passage:

When we start the bundle, it will use the main thread to prompt us for
passages; a passage is a collection of words separated by spaces,
commas, periods, exclamation points, question marks, colons, or
semi-colons. Enter a passage and press the enter key to spell check the
passage or enter a blank line to stop spell checking passages. To
restart the bundle, we must first use the ``stop`` command to stop the
bundle, then the ``start`` command to re-start it.

Since this client uses the Service Tracker to monitor the dynamic
availability of the spell checker service, it is robust in the scenario
where the spell checker service suddenly departs. Further, when a spell
checker service arrives, it automatically gets the service if it needs
it and continues to function. These capabilities are a little difficult
to demonstrate since we are using a simple single-threaded approach, but
in a multi-threaded or GUI-oriented application this robustness is very
useful.
