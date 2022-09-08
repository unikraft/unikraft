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

#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"
#include "ZipFile.h"

#include "json/json.h"

#include <array>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace cppmicroservices;
using namespace cppmicroservices::util;

namespace {

// The value returned by the resource compiler when manifest validation failed
static const int BUNDLE_MANIFEST_VALIDATION_ERROR_CODE(2);

/*
 * Execute a process with the given command line
 * @param executable absolute path to the executable to run along with
 *  and any command line parameters.
 * @return the executable's return code.
 */
int runExecutable(const std::string& executable)
{
// WEXITSTATUS is only available on POSIX. Wrap std::system into a function
// call so that there is consistent and uniform return codes on all platforms.
#if defined US_PLATFORM_WINDOWS
#  define WEXITSTATUS
#endif

  int ret = std::system(executable.c_str());

  // WEXITSTATUS uses an old c-sytle cast
  // clang-format off
US_GCC_PUSH_DISABLE_WARNING(old-style-cast)
  // clang-format on
  return WEXITSTATUS(ret);
  US_GCC_POP_WARNING
}

/*
* @brief remove any line feed and new line characters.
* @param[in,out] str string to be modified
*/
void removeLineEndings(std::string& str)
{
  str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
  str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
}

/*
 * Create a sample directory hierarchy in tempdir
 * to perform testing of ResourceCompiler
 */
void createDirHierarchy(const std::string& tempdir,
                        const std::string& manifest_json)
{
  /*
   * We create the following directory hierarchy
   * current_dir/
   *    |____ manifest.json
   *    |____ sample.dll
   *    |____ sample1.dll
   *    |____ resource1/
   *    |         |______ resource1.txt
   *    |____ resource2/
   *              |______ resource2.txt
   */

  const std::string resource1_txt = "A sample resource to embed\n"
                                    "Inside the text file resource1.txt";

  const std::string resource2_txt = "A sample resource to embed\n"
                                    "Inside the text file resource2.txt";

  std::string manifest_fpath(tempdir + "manifest.json");
  std::ofstream manifest(manifest_fpath);
  if (!manifest.is_open()) {
    throw std::runtime_error("Couldn't open " + manifest_fpath);
  }
  manifest << manifest_json << std::endl;
  manifest.close();

  std::string rc1dir_path(tempdir + "resource1");
  std::string rc2dir_path(tempdir + "resource2");
  MakePath(rc1dir_path);
  MakePath(rc2dir_path);
  std::string rc1file_path(rc1dir_path + DIR_SEP + "resource1.txt");
  std::string rc2file_path(rc2dir_path + DIR_SEP + "resource2.txt");
  std::ofstream rc1file(rc1file_path.c_str());
  std::ofstream rc2file(rc2file_path.c_str());
  if (!rc1file.is_open()) {
    throw std::runtime_error("Couldn't open " + rc1file_path);
  }
  if (!rc2file.is_open()) {
    throw std::runtime_error("Couldn't open " + rc2file_path);
  }
  rc1file << resource1_txt << std::endl;
  rc1file.close();
  rc2file << resource2_txt << std::endl;
  rc2file.close();

  // Create 2 binary files filled with random numbers
  // to test bundle-file functionality.
  auto create_mock_dll = [&tempdir](const std::string& dllname,
                                    const std::array<char, 5>& dat) {
    std::string dll_path(tempdir + dllname);
    std::ofstream dll(dll_path.c_str());
    if (!dll.is_open()) {
      throw std::runtime_error("Couldn't open " + dll_path);
    }
    dll.write(dat.data(), dat.size()); //binary write
    dll.close();
  };
  std::array<char, 5> data1 = { { 2, 4, 6, 8, 10 } };
  std::array<char, 5> data2 = { { 1, 2, 3, 4, 5 } };
  create_mock_dll("sample.dll", data1);
  create_mock_dll("sample1.dll", data2);
}

/*
 * In a vector of strings "entryNames", test if the string "name" exists
 */
void testExists(const std::vector<std::string>& entryNames,
                const std::string& name)
{
  bool exists =
    std::find(entryNames.begin(), entryNames.end(), name) != entryNames.end();
  US_TEST_CONDITION(exists, "Check existence of " + name);
}

/*
 * Transform the path specified in "path", which may contain a combination of spaces
 * and parenthesis, to a path with the spaces and parenthesis escaped with "\".
 * If this isn't escaped, test invocation (std::system) with this path results
 * in an error that the given file is not found.
 *
 * E.g. "/tmp/path (space)/rc" will result in a raw "/tmp/path\ \(space\)/rc"
 */
void escapePath(std::string& path)
{
  std::string delimiters("() ");
  std::string insertstr("\\");
  size_t found = path.find_first_of(delimiters);
  while (found != std::string::npos) {
    path.insert(found, insertstr);
    found = path.find_first_of(delimiters, found + insertstr.size() + 1);
  }
}

/*
 * Use resource compiler to create Example.zip with just manifest.json
 */
void testManifestAdd(const std::string& rcbinpath, const std::string& tempdir)
{
  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name "
      << "mybundle";
  cmd << " --out-file " << tempdir << "Example.zip";
  cmd << " --manifest-add " << tempdir << "manifest.json";

  US_TEST_CONDITION_REQUIRED(EXIT_SUCCESS == runExecutable(cmd.str()),
                             "Cmdline invocation in testManifestAdd returns 0");

  ZipFile zip(tempdir + DIR_SEP + "Example.zip");
  US_TEST_CONDITION(zip.size() == 2, "Check number of entries of zip.");

  auto entryNames = zip.getNames();
  testExists(entryNames, "mybundle/manifest.json");
  testExists(entryNames, "mybundle/");
}

/*
 * Use resource compiler to create Example.zip with manifest.json and
 * one --res-add option
 *
 * Working directory is changed temporarily to tempdir because of --res-add option
 */
void testManifestResAdd(const std::string& rcbinpath,
                        const std::string& tempdir)
{
  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name mybundle ";
  cmd << " --out-file Example2.zip ";
  cmd << " --manifest-add manifest.json ";
  cmd << " --res-add resource1/resource1.txt";

  auto cwdir = GetCurrentWorkingDirectory();
  testing::ChangeDirectory(tempdir);
  US_TEST_CONDITION_REQUIRED(
    EXIT_SUCCESS == runExecutable(cmd.str()),
    "Cmdline invocation in testManifestResAdd returns 0");
  testing::ChangeDirectory(cwdir);

  ZipFile zip(tempdir + "Example2.zip");
  US_TEST_CONDITION(zip.size() == 4, "Check number of entries of zip.");

  auto entryNames = zip.getNames();
  testExists(entryNames, "mybundle/manifest.json");
  testExists(entryNames, "mybundle/");
  testExists(entryNames, "mybundle/resource1/resource1.txt");
  testExists(entryNames, "mybundle/resource1/");
}

/*
 * Use resource compiler to create tomerge.zip with only --res-add option
 *
 * Working directory is changed temporarily to tempdir because of --res-add option
 */
void testResAdd(const std::string& rcbinpath, const std::string& tempdir)
{
  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name mybundle ";
  cmd << " --out-file tomerge.zip ";
  cmd << " --res-add resource2/resource2.txt";

  auto cwdir = GetCurrentWorkingDirectory();
  testing::ChangeDirectory(tempdir);
  US_TEST_CONDITION_REQUIRED(EXIT_SUCCESS == runExecutable(cmd.str()),
                             "Cmdline invocation in testResAdd returns 0");
  testing::ChangeDirectory(cwdir);

  ZipFile zip(tempdir + "tomerge.zip");
  US_TEST_CONDITION(zip.size() == 3, "Check number of entries of zip.");

  auto entryNames = zip.getNames();
  testExists(entryNames, "mybundle/resource2/resource2.txt");
  testExists(entryNames, "mybundle/");
  testExists(entryNames, "mybundle/resource2/");
}

/*
 * Use resource compiler to create Example4.zip
 * add a manifest, add resource1/resource1.txt using --res-add
 * and merge tomerge.zip into Example4.zip using --zip-add
 *
 * Working directory is changed temporarily to tempdir because of --res-add option
 */
void testZipAdd(const std::string& rcbinpath, const std::string& tempdir)
{
  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name mybundle ";
  cmd << " --manifest-add manifest.json ";
  cmd << " --out-file Example4.zip ";
  cmd << " --res-add resource1/resource1.txt ";
  cmd << " --zip-add tomerge.zip";

  auto cwdir = GetCurrentWorkingDirectory();
  testing::ChangeDirectory(tempdir);
  US_TEST_CONDITION_REQUIRED(EXIT_SUCCESS == runExecutable(cmd.str()),
                             "Cmdline invocation in testZipAdd returns 0");
  testing::ChangeDirectory(cwdir);

  ZipFile zip(tempdir + "Example4.zip");
  US_TEST_CONDITION(zip.size() == 6, "Check number of entries of zip.");

  auto entryNames = zip.getNames();
  testExists(entryNames, "mybundle/manifest.json");
  testExists(entryNames, "mybundle/");
  testExists(entryNames, "mybundle/resource1/resource1.txt");
  testExists(entryNames, "mybundle/resource1/");
  testExists(entryNames, "mybundle/resource2/resource2.txt");
  testExists(entryNames, "mybundle/resource2/");
}

/*
 * Use resource compiler to create zip files with various compression levels
 *
 * Working directory is changed temporarily to tempdir because of --res-add option
 */
void testCompressionLevel(const std::string& rcbinpath,
                          const std::string& tempdir)
{
  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name mybundle ";
  cmd << " --manifest-add manifest.json ";
  cmd << " --out-file ExampleCompressionLevel0.zip ";
  cmd << " --res-add resource1/resource1.txt ";
  cmd << " --zip-add tomerge.zip";
  cmd << " --compression-level 0";

  auto cwdir = util::GetCurrentWorkingDirectory();
  testing::ChangeDirectory(tempdir);
  US_TEST_CONDITION_REQUIRED(
    EXIT_SUCCESS == runExecutable(cmd.str()),
    "Test that --compression-level = 0 successfully creates a zip file.");
  testing::ChangeDirectory(cwdir);

  ZipFile zip(tempdir + "ExampleCompressionLevel0.zip");
  US_TEST_CONDITION(zip.size() == 6, "Check number of entries of zip.");

  auto entryNames = zip.getNames();
  testExists(entryNames, "mybundle/manifest.json");
  testExists(entryNames, "mybundle/");
  testExists(entryNames, "mybundle/resource1/resource1.txt");
  testExists(entryNames, "mybundle/resource1/");
  testExists(entryNames, "mybundle/resource2/resource2.txt");
  testExists(entryNames, "mybundle/resource2/");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --bundle-name mybundle ";
  cmd << " --manifest-add manifest.json ";
  cmd << " --out-file ExampleCompressionLevel3.zip ";
  cmd << " --res-add resource1/resource1.txt ";
  cmd << " --zip-add tomerge.zip";
  cmd << " --compression-level 3";

  cwdir = util::GetCurrentWorkingDirectory();
  testing::ChangeDirectory(tempdir);
  US_TEST_CONDITION_REQUIRED(
    EXIT_SUCCESS == runExecutable(cmd.str()),
    "Test that --compression-level = 3 successfully creates a zip file.");
  testing::ChangeDirectory(cwdir);

  zip = ZipFile(tempdir + "ExampleCompressionLevel3.zip");
  US_TEST_CONDITION(zip.size() == 6, "Check number of entries of zip.");

  entryNames = zip.getNames();
  testExists(entryNames, "mybundle/manifest.json");
  testExists(entryNames, "mybundle/");
  testExists(entryNames, "mybundle/resource1/resource1.txt");
  testExists(entryNames, "mybundle/resource1/");
  testExists(entryNames, "mybundle/resource2/resource2.txt");
  testExists(entryNames, "mybundle/resource2/");
}

/*
 * Use resource compiler to append a zip-file (using --zip-add)
 * to the bundle sample.dll using the -b option and also add a manifest
 * while doing so.
 */
void testZipAddBundle(const std::string& rcbinpath, const std::string& tempdir)
{
  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name mybundle ";
  cmd << " --bundle-file " << tempdir << "sample.dll ";
  cmd << " --manifest-add " << tempdir << "manifest.json ";
  cmd << " --zip-add " << tempdir << "tomerge.zip";

  US_TEST_CONDITION_REQUIRED(
    EXIT_SUCCESS == runExecutable(cmd.str()),
    "Cmdline invocation in testZipAddBundle returns 0");

  ZipFile zip(tempdir + "sample.dll");
  US_TEST_CONDITION(zip.size() == 4, "Check number of entries of zip.");

  auto entryNames = zip.getNames();
  testExists(entryNames, "mybundle/manifest.json");
  testExists(entryNames, "mybundle/");
  testExists(entryNames, "mybundle/resource2/resource2.txt");
  testExists(entryNames, "mybundle/resource2/");
}

/*
 * Add two zip-add arguments to a bundle-file.
 */
void testZipAddTwice(const std::string& rcbinpath, const std::string& tempdir)
{
  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-file " << tempdir << "sample1.dll ";
  cmd << " --zip-add " << tempdir << "tomerge.zip ";
  cmd << " --zip-add " << tempdir << "Example2.zip";

  US_TEST_CONDITION_REQUIRED(EXIT_SUCCESS == runExecutable(cmd.str()),
                             "Cmdline invocation in testZipAddTwice returns 0");

  ZipFile zip(tempdir + "sample1.dll");
  US_TEST_CONDITION(zip.size() == 6, "Check number of entries of zip.");

  auto entryNames = zip.getNames();
  testExists(entryNames, "mybundle/resource2/resource2.txt");
  testExists(entryNames, "mybundle/");
  testExists(entryNames, "mybundle/resource2/");
  testExists(entryNames, "mybundle/manifest.json");
  testExists(entryNames, "mybundle/resource1/resource1.txt");
  testExists(entryNames, "mybundle/resource1/");
}

/*
 * Add a manifest under bundle-name anotherbundle and add two zip blobs using the
 * zip-add arguments to a bundle-file.
 */
void testBundleManifestZipAdd(const std::string& rcbinpath,
                              const std::string& tempdir)
{
  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name anotherbundle ";
  cmd << " --manifest-add " << tempdir << "manifest.json ";
  cmd << " --bundle-file " << tempdir << "sample1.dll ";
  cmd << " --zip-add " << tempdir << "tomerge.zip ";
  cmd << " --zip-add " << tempdir << "Example2.zip";

  US_TEST_CONDITION_REQUIRED(
    EXIT_SUCCESS == runExecutable(cmd.str()),
    "Cmdline invocation in testBundleManifestZipAdd returns 0");

  ZipFile zip(tempdir + "sample1.dll");
  US_TEST_CONDITION(zip.size() == 8, "Check number of entries of zip.");

  auto entryNames = zip.getNames();
  testExists(entryNames, "anotherbundle/manifest.json");
  testExists(entryNames, "anotherbundle/");
  testExists(entryNames, "mybundle/resource2/resource2.txt");
  testExists(entryNames, "mybundle/");
  testExists(entryNames, "mybundle/resource2/");
  testExists(entryNames, "mybundle/manifest.json");
  testExists(entryNames, "mybundle/resource1/resource1.txt");
  testExists(entryNames, "mybundle/resource1/");
}

/*
 * Add the same manifest contents multiples times through --manifest-add
 * The intended behavior is that any subsequent duplicate manifest file is ignored
 */
void testDuplicateManifestFileAdd(const std::string& rcbinpath,
                                  const std::string& tempdir)
{
  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name multiple_dups ";
  cmd << " --manifest-add " << tempdir << "manifest.json ";
  cmd << " --manifest-add " << tempdir << "manifest.json ";
  cmd << " --out-file " << tempdir << "testDuplicateManifestFileAdd.zip ";

  US_TEST_CONDITION_REQUIRED(
    EXIT_SUCCESS == runExecutable(cmd.str()),
    "Cmdline invocation in testDuplicateManifestFileAdd returns 0");

  ZipFile zip(tempdir + "testDuplicateManifestFileAdd.zip");
  US_TEST_CONDITION(2 == zip.size(), "Check number of entries of zip.");
}

/*
 * --help returns exit code 0
 */
void testHelpReturnsZero(const std::string& rcbinpath)
{
  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --help";

  US_TEST_CONDITION(EXIT_SUCCESS == runExecutable(cmd.str()),
                    "help option returns zero");
}

/*
 * Test the failure modes of ResourceCompiler command
 */
void testFailureModes(const std::string& rcbinpath, const std::string& tempdir)
{
  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name foo";
  cmd << " --manifest-add ";
  cmd << " --bundle-file test2.dll";
  US_TEST_CONDITION(EXIT_FAILURE == runExecutable(cmd.str()),
                    "Failure mode: Empty --manifest-add option");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --bundle-name foo";
  cmd << " --manifest-add file_does_not_exist.json";
  cmd << " --bundle-file test2.dll";
  US_TEST_CONDITION(EXIT_FAILURE == runExecutable(cmd.str()),
                    "Failure mode: Manifest file does not exist");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --bundle-file test1.dll ";
  cmd << " --bundle-file test2.dll";
  US_TEST_CONDITION(EXIT_FAILURE == runExecutable(cmd.str()),
                    "Failure mode: Multiple bundle-file args");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --out-file test1.zip ";
  cmd << " --out-file test2.zip";
  US_TEST_CONDITION(EXIT_FAILURE == runExecutable(cmd.str()),
                    "Failure mode: Multiple out-file args");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --bundle-name foo ";
  cmd << " --bundle-name bar ";
  cmd << " --bundle-file bundlefile";
  US_TEST_CONDITION(EXIT_FAILURE == runExecutable(cmd.str()),
                    "Failure mode: Multiple bundle-name args");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --manifest-add manifest.json";
  cmd << " --bundle-name foobundle";
  US_TEST_CONDITION(EXIT_FAILURE == runExecutable(cmd.str()),
                    "Failure mode: --bundle-file or --out-file required");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --manifest-add manifest.json";
  US_TEST_CONDITION(EXIT_FAILURE == runExecutable(cmd.str()),
                    "Failure mode: --bundle-name required");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --res-add manifest.json";
  US_TEST_CONDITION(EXIT_FAILURE == runExecutable(cmd.str()),
                    "Failure mode: --bundle-name required");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --manifest-add manifest.json";
  cmd << " --bundle-name foo ";
  cmd << " --compression-level 11";
  US_TEST_CONDITION(EXIT_FAILURE == runExecutable(cmd.str()),
                    "Failure mode: invalid --compression-level argument (11)");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --manifest-add manifest.json";
  cmd << " --bundle-name foo ";
  cmd << " --compression-level -1";
  US_TEST_CONDITION(EXIT_FAILURE == runExecutable(cmd.str()),
                    "Failure mode: invalid --compression-level argument (-1)");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --bundle-name mybundle ";
  cmd << " --bundle-file " << tempdir << DIR_SEP << "sample1.dll ";
  cmd << " --zip-add " << tempdir << DIR_SEP << "tomerge.zip ";
  cmd << " --zip-add " << tempdir << DIR_SEP << "Example2.zip";
  US_TEST_CONDITION(EXIT_SUCCESS == runExecutable(cmd.str()),
                    "--bundle-name arg without either --manifest-add or "
                    "--res-add is just a warning");

  // Example.zip already contains mybundle/manifest.json
  // Should get an error when we are trying to manifest-add
  // mybundle/manifest.json
  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --bundle-name "
      << "mybundle";
  cmd << " --out-file " << tempdir << DIR_SEP << "Example7.zip";
  cmd << " --manifest-add " << tempdir << DIR_SEP << "manifest.json";
  cmd << " --zip-add " << tempdir << DIR_SEP << "Example.zip";
  US_TEST_CONDITION(EXIT_FAILURE == runExecutable(cmd.str()),
                    "Failure mode: duplicate manifest.json");
}

// Test escapePath functionality
void testEscapePath()
{
  // Test escapePath function
  std::string path1("/tmp/path (space)/rc");
  escapePath(path1);
  US_TEST_CONDITION(path1 == "/tmp/path\\ \\(space\\)/rc",
                    "Test escapePath #1");

  std::string path2("/tmp/foo/bar");
  escapePath(path2);
  US_TEST_CONDITION(path2 == "/tmp/foo/bar", "Test escapePath #2");

  std::string path3("/home/travis/CppMicroServices/us builds (Unix "
                    "make)/bin/usResourceCompiler");
  escapePath(path3);
  US_TEST_CONDITION(path3 == "/home/travis/CppMicroServices/us\\ builds\\ "
                             "\\(Unix\\ make\\)/bin/usResourceCompiler",
                    "Test escapePath #3");
}

// Create a manifest.json file
void createManifestFile(const std::string tempdir,
                        const std::string manifest_json,
                        const std::string manifest_json_file = "manifest.json")
{
  std::string manifest_fpath(tempdir + manifest_json_file);
  std::ofstream manifest(manifest_fpath);
  if (!manifest.is_open()) {
    throw std::runtime_error("Couldn't open " + manifest_fpath);
  }
  manifest << manifest_json << std::endl;
  manifest.close();
}

// Create a fake shared library which will be used to test embedding invalid manifest data
// using usResourceCompiler.
void createDummyBundle(const std::string tempdir, const std::string bundle_name)
{
  // Create a binary file filled with random numbers
  // to test usResourceCompiler functionality.
  auto create_mock_dll = [&tempdir](const std::string& dllname,
                                    const std::array<char, 5>& dat) {
    std::string dll_path(tempdir + dllname);
    std::ofstream dll(dll_path.c_str());
    if (!dll.is_open()) {
      throw std::runtime_error("Couldn't open " + dll_path);
    }
    dll.write(dat.data(), dat.size()); //binary write
    dll.close();
  };
  std::array<char, 5> data1 = { { 2, 4, 6, 8, 10 } };
  create_mock_dll(bundle_name, data1);
}

// A helper function designed to validate whether embedding an invalid manifest
// to a fake shared library worked or not.
// returns true if there is a zip file contaning at least one entry in it.
// returns false otherwise.
bool containsBundleZipFile(const std::string bundle_file_path)
{
  try {
    ZipFile bundle(bundle_file_path);
    return (bundle.size() > 0);
  } catch (...) {
    return false;
  }
  return false;
}

// Test the multiple ways in which a manifest can be added...
// --manifest-add to a user defined zip file (using --out-file)
// --manifest-add to a shared library (using --bundle-file)
// --res-add to a user defined zip file (using --out-file)
// --res-add to a shared library (using --bundle-file)
// --zip-add to merge with another zip file (using --out-file)
// --zip-add to append to a bundle (using --bundle-file)
//
// in all cases, the invalid manifest.json should never be added
// to the zip or shared library.

void testManifestAddWithInvalidJSON(const std::string& rcbinpath,
                                    const std::string& tempdir)
{
  std::string invalidSyntax(R"(
		{
		 "bundle.name": "foo",
		 "bundle.version" : "1.0.2"
		 "bundle.description" : "This bundle is broken!",
		 "authors" : ["John Doe", "Douglas Reynolds", "Daniel Cannady"],
		 "rating" : 5
		} 
    )");

  US_TEST_NO_EXCEPTION_REQUIRED(createManifestFile(tempdir, invalidSyntax));

  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name "
      << "mybundle";
  cmd << " --out-file " << tempdir << "invalid_syntax.zip";
  cmd << " --manifest-add " << tempdir << "manifest.json";
  US_TEST_CONDITION(BUNDLE_MANIFEST_VALIDATION_ERROR_CODE ==
                      runExecutable(cmd.str()),
                    "Fail to embed manifest containing JSON syntax errors.");

  ZipFile invalid_syntax_zip(tempdir + "invalid_syntax.zip");
  US_TEST_CONDITION(0 == invalid_syntax_zip.size(),
                    "Test that the invalid manifest.json file was not added");

  std::string invalid_syntax_bundle_name("invalid_syntax.bundle");
  US_TEST_NO_EXCEPTION_REQUIRED(
    createDummyBundle(tempdir, invalid_syntax_bundle_name));

  std::ostringstream cmd2;
  cmd2 << rcbinpath;
  cmd2 << " --bundle-name "
       << "mybundle";
  cmd2 << " --bundle-file " << tempdir << invalid_syntax_bundle_name;
  cmd2 << " --manifest-add " << tempdir << "manifest.json";
  US_TEST_CONDITION(BUNDLE_MANIFEST_VALIDATION_ERROR_CODE ==
                      runExecutable(cmd2.str()),
                    "Fail to embed manifest containing JSON syntax errors.");
  US_TEST_CONDITION(
    false == containsBundleZipFile(tempdir + invalid_syntax_bundle_name),
    "Test that the invalid manifest.json file was not added");

  std::ostringstream cmd3;
  cmd3 << rcbinpath;
  cmd3 << " --bundle-name mybundle ";
  cmd3 << " --out-file invalid_syntax_res_add.zip ";
  cmd3 << " --res-add manifest.json";

  auto origdir = util::GetCurrentWorkingDirectory();
  testing::ChangeDirectory(tempdir);
  US_TEST_CONDITION(BUNDLE_MANIFEST_VALIDATION_ERROR_CODE ==
                      runExecutable(cmd3.str()),
                    "Fail to embed manifest containing JSON syntax errors.");
  testing::ChangeDirectory(origdir);

  ZipFile invalid_syntax_res_add_zip(tempdir + "invalid_syntax_res_add.zip");
  US_TEST_CONDITION(0 == invalid_syntax_res_add_zip.size(),
                    "Test that the invalid manifest.json file was not added");

  std::string invalid_syntax_res_add_bundle_name(
    "invalid_syntax_res_add.bundle");

  std::ostringstream cmd4;
  cmd4 << rcbinpath;
  cmd4 << " --bundle-name mybundle ";
  cmd4 << " --bundle-file " << invalid_syntax_res_add_bundle_name;
  cmd4 << " --res-add manifest.json";

  testing::ChangeDirectory(tempdir);
  US_TEST_CONDITION(BUNDLE_MANIFEST_VALIDATION_ERROR_CODE ==
                      runExecutable(cmd4.str()),
                    "Fail to embed manifest containing JSON syntax errors.");
  testing::ChangeDirectory(origdir);
  US_TEST_CONDITION(false == containsBundleZipFile(
                               tempdir + invalid_syntax_res_add_bundle_name),
                    "Test that the invalid manifest.json file was not added");
}

// In jsoncpp 0.10.6 not allowing comments does NOT result in a parse failure for a JSON file with comments.
// Instead, assuming the JSON is valid, parsing returns JSON stripped of the comments.
// This test will only makes sure that JSON with comments can be added successfully.
// It shouldn't be necessary to check that JSON comments are actually stripped from the output json as
// that should be the responsibility of jsoncpp's tests and we will rely on that.
// There is an issue logged for this behavior in jsoncpp (https://github.com/open-source-parsers/jsoncpp/issues/690)
void testManifestAddWithJSONComments(const std::string& rcbinpath,
                                     const std::string& tempdir)
{
  std::string jsonCommentSyntax(R"(
		{ /* no, no, no. */
		 "bundle.name": "foo",
		 "bundle.version" : "1.0.2",
		 "bundle.description" : "This bundle shouldn't have comments!",
		 "authors" : ["John Doe", "Douglas Reynolds", "Daniel Cannady"],
		 "rating" : 5 // no comments allowed
		} 
    )");

  US_TEST_NO_EXCEPTION_REQUIRED(createManifestFile(tempdir, jsonCommentSyntax));

  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name "
      << "mybundle";
  cmd << " --out-file " << tempdir << "json_comment_syntax.zip";
  cmd << " --manifest-add " << tempdir << "manifest.json";
  US_TEST_CONDITION(EXIT_SUCCESS == runExecutable(cmd.str()),
                    "Test embedding a manifest containing JSON comments.");
}

void testManifestAddWithDuplicateKeys(const std::string& rcbinpath,
                                      const std::string& tempdir)
{
  std::string dupKeys(R"(
		{
		 "bundle.name": "foo",
		 "bundle.version" : "1.0.2",
		 "bundle.description" : "This bundle is broken!",
		 "authors" : ["John Doe", "Douglas Reynolds", "Daniel Cannady"],
		 "bundle.name" : "foobar",
		 "rating" : 5
		}
    )");

  US_TEST_NO_EXCEPTION_REQUIRED(createManifestFile(tempdir, dupKeys));

  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name "
      << "mybundle";
  cmd << " --out-file " << tempdir << "duplicate_keys.zip";
  cmd << " --manifest-add " << tempdir << "manifest.json";
  US_TEST_CONDITION(
    BUNDLE_MANIFEST_VALIDATION_ERROR_CODE == runExecutable(cmd.str()),
    "Fail to embed manifest containing duplicate JSON key names.");

  ZipFile duplicate_keys_zip(tempdir + "duplicate_keys.zip");
  US_TEST_CONDITION(0 == duplicate_keys_zip.size(),
                    "Test that the invalid manifest.json file was not added");

  std::string duplicate_keys_bundle_file("duplicate_keys.bundle");
  std::ostringstream cmd2;
  cmd2 << rcbinpath;
  cmd2 << " --bundle-name "
       << "mybundle";
  cmd2 << " --bundle-file " << tempdir << duplicate_keys_bundle_file;
  cmd2 << " --manifest-add " << tempdir << "manifest.json";
  US_TEST_CONDITION(
    BUNDLE_MANIFEST_VALIDATION_ERROR_CODE == runExecutable(cmd2.str()),
    "Fail to embed manifest containing duplicate JSON key names.");

  US_TEST_CONDITION(
    false == containsBundleZipFile(tempdir + duplicate_keys_bundle_file),
    "Test that the invalid manifest.json file was not added");

  std::ostringstream cmd3;
  cmd3 << rcbinpath;
  cmd3 << " --bundle-name mybundle ";
  cmd3 << " --out-file duplicate_keys_res_add.zip ";
  cmd3 << " --res-add manifest.json";

  auto origdir = util::GetCurrentWorkingDirectory();
  testing::ChangeDirectory(tempdir);
  US_TEST_CONDITION(
    BUNDLE_MANIFEST_VALIDATION_ERROR_CODE == runExecutable(cmd3.str()),
    "Fail to embed manifest containing duplicate JSON key names.");
  testing::ChangeDirectory(origdir);

  ZipFile duplicate_keys_res_add_zip(tempdir + "duplicate_keys_res_add.zip");
  US_TEST_CONDITION(0 == duplicate_keys_res_add_zip.size(),
                    "Test that the invalid manifest.json file was not added");

  std::string duplicate_keys_res_add_bundle_file(
    "duplicate_keys_res_add.bundle");
  std::ostringstream cmd4;
  cmd4 << rcbinpath;
  cmd4 << " --bundle-name mybundle ";
  cmd4 << " --bundle-file " << duplicate_keys_res_add_bundle_file;
  cmd4 << " --res-add manifest.json";

  testing::ChangeDirectory(tempdir);
  US_TEST_CONDITION(
    BUNDLE_MANIFEST_VALIDATION_ERROR_CODE == runExecutable(cmd4.str()),
    "Fail to embed manifest containing duplicate JSON key names.");
  testing::ChangeDirectory(origdir);
  US_TEST_CONDITION(false == containsBundleZipFile(
                               tempdir + duplicate_keys_res_add_bundle_file),
                    "Test that the invalid manifest.json file was not added");
}

// A zip file containing only a manifest.
class ManifestZipFile
{
public:
  // zip_file_name - zip file path
  // manifest_json - contents of the manifest.json file
  // bundle_name - name of the bundle
  ManifestZipFile(const std::string& zip_file_name,
                  const std::string& manifest_json,
                  const std::string& bundle_name)
  {
    std::string archiveEntry(bundle_name + "/manifest.json");
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(mz_zip_archive));

