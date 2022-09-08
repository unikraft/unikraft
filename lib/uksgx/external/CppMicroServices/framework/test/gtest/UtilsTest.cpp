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

#include "cppmicroservices/util/FileSystem.h"

#include <TestUtils.h>

#include <gtest/gtest.h>

using namespace cppmicroservices;
using namespace cppmicroservices::testing;
using namespace cppmicroservices::util;

class UtilsFs : public ::testing::Test
{
public:
  static void SetUpTestCase() { TempDir = MakeUniqueTempDirectory(); }

  static void TearDownTestCase()
  {
    // Avoid valgrind memcheck errors with gcc 4.6 by explicitly
    // cleaning up the directory, instead of relying on static
    // destruction.
    TempDir = cppmicroservices::testing::TempDir();
  }

protected:
  static std::string GetTooLongPath()
  {
#ifdef US_PLATFORM_WINDOWS
    const long name_max = MAX_PATH;
#else
    const long name_max = pathconf("/", _PC_NAME_MAX);
#endif
    if (name_max < 1)
      return std::string();

    std::vector<char> longName(name_max + 2, 'x');
    longName[name_max + 1] = '\0';
    return TempDir.Path + DIR_SEP + longName.data();
  }

  static std::string GetExistingDir() { return GetCurrentWorkingDirectory(); }

  static std::string GetExistingFile() { return GetExecutablePath(); }

  static cppmicroservices::testing::TempDir TempDir;
};

cppmicroservices::testing::TempDir UtilsFs::TempDir;

TEST_F(UtilsFs, Exists)
{
  EXPECT_FALSE(Exists("should not exist"));
  EXPECT_TRUE(Exists(GetExistingDir())) << "Test for existing directory";
  EXPECT_TRUE(Exists(GetExistingFile())) << "Test for existing file";

  auto longPath = GetTooLongPath();
  if (!longPath.empty()) {
#ifdef US_PLATFORM_WINDOWS
    EXPECT_FALSE(Exists(longPath));
#else
    EXPECT_THROW({ Exists(longPath); }, std::invalid_argument);
#endif
  }
}

TEST_F(UtilsFs, IsDirectory)
{
  EXPECT_FALSE(IsDirectory("not a directory"));
  EXPECT_TRUE(IsDirectory(GetExistingDir()));
  EXPECT_FALSE(IsDirectory(GetExistingFile()));

  auto longPath = GetTooLongPath();
  if (!longPath.empty()) {
#ifdef US_PLATFORM_WINDOWS
    EXPECT_FALSE(IsDirectory(longPath));
#else
    EXPECT_THROW({ IsDirectory(longPath); }, std::invalid_argument);
#endif
  }
}

TEST_F(UtilsFs, IsFile)
{
  EXPECT_FALSE(IsFile("not a file"));
  EXPECT_FALSE(IsFile(GetExistingDir()));
  EXPECT_TRUE(IsFile(GetExistingFile()));

  auto longPath = GetTooLongPath();
  if (!longPath.empty()) {
#ifdef US_PLATFORM_WINDOWS
    EXPECT_FALSE(IsFile(longPath));
#else
    EXPECT_THROW({ IsFile(longPath); }, std::invalid_argument);
#endif
  }
}

TEST_F(UtilsFs, IsRelative)
{
  EXPECT_TRUE(IsRelative(""));
  EXPECT_TRUE(IsRelative("rel"));
  EXPECT_FALSE(IsRelative(GetExistingFile()));
}

TEST_F(UtilsFs, GetAbsolute)
{
  EXPECT_EQ(GetAbsolute("rel", GetExistingDir()),
            GetExistingDir() + DIR_SEP + "rel");
  EXPECT_EQ(GetAbsolute(GetExistingFile(), "dummy"), GetExistingFile());
}

TEST_F(UtilsFs, MakeAndRemovePath)
{
  // Try to make a path that contains an existing file as a sub-path
  const File filePath = MakeUniqueTempFile(TempDir);
  const std::string invalidPath = filePath.Path + DIR_SEP + "invalid";
  EXPECT_THROW(MakePath(invalidPath), std::invalid_argument);
  EXPECT_THROW(RemoveDirectoryRecursive(invalidPath), std::invalid_argument);

  // Test path name limits
  const std::string tooLongPath = GetTooLongPath();
  EXPECT_THROW(MakePath(tooLongPath), std::invalid_argument);
  EXPECT_THROW(RemoveDirectoryRecursive(tooLongPath), std::invalid_argument);

  // Create a valid path
  const std::string validPath =
    TempDir.Path + DIR_SEP + "one" + DIR_SEP + "two";
  ASSERT_NO_THROW(MakePath(validPath));
  ASSERT_NO_THROW(RemoveDirectoryRecursive(validPath));
}
