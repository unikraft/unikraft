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

#ifndef CPPMICROSERVICES_SERVICETRACKER_H
#define CPPMICROSERVICES_SERVICETRACKER_H

#include <chrono>
#include <map>

#include "cppmicroservices/LDAPFilter.h"
#include "cppmicroservices/ServiceReference.h"
#include "cppmicroservices/ServiceTrackerCustomizer.h"

namespace cppmicroservices {

namespace detail {
template<class S, class T>
class TrackedService;
template<class S, class T>
class ServiceTrackerPrivate;
}

class BundleContext;

/**
\defgroup gr_servicetracker ServiceTracker

\brief Groups ServiceTracker related symbols.
*/

/**
 * \ingroup MicroServices
 * \ingroup gr_servicetracker
 *
 * The <code>ServiceTracker</code> class simplifies using services from the
 * framework's service registry.
 * <p>
 * A <code>ServiceTracker</code> object is constructed with search criteria and
 * a <code>ServiceTrackerCustomizer</code> object. A <code>ServiceTracker</code>
 * can use a <code>ServiceTrackerCustomizer</code> to customize the service
 * objects to be tracked. The <code>ServiceTracker</code> can then be opened to
 * begin tracking all services in the framework's service registry that match
 * the specified search criteria. The <code>ServiceTracker</code> correctly
 * handles all of the details of listening to <code>ServiceEvent</code>s and
 * getting and ungetting services.
 * <p>
 * The <code>GetServiceReferences</code> method can be called to get references
 * to the services being tracked. The <code>GetService</code> and
 * <code>GetServices</code> methods can be called to get the service objects for
 * the tracked service.
 *
 * \note The <code>ServiceTracker</code> class is thread-safe. It does not call a
 *       <code>ServiceTrackerCustomizer</code> while holding any locks.
 *       <code>ServiceTrackerCustomizer</code> implementations must also be
 *       thread-safe.
 *
 * Customization of the services to be tracked requires the tracked type to be
 * default constructible and convertible to \c bool. To customize a tracked
 * service using a custom type with value-semantics like
 * \snippet uServices-servicetracker/main.cpp tt
 * a custom ServiceTrackerCustomizer is required. It provides code to
 * associate the tracked service with the custom tracked type:
 * \snippet uServices-servicetracker/main.cpp customizer
 * Instantiation of a ServiceTracker with the custom customizer looks like this:
 * \snippet uServices-servicetracker/main.cpp tracker
 *
 * @tparam S The type of the service being tracked. The type S* must be an
 *         assignable datatype.
 * @tparam T The tracked object.
 *
 * @remarks This class is thread safe.
 */
template<class S, class T = S>
class ServiceTracker : protected ServiceTrackerCustomizer<S, T>
{
public:
  /// The type of the tracked object
  typedef
    typename ServiceTrackerCustomizer<S, T>::TrackedParmType TrackedParmType;

  typedef std::map<ServiceReference<S>, std::shared_ptr<TrackedParmType>>
    TrackingMap;

  ~ServiceTracker();

  /**
   * Create a <code>ServiceTracker</code> on the specified
   * <code>ServiceReference</code>.
   *
   * <p>
   * The service referenced by the specified <code>ServiceReference</code>
   * will be tracked by this <code>ServiceTracker</code>.
   *
   * @param context The <code>BundleContext</code> against which the tracking
   *        is done.
   * @param reference The <code>ServiceReference</code> for the service to be
   *        tracked.
   * @param customizer The customizer object to call when services are added,
   *        modified, or removed in this <code>ServiceTracker</code>. If
   *        customizer is <code>null</code>, then this
   *        <code>ServiceTracker</code> will be used as the
   *        <code>ServiceTrackerCustomizer</code> and this
   *        <code>ServiceTracker</code> will call the
   *        <code>ServiceTrackerCustomizer</code> methods on itself.
   */
  ServiceTracker(const BundleContext& context,
                 const ServiceReference<S>& reference,
                 ServiceTrackerCustomizer<S, T>* customizer = nullptr);

