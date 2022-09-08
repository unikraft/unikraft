Emulating Singletons
====================

Integrating C++ Micro Services into an existing code-base can be done
incrementally, e.g. by starting to convert class singletons to services.

Meyers Singleton
----------------

Singletons are a well known pattern to ensure that only one instance of
a class exists during the whole life-time of the application. A
self-deleting variant is the "Meyers Singleton":

.. literalinclude:: /framework/doc/snippets/uServices-singleton/SingletonOne.h
   :language: cpp
   :start-after: s1
   :end-before: s1

where the ``GetInstance()`` method is implemented as

.. literalinclude:: /framework/doc/snippets/uServices-singleton/SingletonOne.cpp
   :language: cpp
   :start-after: s1
   :end-before: s1

If such a singleton is accessed during static deinitialization, your
program might crash or even worse, exhibit undefined behavior, depending
on your compiler and/or weekday. Such an access might happen in
destructors of other objects with static life-time.

For example, suppose that ``SingletonOne`` needs to call a second Meyers
singleton during destruction:

.. literalinclude:: /framework/doc/snippets/uServices-singleton/SingletonOne.cpp
   :language: cpp
   :start-after: s1d
   :end-before: s1d

If ``SingletonTwo`` was destroyed before ``SingletonOne``, this leads to the
mentioned problems. Note that this problem only occurs for static objects
defined in the same shared library.

Since you cannot reliably control the destruction order of global static
objects, you must not introduce dependencies between them during static
deinitialization. This is one reason why one should consider an
alternative approach to singletons (unless you can absolutely make sure
that nothing in your shared library will introduce such dependencies.
Never.)

Of course you could use something like a *Phoenix singleton* but that
will have other drawbacks in certain scenarios. Returning pointers
instead of references in GetInstance() would open up the possibility to
return a ``nullptr``, but than again this would not help if you require a
non-NULL instance in your destructor.

Another reason for an alternative approach is that singletons are
usually not meant to be singletons for eternity. If your design evolves,
you might hit a point where you suddenly need multiple instances of your
singleton.

Singletons as a Service
-----------------------

C++ Micro Services can be used to emulate the singleton pattern
using a non-singleton class. This leaves room for future extensions
without the need for heavy refactoring. Additionally, it gives you full
control about the construction and destruction order of your
"singletons" inside your shared library or executable, making it
possible to have dependencies between them during destruction.

Converting a Classic Singleton
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

We modify the previous ``SingletonOne`` class such that it internally uses
the micro services API. The changes are discussed in detail below.

.. literalinclude:: /framework/doc/snippets/uServices-singleton/SingletonOne.h
   :language: cpp
   :start-after: ss1
   :end-before: ss1

-  In the implementation above, the class ``SingletonOneService`` provides
   the implementation as well as the interface.
-  The ``SingletonOneService`` class looks like a plain C++ class, no need for
   hiding constructors and destructor

Let's have a look at the modified ``GetInstance()`` and
``~SingletonOneService()`` methods.

.. literalinclude:: /framework/doc/snippets/uServices-singleton/SingletonOne.cpp
   :language: cpp
   :start-after: ss1gi
   :end-before: ss1gi

The inline comments should explain the details. Note that we now had to
change the return type to a shared pointer, instead of a reference as in
the classic singleton. This is necessary since we can no longer
guarantee that an instance always exists. Clients of the ``GetInstance()``
method must check if the returned object is empty and react
appropriately.

.. note::

   Newly created "singletons" should not expose a ``GetInstance()`` method.
   They should be handled as proper services and hence should be retrieved
   by clients using the :any:`BundleContext <cppmicroservices::BundleContext>`
   or :any:`ServiceTracker <cppmicroservices::ServiceTracker>` API. The
   ``GetInstance()`` method is for migration purposes only.

.. literalinclude:: /framework/doc/snippets/uServices-singleton/SingletonOne.cpp
   :language: cpp
   :start-after: ss1d
   :end-before: ss1d

The ``SingletonTwoService::GetInstance()`` method is implemented exactly as
in ``SingletonOneService``. Because we know that the bundle activator
guarantees that a ``SingletonTwoService`` instance will always be available
during the life-time of a ``SingletonOneService`` instance (see below), we
can assert a non-null pointer. Otherwise, we would have to handle the
null-pointer case.

The order of construction/registration and destruction/unregistration of
our singletons (or any other services) is defined in the ``Start()`` and
``Stop()`` methods of the bundle activator.

.. literalinclude:: /framework/doc/snippets/uServices-singleton/main.cpp
   :language: cpp
   :start-after: 0
   :end-before: 0

The ``Stop()`` method is defined as:

.. literalinclude:: /framework/doc/snippets/uServices-singleton/main.cpp
   :language: cpp
   :start-after: 1
   :end-before: 1
