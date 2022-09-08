.. _example2b:

Example 2b - Alternative Dictionary Service Bundle
==================================================

This example creates an alternative implementation of the dictionary
service defined in :any:`Example 2 <example2>`. The source code for the
bundle is identical except that:

* Instead of using English words, it uses French words. 
* We do not need to define the dictionary service interface again, as we can just link the definition from the bundle in Example 2. 

The main point
of this example is to illustrate that multiple implementations of the
same service may exist; this example will also be of use to us in
:any:`Example 5 <example5>`.

In the following source code, the bundle uses its bundle context to
register the dictionary service. We implement the dictionary service as
an inner class of the bundle activator class, but we could have also put
it in a separate file. The source code for our bundle is as follows in a
file called ``dictionaryclient/Activator.cpp``:

.. literalinclude:: frenchdictionary/Activator.cpp
   :language: cpp
   :start-after:  [Activator]
   :end-before: [Activator]

We must create a ``manifest.json`` file that contains the
meta-data for our bundle; the manifest file contains the following:

.. literalinclude:: frenchdictionary/resources/manifest.json
   :language: json

For a refresher on how to compile our source code, see
:any:`example1`. Because we use the ``IDictionaryService`` definition
from Example 2, we also need to make sure that the proper include paths
and linker dependencies are set:

.. literalinclude:: frenchdictionary/CMakeLists.txt
   :language: cmake

After running the ``usTutorialDriver`` program, we should make sure that
the bundle from Example 1 is active. We can use the :kbd:`status` shell
command to get a list of all bundles, their state, and their bundle
identifier number. If the Example 1 bundle is not active, we should
start the bundle using the start command and the bundle's identifier
number or name that is displayed by the :kbd:`status` command. Now we can
start our dictionary service bundle by typing the
:kbd:`start frenchdictionary` command::

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
   > start frenchdictionary
   Ex1: Service of type IDictionaryService registered.
   > status
   Id | Symbolic Name        | State
   -----------------------------------
    0 | system_bundle        | ACTIVE
    1 | eventlistener        | ACTIVE
    2 | dictionaryservice    | INSTALLED
    3 | frenchdictionary     | ACTIVE
    4 | dictionaryclient     | INSTALLED
    5 | dictionaryclient2    | INSTALLED
    6 | dictionaryclient3    | INSTALLED
    7 | spellcheckservice    | INSTALLED
    8 | spellcheckclient     | INSTALLED
   >

To stop the bundle, use the :kbd:`stop 3` command. If the bundle
from :any:`Example 1 <example1>` is still
active, then we should see it print out the details of the service event
it receives when our new bundle registers its dictionary service. Using
the ``usTutorialDriver`` commands ``stop`` and ``start`` we can stop and
start it at will, respectively. Each time we start and stop our
dictionary service bundle, we should see the details of the associated
service event printed from the bundle from Example 1. In
:any:`Example 3 <example3>`, we will create a client for our dictionary
service. To exit ``usTutorialDriver``, we use the :kbd:`shutdown` command.

.. note::

   Because our French dictionary bundle has a link dependency on the
   dictionary service bundle from Example 2, this bundle's shared
   library is automatically loaded by the operating system's dynamic
   loader. However, its status remains *INSTALLED* until it is
   explicitly started.

