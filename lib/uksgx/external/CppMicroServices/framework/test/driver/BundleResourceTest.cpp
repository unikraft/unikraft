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
#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/BundleResourceStream.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkFactory.h"
#include "cppmicroservices/GetBundleContext.h"

#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

#include <cassert>
#include <memory>
#include <unordered_set>

using namespace cppmicroservices;

namespace {

// Please confirm that a character count differing from the following targets is not due to
// a misconfiguration of your versioning software (Correct line endings for your system)
// See issue #18 ( https://github.com/CppMicroServices/CppMicroServices/issues/18 )
void checkResourceInfo(const BundleResource& res,
                       const std::string& path,
                       const std::string& baseName,
                       const std::string& completeBaseName,
                       const std::string& suffix,
                       const std::string& completeSuffix,
                       int size,
                       bool children = false)
{
  US_TEST_CONDITION_REQUIRED(res.IsValid(), "Valid resource")
  US_TEST_CONDITION(res.GetBaseName() == baseName, "GetBaseName()")
  US_TEST_CONDITION(res.GetChildren().empty() == !children, "No children")
  US_TEST_CONDITION(res.GetCompleteBaseName() == completeBaseName,
                    "GetCompleteBaseName()")
  US_TEST_CONDITION(res.GetName() == completeBaseName + "." + suffix,
                    "GetName()")
  US_TEST_CONDITION(res.GetResourcePath() ==
                      path + completeBaseName + "." + suffix,
                    "GetResourcePath()")
  US_TEST_CONDITION(res.GetPath() == path, "GetPath()")
  US_TEST_CONDITION(res.GetSize() == size, "Data size")
  US_TEST_CONDITION(res.GetSuffix() == suffix, "Suffix")
  US_TEST_CONDITION(res.GetCompleteSuffix() == completeSuffix,
                    "Complete suffix")
}

void testTextResource(const Bundle& bundle)
{
  BundleResource res = bundle.GetResource("foo.txt");
#ifdef US_PLATFORM_WINDOWS
  checkResourceInfo(res, "/", "foo", "foo", "txt", "txt", 16, false);
  const std::streampos ssize(13);
  const std::string fileData = "foo and\nbar\n\n";
#else
  checkResourceInfo(res, "/", "foo", "foo", "txt", "txt", 13, false);
  const std::streampos ssize(12);
  const std::string fileData = "foo and\nbar\n";
#endif

  BundleResourceStream rs(res);

  rs.seekg(0, std::ios::end);
  US_TEST_CONDITION(rs.tellg() == ssize, "Stream content length");
  rs.seekg(0, std::ios::beg);

  std::string content;
  content.reserve(res.GetSize());
  char buffer[1024];
  while (rs.read(buffer, sizeof(buffer))) {
    content.append(buffer, sizeof(buffer));
  }
  content.append(buffer, static_cast<std::size_t>(rs.gcount()));

  US_TEST_CONDITION(rs.eof(), "EOF check");
  US_TEST_CONDITION(content == fileData, "Resource content");

  rs.clear();
  rs.seekg(0);

  US_TEST_CONDITION_REQUIRED(rs.tellg() == std::streampos(0), "Move to start")
  US_TEST_CONDITION_REQUIRED(rs.good(), "Start re-reading");

  std::vector<std::string> lines;
  std::string line;
  while (std::getline(rs, line)) {
    lines.push_back(line);
  }
  US_TEST_CONDITION_REQUIRED(lines.size() > 1, "Number of lines")
  US_TEST_CONDITION(lines[0] == "foo and", "Check first line")
  US_TEST_CONDITION(lines[1] == "bar", "Check second line")
}

void testTextResourceAsBinary(const Bundle& bundle)
{
  BundleResource res = bundle.GetResource("foo.txt");

#ifdef US_PLATFORM_WINDOWS
  checkResourceInfo(res, "/", "foo", "foo", "txt", "txt", 16, false);
  const std::streampos ssize(16);
  const std::string fileData = "foo and\r\nbar\r\n\r\n";
#else
  checkResourceInfo(res, "/", "foo", "foo", "txt", "txt", 13, false);
  const std::streampos ssize(13);
  const std::string fileData = "foo and\nbar\n\n";
#endif

  BundleResourceStream rs(res, std::ios_base::binary);

  rs.seekg(0, std::ios::end);
  US_TEST_CONDITION(rs.tellg() == ssize, "Stream content length");
  rs.seekg(0, std::ios::beg);

  std::string content;
  content.reserve(res.GetSize());
  char buffer[1024];
  while (rs.read(buffer, sizeof(buffer))) {
    content.append(buffer, sizeof(buffer));
  }
  content.append(buffer, static_cast<std::size_t>(rs.gcount()));

  US_TEST_CONDITION(rs.eof(), "EOF check");
  US_TEST_CONDITION(content == fileData, "Resource content");
}

void testInvalidResource(const Bundle& bundle)
{
  BundleResource res = bundle.GetResource("invalid");
  US_TEST_CONDITION_REQUIRED(res.IsValid() == false, "Check invalid resource")
  US_TEST_CONDITION(res.GetName().empty(), "Check empty name")
  US_TEST_CONDITION(res.GetPath().empty(), "Check empty path")
  US_TEST_CONDITION(res.GetResourcePath().empty(), "Check empty resource path")
  US_TEST_CONDITION(res.GetBaseName().empty(), "Check empty base name")
  US_TEST_CONDITION(res.GetCompleteBaseName().empty(),
                    "Check empty complete base name")
  US_TEST_CONDITION(res.GetSuffix().empty(), "Check empty suffix")

  US_TEST_CONDITION(res.GetChildren().empty(), "Check empty children")
  US_TEST_CONDITION(res.GetSize() == 0, "Check zero size")
  US_TEST_CONDITION(res.GetCompressedSize() == 0, "Check zero compressed size")
  US_TEST_CONDITION(res.GetLastModified() == 0, "Check zero last modification time")
  US_TEST_CONDITION(res.GetCrc32() == 0, "Check zero CRC-32")

  BundleResourceStream rs(res);
  US_TEST_CONDITION(rs.good() == true, "Check invalid resource stream")
  rs.ignore();
  US_TEST_CONDITION(rs.good() == false, "Check invalid resource stream")
  US_TEST_CONDITION(rs.eof() == true, "Check invalid resource stream")
}

void testSpecialCharacters(const Bundle& bundle)
{
  BundleResource res = bundle.GetResource("special_chars.dummy.txt");
#ifdef US_PLATFORM_WINDOWS
  checkResourceInfo(res,
                    "/",
                    "special_chars",
                    "special_chars.dummy",
                    "txt",
                    "dummy.txt",
                    56,
                    false);
  const std::streampos ssize(54);
  const std::string fileData =
    "German Füße (feet)\nFrench garçon de café (waiter)\n";
#else
  checkResourceInfo(res,
                    "/",
                    "special_chars",
                    "special_chars.dummy",
                    "txt",
                    "dummy.txt",
                    54,
                    false);
  const std::streampos ssize(53);
  const std::string fileData =
    "German Füße (feet)\nFrench garçon de café (waiter)";
#endif

  BundleResourceStream rs(res);

  rs.seekg(0, std::ios_base::end);
  US_TEST_CONDITION(rs.tellg() == ssize, "Stream content length");
  rs.seekg(0, std::ios_base::beg);

  std::string content;
  content.reserve(res.GetSize());
  char buffer[1024];
  while (rs.read(buffer, sizeof(buffer))) {
    content.append(buffer, sizeof(buffer));
  }
  content.append(buffer, static_cast<std::size_t>(rs.gcount()));

  US_TEST_CONDITION(rs.eof(), "EOF check");
  US_TEST_CONDITION(content == fileData, "Resource content");
}

void testBinaryResource(const Bundle& bundle)
{
  BundleResource res = bundle.GetResource("/icons/cppmicroservices.png");
  checkResourceInfo(res,
                    "/icons/",
                    "cppmicroservices",
                    "cppmicroservices",
                    "png",
                    "png",
                    2424,
                    false);

  BundleResourceStream rs(res, std::ios_base::binary);
  rs.seekg(0, std::ios_base::end);
  std::streampos resLength = rs.tellg();
  rs.seekg(0);

  std::ifstream png(
    US_FRAMEWORK_SOURCE_DIR
    "/test/bundles/libRWithResources/resources/icons/cppmicroservices.png",
    std::ifstream::in | std::ifstream::binary);

  US_TEST_CONDITION_REQUIRED(png.is_open(), "Open reference file")

  png.seekg(0, std::ios_base::end);
  std::streampos pngLength = png.tellg();
  png.seekg(0);
  US_TEST_CONDITION(res.GetSize() == resLength, "Check resource size")
  US_TEST_CONDITION_REQUIRED(resLength == pngLength, "Compare sizes")

  char c1 = 0;
  char c2 = 0;
  bool isEqual = true;
  int count = 0;
  while (png.get(c1) && rs.get(c2)) {
    ++count;
    if (c1 != c2) {
      isEqual = false;
      break;
    }
  }

  US_TEST_CONDITION_REQUIRED(count == pngLength,
                             "Check if everything was read");
  US_TEST_CONDITION_REQUIRED(isEqual, "Equal binary contents");
  US_TEST_CONDITION(png.eof(), "EOF check");
}

void testCompressedResource(const Bundle& bundle)
{
  BundleResource res = bundle.GetResource("/icons/compressable.bmp");
  checkResourceInfo(res,
                    "/icons/",
                    "compressable",
                    "compressable",
                    "bmp",
                    "bmp",
                    300122,
                    false);

  BundleResourceStream rs(res, std::ios_base::binary);
  rs.seekg(0, std::ios_base::end);
  std::streampos resLength = rs.tellg();
  rs.seekg(0);

  std::ifstream bmp(
    US_FRAMEWORK_SOURCE_DIR
    "/test/bundles/libRWithResources/resources/icons/compressable.bmp",
    std::ifstream::in | std::ifstream::binary);

  US_TEST_CONDITION_REQUIRED(bmp.is_open(), "Open reference file")

  bmp.seekg(0, std::ios_base::end);
  std::streampos bmpLength = bmp.tellg();
  bmp.seekg(0);
  US_TEST_CONDITION(300122 == resLength, "Check resource size")
  US_TEST_CONDITION_REQUIRED(resLength == bmpLength, "Compare sizes")

  char c1 = 0;
  char c2 = 0;
  bool isEqual = true;
  int count = 0;
  while (bmp.get(c1) && rs.get(c2)) {
    ++count;
    if (c1 != c2) {
      isEqual = false;
      break;
    }
  }

  US_TEST_CONDITION_REQUIRED(count == bmpLength,
                             "Check if everything was read");
  US_TEST_CONDITION_REQUIRED(isEqual, "Equal binary contents");
  US_TEST_CONDITION(bmp.eof(), "EOF check");
}

struct ResourceComparator
{
  bool operator()(const BundleResource& mr1, const BundleResource& mr2) const
  {
    return mr1 < mr2;
  }
};

void testResourceTree(const Bundle& bundle)
{
  BundleResource res = bundle.GetResource("");
  US_TEST_CONDITION(res.GetResourcePath() == "/", "Check root file path")
  US_TEST_CONDITION(res.IsDir() == true, "Check type")

  std::vector<std::string> children = res.GetChildren();
  std::sort(children.begin(), children.end());
  US_TEST_CONDITION_REQUIRED(children.size() == 6, "Check child count")
  US_TEST_CONDITION(children[0] == "foo.txt", "Check child name")
  US_TEST_CONDITION(children[1] == "foo2.txt", "Check child name")
  US_TEST_CONDITION(children[2] == "icons/", "Check child name")
  US_TEST_CONDITION(children[3] == "manifest.json", "Check child name")
  US_TEST_CONDITION(children[4] == "special_chars.dummy.txt",
                    "Check child name")
  US_TEST_CONDITION(children[5] == "test.xml", "Check child name")

  US_TEST_CONDITION(
    bundle.FindResources("!$noexist=?", std::string(), "true").empty(),
    "Check not existant path");

  BundleResource readme = bundle.GetResource("/icons/readme.txt");
  US_TEST_CONDITION(readme.IsFile() && readme.GetChildren().empty(),
                    "Check file resource")

  BundleResource icons = bundle.GetResource("icons/");
  US_TEST_CONDITION(icons.IsDir() && !icons.IsFile() &&
                      !icons.GetChildren().empty(),
                    "Check directory resource")

  children = icons.GetChildren();
  US_TEST_CONDITION_REQUIRED(children.size() == 3, "Check icons child count")
  std::sort(children.begin(), children.end());
  US_TEST_CONDITION(children[0] == "compressable.bmp", "Check child name")
  US_TEST_CONDITION(children[1] == "cppmicroservices.png", "Check child name")
  US_TEST_CONDITION(children[2] == "readme.txt", "Check child name")

  ResourceComparator resourceComparator;

  // find all .txt files
  std::vector<BundleResource> nodes = bundle.FindResources("", "*.txt", false);
  std::sort(nodes.begin(), nodes.end(), resourceComparator);
  US_TEST_CONDITION_REQUIRED(nodes.size() == 3, "Found child count")
  US_TEST_CONDITION(nodes[0].GetResourcePath() == "/foo.txt",
                    "Check child name")
  US_TEST_CONDITION(nodes[1].GetResourcePath() == "/foo2.txt",
                    "Check child name")
  US_TEST_CONDITION(nodes[2].GetResourcePath() == "/special_chars.dummy.txt",
                    "Check child name")

  nodes = bundle.FindResources("", "*.txt", true);
  std::sort(nodes.begin(), nodes.end(), resourceComparator);
  US_TEST_CONDITION_REQUIRED(nodes.size() == 4, "Found child count")
  US_TEST_CONDITION(nodes[0].GetResourcePath() == "/foo.txt",
                    "Check child name")
  US_TEST_CONDITION(nodes[1].GetResourcePath() == "/foo2.txt",
                    "Check child name")
  US_TEST_CONDITION(nodes[2].GetResourcePath() == "/icons/readme.txt",
                    "Check child name")
  US_TEST_CONDITION(nodes[3].GetResourcePath() == "/special_chars.dummy.txt",
                    "Check child name")

  // find all resources
  nodes = bundle.FindResources("", "", true);
  US_TEST_CONDITION(nodes.size() == 9, "Total resource number")
  nodes = bundle.FindResources("", "**", true);
  US_TEST_CONDITION(nodes.size() == 9, "Total resource number")

  // test pattern matching
  nodes.clear();
  nodes = bundle.FindResources("/icons", "*micro*.png", false);
  US_TEST_CONDITION(nodes.size() == 1 && nodes[0].GetResourcePath() ==
                                           "/icons/cppmicroservices.png",
                    "Check file pattern matches")

  nodes.clear();
  nodes = bundle.FindResources("", "*.txt", true);
  US_TEST_CONDITION(nodes.size() == 4, "Check recursive pattern matches")
}

void testResourceOperators(const Bundle& bundle)
{
  BundleResource invalid = bundle.GetResource("invalid");
  BundleResource foo = bundle.GetResource("foo.txt");
  US_TEST_CONDITION_REQUIRED(foo.IsValid() && foo, "Check valid resource")
  BundleResource foo2(foo);
  US_TEST_CONDITION(foo == foo, "Check equality operator")
  US_TEST_CONDITION(foo == foo2, "Check copy constructor and equality operator")
  US_TEST_CONDITION(foo != invalid, "Check inequality with invalid resource")

  BundleResource xml = bundle.GetResource("/test.xml");
  US_TEST_CONDITION_REQUIRED(xml.IsValid() && xml, "Check valid resource")
  US_TEST_CONDITION(foo != xml, "Check inequality")
  US_TEST_CONDITION(foo < xml, "Check operator<")

  // check operator< by using a set
  std::set<BundleResource> resources;
  resources.insert(foo);
  resources.insert(foo);
  resources.insert(xml);
  US_TEST_CONDITION(resources.size() == 2, "Check operator< with set")

  // check hash function specialization
  std::unordered_set<BundleResource> resources2;
  resources2.insert(foo);
  resources2.insert(foo);
  resources2.insert(xml);
  US_TEST_CONDITION(resources2.size() == 2,
                    "Check operator< with unordered set")

  // check operator<<
  std::ostringstream oss;
  oss << foo;
  US_TEST_CONDITION(oss.str() == foo.GetResourcePath(), "Check operator<<")
}

void testResourceFromExecutable(const Bundle& bundle)
{
  BundleResource resource = bundle.GetResource("TestResource.txt");
  US_TEST_CONDITION_REQUIRED(resource.IsValid(),
                             "Check valid executable resource")

  std::string line;
  BundleResourceStream rs(resource);
  std::getline(rs, line);
  US_TEST_CONDITION(line == "meant to be compiled into the test driver",
                    "Check executable resource content")
}

void testResourcesFrom(const std::string& bundleName,
                       const BundleContext& context)
{
  auto bundleR = testing::InstallLib(context, bundleName);
  US_TEST_CONDITION_REQUIRED(bundleR, "Test for existing bundle")

  US_TEST_CONDITION(bundleR.GetSymbolicName() == bundleName, "Test bundle name")

  US_TEST_CONDITION(bundleR.FindResources("", "*.txt", true).size() == 2,
                    "Resource count")
}

} // end unnamed namespace

int BundleResourceTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("BundleResourceTest");

  FrameworkFactory factory;
  auto framework = factory.NewFramework();
  framework.Start();

  auto context = framework.GetBundleContext();
  assert(context);

  auto bundleR = testing::InstallLib(context, "TestBundleR");
  US_TEST_CONDITION_REQUIRED(bundleR &&
                               bundleR.GetSymbolicName() == "TestBundleR",
                             "Test for existing bundle TestBundleR")

  testInvalidResource(bundleR);

  Bundle executableBundle;
  try {
    executableBundle = testing::GetBundle("main", context);
    US_TEST_CONDITION_REQUIRED(executableBundle,
                               "Test installation of bundle main")
  } catch (const std::exception& e) {
    US_TEST_FAILED_MSG(<< "Install bundle exception: " << e.what())
  }

  testResourceFromExecutable(executableBundle);

  testResourceTree(bundleR);

  testResourceOperators(bundleR);

  testTextResource(bundleR);
  testTextResourceAsBinary(bundleR);
  testSpecialCharacters(bundleR);

  testBinaryResource(bundleR);

  testCompressedResource(bundleR);

  BundleResource foo = bundleR.GetResource("foo.txt");
  US_TEST_CONDITION(foo.IsValid() == true, "Valid resource")

  testResourcesFrom("TestBundleRL", framework.GetBundleContext());
  testResourcesFrom("TestBundleRA", framework.GetBundleContext());

  US_TEST_END()
}