    mz_zip_writer_init_file(&zip, zip_file_name.c_str(), 0);
    mz_zip_writer_add_mem(&zip,
                          archiveEntry.c_str(),
                          manifest_json.c_str(),
                          manifest_json.size() *
                            sizeof(std::string::value_type),
                          MZ_DEFAULT_COMPRESSION);
    mz_zip_writer_finalize_archive(&zip);
    mz_zip_writer_end(&zip);
  }

  ManifestZipFile(const ManifestZipFile&) = delete;
  void operator=(const ManifestZipFile&) = delete;
  ManifestZipFile(const ManifestZipFile&&) = delete;
  ManifestZipFile& operator=(const ManifestZipFile&&) = delete;

  ~ManifestZipFile() {}
};

// Use case: A zip file created using an older usResourceCompiler (which doesn't validate manifest.json)
// and containing an invalid manifest.json is appended to a bundle.
void testAppendZipWithInvalidManifest(const std::string& rcbinpath,
                                      const std::string& tempdir)
{
  const std::string manifest_json = R"({
     "bundle.symbolic_name" : "main",
     "bundle.version" : "0.1.0",
     "bundle.activator" : true
    })";

  US_TEST_NO_EXCEPTION_REQUIRED(createDirHierarchy(tempdir, manifest_json));

  std::ostringstream cmdCreateBundle;
  cmdCreateBundle << rcbinpath;
  cmdCreateBundle << " --bundle-name main ";
  cmdCreateBundle << " --bundle-file " << tempdir << "sample.dll ";
  cmdCreateBundle << " --manifest-add " << tempdir << "manifest.json ";
  US_TEST_CONDITION_REQUIRED(
    EXIT_SUCCESS == runExecutable(cmdCreateBundle.str()),
    "Test that the manifest.json file was embedded correctly.");

  std::string invalidSyntax(R"(
		{
         "bundle.name": "foo",
         "bundle.version" : "1.0.2"
         "bundle.description" : "This bundle is broken!",
         "authors" : ["John Doe", "Douglas Reynolds", "Daniel Cannady"],
         "rating" : 5
		} 
    )");

  ManifestZipFile badZip(
    tempdir + "append_invalid_manifest_zip.zip", invalidSyntax, "invalid");
  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name "
      << "invalid";
  cmd << " --bundle-file " << tempdir << "sample.dll";
  cmd << " --zip-add " << tempdir << "append_invalid_manifest_zip.zip";
  US_TEST_CONDITION(
    BUNDLE_MANIFEST_VALIDATION_ERROR_CODE == runExecutable(cmd.str()),
    "Fail to append a zip file containing an invalid manifest.json");

  ZipFile append_invalid_manifest_zip(tempdir + "sample.dll");
  /// TODO: when --add-manifest is used, two archive entries exist; one for the directory and one for the manifest.json in that directory.
  US_TEST_CONDITION(
    2 == append_invalid_manifest_zip.size(),
    "Test that the invalid manifest.json file was not appended to the bundle.");
  US_TEST_CONDITION("main/manifest.json" == append_invalid_manifest_zip[0].name,
                    "Test that only the valid manifest.json exists.");
}

