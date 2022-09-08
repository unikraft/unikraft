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

#include "miniz.h"

#include <cstring>
#include <vector>

/*
* @brief Struct which represents an entry in the zip archive.
*/
struct EntryInfo
{
  enum class EntryType
  {
    FILE,
    DIRECTORY
  };

  std::string name;
  EntryType type;
  mz_uint64 compressedSize;
  mz_uint64 uncompressedSize;
  mz_uint32 crc32;
};

/*
* @brief A simple container for storing entries of a zipped archive
*/
class ZipFile
{

public:
  typedef std::vector<EntryInfo>::const_reference const_reference;
  typedef std::vector<EntryInfo>::size_type size_type;

  /*
  * @brief Read a zip archive and fill entries
  * @param filename is the path of the filename
  * @throw std::runtime exception if zip archive couldn't be read
  */
  ZipFile(std::string filename)
  {
    mz_zip_archive ziparchive;
    memset(&ziparchive, 0, sizeof(mz_zip_archive));
    if (!mz_zip_reader_init_file(&ziparchive, filename.c_str(), 0)) {
      throw std::runtime_error("Could not read zip archive file " + filename);
    }

    mz_uint numindices = mz_zip_reader_get_num_files(&ziparchive);
    for (mz_uint index = 0; index < numindices; ++index) {
      mz_zip_archive_file_stat filestat;
      EntryInfo entry;
      mz_zip_reader_file_stat(&ziparchive, index, &filestat);

      entry.name = filestat.m_filename;
      entry.type = mz_zip_reader_is_file_a_directory(&ziparchive, index)
                     ? EntryInfo::EntryType::DIRECTORY
                     : EntryInfo::EntryType::FILE;
      entry.compressedSize = filestat.m_comp_size;
      entry.uncompressedSize = filestat.m_uncomp_size;
      entry.crc32 = filestat.m_crc32;
      entries.push_back(entry);
    }

    if (!mz_zip_reader_end(&ziparchive)) {
      throw std::runtime_error("Could not close zip archive file " + filename);
    }
  }

  size_type size() const { return entries.size(); }

  const_reference operator[](size_type i) const { return entries[i]; }

  /*
  * @brief: Returns the names of all entries in a std::vector
  */
  std::vector<std::string> getNames() const
  {
    std::vector<std::string> entryNames;
    for (const auto& entry : entries) {
      entryNames.push_back(entry.name);
    }
    return entryNames;
  }

private:
  std::vector<EntryInfo> entries;
};
