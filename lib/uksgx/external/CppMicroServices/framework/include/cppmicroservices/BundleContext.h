/*=============================================================================

  Library: CppMicroServices

  Copyright (c) The CppMicroServices developers. See the COPYRIGHT
  file at the top-level directory of this distribution and at
  https://github.com/CppMicroServices/CppMicroServices/COPYRIGHT .

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=============================================================================*/

#ifndef CPPMICROSERVICES_BUNDLECONTEXT_H
#define CPPMICROSERVICES_BUNDLECONTEXT_H

#include "cppmicroservices/GlobalConfig.h"
#include "cppmicroservices/ListenerFunctors.h"
#include "cppmicroservices/ListenerToken.h"
#include "cppmicroservices/ServiceInterface.h"
#include "cppmicroservices/ServiceRegistration.h"

#include <memory>

namespace cppmicroservices {

class AnyMap;
class Bundle;
class BundleContext;
class BundleContextPrivate;
class ServiceFactory;
namespace detail {
class LogSink;
template<class S, class TTT, class R>
class BundleAbstractTracked;
template<class S, class TTT>
class ServiceTrackerPrivate;
template<class S, class TTT>
class TrackedService;
}

template<class S>
class ServiceObjects;
template<class S>
struct ServiceHolder;

/**
 * \ingroup MicroServices
 *
 * A bundle's execution context within the framework. The context is used to
 * grant access to other methods so that this bundle can interact with the
 * framework.
 *
 * <p>
 * <code>BundleContext</code> methods allow a bundle to:
 * <ul>
 * <li>Install other bundles.
 * <li>Subscribe to events published by the framework.
 * <li>Register service objects with the framework service registry.
 * <li>Retrieve <code>ServiceReference</code>s from the framework service
 * registry.
 * <li>Get and release service objects for a referenced service.
 * <li>Get the list of bundles installed in the framework.
 * <li>Get the {@link Bundle} object for a bundle.
 * </ul>
 *
 * <p>
 * A <code>BundleContext</code> object will be created and provided to the
 * bundle associated with this context when it is started using the
 * {@link BundleActivator::Start} method. The same <code>BundleContext</code>
 * object will be passed to the bundle associated with this context when it is
 * stopped using the {@link BundleActivator::Stop} method. A
 * <code>BundleContext</code> object is generally for the private use of its
 * associated bundle and is not meant to be shared with other bundles in the
 * bundle environment.
 *
 * <p>
 * The <code>Bundle</code> object associated with a <code>BundleContext</code>
 * object is called the <em>context bundle</em>.
 *
 * <p>
 * The <code>BundleContext</code> object is only valid during the execution of
 * its context bundle; that is, during the period when the context bundle
 * is started. If the <code>BundleContext</code>
 * object is used subsequently, a <code>std::runtime_error</code> is
 * thrown. The <code>BundleContext</code> object is never reused after
 * its context bundle is stopped.
 *
 * <p>
 * The framework is the only entity that can create <code>BundleContext</code>
 * objects.
 *
 * @remarks This class is thread safe.
 */
class US_Framework_EXPORT BundleContext
{

public:
  /**
   * Constructs an invalid %BundleContext object.
   *
   * Valid bundle context objects can only be created by the framework
   * and are supplied to a bundle via its \c BundleActivator or as a
   * return value of the \c GetBundleContext() method.
   *
   * @see operator bool() const
   */
  BundleContext();

  /**
   * Compares this \c BundleContext object with the specified
   * bundle context.
   *
   * Valid \c BundleContext objects are equal if and only if
   * they represent the same context. Invalid \c BundleContext
   * objects are always considered to be equal.
   *
   * @param rhs The \c BundleContext object to compare this object with.
   * @return \c true if this \c BundleContext object is equal to \c rhs,
   *         \c false otherwise.
   */
  bool operator==(const BundleContext& rhs) const;

  /**
   * Compares this \c BundleContext object with the specified bundle
   * context for inequality.
   *
   * @param rhs The \c BundleContext object to compare this object with.
   * @return Returns the result of <code>!(*this == rhs)</code>.
   */
  bool operator!=(const BundleContext& rhs) const;

  /**
   * Compares this \c BundleContext with the specified bundle
   * context for order.
   *
   * How valid %BundleContext objects are ordered is an implementation
   * detail and must not be relied on. Invalid \c BundleContext objects
   * will always compare greater then valid \c BundleContext objects.
   *
   * @param rhs The \c BundleContext object to compare this object with.
   * @return \c true if this object is orderded before \c rhs, \c false
   *         otherwise.
   */
  bool operator<(const BundleContext& rhs) const;

  /**
   * Tests this %BundleContext object for validity.
   *
   * Invalid \c BundleContext objects are created by the default constructor or
   * can be returned by certain framework methods if the context bundle has been
   * uninstalled.
   *
   * A \c BundleContext object can become invalid by assigning a \c nullptr to
   * it or if the context bundle is stopped.
   *
   * @return \c true if this %BundleContext object is valid and can safely be used,
   *         \c false otherwise.
   */
  explicit operator bool() const;

  /**
   * Releases any resources held or locked by this
   * \c BundleContext and renders it invalid.
   */
  BundleContext& operator=(std::nullptr_t);

