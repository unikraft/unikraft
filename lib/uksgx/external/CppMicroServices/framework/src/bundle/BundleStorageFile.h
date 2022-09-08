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

#ifndef CPPMICROSERVICES_BUNDLESTORAGEFILE_H
#define CPPMICROSERVICES_BUNDLESTORAGEFILE_H

#include "BundleStorage.h"

namespace cppmicroservices {

class BundleStorageFile : public BundleStorage
{

public:
  BundleStorageFile();

  std::vector<std::shared_ptr<BundleArchive>> InsertBundleLib(
    const std::string& location);

  std::vector<std::shared_ptr<BundleArchive>> InsertArchives(
    const std::shared_ptr<BundleResourceContainer>& resCont,
    const std::vector<std::string>& topLevelEntries);

  bool RemoveArchive(const BundleArchive* ba);

  std::vector<std::shared_ptr<BundleArchive>> GetAllBundleArchives() const;

  std::vector<long> GetStartOnLaunchBundles() const;

  void Close();
};
}

#endif // CPPMICROSERVICES_BUNDLESTORAGEFILE_H