  /**
   * Create a <code>ServiceTracker</code> on the specified class name.
   *
   * <p>
   * Services registered under the specified class name will be tracked by
   * this <code>ServiceTracker</code>.
   *
   * @param context The <code>BundleContext</code> against which the tracking
   *        is done.
   * @param clazz The class name of the services to be tracked.
   * @param customizer The customizer object to call when services are added,
   *        modified, or removed in this <code>ServiceTracker</code>. If
   *        customizer is <code>null</code>, then this
   *        <code>ServiceTracker</code> will be used as the
   *        <code>ServiceTrackerCustomizer</code> and this
   *        <code>ServiceTracker</code> will call the
   *        <code>ServiceTrackerCustomizer</code> methods on itself.
   */
  ServiceTracker(const BundleContext& context,
                 const std::string& clazz,
                 ServiceTrackerCustomizer<S, T>* customizer = nullptr);

  /**
   * Create a <code>ServiceTracker</code> on the specified
   * <code>LDAPFilter</code> object.
   *
   * <p>
   * Services which match the specified <code>LDAPFilter</code> object will be
   * tracked by this <code>ServiceTracker</code>.
   *
   * @param context The <code>BundleContext</code> against which the tracking
   *        is done.
   * @param filter The <code>LDAPFilter</code> to select the services to be
   *        tracked.
   * @param customizer The customizer object to call when services are added,
   *        modified, or removed in this <code>ServiceTracker</code>. If
   *        customizer is null, then this <code>ServiceTracker</code> will be
   *        used as the <code>ServiceTrackerCustomizer</code> and this
   *        <code>ServiceTracker</code> will call the
   *        <code>ServiceTrackerCustomizer</code> methods on itself.
   */
  ServiceTracker(const BundleContext& context,
                 const LDAPFilter& filter,
                 ServiceTrackerCustomizer<S, T>* customizer = nullptr);

  /**
   * Create a <code>ServiceTracker</code> on the class template
   * argument S.
   *
   * <p>
   * Services registered under the interface name of the class template
   * argument S will be tracked by this <code>ServiceTracker</code>.
   *
   * @param context The <code>BundleContext</code> against which the tracking
   *        is done.
   * @param customizer The customizer object to call when services are added,
   *        modified, or removed in this <code>ServiceTracker</code>. If
   *        customizer is null, then this <code>ServiceTracker</code> will be
   *        used as the <code>ServiceTrackerCustomizer</code> and this
   *        <code>ServiceTracker</code> will call the
   *        <code>ServiceTrackerCustomizer</code> methods on itself.
   */
  ServiceTracker(const BundleContext& context,
                 ServiceTrackerCustomizer<S, T>* customizer = nullptr);

  /**
   * Open this <code>ServiceTracker</code> and begin tracking services.
   *
   * <p>
   * Services which match the search criteria specified when this
   * <code>ServiceTracker</code> was created are now tracked by this
   * <code>ServiceTracker</code>.
   *
   * @throws std::runtime_error If the <code>BundleContext</code>
   *         with which this <code>ServiceTracker</code> was created is no
   *         longer valid.
   * @throws std::runtime_error If the LDAP filter used to construct
   *         the <code>ServiceTracker</code> contains an invalid filter 
   *         expression that cannot be parsed.
   */
  virtual void Open();

  /**
   * Close this <code>ServiceTracker</code>.
   *
   * <p>
   * This method should be called when this <code>ServiceTracker</code> should
   * end the tracking of services.
   *
   * <p>
   * This implementation calls GetServiceReferences() to get the list
   * of tracked services to remove.
   *
   * @throws std::runtime_error If the <code>BundleContext</code>
   *         with which this <code>ServiceTracker</code> was created is no
   *         longer valid.
   */
  virtual void Close();

  /**
   * Wait for at least one service to be tracked by this
   * <code>ServiceTracker</code>. This method will also return when this
   * <code>ServiceTracker</code> is closed.
   *
   * <p>
   * It is strongly recommended that <code>WaitForService</code> is not used
   * during the calling of the <code>BundleActivator</code> methods.
   * <code>BundleActivator</code> methods are expected to complete in a short
   * period of time.
   *
   * <p>
   * This implementation calls GetService() to determine if a service
   * is being tracked.
   *
   * @return Returns the result of GetService().
   */
  std::shared_ptr<TrackedParmType> WaitForService();