  /**
   * Returns the value of the specified property. If the key is not found in
   * the Framework properties, the method returns an empty \c Any.
   *
   * @param key The name of the requested property.
   * @return The value of the requested property, or an empty \c Any if the
   *         property is undefined.
   */
  Any GetProperty(const std::string& key) const;

  /**
   * Returns all known properties.
   *
   * @return A map of all framework properties.
   */
  AnyMap GetProperties() const;

  /**
   * Returns the <code>Bundle</code> object associated with this
   * <code>BundleContext</code>. This bundle is called the context bundle.
   *
   * @return The <code>Bundle</code> object associated with this
   *         <code>BundleContext</code>.
   * @throws std::runtime_error If this BundleContext is no
   *         longer valid.
   */
  Bundle GetBundle() const;

  /**
   * Returns the bundle with the specified identifier.
   *
   * @param id The identifier of the bundle to retrieve.
   * @return A <code>Bundle</code> object or <code>nullptr</code> if the
   *         identifier does not match any previously installed bundle.
   * @throws std::logic_error If the framework instance is not active.
   * @throws std::runtime_error If this BundleContext is no
   *         longer valid.
   */
  Bundle GetBundle(long id) const;

  /**
   * Get the bundles with the specified bundle location.
   *
   * @param location The location of the bundles to get.
   * @return The requested {\c Bundle}s or an empty list.
   * @throws std::logic_error If the framework instance is not active.
   * @throws std::runtime_error If the BundleContext is no longer valid.
   */
  std::vector<Bundle> GetBundles(const std::string& location) const;

  /**
   * Returns a list of all known bundles.
   * <p>
   * This method returns a list of all bundles installed in the bundle
   * environment at the time of the call to this method. This list will
   * also contain bundles which might already have been stopped.
   *
   * @return A std::vector of <code>Bundle</code> objects which
   *         will hold one object per known bundle.
   * @throws std::runtime_error If the BundleContext is no longer valid.
   */
  std::vector<Bundle> GetBundles() const;

  /**
   * Registers the specified service object with the specified properties
   * under the specified class names into the framework. A
   * <code>ServiceRegistration</code> object is returned. The
   * <code>ServiceRegistration</code> object is for the private use of the
   * bundle registering the service and should not be shared with other
   * bundles. The registering bundle is defined to be the context bundle.
   * Other bundles can locate the service by using either the
   * {@link #GetServiceReferences} or {@link #GetServiceReference} method.
   *
   * <p>
   * A bundle can register a service object that implements the
   * ServiceFactory or PrototypeServiceFactory interface to have more
   * flexibility in providing service objects to other bundles.
   *
   * <p>
   * The following steps are taken when registering a service:
   * <ol>
   * <li>The framework adds the following service properties to the service
   * properties from the specified <code>ServiceProperties</code> (which may be
   * omitted): <br/>
   * A property named Constants#SERVICE_ID identifying the
   * registration number of the service <br/>
   * A property named Constants#OBJECTCLASS containing all the
   * specified classes. <br/>
   * A property named Constants#SERVICE_SCOPE identifying the scope
   * of the service. <br/>
   * Properties with these names in the specified <code>ServiceProperties</code> will
   * be ignored.
   * <li>The service is added to the framework service registry and may now be
   * used by other bundles.
   * <li>A service event of type ServiceEvent#SERVICE_REGISTERED is fired.
   * <li>A <code>ServiceRegistration</code> object for this registration is
   * returned.
   * </ol>
   *
   * @note This is a low-level method and should normally not be used directly.
   *       Use one of the templated RegisterService methods instead.
   *
   * @param service A shared_ptr to a map of interface identifiers to service objects.
   * @param properties The properties for this service. The keys in the
   *        properties object must all be <code>std::string</code> objects. See
   *        {@link Constants} for a list of standard service property keys.
   *        Changes should not be made to this object after calling this
   *        method. To update the service's properties the
   *        {@link ServiceRegistration::SetProperties} method must be called.
   *        The set of properties may be omitted if the service has
   *        no properties.
   * @return A <code>ServiceRegistration</code> object for use by the bundle
   *         registering the service to update the service's properties or to
   *         unregister the service.
   *
   * @throws std::runtime_error If this BundleContext is no longer valid, or if there are
             case variants of the same key in the supplied properties map.
   * @throws std::invalid_argument If the InterfaceMap is empty, or
   *         if a service is registered as a null class.
   *
   * @see ServiceRegistration
   * @see ServiceFactory
   * @see PrototypeServiceFactory
   */
  ServiceRegistrationU RegisterService(
    const InterfaceMapConstPtr& service,
    const ServiceProperties& properties = ServiceProperties());

