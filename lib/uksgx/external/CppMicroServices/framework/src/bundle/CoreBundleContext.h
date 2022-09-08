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

/**

\defgroup MicroServices Micro Services Classes

\brief This category includes classes related to the C++ Micro Services component.

The C++ Micro Services component provides a dynamic service registry based on the service layer
as specified in the OSGi R4.2 specifications.

*/

/**

\defgroup MicroServicesUtils Utility Classes

\brief This category includes utility classes which can be used by others.

*/

#ifndef CPPMICROSERVICES_COREBUNDLECONTEXT_H
#define CPPMICROSERVICES_COREBUNDLECONTEXT_H

#include "cppmicroservices/Any.h"
#include "cppmicroservices/detail/Log.h"
#include "cppmicroservices/detail/Threads.h"

#include "BundleHooks.h"
#include "BundleRegistry.h"
#include "Debug.h"
#include "Resolver.h"
#include "ServiceHooks.h"
#include "ServiceListeners.h"
#include "ServiceRegistry.h"

#include <map>
#include <ostream>
#include <string>

namespace cppmicroservices {

class ResolverHooks
{
  // dummy implementation
public:
  void CheckResolveBlocked() {}
  void BeginResolve(BundlePrivate*) {}
  void EndResolve(BundlePrivate*) {}
};

struct BundleStorage;
class BundleThread;
class FrameworkPrivate;

/**
 * This class is not part of the public API.
 */
class CoreBundleContext
{
public:
  /**
   * Framework id.
   */
  int id;

  /**
   * Id to use for next instance of the framework.
   */
  static std::atomic<int> globalId;

  /*
  * Framework properties, which contain both the
  * launch properties and the system properties.
  * See OSGi spec revision 6, section 4.2.2
  *
  * Note: CppMicroServices currently has no concept
  * of "system properties".
  */
  std::unordered_map<std::string, Any> frameworkProperties;

  const std::string& workingDir;

  /**
  * The diagnostic logging sink
  * For internal Framework use only. Do not expose
  * to Framework clients.
  */
  std::shared_ptr<detail::LogSink> sink;

  /**
   * Debug handle.
   */
  Debug debug;

  /**
   * Threads for running listeners and activators
   */
  struct : detail::MultiThreaded<>
  {
    std::list<std::shared_ptr<BundleThread>> value;
    std::list<std::shared_ptr<BundleThread>> zombies;
  } bundleThreads;

  /**
   * Bundle Storage
   */
  std::unique_ptr<BundleStorage> storage;

  /**
   * Private Bundle Data Storage
   */
  std::string dataStorage;

  /**
   * All listeners in this framework.
   */
  ServiceListeners listeners;

  /**
   * All registered services in this framework.
   */
  ServiceRegistry services;

  /**
   * All service hooks.
   */
  ServiceHooks serviceHooks;

  /**
   * All bundle hooks.
   */
  BundleHooks bundleHooks;

  /**
   * All resolver hooks.
   */
  ResolverHooks resolverHooks;

  /**
   * All capabilities, exported and imported packages in this framework.
   */
  Resolver resolver;

  /**
   * All installed bundles.
   */
  BundleRegistry bundleRegistry;

  bool firstInit;

  /**
   * Framework init count.
   */
  int initCount;

  std::shared_ptr<FrameworkPrivate> systemBundle;

  ~CoreBundleContext();

  // thread-safe shared_from_this implementation
  std::shared_ptr<CoreBundleContext> shared_from_this() const;
  void SetThis(const std::shared_ptr<CoreBundleContext>& self);

  void Init();

  // must be called without any locks held
  void Uninit0();

  void Uninit1();

  /**
   * Get private bundle data storage file handle.
   *
   */
  std::string GetDataStorage(long id) const;

private:
  // The core context is exclusively constructed by the FrameworkFactory class
  friend class FrameworkFactory;

  /**
   * Construct a core context
   *
   */
  CoreBundleContext(const std::unordered_map<std::string, Any>& props,
                    std::ostream* logger);

  struct : detail::MultiThreaded<>
  {
    std::weak_ptr<CoreBundleContext> v;
  } self;
};
}

#endif // CPPMICROSERVICES_COREBUNDLECONTEXT_H
