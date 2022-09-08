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

#ifndef CPPMICROSERVICES_SERVICELISTENERS_H
#define CPPMICROSERVICES_SERVICELISTENERS_H

#include "cppmicroservices/GlobalConfig.h"
#include "cppmicroservices/detail/Threads.h"

#include "ServiceListenerEntry.h"

#include <list>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace cppmicroservices {

class CoreBundleContext;
class BundleContextPrivate;

/**
 * Here we handle all listeners that bundles have registered.
 *
 */
class ServiceListeners : private detail::MultiThreaded<>
{

public:
  typedef std::tuple<BundleListener, void*> BundleListenerEntry;
  typedef std::unordered_map<
    std::shared_ptr<BundleContextPrivate>,
    std::unordered_map<ListenerTokenId, BundleListenerEntry>>
    BundleListenerMap;
  struct : public MultiThreaded<>
  {
    BundleListenerMap value;
  } bundleListenerMap;

  typedef std::unordered_map<std::string, std::list<ServiceListenerEntry>>
    CacheType;
  typedef std::unordered_set<ServiceListenerEntry> ServiceListenerEntries;

  typedef std::tuple<FrameworkListener, void*> FrameworkListenerEntry;
  typedef std::unordered_map<
    std::shared_ptr<BundleContextPrivate>,
    std::unordered_map<ListenerTokenId, FrameworkListenerEntry>>
    FrameworkListenerMap;

private:
  std::atomic<uint64_t> listenerId;

  struct : public MultiThreaded<>
  {
    FrameworkListenerMap value;
  } frameworkListenerMap;

  std::vector<std::string> hashedServiceKeys;
  static const int OBJECTCLASS_IX = 0;
  static const int SERVICE_ID_IX = 1;

  /* Service listeners with complicated or empty filters */
  std::list<ServiceListenerEntry> complicatedListeners;

  /* Service listeners with "simple" filters are cached. */
  CacheType cache[2];

  ServiceListenerEntries serviceSet;

  CoreBundleContext* coreCtx;

public:
  ServiceListeners(CoreBundleContext* coreCtx);

  void Clear();

  /**
   * Add a new service listener. If an old one exists, and it has the
   * same owning bundle, the old listener is removed first.
   *
   * @param context The bundle context adding this listener.
   * @param listener The service listener to add.
   * @param data Additional data to distinguish ServiceListener objects.
   * @param filter An LDAP filter string to check when a service is modified.
   * @returns a ListenerToken object that corresponds to the listener.
   * @exception org.osgi.framework.InvalidSyntaxException
   * If the filter is not a correct LDAP expression.
   */
  ListenerToken AddServiceListener(
    const std::shared_ptr<BundleContextPrivate>& context,
    const ServiceListener& listener,
    void* data,
    const std::string& filter);

  /**
   * Remove service listener from current framework. Silently ignore
   * if listener doesn't exist.
   *
   * @param context The bundle context who wants to remove listener.
   * @param tokenId The ListenerTokenId associated with the listener.
   * @param listener Object to remove.
   * @param data Additional data to distinguish ServiceListener objects.
   */
  void RemoveServiceListener(
    const std::shared_ptr<BundleContextPrivate>& context,
    ListenerTokenId tokenId,
    const ServiceListener& listener,
    void* data);

  /**
   * Add a new bundle listener.
   *
   * @param context The bundle context adding this listener.
   * @param listener The bundle listener to add.
   * @param data Additional data to distinguish BundleListener objects.
   * @returns a ListenerToken object that corresponds to the listener.
   */
  ListenerToken AddBundleListener(
    const std::shared_ptr<BundleContextPrivate>& context,
    const BundleListener& listener,
    void* data);

  /**
   * Remove bundle listener from current framework. If listener doesn't
   * exist, this method does nothing.
   *
   * @param context The bundle context who wants to remove listener.
   * @param listener Object to remove.
   * @param data Additional data to distinguish BundleListener objects.
   */
  void RemoveBundleListener(
    const std::shared_ptr<BundleContextPrivate>& context,
    const BundleListener& listener,
    void* data);

  /**
  * Add a new framework listener.
  *
  * @param context The bundle context adding this listener.
  * @param listener The framework listener to add.
  * @param data Additional data to distinguish FrameworkListener objects.
  * @returns a ListenerToken object that corresponds to the listener.
  */
  ListenerToken AddFrameworkListener(
    const std::shared_ptr<BundleContextPrivate>& context,
    const FrameworkListener& listener,
    void* data);

  /**
  * Remove framework listener from current framework. If listener doesn't
  * exist, this method does nothing.
  *
  * @param context The bundle context who wants to remove listener.
  * @param listener Object to remove.
  * @param data Additional data to distinguish FrameworkListener objects.
  */
  void RemoveFrameworkListener(
    const std::shared_ptr<BundleContextPrivate>& context,
    const FrameworkListener& listener,
    void* data);

  /**
   * Remove either a service, bundle or framework listener from current framework.
   * If the token is invalid, this method does nothing.
   *
   * @param context The bundle context who wants to remove the listener.
   * @param token A ListenerToken type object which corresponds to the listener.
   */
  void RemoveListener(const std::shared_ptr<BundleContextPrivate>& context,
                      ListenerToken token);

  void SendFrameworkEvent(const FrameworkEvent& evt);

  void BundleChanged(const BundleEvent& evt);

  /**
   * Remove all listener registered by a bundle in the current framework.
   *
   * @param context Bundle context which listeners we want to remove.
   */
  void RemoveAllListeners(const std::shared_ptr<BundleContextPrivate>& context);

  /**
   * Notify hooks that a bundle is about to be stopped
   *
   * @param context Bundle context which listeners are about to be removed.
   */
  void HooksBundleStopped(const std::shared_ptr<BundleContextPrivate>& context);

  /**
   * Receive notification that a service has had a change occur in its lifecycle.
   *
   * @see org.osgi.framework.ServiceListener#serviceChanged
   */
  void ServiceChanged(ServiceListenerEntries& receivers,
                      const ServiceEvent& evt,
                      ServiceListenerEntries& matchBefore);

  void ServiceChanged(ServiceListenerEntries& receivers,
                      const ServiceEvent& evt);

  /**
   *
   *
   */
  void GetMatchingServiceListeners(const ServiceEvent& evt,
                                   ServiceListenerEntries& listeners);

  std::vector<ServiceListenerHook::ListenerInfo> GetListenerInfoCollection()
    const;

private:
  /**
   * Factory method that returns an unique ListenerToken object.
   * Called by methods which add listeners.
   */
  ListenerToken MakeListenerToken();

  /**
   * Remove all references to a service listener from the service listener
   * cache.
   */
  void RemoveFromCache_unlocked(const ServiceListenerEntry& sle);

  /**
   * Checks if the specified service listener's filter is simple enough
   * to cache.
   */
  void CheckSimple_unlocked(const ServiceListenerEntry& sle);

  void AddToSet_unlocked(ServiceListenerEntries& set,
                         const ServiceListenerEntries& receivers,
                         int cache_ix,
                         const std::string& val);
};
}

#endif // CPPMICROSERVICES_SERVICELISTENERS_H