  /**
   * Registers the specified service object with the specified properties
   * using the specified interfaces types with the framework.
   *
   * <p>
   * This method is provided as a convenience when registering a service under
   * two interface classes whose type is available to the caller. It is otherwise identical to
   * RegisterService(const InterfaceMap&, const ServiceProperties&) but should be preferred
   * since it avoids errors in the string literal identifying the class name or interface identifier.
   *
   * Example usage:
   * \snippet uServices-registration/main.cpp 2-1
   * \snippet uServices-registration/main.cpp 2-2
   *
   * @tparam I1 The first interface type under which the service can be located.
   * @tparam Interfaces Additional interface types under which the service can be located.
   * @param impl A \c shared_ptr to the service object
   * @param properties The properties for this service.
   * @return A ServiceRegistration object for use by the bundle
   *         registering the service to update the service's properties or to
   *         unregister the service.
   * @throws std::logic_error If this BundleContext is no longer valid.
   * @throws ServiceException If the service type \c S is invalid or the
   *         \c service object is nullptr.
   *
   * @see RegisterService(const InterfaceMap&, const ServiceProperties&)
   */
  template<class I1, class... Interfaces, class Impl>
  ServiceRegistration<I1, Interfaces...> RegisterService(
    const std::shared_ptr<Impl>& impl,
    const ServiceProperties& properties = ServiceProperties())
  {
    InterfaceMapConstPtr servicePointers =
      MakeInterfaceMap<I1, Interfaces...>(impl);
    return RegisterService(servicePointers, properties);
  }

  /**
   * Registers the specified service factory as a service with the specified properties
   * using the specified template argument as service interface type with the framework.
   *
   * <p>
   * This method is provided as a convenience when <code>factory</code> will only be registered under
   * a single class name whose type is available to the caller. It is otherwise identical to
   * RegisterService(const InterfaceMap&, const ServiceProperties&) but should be preferred
   * since it avoids errors in the string literal identifying the class name or interface identifier.
   *
   * Example usage:
   * \snippet uServices-registration/main.cpp 2-1
   * \snippet uServices-registration/main.cpp f2
   *
   * @tparam I1 The first interface type under which the service can be located.
   * @tparam Interfaces Additional interface types under which the service can be located.
   * @param factory A \c shared_ptr to the ServiceFactory object.
   * @param properties The properties for this service.
   * @return A ServiceRegistration object for use by the bundle
   *         registering the service to update the service's properties or to
   *         unregister the service.
   * @throws std::logic_error If this BundleContext is no longer valid.
   * @throws ServiceException If the service type \c S is invalid or the
   *         \c service factory object is nullptr.
   *
   * @see RegisterService(const InterfaceMap&, const ServiceProperties&)
   */
  template<class I1, class... Interfaces>
  ServiceRegistration<I1, Interfaces...> RegisterService(
    const std::shared_ptr<ServiceFactory>& factory,
    const ServiceProperties& properties = ServiceProperties())
  {
    InterfaceMapConstPtr servicePointers =
      MakeInterfaceMap<I1, Interfaces...>(factory);
    return RegisterService(servicePointers, properties);
  }

  /**
   * Returns a list of <code>ServiceReference</code> objects. The returned
   * list contains services that were registered under the specified class
   * and match the specified filter expression.
   *
   * <p>
   * The list is valid at the time of the call to this method. However, since
   * the framework is a very dynamic environment, services can be modified or
   * unregistered at any time.
   *
   * <p>
   * The specified <code>filter</code> expression is used to select the
   * registered services whose service properties contain keys and values
   * that satisfy the filter expression. See LDAPFilter for a description
   * of the filter syntax. If the specified <code>filter</code> is
   * empty, all registered services are considered to match the
   * filter. If the specified <code>filter</code> expression cannot be parsed,
   * an <code>std::invalid_argument</code> will be thrown with a human-readable
   * message where the filter became unparsable.
   *
   * <p>
   * The result is a list of <code>ServiceReference</code> objects for all
   * services that meet all of the following conditions:
   * <ul>
   * <li>If the specified class name, <code>clazz</code>, is not
   * empty, the service must have been registered with the
   * specified class name. The complete list of class names with which a
   * service was registered is available from the service's
   * {@link Constants#OBJECTCLASS objectClass} property.
   * <li>If the specified <code>filter</code> is not empty, the
   * filter expression must match the service.
   * </ul>
   *
   * @param clazz The class name with which the service was registered or
   *        an empty string for all services.
   * @param filter The filter expression or empty for all
   *        services.
   * @return A list of <code>ServiceReference</code> objects or
   *         an empty list if no services are registered that satisfy the
   *         search.
   * @throws std::invalid_argument If the specified <code>filter</code>
   *         contains an invalid filter expression that cannot be parsed.
   * @throws std::runtime_error If this BundleContext is no longer valid.
   * @throws std::logic_error If the ServiceRegistrationBase object is invalid,
   *         or if the service is unregistered.
   */
  std::vector<ServiceReferenceU> GetServiceReferences(
    const std::string& clazz,
    const std::string& filter = std::string());