  /**
   * Wait for at least one service to be tracked by this
   * <code>ServiceTracker</code>. This method will also return when this
   * <code>ServiceTracker</code> is closed.
   *
   * <p>
   * It is strongly recommended that <code>WaitForService</code> is not used
   * during the calling of the <code>BundleActivator</code> methods.
   * <code>BundleActivator</code> methods are expected to complete in a short
   * period of time.
   *
   * <p>
   * This implementation calls GetService() to determine if a service
   * is being tracked.
   *
   * @param rel_time The relative time duration to wait for a service. If
   *        zero, the method will wait indefinitely.
   * @throws std::invalid_argument exception if \c rel_time is negative.
   * @return Returns the result of GetService().
   */
  template<class Rep, class Period>
  std::shared_ptr<TrackedParmType> WaitForService(
    const std::chrono::duration<Rep, Period>& rel_time);

  /**
   * Return a list of <code>ServiceReference</code>s for all services being
   * tracked by this <code>ServiceTracker</code>.
   *
   * @return List of <code>ServiceReference</code>s.
   */
  virtual std::vector<ServiceReference<S>> GetServiceReferences() const;

  /**
   * Returns a <code>ServiceReference</code> for one of the services being
   * tracked by this <code>ServiceTracker</code>.
   *
   * <p>
   * If multiple services are being tracked, the service with the highest
   * ranking (as specified in its <code>service.ranking</code> property) is
   * returned. If there is a tie in ranking, the service with the lowest
   * service ID (as specified in its <code>service.id</code> property); that
   * is, the service that was registered first is returned. This is the same
   * algorithm used by <code>BundleContext::GetServiceReference()</code>.
   *
   * <p>
   * This implementation calls GetServiceReferences() to get the list
   * of references for the tracked services.
   *
   * @return A <code>ServiceReference</code> for a tracked service.
   * @throws ServiceException if no services are being tracked.
   */
  virtual ServiceReference<S> GetServiceReference() const;

  /**
   * Returns the service object for the specified
   * <code>ServiceReference</code> if the specified referenced service is
   * being tracked by this <code>ServiceTracker</code>.
   *
   * @param reference The reference to the desired service.
   * @return A service object or <code>null</code> if the service referenced
   *         by the specified <code>ServiceReference</code> is not being
   *         tracked.
   */
  virtual std::shared_ptr<TrackedParmType> GetService(
    const ServiceReference<S>& reference) const;

  /**
   * Return a list of service objects for all services being tracked by this
   * <code>ServiceTracker</code>.
   *
   * <p>
   * This implementation calls GetServiceReferences() to get the list
   * of references for the tracked services and then calls
   * GetService(const ServiceReference&) for each reference to get the
   * tracked service object.
   *
   * @return A list of service objects or an empty list if no services
   *         are being tracked.
   */
  virtual std::vector<std::shared_ptr<TrackedParmType>> GetServices() const;

  /**
   * Returns a service object for one of the services being tracked by this
   * <code>ServiceTracker</code>.
   *
   * <p>
   * If any services are being tracked, this implementation returns the result
   * of calling <code>%GetService(%GetServiceReference())</code>.
   *
   * @return A service object or <code>null</code> if no services are being
   *         tracked.
   */
  virtual std::shared_ptr<TrackedParmType> GetService() const;

  /**
   * Remove a service from this <code>ServiceTracker</code>.
   *
   * The specified service will be removed from this
   * <code>ServiceTracker</code>. If the specified service was being tracked
   * then the <code>ServiceTrackerCustomizer::RemovedService</code> method will
   * be called for that service.
   *
   * @param reference The reference to the service to be removed.
   */
  virtual void Remove(const ServiceReference<S>& reference);

  /**
   * Return the number of services being tracked by this
   * <code>ServiceTracker</code>.
   *
   * @return The number of services being tracked.
   */
  virtual int Size() const;

  /**
   * Returns the tracking count for this <code>ServiceTracker</code>.
   *
   * The tracking count is initialized to 0 when this
   * <code>ServiceTracker</code> is opened. Every time a service is added,
   * modified or removed from this <code>ServiceTracker</code>, the tracking
   * count is incremented.
   *
   * <p>
   * The tracking count can be used to determine if this
   * <code>ServiceTracker</code> has added, modified or removed a service by
   * comparing a tracking count value previously collected with the current
   * tracking count value. If the value has not changed, then no service has
   * been added, modified or removed from this <code>ServiceTracker</code>
   * since the previous tracking count was collected.
   *
   * @return The tracking count for this <code>ServiceTracker</code> or -1 if
   *         this <code>ServiceTracker</code> is not open.
   */
  virtual int GetTrackingCount() const;

