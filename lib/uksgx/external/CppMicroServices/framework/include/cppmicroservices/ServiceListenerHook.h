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

#ifndef CPPMICROSERVICES_SERVICELISTENERHOOK_H
#define CPPMICROSERVICES_SERVICELISTENERHOOK_H

#include "cppmicroservices/ServiceInterface.h"
#include "cppmicroservices/SharedData.h"
#include "cppmicroservices/ShrinkableVector.h"

#include <string>

namespace cppmicroservices {

class BundleContext;
class ServiceListenerEntry;

/**
\defgroup gr_servicelistenerhook ServiceListenerHook

\brief Groups ServiceListenerHook class related symbols.
*/

/**
 * @ingroup MicroServices
 * @ingroup gr_servicelistenerhook
 *
 * Service Listener Hook Service.
 *
 * <p>
 * Bundles registering this service will be called during service listener
 * addition and removal.
 *
 * @remarks Implementations of this interface are required to be thread-safe.
 */
struct US_Framework_EXPORT ServiceListenerHook
{

  class ListenerInfoData;

  /**
   * Information about a Service Listener. This class describes the bundle
   * which added the Service Listener and the filter with which it was added.
   *
   * @remarks This class is not intended to be implemented by clients.
   */
  struct US_Framework_EXPORT ListenerInfo
  {
    ListenerInfo();

    ListenerInfo(const ListenerInfo& other);

    ~ListenerInfo();

    ListenerInfo& operator=(const ListenerInfo& other);

    /**
     * Can be used to check if this ListenerInfo instance is valid,
     * or if it has been constructed using the default constructor.
     *
     * @return <code>true</code> if this listener object is valid,
     *         <code>false</code> otherwise.
     */
    bool IsNull() const;

    /**
     * Return the context of the bundle which added the listener.
     *
     * @return The context of the bundle which added the listener.
     */
    BundleContext GetBundleContext() const;

    /**
     * Return the filter string with which the listener was added.
     *
     * @return The filter string with which the listener was added. This may
     *         be empty if the listener was added without a filter.
     */
    std::string GetFilter() const;

    /**
     * Return the state of the listener for this addition and removal life
     * cycle. Initially this method will return \c false indicating the
     * listener has been added but has not been removed. After the listener
     * has been removed, this method must always returns \c true.
     *
     * <p>
     * There is an extremely rare case in which removed notification to
     * {@link ServiceListenerHook}s can be made before added notification if two
     * threads are racing to add and remove the same service listener.
     * Because {@link ServiceListenerHook}s are called synchronously during service
     * listener addition and removal, the CppMicroServices library cannot guarantee
     * in-order delivery of added and removed notification for a given
     * service listener. This method can be used to detect this rare
     * occurrence.
     *
     * @return \c false if the listener has not been been removed,
     *         \c true otherwise.
     */
    bool IsRemoved() const;

    /**
     * Compares this \c ListenerInfo to another \c ListenerInfo.
     * Two \c ListenerInfos are equal if they refer to the same
     * listener for a given addition and removal life cycle. If the same
     * listener is added again, it will have a different
     * \c ListenerInfo which is not equal to this \c ListenerInfo.
     *
     * @param other The object to compare against this \c ListenerInfo.
     * @return \c true if the other object is a \c ListenerInfo
     *         object and both objects refer to the same listener for a
     *         given addition and removal life cycle.
     */
    bool operator==(const ListenerInfo& other) const;

  private:
    friend class ServiceListenerEntry;

    friend struct ::std::hash<ServiceListenerHook::ListenerInfo>;

    ListenerInfo(ListenerInfoData* data);

    ExplicitlySharedDataPointer<ListenerInfoData> d;
  };

  virtual ~ServiceListenerHook();

  /**
   * Added listeners hook method. This method is called to provide the hook
   * implementation with information on newly added service listeners. This
   * method will be called as service listeners are added while this hook is
   * registered. Also, immediately after registration of this hook, this
   * method will be called to provide the current collection of service
   * listeners which had been added prior to the hook being registered.
   *
   * @param listeners A collection of \c ListenerInfo objects for newly added
   *        service listeners which are now listening to service events.
   */
  virtual void Added(const std::vector<ListenerInfo>& listeners) = 0;

  /**
   * Removed listeners hook method. This method is called to provide the hook
   * implementation with information on newly removed service listeners. This
   * method will be called as service listeners are removed while this hook is
   * registered.
   *
   * @param listeners A collection of \c ListenerInfo objects for newly removed
   *        service listeners which are no longer listening to service events.
   */
  virtual void Removed(const std::vector<ListenerInfo>& listeners) = 0;
};
}

/**
 * \ingroup MicroServices
 * \ingroup gr_servicelistenerhook
 *
 * \struct std::hash<cppmicroservices::ServiceListenerHook::ListenerInfo> ServiceListenerHook.h <cppmicroservices/ServiceListenerHook.h>
 *
 * Hash functor specialization for \link cppmicroservices#ServiceListenerHook::ListenerInfo ServiceListenerHook::ListenerInfo\endlink objects.
 */

US_HASH_FUNCTION_BEGIN(cppmicroservices::ServiceListenerHook::ListenerInfo)
return hash<const cppmicroservices::ServiceListenerHook::ListenerInfoData*>()(
  arg.d.Data());
US_HASH_FUNCTION_END

#endif // CPPMICROSERVICES_SERVICELISTENERHOOK_H