  /**
   * Returns a list of <code>ServiceReference</code> objects. The returned
   * list contains services that
   * were registered under the interface id of the template argument <code>S</code>
   * and match the specified filter expression.
   *
   * <p>
   * This method is identical to GetServiceReferences(const std::string&, const std::string&) except that
   * the class name for the service object is automatically deduced from the template argument.
   *
   * @tparam S The type under which the requested service objects must have been registered.
   * @param filter The filter expression or empty for all
   *        services.
   * @return A list of <code>ServiceReference</code> objects or
   *         an empty list if no services are registered which satisfy the
   *         search.
   * @throws std::invalid_argument If the specified <code>filter</code>
   *         contains an invalid filter expression that cannot be parsed.
   * @throws std::logic_error If this BundleContext is no longer valid.
   * @throws ServiceException If the service interface id of \c S is empty, see @gr_serviceinterface.
   *
   * @see GetServiceReferences(const std::string&, const std::string&)
   */
  template<class S>
  std::vector<ServiceReference<S>> GetServiceReferences(
    const std::string& filter = std::string())
  {
    auto& clazz = us_service_interface_iid<S>();
    if (clazz.empty())
      throw ServiceException(
        "The service interface class has no "
        "CPPMICROSERVICES_DECLARE_SERVICE_INTERFACE macro");
    typedef std::vector<ServiceReferenceU> BaseVectorT;
    BaseVectorT serviceRefs = GetServiceReferences(clazz, filter);
    std::vector<ServiceReference<S>> result;
    for (BaseVectorT::const_iterator i = serviceRefs.begin();
         i != serviceRefs.end();
         ++i) {
      result.push_back(ServiceReference<S>(*i));
    }
    return result;
  }

  /**
   * Returns a <code>ServiceReference</code> object for a service that
   * implements and was registered under the specified class.
   *
   * <p>
   * The returned <code>ServiceReference</code> object is valid at the time of
   * the call to this method. However as the Micro Services framework is a very dynamic
   * environment, services can be modified or unregistered at any time.
   *
   * <p>
   * This method is the same as calling
   * {@link BundleContext::GetServiceReferences(const std::string&, const std::string&)} with an
   * empty filter expression. It is provided as a convenience for
   * when the caller is interested in any service that implements the
   * specified class.
   * <p>
   * If multiple such services exist, the service with the highest ranking (as
   * specified in its Constants::SERVICE_RANKING property) is returned.
   * <p>
   * If there is a tie in ranking, the service with the lowest service ID (as
   * specified in its Constants::SERVICE_ID property); that is, the
   * service that was registered first is returned.
   *
   * @param clazz The class name with which the service was registered.
   * @return A <code>ServiceReference</code> object, or an invalid <code>ServiceReference</code> if
   *         no services are registered which implement the named class.
   * @throws std::runtime_error If this BundleContext is no longer valid.
   *
   * @see #GetServiceReferences(const std::string&, const std::string&)
   */
  ServiceReferenceU GetServiceReference(const std::string& clazz);

  /**
   * Returns a <code>ServiceReference</code> object for a service that
   * implements and was registered under the specified template class argument.
   *
   * <p>
   * This method is identical to GetServiceReference(const std::string&) except that
   * the class name for the service object is automatically deduced from the template argument.
   *
   * @tparam S The type under which the requested service must have been registered.
   * @return A <code>ServiceReference</code> object, or an invalid <code>ServiceReference</code> if
   *         no services are registered which implement the type <code>S</code>.
   * @throws std::runtime_error If this BundleContext is no longer valid.
   * @throws ServiceException If the service interface id of \c S is empty, see @gr_serviceinterface.
   * @see #GetServiceReference(const std::string&)
   * @see #GetServiceReferences(const std::string&)
   */
  template<class S>
  ServiceReference<S> GetServiceReference()
  {
    auto& clazz = us_service_interface_iid<S>();
    if (clazz.empty())
      throw ServiceException(
        "The service interface class has no "
        "CPPMICROSERVICES_DECLARE_SERVICE_INTERFACE macro");
    return ServiceReference<S>(GetServiceReference(clazz));
  }

  /**
   * Returns the service object referenced by the specified
   * <code>ServiceReferenceBase</code> object.
   * <p>
   * A bundle's use of a service is tracked by the bundle's use count of that
   * service. Each call to {@link #GetService(const ServiceReference<S>&)} increments
   * the context bundle's use count by one. The deleter function of the returned shared_ptr
   * object is responsible for decrementing the context bundle's use count.
   * <p>
   * When a bundle's use count for a service drops to zero, the bundle should
   * no longer use that service.
   *
   * <p>
   * This method will always return an empty object when the service
   * associated with this <code>reference</code> has been unregistered.
   *
   * <p>
   * The following steps are taken to get the service object:
   * <ol>
   * <li>If the service has been unregistered, empty object is returned.
   * <li>The context bundle's use count for this service is incremented by
   * one.
   * <li>If the context bundle's use count for the service is currently one
   * and the service was registered with an object implementing the
   * <code>ServiceFactory</code> interface, the
   * {@link ServiceFactory::GetService} method is
   * called to create a service object for the context bundle. This service
   * object is cached by the framework. While the context bundle's use count
   * for the service is greater than zero, subsequent calls to get the
   * services's service object for the context bundle will return the cached
   * service object. <br>
   * If the <code>ServiceFactory</code> object throws an
   * exception, empty object is returned and a warning is logged.
   * <li>A shared_ptr to the service object is returned.
   * </ol>
   *
   * @param reference A reference to the service.
   * @return A shared_ptr to the service object associated with <code>reference</code>.
   *         An empty shared_ptr is returned if the service is not registered or the
   *         <code>ServiceFactory</code> threw an exception
   * @throws std::runtime_error If this BundleContext is no longer valid.
   * @throws std::invalid_argument If the specified
   *         <code>ServiceReferenceBase</code> is invalid (default constructed).
   * @see ServiceFactory
   */
  std::shared_ptr<void> GetService(const ServiceReferenceBase& reference);

