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

#include "BundleRegistry.h"

#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/BundleEvent.h"
#include "cppmicroservices/FrameworkEvent.h"
#include "cppmicroservices/GetBundleContext.h"

#include "cppmicroservices/util/Error.h"
#include "cppmicroservices/util/String.h"

#include "BundleContextPrivate.h"
#include "BundlePrivate.h"
#include "BundleResourceContainer.h"
#include "BundleStorage.h"
#include "CoreBundleContext.h"
#include "FrameworkPrivate.h"

#include <cassert>
#include <map>

namespace cppmicroservices {

BundleRegistry::BundleRegistry(CoreBundleContext* coreCtx)
  : coreCtx(coreCtx)
{}

BundleRegistry::~BundleRegistry(void) {}

void BundleRegistry::Init()
{
  bundles.v.insert(
    std::make_pair(coreCtx->systemBundle->location, coreCtx->systemBundle));
}

void BundleRegistry::Clear()
{
  auto l = bundles.Lock();
  US_UNUSED(l);
  bundles.v.clear();
}

std::vector<Bundle> BundleRegistry::Install(const std::string& location,
                                            BundlePrivate* caller)
{
  CheckIllegalState();

  auto l = this->Lock();
  US_UNUSED(l);
  auto range = (bundles.Lock(), bundles.v.equal_range(location));
  if (range.first != range.second) {
    std::vector<Bundle> res;
    std::vector<std::shared_ptr<BundlePrivate>> alreadyInstalled;
    while (range.first != range.second) {
      auto b = range.first->second;
      alreadyInstalled.push_back(b);
      auto bu = coreCtx->bundleHooks.FilterBundle(
        MakeBundleContext(b->bundleContext.Load()), MakeBundle(b));
      if (bu)
        res.push_back(bu);
      ++range.first;
    }

    auto newBundles = Install0(location, alreadyInstalled, caller);
    res.insert(res.end(), newBundles.begin(), newBundles.end());
    if (res.empty()) {
      throw std::runtime_error("All bundles rejected by a bundle hook");
    } else {
      return res;
    }
  }
  return Install0(location, {}, caller);
}

std::vector<Bundle> BundleRegistry::Install0(
  const std::string& location,
  const std::vector<std::shared_ptr<BundlePrivate>>& exclude,
  BundlePrivate* /*caller*/)
{
  std::vector<Bundle> res;
  std::vector<std::shared_ptr<BundleArchive>> barchives;
  try {
    if (exclude.empty()) {
      barchives = coreCtx->storage->InsertBundleLib(location);
    } else {
      auto resCont =
        exclude.front()->GetBundleArchive()->GetResourceContainer();
      auto entries = resCont->GetTopLevelDirs();
      for (auto const& b : exclude) {
        entries.erase(
          std::remove(entries.begin(), entries.end(), b->symbolicName),
          entries.end());
      }
      barchives = coreCtx->storage->InsertArchives(resCont, entries);
    }

    for (auto& ba : barchives) {
      auto d = std::shared_ptr<BundlePrivate>(
        new BundlePrivate(coreCtx, std::move(ba)));
      res.emplace_back(MakeBundle(d));
    }

    {
      auto l = bundles.Lock();
      US_UNUSED(l);
      for (auto& b : res) {
        bundles.v.insert(std::make_pair(location, b.d));
      }
    }

    for (auto& b : res) {
      coreCtx->listeners.BundleChanged(
        BundleEvent(BundleEvent::BUNDLE_INSTALLED, b));
    }
    return res;
  } catch (...) {
    for (auto& ba : barchives) {
      ba->Purge();
    }
    throw std::runtime_error("Failed to install bundle library at " + location +
                             ": " + util::GetLastExceptionStr());
  }
}

void BundleRegistry::Remove(const std::string& location, long id)
{
  auto l = bundles.Lock();
  US_UNUSED(l);
  auto range = bundles.v.equal_range(location);
  for (auto iter = range.first; iter != range.second; ++iter) {
    if (iter->second->id == id) {
      bundles.v.erase(iter);
      return;
    }
  }
}

std::shared_ptr<BundlePrivate> BundleRegistry::GetBundle(long id) const
{
  CheckIllegalState();

  auto l = bundles.Lock();
  US_UNUSED(l);

  for (auto& m : bundles.v) {
    if (m.second->id == id) {
      return m.second;
    }
  }
  return nullptr;
}

std::vector<std::shared_ptr<BundlePrivate>> BundleRegistry::GetBundles(
  const std::string& location) const
{
  CheckIllegalState();

  auto l = bundles.Lock();
  US_UNUSED(l);

  auto range = bundles.v.equal_range(location);
  std::vector<std::shared_ptr<BundlePrivate>> result;
  std::transform(range.first,
                 range.second,
                 std::back_inserter(result),
                 [](const BundleMap::value_type& p) { return p.second; });
  return result;
}

std::vector<std::shared_ptr<BundlePrivate>> BundleRegistry::GetBundles(
  const std::string& name,
  const BundleVersion& version) const
{
  CheckIllegalState();

  std::vector<std::shared_ptr<BundlePrivate>> res;

  auto l = bundles.Lock();
  US_UNUSED(l);

  for (auto& p : bundles.v) {
    auto& b = p.second;
    if (name == b->symbolicName && version == b->version) {
      res.push_back(b);
    }
  }

  return res;
}

std::vector<std::shared_ptr<BundlePrivate>> BundleRegistry::GetBundles() const
{
  auto l = bundles.Lock();
  US_UNUSED(l);

  std::vector<std::shared_ptr<BundlePrivate>> result;
  std::transform(bundles.v.begin(),
                 bundles.v.end(),
                 std::back_inserter(result),
                 [](const BundleMap::value_type& p) { return p.second; });
  return result;
}

std::vector<std::shared_ptr<BundlePrivate>> BundleRegistry::GetActiveBundles()
  const
{
  CheckIllegalState();
  std::vector<std::shared_ptr<BundlePrivate>> result;

  auto l = bundles.Lock();
  US_UNUSED(l);
  for (auto& b : bundles.v) {
    auto s = b.second->state.load();
    if (s == Bundle::STATE_ACTIVE || s == Bundle::STATE_STARTING) {
      result.push_back(b.second);
    }
  }
  return result;
}

void BundleRegistry::Load()
{
  auto l = this->Lock();
  US_UNUSED(l);
  auto bas = coreCtx->storage->GetAllBundleArchives();
  for (auto const& ba : bas) {
    try {
      std::shared_ptr<BundlePrivate> impl(new BundlePrivate(coreCtx, ba));
      bundles.v.insert(std::make_pair(impl->location, impl));
    } catch (...) {
      ba->SetAutostartSetting(-1); // Do not start on launch
      std::cerr << "Failed to load bundle " << util::ToString(ba->GetBundleId())
                << " (" + ba->GetBundleLocation() + ") uninstalled it!"
                << " (execption: "
                << util::GetExceptionStr(std::current_exception()) << ")"
                << std::endl;
    }
  }
}

void BundleRegistry::CheckIllegalState() const
{
  if (coreCtx == nullptr) {
    throw std::logic_error("This framework instance is not active.");
  }
}
}
