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

#ifndef CPPMICROSERVICES_BUNDLESTORAGE_H
#define CPPMICROSERVICES_BUNDLESTORAGE_H

#include "BundleResourceContainer.h"

#include <memory>
#include <string>
#include <vector>

namespace cppmicroservices {

struct BundleArchive;

/**
 * Interface for managing all bundles library content.
 */
struct BundleStorage
{

  virtual ~BundleStorage() {}

  /**
   * Insert bundle library into persistent storagedata.
   *
   * @param location Location of bundle to install.
   * @return A list of BundleArchive instances representing the installed bundles.
   */
  virtual std::vector<std::shared_ptr<BundleArchive>> InsertBundleLib(
    const std::string& location) = 0;

  /**
   * Insert bundles from a container into persistent storagedata.
   *
   * @param resCont The container for the bundle data and resources.
   * @param topLevelEntries The top level entries in the container to be inserted as bundle archives.
   * @return A list of BundleArchive instances representing the installed bundles.
   */
  virtual std::vector<std::shared_ptr<BundleArchive>> InsertArchives(
    const std::shared_ptr<BundleResourceContainer>& resCont,
    const std::vector<std::string>& topLevelEntries) = 0;

  /**
   * Get all bundle archive objects.
   *
   * @return A list with bundle archive objects.
   */
  virtual std::vector<std::shared_ptr<BundleArchive>> GetAllBundleArchives()
    const = 0;

  /**
   * Get all bundles tagged to start at next launch of framework.
   * This list is sorted in suggest start order.
   *
   * @return A list with bundle ids.
   */
  virtual std::vector<long> GetStartOnLaunchBundles() const = 0;

  /**
   * Close this bundle storage and all bundles in it.
   */
  virtual void Close() = 0;

private:
  friend struct BundleArchive;

  /**
   * Remove bundle archive from archives list.
   *
   * @param ba Bundle archive to remove.
   * @return true if element was removed.
   */
  virtual bool RemoveArchive(const BundleArchive* ba) = 0;
};
}

#endif // CPPMICROSERVICES_BUNDLESTORAGE_H