// Use case: A zip file created using an older usResourceCompiler (which doesn't validate manifest.json)
// and containing an invalid manifest.json is merged with a valid zip file.
void testZipMergeWithInvalidManifest(const std::string& rcbinpath,
                                     const std::string& tempdir)
{
  const std::string manifest_json = R"({
    "bundle.symbolic_name" : "main",
    "bundle.version" : "0.1.0",
    "bundle.activator" : true
    })";

  ManifestZipFile validZip(tempdir + "merged_zip.zip", manifest_json, "main");

  std::string invalidSyntax(R"(
		{
		 "bundle.name": "foo",
		 "bundle.version" : "1.0.2"
		 "bundle.description" : "This bundle is broken!",
		 "authors" : ["John Doe", "Douglas Reynolds", "Daniel Cannady"],
		 "rating" : 5
		} 
    )");

  ManifestZipFile badZip(
    tempdir + "invalid_manifest.zip", invalidSyntax, "invalid");

  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name "
      << "main";
  cmd << " --out-file " << tempdir << "new_merged_zip.zip";
  cmd << " --zip-add " << tempdir << "merged_zip.zip";
  cmd << " --zip-add " << tempdir << "invalid_manifest.zip";
  US_TEST_CONDITION(
    BUNDLE_MANIFEST_VALIDATION_ERROR_CODE == runExecutable(cmd.str()),
    "Fail to merge a zip file containing an invalid manifest.json");

  ZipFile new_merged_zip(tempdir + "new_merged_zip.zip");
  US_TEST_CONDITION_REQUIRED(1 == new_merged_zip.size(),
                             "Test that the invalid manifest.json file was not "
                             "merged into the zip file.");
  US_TEST_CONDITION_REQUIRED("main/manifest.json" == new_merged_zip[0].name,
                             "Test that only the valid manifest.json exists.");
}