  InterfaceMapConstPtr GetService(const ServiceReferenceU& reference);

  /**
   * Returns the service object referenced by the specified
   * <code>ServiceReference</code> object.
   * <p>
   * This is a convenience method which is identical to void* GetService(const ServiceReferenceBase&)
   * except that it casts the service object to the supplied template argument type
   *
   * @tparam S The type the service object will be cast to.
   * @return A shared_ptr to the service object associated with <code>reference</code>.
   *         An empty object is returned if the service is not registered, the
   *         <code>ServiceFactory</code> threw an exception or the service could not be
   *         cast to the desired type.
   * @throws std::runtime_error If this BundleContext is no
   *         longer valid.
   * @throws std::invalid_argument If the specified
   *         <code>ServiceReference</code> is invalid (default constructed).
   * @see #GetService(const ServiceReferenceBase&)
   * @see ServiceFactory
   */
  template<class S>
  std::shared_ptr<S> GetService(const ServiceReference<S>& reference)
  {
    const ServiceReferenceBase& baseRef = reference;
    return std::static_pointer_cast<S>(GetService(baseRef));
  }

  /**
   * Returns the ServiceObjects object for the service referenced by the specified
   * ServiceReference object. The ServiceObjects object can be used to obtain
   * multiple service objects for services with prototype scope. For services with
   * singleton or bundle scope, the ServiceObjects::GetService() method behaves
   * the same as the GetService(const ServiceReference<S>&) method and the
   * ServiceObjects::UngetService(const ServiceReferenceBase&) method behaves the
   * same as the UngetService(const ServiceReferenceBase&) method. That is, only one,
   * use-counted service object is available from the ServiceObjects object.
   *
   * @tparam S Type of Service.
   * @param reference A reference to the service.
   * @return A ServiceObjects object for the service associated with the specified
   * reference or an invalid instance if the service is not registered.
   * @throws std::runtime_error If this BundleContext is no longer valid.
   * @throws std::invalid_argument If the specified ServiceReference is invalid
   * (default constructed or the service has been unregistered)
   *
   * @see PrototypeServiceFactory
   */
  template<class S>
  ServiceObjects<S> GetServiceObjects(const ServiceReference<S>& reference)
  {
    return ServiceObjects<S>(d, reference);
  }

  /**
   * Adds the specified <code>listener</code> with the
   * specified <code>filter</code> to the context bundles's list of listeners.
   * See <code>LDAPFilter</code> for a description of the filter syntax. Listeners
   * are notified when a service has a lifecycle state change.
   *
   * <p>
   * The framework takes care of removing all listeners registered by this
   * context bundle's classes after the bundle is stopped.
   *
   * <p>
   * The <code>listener</code> is called if the filter criteria is met.
   * To filter based upon the class of the service, the filter should reference
   * the Constants#OBJECTCLASS property. If <code>filter</code> is
   * empty, all services are considered to match the filter.
   *
   * <p>
   * When using a <code>filter</code>, it is possible that the
   * <code>ServiceEvent</code>s for the complete lifecycle of a service
   * will not be delivered to the <code>listener</code>. For example, if the
   * <code>filter</code> only matches when the property <code>example_property</code>
   * has the value <code>1</code>, the <code>listener</code> will not be called if the
   * service is registered with the property <code>example_property</code> not
   * set to the value <code>1</code>. Subsequently, when the service is modified
   * setting property <code>example_property</code> to the value <code>1</code>,
   * the filter will match and the <code>listener</code> will be called with a
   * <code>ServiceEvent</code> of type <code>SERVICE_MODIFIED</code>. Thus, the
   * <code>listener</code> will not be called with a <code>ServiceEvent</code> of type
   * <code>SERVICE_REGISTERED</code>.
   *
   * @param listener Any callable object.
   * @param filter The filter criteria.
   * @returns a ListenerToken object which can be used to remove the
   *          <code>listener</code> from the list of registered listeners.
   * @throws std::invalid_argument If <code>filter</code> contains an
   *         invalid filter string that cannot be parsed.
   * @throws std::runtime_error If this BundleContext is no
   *         longer valid.
   * @see ServiceEvent
   * @see ServiceListener
   * @see RemoveServiceListener()
   */
  ListenerToken AddServiceListener(const ServiceListener& listener,
                                   const std::string& filter = std::string());

  /**
   * Removes the specified <code>listener</code> from the context bundle's
   * list of listeners.
   *
   * <p>
   * If the <code>listener</code> is not contained in this
   * context bundle's list of listeners, this method does nothing.
   *
   * \rststar
   * .. deprecated:: 3.1.0
   *
   *    This function exists only to maintain backwards compatibility
   *    and will be removed in the next major release.
   *    Use :any:`RemoveListener() <cppmicroservices::BundleContext::RemoveListener>` instead.
   * \endrststar
   *
   * @param listener The callable object to remove.
   * @throws std::runtime_error If this BundleContext is no
   *         longer valid.
   * @see AddServiceListener()
   */
  US_DEPRECATED void RemoveServiceListener(const ServiceListener& listener);

