.. _example3:

Example 3 - Dictionary Client Bundle
====================================

This example creates a bundle that is a client of the dictionary service
implemented in :any:`Example 2 <example2>`. In
the following source code, our bundle uses its bundle context to query
for a dictionary service. Our client bundle uses the first dictionary
service it finds, and if none are found, it prints a message and stops. 
Services operate with no additional overhead. The
source code for our bundle is as follows in a file called
``dictionaryclient/Activator.cpp``:

.. literalinclude:: dictionaryclient/Activator.cpp
   :language: cpp
   :start-after: [Activator]
   :end-before: [Activator]

Note that we do not need to unget or release the service in the ``Stop()``
method, because the C++ Micro Services library will automatically do so
for us. We must create a ``manifest.json`` file with the
meta-data for our bundle, which contains the following:

.. literalinclude:: dictionaryclient/resources/manifest.json
   :language: json

Since we are using the ``IDictionaryService`` interface defined in
Example 1, we must link our bundle to the ``dictionaryservice`` bundle:

.. literalinclude:: dictionaryclient/CMakeLists.txt
   :language: cmake

After running the ``usTutorialDriver`` executable, and starting the
event listener bundle, we can use the :kbd:`start dictionaryclient` command
to start our dictionary client bundle::

   CppMicroServices-debug> bin/usTutorialDriver
   > start eventlistener
   Starting to listen for service events.
   > start dictionaryclient
   Ex1: Service of type IDictionaryService/1.0 registered.
   Enter a blank line to exit.
   Enter word:

The above command starts the pre-installed bundle. When we start the bundle,
it will use the main thread to prompt us for words. Enter one word at a
time to check the words, and enter a blank line to stop checking words.
To reload the bundle, we must first use the :kbd:`stop dictionaryclient` command
to stop the bundle, then the :kbd:`start dictionaryclient` command to re-start
it. To test the dictionary service, enter any of the words in the
dictionary (e.g., "welcome", "to", "the", "micro", "services",
"tutorial") or any word not in the dictionary.

This example client is simple enough and, in fact, is too simple. What
would happen if the dictionary service were to unregister suddenly? Our
client would abort with a segmentation fault due to a null pointer
access when trying to use the service object. This dynamic service
availability issue is a central tenent of the service model. As a
result, we must make our client more robust in dealing with such
situations. In :any:`Example 4 <example4>`, we explore a slightly more
complicated dictionary client that dynamically monitors service
availability.