// test that adding multiple manifests result in only one manifest.json being embedded correctly.
// test that one or more invalid manifest results in failing to embed the one and only manifest.json.
void testMultipleManifestAdd(const std::string& rcbinpath,
                             const std::string& tempdir)
{
  const std::string manifest_json_part_1 = R"({
    "bundle.symbolic_name" : "main",
    "bundle.version" : "0.1.0",
    "bundle.activator" : true
    })";

  const std::string manifest_json_part_2 = R"({
    "bundle.description" : "second manifest part"
    })";

  const std::string manifest_json_part_3 = R"({
    "test" : { 
        "foobar" : [1, 2, 5, 7]
     },
    "bundle.foo" : "foo",
    "bundle.bar" : "bar",
    "bundle.name" : "baz"
    })";

  createManifestFile(tempdir, manifest_json_part_1, "manifest_part1.json");
  createManifestFile(tempdir, manifest_json_part_2, "manifest_part2.json");
  createManifestFile(tempdir, manifest_json_part_3, "manifest_part3.json");

  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name "
      << "main";
  cmd << " --out-file " << tempdir << "merged_zip.zip";
  cmd << " --manifest-add " << tempdir << "manifest_part1.json";
  cmd << " --manifest-add " << tempdir << "manifest_part2.json";
  cmd << " --manifest-add " << tempdir << "manifest_part3.json";
  US_TEST_CONDITION(
    EXIT_SUCCESS == runExecutable(cmd.str()),
    "Test successful concatenation of multiple manifest.json files into one.");

  ZipFile merged_zip(tempdir + "merged_zip.zip");
  US_TEST_CONDITION_REQUIRED(2 == merged_zip.size(),
                             "Test that the manifest.json file parts were "
                             "merged into one manifest.json.");
  US_TEST_CONDITION("main/manifest.json" == merged_zip[0].name,
                    "Test that only one manifest.json was embedded.");

  std::string invalidManifestPart(R"(
		{
		 "invalid": "no comma" 
		 "never.getting" : "here"
		} 
    )");

  createManifestFile(tempdir, invalidManifestPart, "invalid_manifest.json");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --bundle-name "
      << "main";
  cmd << " --out-file " << tempdir << "invalid_merged_zip.zip";
  cmd << " --manifest-add " << tempdir << "manifest_part1.json";
  cmd << " --manifest-add " << tempdir << "manifest_part2.json";
  cmd << " --manifest-add " << tempdir << "invalid_manifest.json";
  cmd << " --manifest-add " << tempdir << "manifest_part3.json";
  US_TEST_CONDITION(
    BUNDLE_MANIFEST_VALIDATION_ERROR_CODE == runExecutable(cmd.str()),
    "Test that an invalid manifest json part fails to embed the manifest.");

  ZipFile invalid_merged_zip(tempdir + "invalid_merged_zip.zip");
  US_TEST_CONDITION(0 == invalid_merged_zip.size(),
                    "Test that no manifest.json was embedded since there was "
                    "an invalid manifest part.");

  const std::string manifest_json_part_3_duplicate = R"({
    "test" : { 
        "foobar" : [1, 2, 5, 7]
    },
    "embedded" : { 
        "object" : { 
            "array" : ["one", "two", "twenty"],
            "string" : "boo",
            "integer" : 42,
            "float" : 101.1,
            "boolean" : false,
            "nullvalue" : null
        } 
    },
    "name" : " ",
    "nullvalue" : null
    })";

  createManifestFile(
    tempdir, manifest_json_part_3_duplicate, "duplicate_manifest.json");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --bundle-name "
      << "main";
  cmd << " --out-file " << tempdir << "duplicate_merged_zip.zip";
  cmd << " --manifest-add " << tempdir << "manifest_part1.json";
  cmd << " --manifest-add " << tempdir << "manifest_part2.json";
  cmd << " --manifest-add " << tempdir << "duplicate_manifest.json";
  cmd << " --manifest-add " << tempdir << "manifest_part3.json";
  US_TEST_CONDITION(
    BUNDLE_MANIFEST_VALIDATION_ERROR_CODE == runExecutable(cmd.str()),
    "Test that a duplicate manifest json part fails to embed the manifest.");

  ZipFile duplicate_merged_zip(tempdir + "duplicate_merged_zip.zip");
  US_TEST_CONDITION(0 == duplicate_merged_zip.size(),
                    "Test that no manifest.json was embedded since there was "
                    "an duplicate manifest part.");
}