  /**
   * Adds the specified <code>listener</code> to the context bundles's list
   * of listeners. Listeners are notified when a bundle has a lifecycle
   * state change.
   *
   * @param listener Any callable object.
   * @returns a ListenerToken object which can be used to remove the
   *          <code>listener</code> from the list of registered listeners.
   * @throws std::runtime_error If this BundleContext is no
   *         longer valid.
   * @see BundleEvent
   * @see BundleListener
   */
  ListenerToken AddBundleListener(const BundleListener& listener);

  /**
   * Removes the specified <code>listener</code> from the context bundle's
   * list of listeners.
   *
   * <p>
   * If the <code>listener</code> is not contained in this
   * context bundle's list of listeners, this method does nothing.
   *
   * \rststar
   * .. deprecated:: 3.1.0
   *
   *    This function exists only to maintain backwards compatibility
   *    and will be removed in the next major release.
   *    Use :any:`RemoveListener() <cppmicroservices::BundleContext::RemoveListener>` instead.
   * \endrststar
   *
   * @param listener The callable object to remove.
   * @throws std::runtime_error If this BundleContext is no
   *         longer valid.
   * @see AddBundleListener()
   * @see BundleListener
   */
  US_DEPRECATED void RemoveBundleListener(const BundleListener& listener);

  /**
   * Adds the specified <code>listener</code> to the context bundles's list
   * of framework listeners. Listeners are notified of framework events.
   *
   * @param listener Any callable object.
   * @returns a ListenerToken object which can be used to remove the
   *          <code>listener</code> from the list of registered listeners.
   * @throws std::runtime_error If this BundleContext is no longer valid.
   * @see FrameworkEvent
   * @see FrameworkListener
   */
  ListenerToken AddFrameworkListener(const FrameworkListener& listener);

  /**
   * Removes the specified <code>listener</code> from the context bundle's
   * list of framework listeners.
   *
   * <p>
   * If the <code>listener</code> is not contained in this
   * context bundle's list of listeners, this method does nothing.
   *
   * \rststar
   * .. deprecated:: 3.1.0
   *
   *    This function exists only to maintain backwards compatibility
   *    and will be removed in the next major release.
   *    Use :any:`RemoveListener() <cppmicroservices::BundleContext::RemoveListener>` instead.
   * \endrststar
   *
   * @param listener The callable object to remove.
   * @throws std::runtime_error If this BundleContext is no longer valid.
   * @see AddFrameworkListener()
   * @see FrameworkListener
   */
  US_DEPRECATED void RemoveFrameworkListener(const FrameworkListener& listener);

  /**
   * Removes the registered listener associated with the <code>token</code>
   *
   * <p>
   * If the listener associated with the <code>token</code> is not contained in this
   * context bundle's list of listeners or if <code>token</code> is an invalid token,
   * this method does nothing.
   *
   * <p>
   * The token can correspond to one of Service, Bundle or Framework listeners. Using
   * this function to remove the registered listeners is the recommended approach over
   * using any of the other deprecated functions -
   * Remove{Bundle,Framework,Service}Listener.
   *
   * @param token is an object of type ListenerToken.
   * @throws std::runtime_error If this BundleContext is no longer valid.
   * @see AddServiceListener()
   * @see AddBundleListener()
   * @see AddFrameworkListener()
   *
   */
  void RemoveListener(ListenerToken token);

  /**
   * Adds the specified <code>callback</code> with the
   * specified <code>filter</code> to the context bundles's list of listeners.
   * See <code>LDAPFilter</code> for a description of the filter syntax. Listeners
   * are notified when a service has a lifecycle state change.
   *
   * <p>
   * You must take care to remove registered listeners before the <code>receiver</code>
   * object is destroyed. However, the Micro Services framework takes care
   * of removing all listeners registered by this context bundle's classes
   * after the bundle is stopped.
   *
   * <p>
   * If the context bundle's list of listeners already contains a pair <code>(r,c)</code>
   * of <code>receiver</code> and <code>callback</code> such that
   * <code>(r == receiver && c == callback)</code>, then this
   * method replaces that callback's filter (which may be empty)
   * with the specified one (which may be empty).
   *
   * <p>
   * The callback is called if the filter criteria is met. To filter based
   * upon the class of the service, the filter should reference the
   * Constants#OBJECTCLASS property. If <code>filter</code> is
   * empty, all services are considered to match the filter.
   *
   * <p>
   * When using a <code>filter</code>, it is possible that the
   * <code>ServiceEvent</code>s for the complete lifecycle of a service
   * will not be delivered to the callback. For example, if the
   * <code>filter</code> only matches when the property <code>example_property</code>
   * has the value <code>1</code>, the callback will not be called if the
   * service is registered with the property <code>example_property</code> not
   * set to the value <code>1</code>. Subsequently, when the service is modified
   * setting property <code>example_property</code> to the value <code>1</code>, the
   * filter will match and the callback will be called with a
   * <code>ServiceEvent</code> of type <code>SERVICE_MODIFIED</code>. Thus, the
   * callback will not be called with a <code>ServiceEvent</code> of type
   * <code>SERVICE_REGISTERED</code>.
   *
   * \rststar
   * .. deprecated:: 3.1.0
   *
   *    This function exists only to maintain backwards compatibility
   *    and will be removed in the next major release.
   *    Use `std::bind` to bind the member function and then pass the result
   *    to :any:`AddServiceListener(const ServiceListener&) <cppmicroservices::BundleContext::AddServiceListener>` instead.
   * \endrststar
   *
   * @tparam R The type of the receiver (containing the member function to be called)
   * @param receiver The object to connect to.
   * @param callback The member function pointer to call.
   * @param filter The filter criteria.
   * @returns a ListenerToken object which can be used to remove the callable from the
   *          registered listeners.
   * @throws std::invalid_argument If <code>filter</code> contains an
   *         invalid filter string that cannot be parsed.
   * @throws std::runtime_error If this BundleContext is no
   *         longer valid.
   * @see ServiceEvent
   * @see RemoveServiceListener()
   */
  template<class R>
  US_DEPRECATED ListenerToken
  AddServiceListener(R* receiver,
                     void (R::*callback)(const ServiceEvent&),
                     const std::string& filter = std::string())
  {
    return AddServiceListener(ServiceListenerMemberFunctor(receiver, callback),
                              static_cast<void*>(receiver),
                              filter);
  }

