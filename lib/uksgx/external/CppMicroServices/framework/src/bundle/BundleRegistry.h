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

#ifndef CPPMICROSERVICES_BUNDLEREGISTRY_H
#define CPPMICROSERVICES_BUNDLEREGISTRY_H

#include "cppmicroservices/detail/Threads.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace cppmicroservices {

class CoreBundleContext;
class Framework;
class Bundle;
class BundlePrivate;
class BundleVersion;
struct BundleActivator;

/**
 * Here we handle all the bundles that are known to the framework.
 * @remarks This class is thread-safe.
 */
class BundleRegistry : private detail::MultiThreaded<>
{

public:
  BundleRegistry(CoreBundleContext* coreCtx);
  virtual ~BundleRegistry(void);

  void Init();

  void Clear();

  /**
   * Install a new bundle library.
   *
   * @param location The location to be installed
   * @param caller The bundle performing the install
   * @return A vector of bundles installed
   */
  std::vector<Bundle> Install(const std::string& location,
                              BundlePrivate* caller);

  std::vector<Bundle> Install0(
    const std::string& location,
    const std::vector<std::shared_ptr<BundlePrivate>>& exclude,
    BundlePrivate* caller);

  /**
   * Remove bundle registration.
   *
   * @param location The location to be removed
   */
  void Remove(const std::string& location, long id);

  /**
   * Get the bundle that has the specified bundle identifier.
   *
   * @param id The identifier of the bundle to get.
   * @return Bundle or null
   *         if the bundle was not found.
   */
  std::shared_ptr<BundlePrivate> GetBundle(long id) const;

  /**
   * Get the bundles that have the specified bundle location.
   *
   * @param location The location of the bundles to get.
   * @return A list of Bundle instances.
   */
  std::vector<std::shared_ptr<BundlePrivate>> GetBundles(
    const std::string& location) const;

  /**
   * Get all bundles that have the specified bundle symbolic
   * name and version.
   *
   * @param name The symbolic name of bundle to get.
   * @param version The bundle version of bundle to get.
   * @return Collection of BundleImpls.
   */
  std::vector<std::shared_ptr<BundlePrivate>> GetBundles(
    const std::string& name,
    const BundleVersion& version) const;

  /**
   * Get all known bundles.
   *
   * @return A list which is filled with all known bundles.
   */
  std::vector<std::shared_ptr<BundlePrivate>> GetBundles() const;

  /**
   * Get all bundles currently in bundle state ACTIVE.
   *
   * @return A List of Bundle's.
   */
  std::vector<std::shared_ptr<BundlePrivate>> GetActiveBundles() const;

  /**
   * Try to load any saved framework state.
   * This is done by installing all saved bundles from the local archive
   * copy, and restoring the saved state for each bundle. This is only
   * intended to be executed during the start of the framework.
   *
   */
  void Load();

private:
  // don't allow copying the BundleRegistry.
  BundleRegistry(const BundleRegistry&);
  BundleRegistry& operator=(const BundleRegistry&);

  void CheckIllegalState() const;

  CoreBundleContext* coreCtx;

  typedef std::multimap<std::string, std::shared_ptr<BundlePrivate>> BundleMap;

  /**
   * Table of all installed bundles in this framework.
   * Key is the bundle location.
   */
  struct : MultiThreaded<>
  {
    BundleMap v;
  } bundles;
};
}

#endif // CPPMICROSERVICES_BUNDLEREGISTRY_H