// return the contents of the manifest file for the given bundle_name
std::string getManifestContent(const std::string& zipfile,
                               std::string bundle_name)
{
  mz_zip_archive zipArchive;
  memset(&zipArchive, 0, sizeof(mz_zip_archive));

  if (!mz_zip_reader_init_file(&zipArchive, zipfile.c_str(), 0)) {
    throw std::runtime_error("Could not initialize zip archive " + zipfile);
  }

  std::string manifestArchiveEntry(bundle_name + "/manifest.json");
  char archiveEntryContents[MZ_ZIP_MAX_IO_BUF_SIZE];
  memset(archiveEntryContents, 0, MZ_ZIP_MAX_IO_BUF_SIZE);
  if (MZ_TRUE == mz_zip_reader_extract_file_to_mem(&zipArchive,
                                                   manifestArchiveEntry.c_str(),
                                                   archiveEntryContents,
                                                   MZ_ZIP_MAX_IO_BUF_SIZE,
                                                   0)) {
    mz_zip_reader_end(&zipArchive);
    return std::string(archiveEntryContents);
  }

  mz_zip_reader_end(&zipArchive);
  return std::string();
}

// test that specifying multiple manifests concatenates them all into one correctly.
void testMultipleManifestConcatenation(const std::string& rcbinpath,
                                       const std::string& tempdir)
{
  const std::string manifest_json_part_1 = R"({
    "bundle.symbolic_name" : "main",
    "bundle.version" : "0.1.0",
    "bundle.activator" : true
    })";

  const std::string manifest_json_part_2 = R"({
    "bundle.description" : "second manifest part"
    })";

  const std::string manifest_json_part_3 = R"({
    "test" : { 
        "foo" : [1, 2, 5, 7],
        "bar" : "baz"
    },
    "embedded" : { 
        "object" : { 
            "array" : ["one", "two", "twenty"],
            "string" : "boo",
            "integer" : 42,
            "float" : 101.1,
            "boolean" : false
        } 
    },
    "name" : " "
    })";

  createManifestFile(tempdir, manifest_json_part_1, "manifest_part1.json");
  createManifestFile(tempdir, manifest_json_part_2, "manifest_part2.json");
  createManifestFile(tempdir, manifest_json_part_3, "manifest_part3.json");

  const std::string manifest_json = R"({
    "bundle.symbolic_name" : "main",
    "bundle.version" : "0.1.0",
    "bundle.activator" : true,
    "bundle.description" : "second manifest part",
    "test" : { 
        "foo" : [1, 2, 5, 7],
        "bar" : "baz"
    },
    "embedded" : { 
        "object" : { 
            "array" : ["one", "two", "twenty"],
            "string" : "boo",
            "integer" : 42,
            "float" : 101.1,
            "boolean" : false
        } 
    },
    "name" : " "
    })";

  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name "
      << "main";
  cmd << " --out-file " << tempdir << "merged_zip.zip";
  cmd << " --manifest-add " << tempdir << "manifest_part1.json";
  cmd << " --manifest-add " << tempdir << "manifest_part2.json";
  cmd << " --manifest-add " << tempdir << "manifest_part3.json";
  US_TEST_CONDITION(EXIT_SUCCESS == runExecutable(cmd.str()),
                    "Test the successful concatenation of multiple "
                    "manifest.json files into one.");

  Json::Reader reader;
  Json::Value root;
  bool ok = reader.parse(manifest_json, root, false);

  US_TEST_CONDITION(
    true == ok, "Test that the expected JSON content was parsed correctly.");

  std::string expectedJSON(root.toStyledString());
  // retrieve the JSON which was concatenated by usResourceCompiler
  std::string concatenatedJSON;
  US_TEST_NO_EXCEPTION_REQUIRED(
    concatenatedJSON = getManifestContent(tempdir + "merged_zip.zip", "main"));

  std::cout << "Expected JSON:\n\n"
            << expectedJSON << "JSON concatenated by usResourceCompiler:\n\n"
            << concatenatedJSON << std::endl;

  // line feed and new line characters may be lurking in the strings. I don't know how to use miniz and jsoncpp to embed
  // these characters in a consistent manner, so I'm opting to remove them afterwards.
  removeLineEndings(concatenatedJSON);
  removeLineEndings(expectedJSON);

  US_TEST_CONDITION(0 == concatenatedJSON.compare(expectedJSON),
                    "Test that the concatenated JSON content matches the "
                    "expected JSON content.");
}