  /**
   * Removes the specified <code>callback</code> from the context bundle's
   * list of listeners.
   *
   * <p>
   * If the <code>(receiver,callback)</code> pair is not contained in this
   * context bundle's list of listeners, this method does nothing.
   *
   * \rststar
   * .. deprecated:: 3.1.0
   *
   *    This function exists only to maintain backwards compatibility
   *    and will be removed in the next major release.
   *    Use :any:`RemoveListener() <cppmicroservices::BundleContext::RemoveListener>` instead.
   * \endrststar
   *
   * @tparam R The type of the receiver (containing the member function to be removed)
   * @param receiver The object from which to disconnect.
   * @param callback The member function pointer to remove.
   * @throws std::runtime_error If this BundleContext is no
   *         longer valid.
   * @see AddServiceListener()
   */
  template<class R>
  US_DEPRECATED void RemoveServiceListener(
    R* receiver,
    void (R::*callback)(const ServiceEvent&))
  {
    RemoveServiceListener(ServiceListenerMemberFunctor(receiver, callback),
                          static_cast<void*>(receiver));
  }

  /**
   * Adds the specified <code>callback</code> to the context bundles's list
   * of listeners. Listeners are notified when a bundle has a lifecycle
   * state change.
   *
   * <p>
   * If the context bundle's list of listeners already contains a pair <code>(r,c)</code>
   * of <code>receiver</code> and <code>callback</code> such that
   * <code>(r == receiver && c == callback)</code>, then this method does nothing.

   * \rststar
   * .. deprecated:: 3.1.0
   *
   *    This function exists only to maintain backwards compatibility
   *    and will be removed in the next major release.
   *    Use `std::bind` to bind the member function and then pass the result to
   *    :any:`AddBundleListener(const BundleListener&) <cppmicroservices::BundleContext::AddBundleListener>` instead.
   * \endrststar
   *
   * @tparam R The type of the receiver (containing the member function to be called)
   * @param receiver The object to connect to.
   * @param callback The member function pointer to call.
   * @returns a ListenerToken object which can be used to remove the callable from the
   *          registered listeners.
   * @throws std::runtime_error If this BundleContext is no
   *         longer valid.
   * @see BundleEvent
   */
  template<class R>
  US_DEPRECATED ListenerToken
  AddBundleListener(R* receiver, void (R::*callback)(const BundleEvent&))
  {
    return AddBundleListener(BundleListenerMemberFunctor(receiver, callback),
                             static_cast<void*>(receiver));
  }

  /**
   * Removes the specified <code>callback</code> from the context bundle's
   * list of listeners.
   *
   * <p>
   * If the <code>(receiver,callback)</code> pair is not contained in this
   * context bundle's list of listeners, this method does nothing.
   *
   * \rststar
   * .. deprecated:: 3.1.0
   *
   *    This function exists only to maintain backwards compatibility
   *    and will be removed in the next major release.
   *    Use :any:`RemoveListener() <cppmicroservices::BundleContext::RemoveListener>` instead.
   * \endrststar
   *
   * @tparam R The type of the receiver (containing the member function to be removed)
   * @param receiver The object from which to disconnect.
   * @param callback The member function pointer to remove.
   * @throws std::runtime_error If this BundleContext is no
   *         longer valid.
   * @see AddBundleListener()
   */
  template<class R>
  US_DEPRECATED void RemoveBundleListener(
    R* receiver,
    void (R::*callback)(const BundleEvent&))
  {
    RemoveBundleListener(BundleListenerMemberFunctor(receiver, callback),
                         static_cast<void*>(receiver));
  }

