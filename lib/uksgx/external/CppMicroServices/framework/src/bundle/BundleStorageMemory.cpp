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

#include "BundleStorageMemory.h"

#include "cppmicroservices/Constants.h"

#include "BundleArchive.h"
#include "BundleResourceContainer.h"

#include <chrono>

namespace cppmicroservices {

BundleStorageMemory::BundleStorageMemory()
  : nextFreeId(1)
{}

std::vector<std::shared_ptr<BundleArchive>>
BundleStorageMemory::InsertBundleLib(const std::string& location)
{
  auto resCont = std::make_shared<BundleResourceContainer>(location);
  return InsertArchives(resCont, resCont->GetTopLevelDirs());
}

std::vector<std::shared_ptr<BundleArchive>> BundleStorageMemory::InsertArchives(
  const std::shared_ptr<BundleResourceContainer>& resCont,
  const std::vector<std::string>& topLevelEntries)
{
  std::vector<std::shared_ptr<BundleArchive>> res;
  auto l = archives.Lock();
  US_UNUSED(l);
  for (auto const& prefix : topLevelEntries) {
#ifndef US_BUILD_SHARED_LIBS
    // The system bundle is already installed
    if (prefix == Constants::SYSTEM_BUNDLE_SYMBOLICNAME) {
      continue;
    }
#endif
    auto id = nextFreeId++;
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    std::unique_ptr<BundleArchive::Data> data(new BundleArchive::Data{ id, ts, -1 });
    auto p = archives.v.insert(std::make_pair(id,
                                              std::make_shared<BundleArchive>(this,
                                                                              std::move(data),
                                                                              resCont,
                                                                              prefix,
                                                                              resCont->GetLocation())));
    res.push_back(p.first->second);
  }
  return res;
}

bool BundleStorageMemory::RemoveArchive(const BundleArchive* ba)
{
  auto l = archives.Lock();
  US_UNUSED(l);
  auto iter = archives.v.find(ba->GetBundleId());
  if (iter != archives.v.end()) {
    archives.v.erase(iter);
    return true;
  }
  return false;
}

std::vector<std::shared_ptr<BundleArchive>>
BundleStorageMemory::GetAllBundleArchives() const
{
  std::vector<std::shared_ptr<BundleArchive>> res;
  auto l = archives.Lock();
  US_UNUSED(l);
  for (auto const& v : archives.v) {
    res.emplace_back(v.second);
  }
  return res;
}

std::vector<long> BundleStorageMemory::GetStartOnLaunchBundles() const
{
  std::vector<long> res;
  auto l = archives.Lock();
  US_UNUSED(l);
  for (auto& v : archives.v) {
    if (v.second->GetAutostartSetting() != -1) {
      res.emplace_back(v.second->GetBundleId());
    }
  }
  return res;
}

void BundleStorageMemory::Close()
{
  // Not need to lock "archives" here: at this point, the framework
  // is going down and no other threads can access it.
  archives.v.clear();
}
}
