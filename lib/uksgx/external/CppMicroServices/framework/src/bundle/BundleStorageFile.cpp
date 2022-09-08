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

#include "BundleStorageFile.h"

#include "BundleArchive.h"

#include <stdexcept>

namespace cppmicroservices {

static const std::size_t MAX_ARCHIVE_SIZE = 512;
static const std::size_t MAX_LOCATION_LEN = 256;

struct ExtraData
{
  int32_t loc_index; // -1 -> use location field
  char location[MAX_LOCATION_LEN];
};

struct PeristentData
{
  BundleArchive::Data data;
  char reserved[MAX_ARCHIVE_SIZE - sizeof(BundleArchive::Data) -
                sizeof(ExtraData)];
  ExtraData extra;
};

BundleStorageFile::BundleStorageFile()
{
  throw std::logic_error("not implemented");
}

std::vector<std::shared_ptr<BundleArchive>> BundleStorageFile::InsertBundleLib(
  const std::string& /*location*/)
{
  throw std::logic_error("not implemented");
}

std::vector<std::shared_ptr<BundleArchive>> BundleStorageFile::InsertArchives(
  const std::shared_ptr<BundleResourceContainer>& /*resCont*/,
  const std::vector<std::string>& /*topLevelEntries*/)
{
  throw std::logic_error("not implemented");
}

bool BundleStorageFile::RemoveArchive(const BundleArchive* /*ba*/)
{
  throw std::logic_error("not implemented");
}

std::vector<std::shared_ptr<BundleArchive>>
BundleStorageFile::GetAllBundleArchives() const
{
  throw std::logic_error("not implemented");
}

std::vector<long> BundleStorageFile::GetStartOnLaunchBundles() const
{
  throw std::logic_error("not implemented");
}

void BundleStorageFile::Close()
{
  throw std::logic_error("not implemented");
}
}