  /**
   * Return a sorted map of the <code>ServiceReference</code>s and
   * service objects for all services being tracked by this
   * <code>ServiceTracker</code>. The map is sorted in natural order
   * of <code>ServiceReference</code>. That is, the last entry is the service
   * with the highest ranking and the lowest service id.
   *
   * @param tracked A <code>TrackingMap</code> with the <code>ServiceReference</code>s
   *         and service objects for all services being tracked by this
   *         <code>ServiceTracker</code>. If no services are being tracked,
   *         then the returned map is empty.
   */
  virtual void GetTracked(TrackingMap& tracked) const;

  /**
   * Return if this <code>ServiceTracker</code> is empty.
   *
   * @return <code>true</code> if this <code>ServiceTracker</code> is not tracking any
   *         services.
   */
  virtual bool IsEmpty() const;

protected:
  /**
   * Default implementation of the
   * <code>ServiceTrackerCustomizer::AddingService</code> method.
   *
   * <p>
   * This method is only called when this <code>ServiceTracker</code> has been
   * constructed with a <code>null</code> ServiceTrackerCustomizer argument.
   *
   * <p>
   * This implementation returns the result of calling <code>GetService</code>
   * on the <code>BundleContext</code> with which this
   * <code>ServiceTracker</code> was created passing the specified
   * <code>ServiceReference</code>.
   * <p>
   * This method can be overridden in a subclass to customize the service
   * object to be tracked for the service being added. In that case, take care
   * not to rely on the default implementation of #RemovedService to unget the
   * service.
   *
   * @param reference The reference to the service being added to this
   *        <code>ServiceTracker</code>.
   * @return The service object to be tracked for the service added to this
   *         <code>ServiceTracker</code>.
   * @see ServiceTrackerCustomizer::AddingService(const ServiceReference&)
   */
  std::shared_ptr<TrackedParmType> AddingService(
    const ServiceReference<S>& reference);

  /**
   * Default implementation of the
   * <code>ServiceTrackerCustomizer::ModifiedService</code> method.
   *
   * <p>
   * This method is only called when this <code>ServiceTracker</code> has been
   * constructed with a <code>null</code> ServiceTrackerCustomizer argument.
   *
   * <p>
   * This implementation does nothing.
   *
   * @param reference The reference to modified service.
   * @param service The service object for the modified service.
   * @see ServiceTrackerCustomizer::ModifiedService(const ServiceReference&, TrackedArgType)
   */
  void ModifiedService(const ServiceReference<S>& reference,
                       const std::shared_ptr<TrackedParmType>& service);

  /**
   * Default implementation of the
   * <code>ServiceTrackerCustomizer::RemovedService</code> method.
   *
   * <p>
   * This method is only called when this <code>ServiceTracker</code> has been
   * constructed with a <code>null</code> ServiceTrackerCustomizer argument.
   *
   * This method can be overridden in a subclass. If the default
   * implementation of the #AddingService
   * method was used, this method must unget the service.
   *
   * @param reference The reference to removed service.
   * @param service The service object for the removed service.
   * @see ServiceTrackerCustomizer::RemovedService(const ServiceReferenceType&, TrackedArgType)
   */
  void RemovedService(const ServiceReference<S>& reference,
                      const std::shared_ptr<TrackedParmType>& service);

private:
  typedef typename ServiceTrackerCustomizer<S, T>::TypeTraits TypeTraits;

  typedef ServiceTracker<S, T> _ServiceTracker;
  typedef detail::TrackedService<S, TypeTraits> _TrackedService;
  typedef detail::ServiceTrackerPrivate<S, TypeTraits> _ServiceTrackerPrivate;
  typedef ServiceTrackerCustomizer<S, T> _ServiceTrackerCustomizer;

  friend class detail::TrackedService<S, TypeTraits>;
  friend class detail::ServiceTrackerPrivate<S, TypeTraits>;

  std::unique_ptr<_ServiceTrackerPrivate> d;
};
}

#include "cppmicroservices/detail/ServiceTracker.tpp"

#endif // CPPMICROSERVICES_SERVICETRACKER_H
