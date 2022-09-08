Service Hooks
=============

The CppMicroServices library implements the Service Hook Service
Specification Version 1.1 from OSGi Core Release 5 for C++. Below is a
summary of the concept - consult the OSGi specifications for more
details.

Service hooks provide mechanisms for bundle writers to closely interact
with the CppMicroServices service registry. These mechanisms are not
intended for use by application bundles but rather by bundles in need of
*hooking* into the service registry and modifying the behaviour of
application bundles.

Some example use cases for service hooks include:

-  Proxying of existing services by hiding the original service and
   registering a *proxy service* with the same properties
-  Providing services *on demand* based on registered service listeners
   from external bundles

Event Listener Hook
-------------------

A bundle can intercept events being delivered to other bundles by
registering a :any:`ServiceEventListenerHook <cppmicroservices::ServiceEventListenerHook>`
object as a service. The CppMicroServices library will send all service
events to all the registered hooks using the reversed ordering of their
ServiceReference objects. 

Note that event listener hooks are called
*after* the event was created, but *before* it is filtered by the optional
filter expression of the service listeners. Therefore, an event listener hook
receives all
:any:`SERVICE_REGISTERED <gr_serviceevent::SERVICE_REGISTERED>`,
:any:`SERVICE_MODIFIED <gr_serviceevent::SERVICE_MODIFIED>`,
:any:`SERVICE_UNREGISTERING <gr_serviceevent::SERVICE_UNREGISTERING>`, and
:any:`SERVICE_MODIFIED_ENDMATCH <gr_serviceevent::SERVICE_MODIFIED_ENDMATCH>`
events regardless of the presence of a service listener filter. It may
then remove bundles or specific service listeners from the
:any:`ServiceEventListenerHook::ShrinkableMapType <cppmicroservices::ServiceEventListenerHook::ShrinkableMapType>`
object passed to the
:any:`ServiceEventListenerHook::Event() <cppmicroservices::ServiceEventListenerHook::Event>`
method to hide service events.

Implementers of the Event Listener Hook must ensure that bundles
continue to see a consistent set of service events.

Find Hook
---------

Find Hook objects registered using the
:any:`ServiceFindHook <cppmicroservices::ServiceFindHook>` interface
will be called when bundles look up service references via the
:any:`BundleContext::GetServiceReference() <cppmicroservices::BundleContext::GetServiceReference>`
or :any:`BundleContext::GetServiceReferences() <cppmicroservices::BundleContext::GetServiceReferences>`
methods. The order in which the CppMicroServices library calls the find
hooks is the reverse ``operator<`` ordering of their ServiceReference
objects. The hooks may remove service references from the ``ShrinkableVector``
object passed to the
:any:`ServiceFindHook::Find() <cppmicroservices::ServiceFindHook::Find>`
method to hide services from specific bundles.

Listener Hook
-------------

The CppMicroServices API provides information about the registration,
unregistration, and modification of services. However, it does not
directly allow the introspection of bundles to get information about
what services a bundle is waiting for. 

Bundles may need to wait for a service to arrive (via a registered service
listener) before performing their functions. Listener Hooks provide a mechanism to get
informed about all existing, newly registered, and removed service
listeners.

A Listener Hook object registered using the
:any:`ServiceListenerHook <cppmicroservices::ServiceListenerHook>`
interface will be notified about service listeners by being passed
:any:`ServiceListenerHook::ListenerInfo <cppmicroservices::ServiceListenerHook::ListenerInfo>`
objects. Each ``ListenerInfo`` object is related to the registration /
unregistration cycle of a specific service listener. That is,
registering the same service listener again (even with a different
filter) will automatically unregister the previous registration and
the newly registered service listener is related to a different
``ListenerInfo`` object. ``ListenerInfo`` objects can be stored in
unordered containers and compared with each other- for example, to match
:any:`ServiceListenerHook::Added() <cppmicroservices::ServiceListenerHook::Added>`
and :any:`ServiceListenerHook::Removed() <cppmicroservices::ServiceListenerHook::Removed>`
calls.

The Listener Hooks are called synchronously in the same order of their
registration. However, in rare cases the removal of a service listener
may be reported before its corresponding addition. To handle this case,
the :any:`ListenerInfo::IsRemoved() <cppmicroservices::ServiceListenerHook::ListenerInfo::IsRemoved>`
method is provided which can be used in the
:any:`ServiceListenerHook::Added() <cppmicroservices::ServiceListenerHook::Added>`
method to detect a delivery that is out of order. A simple strategy is to
ignore removed events without corresponding added events and ignore
added events where the ``ListenerInfo`` object is already removed:

.. literalinclude:: snippets/uServices-servicelistenerhook/main.cpp
   :language: cpp
   :start-after: [1]
   :end-before: [1]

Architectural Notes
-------------------

Ordinary Services
~~~~~~~~~~~~~~~~~

All service hooks are treated as ordinary services. If the
CppMicroServices library uses them, their Service References will show
that the CppMicroServices bundles are using them, and if a hook is a
Service Factory, then the actual instance will be properly created.

The only speciality of the service hooks is that the CppMicroServices
library does not use them for the hooks themselves. That is, the Service
Event and Service Find Hooks cannot be used to hide the services from
the CppMicroServices library.

Ordering
~~~~~~~~

The hooks are very sensitive to ordering because they interact directly
with the service registry. In general, implementers of the hooks must be
aware that other bundles can be started before or after the bundle which
provides the hooks. To ensure early registration of the hooks, they
should be registered within the ``BundleActivator::Start()`` method of the
program executable.

Multi Threading
~~~~~~~~~~~~~~~

All hooks must be thread-safe because the hooks can be called at any
time. All hook methods must be re-entrant, as they can be entered at any
time and in rare cases in the wrong order. The CppMicroServices library
calls all hook methods synchronously, but the calls might be triggered
from any user thread interacting with the CppMicroServices API. The
CppMicroServices API can be called from any of the hook methods, but
implementers must be careful to not hold any lock while calling
CppMicroServices methods.
