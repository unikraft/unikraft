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

#include "cppmicroservices/BundleResource.h"

#include "cppmicroservices/GetBundleContext.h"
#include "cppmicroservices/detail/Log.h"

#include "BundleArchive.h"
#include "BundleResourceContainer.h"

#include <atomic>
#include <string>

namespace cppmicroservices {

class BundleResourcePrivate
{

public:
  BundleResourcePrivate(const std::shared_ptr<const BundleArchive>& archive)
    : archive(archive)
    , ref(1)
  {}

  void InitFilePath(const std::string& file);

  const std::shared_ptr<const BundleArchive> archive;

  BundleResourceContainer::Stat stat;

  std::string fileName;
  std::string path;

  mutable std::vector<std::string> children;
  mutable std::vector<uint32_t> childNodes;

  /**
   * Reference count for implicitly shared private implementation.
   */
  std::atomic<int> ref;
};

void BundleResourcePrivate::InitFilePath(const std::string& file)
{
  std::string normalizedFile = file;
  if (normalizedFile.empty() || normalizedFile[0] != '/') {
    normalizedFile = '/' + normalizedFile;
  }

  std::string rawPath;
  std::size_t index = normalizedFile.find_last_of('/');
  if (index == std::string::npos) {
    fileName = normalizedFile;
  } else if (index < normalizedFile.size() - 1) {
    fileName = normalizedFile.substr(index + 1);
    rawPath = normalizedFile.substr(0, index + 1);
  } else {
    rawPath = normalizedFile;
  }

  // remove duplicate /
  std::string::value_type lastChar = 0;
  for (std::size_t i = 0; i < rawPath.size(); ++i) {
    if (rawPath[i] == '/' && lastChar == '/') {
      continue;
    }
    lastChar = rawPath[i];
    path.push_back(lastChar);
  }
  if (path.empty()) {
    path.push_back('/');
  }
}

BundleResource::BundleResource()
  : d(new BundleResourcePrivate(nullptr))
{}

BundleResource::BundleResource(const BundleResource& resource)
  : d(resource.d)
{
  ++d->ref;
}

BundleResource::BundleResource(
  const std::string& file,
  const std::shared_ptr<const BundleArchive>& archive)
  : d(new BundleResourcePrivate(archive))
{
  d->InitFilePath(file);

  d->stat.filePath = d->archive->GetResourcePrefix() + d->path + d->fileName;

  d->archive->GetResourceContainer()->GetStat(d->stat);
}

BundleResource::BundleResource(
  int index,
  const std::shared_ptr<const BundleArchive>& archive)
  : d(new BundleResourcePrivate(archive))
{
  d->archive->GetResourceContainer()->GetStat(index, d->stat);
  d->InitFilePath(
    d->stat.filePath.substr(d->archive->GetResourcePrefix().size()));
}

BundleResource::~BundleResource()
{
  if (!--d->ref)
    delete d;
}

BundleResource& BundleResource::operator=(const BundleResource& resource)
{
  BundleResourcePrivate* curr_d = d;
  d = resource.d;
  ++d->ref;

  if (!--curr_d->ref)
    delete curr_d;

  return *this;
}

bool BundleResource::operator<(const BundleResource& resource) const
{
  return this->GetResourcePath() < resource.GetResourcePath();
}

bool BundleResource::operator==(const BundleResource& resource) const
{
  if (!this->IsValid())
    return !resource.IsValid();
  if (!resource.IsValid())
    return false;
  return d->archive->GetResourceContainer() ==
           resource.d->archive->GetResourceContainer() &&
         d->archive->GetResourcePrefix() ==
           resource.d->archive->GetResourcePrefix() &&
         this->GetResourcePath() == resource.GetResourcePath();
}

bool BundleResource::operator!=(const BundleResource& resource) const
{
  return !(*this == resource);
}

bool BundleResource::IsValid() const
{
  return d->archive && d->archive->IsValid() && d->stat.index > -1;
}

BundleResource::operator bool() const
{
  return IsValid();
}

std::string BundleResource::GetName() const
{
  return d->fileName;
}

std::string BundleResource::GetPath() const
{
  return d->path;
}

std::string BundleResource::GetResourcePath() const
{
  return d->path + d->fileName;
}

std::string BundleResource::GetBaseName() const
{
  return d->fileName.substr(0, d->fileName.find_first_of('.'));
}

std::string BundleResource::GetCompleteBaseName() const
{
  return d->fileName.substr(0, d->fileName.find_last_of('.'));
}

std::string BundleResource::GetSuffix() const
{
  std::size_t index = d->fileName.find_last_of('.');
  return index < d->fileName.size() - 1 ? d->fileName.substr(index + 1)
                                        : std::string("");
}

std::string BundleResource::GetCompleteSuffix() const
{
  std::size_t index = d->fileName.find_first_of('.');
  return index < d->fileName.size() - 1 ? d->fileName.substr(index + 1)
                                        : std::string("");
}

bool BundleResource::IsDir() const
{
  return d->stat.isDir;
}

bool BundleResource::IsFile() const
{
  return !d->stat.isDir;
}

std::vector<std::string> BundleResource::GetChildren() const
{
  if (!IsValid() || !IsDir())
    return d->children;

  if (d->children.empty()) {
    d->archive->GetResourceContainer()->GetChildren(
      d->stat.filePath, true, d->children, d->childNodes);
  }
  return d->children;
}

std::vector<BundleResource> BundleResource::GetChildResources() const
{
  std::vector<BundleResource> childResources;

  if (!IsValid() || !IsDir())
    return childResources;

  if (d->childNodes.empty()) {
    d->archive->GetResourceContainer()->GetChildren(
      this->GetResourcePath(), true, d->children, d->childNodes);
  }

  for (std::vector<uint32_t>::const_iterator iter = d->childNodes.begin(),
                                             iterEnd = d->childNodes.end();
       iter != iterEnd;
       ++iter) {
    childResources.push_back(
      BundleResource(static_cast<int>(*iter), d->archive));
  }
  return childResources;
}

int BundleResource::GetSize() const
{
  return d->stat.uncompressedSize;
}

int BundleResource::GetCompressedSize() const
{
  return d->stat.compressedSize;
}

time_t BundleResource::GetLastModified() const
{
  return d->stat.modifiedTime;
}

uint32_t BundleResource::GetCrc32() const
{
  return d->stat.crc32;
}

std::size_t BundleResource::Hash() const
{
  using namespace std;
  return hash<std::string>()(d->archive->GetResourcePrefix() +
                             this->GetResourcePath());
}

std::unique_ptr<void, void (*)(void*)> BundleResource::GetData() const
{
  if (!IsValid())
    return { nullptr, ::free };

  auto data = d->archive->GetResourceContainer()->GetData(d->stat.index);
  if (!data) {
    auto sink = GetBundleContext().GetLogSink();
    DIAG_LOG(*sink) << "Error uncompressing resource data for "
                    << this->GetResourcePath() << " from "
                    << d->archive->GetBundleLocation();
  }

  return data;
}

std::ostream& operator<<(std::ostream& os, const BundleResource& resource)
{
  return os << resource.GetResourcePath();
}

} // namespace cppmicroservices
