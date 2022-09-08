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

#include "BundleArchive.h"

#include "cppmicroservices/BundleResource.h"

#include "BundleResourceContainer.h"
#include "BundleStorage.h"

namespace cppmicroservices {

const std::string BundleArchive::AUTOSTART_SETTING_STOPPED = "stopped";
const std::string BundleArchive::AUTOSTART_SETTING_EAGER = "eager";
const std::string BundleArchive::AUTOSTART_SETTING_ACTIVATION_POLICY =
  "activation_policy";

BundleArchive::BundleArchive()
  : storage(nullptr)
{}

BundleArchive::BundleArchive(
  BundleStorage* storage,
  std::unique_ptr<Data>&& data,
  const std::shared_ptr<BundleResourceContainer>& resourceContainer,
  const std::string& resourcePrefix,
  const std::string& location)
  : storage(storage)
  , data(std::move(data))
  , resourceContainer(resourceContainer)
  , resourcePrefix(resourcePrefix)
  , location(location)
{}

bool BundleArchive::IsValid() const
{
  return data != nullptr;
}

void BundleArchive::Purge()
{
  storage->RemoveArchive(this);
}

long BundleArchive::GetBundleId() const
{
  return data->bundleId;
}

std::string BundleArchive::GetBundleLocation() const
{
  return location;
}

std::string BundleArchive::GetResourcePrefix() const
{
  return resourcePrefix;
}

BundleResource BundleArchive::GetResource(const std::string& path) const
{
  if (!resourceContainer) {
    return BundleResource();
  }
  BundleResource result(path, this->shared_from_this());
  if (result)
    return result;
  return BundleResource();
}

std::vector<BundleResource> BundleArchive::FindResources(
  const std::string& path,
  const std::string& filePattern,
  bool recurse) const
{
  std::vector<BundleResource> result;
  if (!resourceContainer) {
    return result;
  }

  std::string normalizedPath = path;
  // add a leading and trailing slash
  if (normalizedPath.empty())
    normalizedPath.push_back('/');
  if (*normalizedPath.begin() != '/')
    normalizedPath = '/' + normalizedPath;
  if (*normalizedPath.rbegin() != '/')
    normalizedPath.push_back('/');
  resourceContainer->FindNodes(this->shared_from_this(),
                               resourcePrefix + normalizedPath,
                               filePattern.empty() ? "*" : filePattern,
                               recurse,
                               result);
  return result;
}

BundleArchive::TimeStamp BundleArchive::GetLastModified() const
{
  return TimeStamp() + std::chrono::milliseconds(data->lastModified);
}

void BundleArchive::SetLastModified(const TimeStamp& ts)
{
  data->lastModified =
    std::chrono::duration_cast<std::chrono::milliseconds>(ts.time_since_epoch())
      .count();
}

int32_t BundleArchive::GetAutostartSetting() const
{
  return data->autostartSetting;
}

void BundleArchive::SetAutostartSetting(int32_t setting)
{
  data->autostartSetting = setting;
}

std::shared_ptr<BundleResourceContainer>
BundleArchive::GetResourceContainer() const
{
  return resourceContainer;
}
}
