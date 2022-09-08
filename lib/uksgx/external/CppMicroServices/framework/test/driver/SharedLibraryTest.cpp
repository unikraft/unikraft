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

#include "cppmicroservices/SharedLibrary.h"
#include "cppmicroservices/util/FileSystem.h"

#include "TestingConfig.h"
#include "TestingMacros.h"

#include <cstdlib>
#include <stdexcept>

using namespace cppmicroservices;

int SharedLibraryTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("SharedLibraryTest");

  const std::string libAFilePath = testing::LIB_PATH + util::DIR_SEP +
                                   US_LIB_PREFIX + "TestBundleA" + US_LIB_EXT;
  SharedLibrary lib1(libAFilePath);
  US_TEST_CONDITION(lib1.GetFilePath() == libAFilePath, "Absolute file path")
  US_TEST_CONDITION(lib1.GetLibraryPath() == testing::LIB_PATH, "Library path")
  US_TEST_CONDITION(lib1.GetName() == "TestBundleA", "Name")
  US_TEST_CONDITION(lib1.GetPrefix() == US_LIB_PREFIX, "Prefix")
  US_TEST_CONDITION(lib1.GetSuffix() == US_LIB_EXT, "Suffix")
  lib1.SetName("bla");
  US_TEST_CONDITION(lib1.GetName() == "TestBundleA", "Name after SetName()")
  lib1.SetLibraryPath("bla");
  US_TEST_CONDITION(lib1.GetLibraryPath() == testing::LIB_PATH,
                    "Library path after SetLibraryPath()")
  lib1.SetPrefix("bla");
  US_TEST_CONDITION(lib1.GetPrefix() == US_LIB_PREFIX,
                    "Prefix after SetPrefix()")
  lib1.SetSuffix("bla");
  US_TEST_CONDITION(lib1.GetSuffix() == US_LIB_EXT, "Suffix after SetSuffix()")
  US_TEST_CONDITION(lib1.GetFilePath() == libAFilePath,
                    "File path after setters")

  lib1.SetFilePath("bla");
  US_TEST_CONDITION(lib1.GetFilePath() == "bla", "Invalid file path")
  US_TEST_CONDITION(lib1.GetLibraryPath().empty(), "Empty lib path")
  US_TEST_CONDITION(lib1.GetName() == "bla", "Invalid file name")
  US_TEST_CONDITION(lib1.GetPrefix() == US_LIB_PREFIX, "Invalid prefix")
  US_TEST_CONDITION(lib1.GetSuffix() == US_LIB_EXT, "Invalid suffix")

  US_TEST_FOR_EXCEPTION(std::runtime_error, lib1.Load())
  US_TEST_CONDITION(lib1.IsLoaded() == false, "Is loaded")
  US_TEST_CONDITION(lib1.GetHandle() == nullptr, "Handle")

  lib1.SetFilePath(libAFilePath);
  lib1.Load();
  US_TEST_CONDITION(lib1.IsLoaded() == true, "Is loaded")
  US_TEST_CONDITION(lib1.GetHandle() != nullptr, "Handle")
  US_TEST_FOR_EXCEPTION(std::logic_error, lib1.Load())

  lib1.SetFilePath("bla");
  US_TEST_CONDITION(lib1.GetFilePath() == libAFilePath, "File path")
  lib1.Unload();

  SharedLibrary lib2(testing::LIB_PATH, "TestBundleA");
  US_TEST_CONDITION(lib2.GetFilePath() == libAFilePath, "File path")
  lib2.SetPrefix("");
  US_TEST_CONDITION(lib2.GetPrefix().empty(), "Lib prefix")
  US_TEST_CONDITION(lib2.GetFilePath() == testing::LIB_PATH + util::DIR_SEP +
                                            "TestBundleA" + US_LIB_EXT,
                    "File path")

  SharedLibrary lib3 = lib2;
  US_TEST_CONDITION(lib3.GetFilePath() == lib2.GetFilePath(),
                    "Compare file path")
  lib3.SetPrefix(US_LIB_PREFIX);
  US_TEST_CONDITION(lib3.GetFilePath() == libAFilePath, "Compare file path")
  lib3.Load();
  US_TEST_CONDITION(lib3.IsLoaded(), "lib3 loaded")
  US_TEST_CONDITION(!lib2.IsLoaded(), "lib2 not loaded")
  lib1 = lib3;
  US_TEST_FOR_EXCEPTION(std::logic_error, lib1.Load())
  lib2.SetPrefix(US_LIB_PREFIX);
  lib2.Load();

  lib3.Unload();
  US_TEST_CONDITION(!lib3.IsLoaded(), "lib3 unloaded")
  US_TEST_CONDITION(!lib1.IsLoaded(), "lib3 unloaded")

// gcov on Mac OS X writes coverage files during static destruction
// resulting in a crash if a dylib is completely unloaded from the process.
// https://bugs.llvm.org/show_bug.cgi?id=27224
#if !defined(US_PLATFORM_APPLE) || !defined(US_COVERAGE_ENABLED)
  lib2.Unload();
  US_TEST_CONDITION(!lib2.IsLoaded(), "lib2 loaded")
#endif
  lib1.Unload();

  US_TEST_END()
}
