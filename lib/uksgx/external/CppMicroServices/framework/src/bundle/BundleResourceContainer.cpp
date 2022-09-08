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

#include "BundleResourceContainer.h"

#include "cppmicroservices/BundleResource.h"

#include "cppmicroservices/util/FileSystem.h"

#include <cassert>
#include <climits>
#include <cstring>
#include <exception>
#include <sstream>
#include <stdexcept>

namespace cppmicroservices {

BundleResourceContainer::BundleResourceContainer(const std::string& location)
  : m_Location(location)
  , m_ZipArchive()
  , m_ZipFileMutex()
  , m_IsContainerOpen(false)
{
  if (!util::Exists(location)) {
    throw std::runtime_error(m_Location + " does not exist");
  }

  if (!mz_zip_reader_init_file(&m_ZipArchive, m_Location.c_str(), 0)) {
    throw std::runtime_error("Could not init zip archive for bundle at " +
                             m_Location);
  }
  InitSortedEntries();
  if (m_SortedToplevelDirs.empty()) {
    throw std::runtime_error("Invalid zip archive layout for bundle at " +
                             m_Location);
  }
  m_IsContainerOpen = true;
}

BundleResourceContainer::~BundleResourceContainer()
{
  try {
    CloseContainer();
  } catch(const std::exception&) {}
}

std::string BundleResourceContainer::GetLocation() const
{
  return m_Location;
}

std::vector<std::string> BundleResourceContainer::GetTopLevelDirs() const
{
  return std::vector<std::string>{ m_SortedToplevelDirs.begin(),
                                   m_SortedToplevelDirs.end() };
}

bool BundleResourceContainer::GetStat(BundleResourceContainer::Stat& stat)
{
  OpenContainer();
  int fileIndex =
    mz_zip_reader_locate_file(const_cast<mz_zip_archive*>(&m_ZipArchive),
                              stat.filePath.c_str(),
                              nullptr,
                              0);
  if (fileIndex >= 0) {
    return GetStat(fileIndex, stat);
  }
  return false;
}

bool BundleResourceContainer::GetStat(int index,
                                      BundleResourceContainer::Stat& stat)
{
  OpenContainer();
  if (index >= 0) {
    mz_zip_archive_file_stat zipStat;
    if (!mz_zip_reader_file_stat(
          const_cast<mz_zip_archive*>(&m_ZipArchive), index, &zipStat)) {
      return false;
    }
    stat.index = index;
    stat.filePath = zipStat.m_filename;
    stat.isDir = mz_zip_reader_is_file_a_directory(
                   const_cast<mz_zip_archive*>(&m_ZipArchive), index)
                   ? true
                   : false;
    stat.modifiedTime = zipStat.m_time;
    stat.crc32 = zipStat.m_crc32;
    // This will limit the size info from uint64 to uint32 on 32-bit
    // architectures. We don't care because we assume resources > 2GB
    // don't make sense to be embedded in a bundle anyway.
    assert(zipStat.m_comp_size < INT_MAX);
    assert(zipStat.m_uncomp_size < INT_MAX);
    stat.compressedSize = static_cast<int>(zipStat.m_comp_size);
    stat.uncompressedSize = static_cast<int>(zipStat.m_uncomp_size);
    return true;
  }
  return false;
}

std::unique_ptr<void, void (*)(void*)> BundleResourceContainer::GetData(
  int index)
{
  OpenContainer();
  std::unique_lock<std::mutex> l(m_ZipFileStreamMutex);
  void* data = mz_zip_reader_extract_to_heap(
    const_cast<mz_zip_archive*>(&m_ZipArchive), index, nullptr, 0);
  return { data, ::free };
}

void BundleResourceContainer::GetChildren(const std::string& resourcePath,
                                          bool relativePaths,
                                          std::vector<std::string>& names,
                                          std::vector<uint32_t>& indices) const
{
  auto iter = m_SortedEntries.find(std::make_pair(resourcePath, 0));
  if (iter == m_SortedEntries.end()) {
    return;
  }

  for (++iter; iter != m_SortedEntries.end(); ++iter) {
    if (resourcePath.size() > iter->first.size())
      break;
    if (iter->first.compare(0, resourcePath.size(), resourcePath) == 0) {
      std::size_t pos = iter->first.find_first_of('/', resourcePath.size());
      if (pos == std::string::npos || pos == iter->first.size() - 1) {
        if (relativePaths) {
          names.push_back(iter->first.substr(resourcePath.size()));
        } else {
          names.push_back(iter->first);
        }
        indices.push_back(iter->second);
      }
    }
  }
}

void BundleResourceContainer::FindNodes(
  const std::shared_ptr<const BundleArchive>& archive,
  const std::string& path,
  const std::string& filePattern,
  bool recurse,
  std::vector<BundleResource>& resources) const
{
  std::vector<std::string> names;
  std::vector<uint32_t> indices;

  this->GetChildren(path, true, names, indices);

  for (std::size_t i = 0, s = names.size(); i < s; ++i) {
    if (*names[i].rbegin() == '/' && recurse) {
      this->FindNodes(
        archive, path + names[i], filePattern, recurse, resources);
    }
    if (this->Matches(names[i], filePattern)) {
      resources.push_back(BundleResource(indices[i], archive));
    }
  }
}

void BundleResourceContainer::InitSortedEntries()
{
  mz_uint numFiles =
    mz_zip_reader_get_num_files(const_cast<mz_zip_archive*>(&m_ZipArchive));
  for (mz_uint fileIndex = 0; fileIndex < numFiles; ++fileIndex) {
    char fileName[MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE];
    if (mz_zip_reader_get_filename(&m_ZipArchive,
                                   fileIndex,
                                   fileName,
                                   MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE)) {
      std::string strFileName = fileName;
      m_SortedEntries.insert(std::make_pair(strFileName, fileIndex));
      std::size_t pos = strFileName.find_first_of('/');
      if (pos != std::string::npos) {
        m_SortedToplevelDirs.insert(strFileName.substr(0, pos));
      }
    }
  }
}

bool BundleResourceContainer::Matches(const std::string& name,
                                      const std::string& filePattern) const
{
  // short-cut
  if (filePattern == "*")
    return true;

  std::stringstream ss(filePattern);
  std::string tok;
  std::size_t pos = 0;
  while (std::getline(ss, tok, '*')) {
    std::size_t index = name.find(tok, pos);
    if (index == std::string::npos)
      return false;
    pos = index + tok.size();
  }
  return true;
}

void BundleResourceContainer::OpenContainer()
{
  std::lock_guard<std::mutex> lock(m_ZipFileMutex);
  if(!m_IsContainerOpen) {
    if (!mz_zip_reader_init_file(&m_ZipArchive, m_Location.c_str(), 0)) {
        throw std::runtime_error("Could not init zip archive for bundle at " +
            m_Location);
    }
    m_IsContainerOpen = true;
  }
}

void BundleResourceContainer::CloseContainer()
{
  std::lock_guard<std::mutex> lock(m_ZipFileMutex);
  if(m_IsContainerOpen) {
    mz_zip_reader_end(&m_ZipArchive);
    m_IsContainerOpen = false;
  }
}
}