  /**
   * Adds the specified <code>callback</code> to the context bundles's list
   * of framework listeners. Listeners are notified of framework events.
   *
   * <p>
   * If the context bundle's list of listeners already contains a pair <code>(r,c)</code>
   * of <code>receiver</code> and <code>callback</code> such that
   * <code>(r == receiver && c == callback)</code>, then this method does nothing.
   *
   * \rststar
   * .. deprecated:: 3.1.0
   *
   *    This function exists only to maintain backwards compatibility
   *    and will be removed in the next major release.
   *    Use `std::bind` to bind the member function and then pass the result to
   *    :any:`AddFrameworkListener(const FrameworkListener&) <cppmicroservices::BundleContext::AddFrameworkListener>` instead.
   * \endrststar
   *
   * @tparam R The type of the receiver (containing the member function to be called)
   * @param receiver The object to connect to.
   * @param callback The member function pointer to call.
   * @returns a ListenerToken object which can be used to remove the callable from the
   *          registered listeners.
   * @throws std::runtime_error If this BundleContext is no longer valid.
   * @see FrameworkEvent
   */
  template<class R>
  US_DEPRECATED ListenerToken
  AddFrameworkListener(R* receiver, void (R::*callback)(const FrameworkEvent&))
  {
    return AddFrameworkListener(
      BindFrameworkListenerToFunctor(receiver, callback));
  }

  /**
   * Removes the specified <code>callback</code> from the context bundle's
   * list of framework listeners.
   *
   * <p>
   * If the <code>(receiver,callback)</code> pair is not contained in this
   * context bundle's list of listeners, this method does nothing.
   *
   * \rststar
   * .. deprecated:: 3.1.0
   *
   *    This function exists only to maintain backwards compatibility
   *    and will be removed in the next major release.
   *    Use :any:`RemoveListener() <cppmicroservices::BundleContext::RemoveListener>` instead.
   * \endrststar
   *
   * @tparam R The type of the receiver (containing the member function to be removed)
   * @param receiver The object from which to disconnect.
   * @param callback The member function pointer to remove.
   * @throws std::runtime_error If this BundleContext is no longer valid.
   * @see AddFrameworkListener()
   */
  template<class R>
  US_DEPRECATED void RemoveFrameworkListener(
    R* receiver,
    void (R::*callback)(const FrameworkEvent&))
  {
    RemoveFrameworkListener(BindFrameworkListenerToFunctor(receiver, callback));
  }

  /**
   * Get the absolute path for a file or directory in the persistent
   * storage area provided for the bundle.
   *
   * The absolute path for the base directory of the persistent storage
   * area provided for the context bundle by the Framework can be obtained by
   * calling this method with an empty string as \c filename.
   *
   * @param filename A relative name to the file or directory to be accessed.
   * @return The absolute path to the persistent storage area for the given file name.
   * @throws std::runtime_error If this BundleContext is no longer valid.
   * @throws std::invalid_argument If the input param filename is not a valid 
   *         UTF-8 string.
   */
  std::string GetDataFile(const std::string& filename) const;

  /**
   * Installs all bundles from the bundle library at the specified location.
   *
   * The following steps are required to install a bundle:
   * -# If a bundle containing the same install location is already installed, the Bundle object for that
   *    bundle is returned.
   * -# The bundle's associated resources are allocated. The associated resources minimally consist of a
   *    unique identifier and a persistent storage area if the platform has file system support. If this step
   *    fails, a std::runtime_error is thrown.
   * -# A bundle event of type <code>BundleEvent::BUNDLE_INSTALLED</code> is fired.
   * -# The Bundle object for the newly or previously installed bundle is returned.
   *
   * @remarks An install location is an absolute path to a shared library or executable file
   * which may contain several bundles, i. e. acts as a bundle library.
   *
   * @param location The location of the bundle library to install.
   * @return The Bundle objects of the installed bundle library.
   * @throws std::runtime_error If the BundleContext is no longer valid, or if the installation failed.
   * @throws std::logic_error If the framework instance is no longer active
   * @throws std::invalid_argument If the location is not a valid UTF8 string
   */
  std::vector<Bundle> InstallBundles(const std::string& location);

private:
  friend US_Framework_EXPORT BundleContext
  MakeBundleContext(BundleContextPrivate*);
  friend BundleContext MakeBundleContext(
    const std::shared_ptr<BundleContextPrivate>&);
  friend std::shared_ptr<BundleContextPrivate> GetPrivate(const BundleContext&);

  BundleContext(const std::shared_ptr<BundleContextPrivate>& ctx);
  // allow templated code to use the internal logger
  template<class S, class TTT, class R>
  friend class detail::BundleAbstractTracked;
  template<class S, class T>
  friend class ServiceTracker;
  template<class S, class TTT>
  friend class detail::ServiceTrackerPrivate;
  template<class S, class TTT>
  friend class detail::TrackedService;
  friend class BundleResource;

  // Not for use by clients of the Framework.
  // Provides access to the Framework's log sink to allow templated code
  // to log diagnostic information.
  std::shared_ptr<detail::LogSink> GetLogSink() const;

  ListenerToken AddServiceListener(const ServiceListener& delegate,
                                   void* data,
                                   const std::string& filter);
  void RemoveServiceListener(const ServiceListener& delegate, void* data);

  ListenerToken AddBundleListener(const BundleListener& delegate, void* data);
  void RemoveBundleListener(const BundleListener& delegate, void* data);

  std::shared_ptr<BundleContextPrivate> d;
};

} // namespace cppmicroservices

#endif /* CPPMICROSERVICES_BUNDLECONTEXT_H */