// test adding a manifest and merging a zip file with a manifest containing a null terminator works.
// This test ensures that the code which extracts the manifest contents from an archive does not truncate the file due to a null terminator.
// NOTE: The C string null terminator in JSON must be escaped. Otherwise, there is a JSON syntax error.
void testManifestWithNullTerminator(const std::string& rcbinpath,
                                    const std::string& tempdir)
{
  const std::string manifest_json = R"({
    "test" : { 
        "bar" : "baz\\0bar",
        "foo\\0bar" : [1, 2, 5, 7]
    }})";

  const std::string jsonFileName("manifest_with_embedded_null_terminator.json");
  const std::string zipFile("embedded_null_terminator.zip");

  createManifestFile(tempdir, manifest_json, jsonFileName);

  std::ostringstream cmd;
  cmd << rcbinpath;
  cmd << " --bundle-name "
      << "main";
  cmd << " --out-file " << tempdir << zipFile;
  cmd << " --manifest-add " << tempdir << jsonFileName;
  US_TEST_CONDITION(EXIT_SUCCESS == runExecutable(cmd.str()),
                    "Test the successful embedding of a manifest containing an "
                    "embedded null terminator.");

  Json::Reader reader;
  Json::Value root;
  bool ok = reader.parse(manifest_json, root, false);
  US_TEST_CONDITION(
    true == ok, "Test that the expected JSON content was parsed correctly.");

  std::string expectedJSON(root.toStyledString());

  std::string nullTerminatorJSON;
  US_TEST_NO_EXCEPTION_REQUIRED(
    nullTerminatorJSON =
      getManifestContent(tempdir + "embedded_null_terminator.zip", "main"));

  std::cout << "Expected JSON:\n\n"
            << expectedJSON << "JSON embedded by usResourceCompiler:\n\n"
            << nullTerminatorJSON << std::endl;

  // line feed and new line characters may be lurking in the strings. I don't know how to use miniz and jsoncpp to embed
  // these characters in a consistent manner, so I'm opting to remove them afterwards.
  removeLineEndings(nullTerminatorJSON);
  removeLineEndings(expectedJSON);

  US_TEST_CONDITION(
    0 == nullTerminatorJSON.compare(expectedJSON),
    "Test that the JSON content matches the expected JSON content.");

  const std::string mergedZipFile("merged_null_terminator.zip");

  cmd.str("");
  cmd.clear();
  cmd << rcbinpath;
  cmd << " --out-file " << tempdir << mergedZipFile;
  cmd << " --zip-add " << tempdir << zipFile;
  US_TEST_CONDITION(EXIT_SUCCESS == runExecutable(cmd.str()),
                    "Test the successful merging of zip file containing a "
                    "manifest with an embedded null terminator.");

  US_TEST_NO_EXCEPTION_REQUIRED(
    nullTerminatorJSON = getManifestContent(tempdir + mergedZipFile, "main"));

  std::cout << "Expected JSON:\n\n"
            << expectedJSON << "\n\nJSON merged by usResourceCompiler:\n\n"
            << nullTerminatorJSON << std::endl;

  // line feed and new line characters may be lurking in the strings. I don't know how to use miniz and jsoncpp to embed
  // these characters in a consistent manner, so I'm opting to remove them afterwards.
  removeLineEndings(nullTerminatorJSON);

  US_TEST_CONDITION(
    0 == nullTerminatorJSON.compare(expectedJSON),
    "Test that the JSON content matches the expected JSON content.");
}
}

int ResourceCompilerTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("ResourceCompilerTest");

  testEscapePath();

  auto rcbinpath = testing::RCC_PATH;
  /*
  * If ResourceCompiler executable is not found, we can't run the tests, we
  * mark it as a failure and exit
  */
  std::ifstream binf(rcbinpath.c_str());
  if (!binf.good()) {
    US_TEST_FAILED_MSG(<< "Cannot find usResourceCompiler executable:"
                       << rcbinpath);
  }

  testing::TempDir uniqueTempdir = testing::MakeUniqueTempDirectory();
  auto tempdir = uniqueTempdir.Path + DIR_SEP;
#ifndef US_PLATFORM_WINDOWS
  escapePath(tempdir);
  escapePath(rcbinpath);
#endif

  const std::string manifest_json = R"({
    "bundle.symbolic_name" : "main",
    "bundle.version" : "0.1.0",
    "bundle.activator" : true
    })";

  US_TEST_NO_EXCEPTION_REQUIRED(createDirHierarchy(tempdir, manifest_json));

  US_TEST_NO_EXCEPTION(testManifestAdd(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testManifestResAdd(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testResAdd(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testZipAdd(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testZipAddBundle(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testZipAddTwice(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testBundleManifestZipAdd(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testCompressionLevel(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testDuplicateManifestFileAdd(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testHelpReturnsZero(rcbinpath));

  US_TEST_NO_EXCEPTION(testFailureModes(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testManifestAddWithInvalidJSON(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testManifestAddWithJSONComments(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testManifestAddWithDuplicateKeys(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testAppendZipWithInvalidManifest(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testZipMergeWithInvalidManifest(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testMultipleManifestAdd(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testMultipleManifestConcatenation(rcbinpath, tempdir));

  US_TEST_NO_EXCEPTION(testManifestWithNullTerminator(rcbinpath, tempdir));

  US_TEST_END()
}
