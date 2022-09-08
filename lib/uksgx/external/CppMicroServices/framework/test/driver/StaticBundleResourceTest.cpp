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

#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/BundleResource.h"
#include "cppmicroservices/BundleResourceStream.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkFactory.h"
#include "cppmicroservices/GetBundleContext.h"

#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

#include <cassert>
#include <set>

using namespace cppmicroservices;

namespace {

std::string GetResourceContent(const BundleResource& resource)
{
  std::string line;
  BundleResourceStream rs(resource);
  std::getline(rs, line);
  return line;
}

struct ResourceComparator
{
  bool operator()(const BundleResource& mr1, const BundleResource& mr2) const
  {
    return mr1 < mr2;
  }
};

void testResourceOperators(const Bundle& bundle)
{
  std::vector<BundleResource> resources =
    bundle.FindResources("", "res.txt", false);
  US_TEST_CONDITION_REQUIRED(resources.size() == 1, "Check resource count")
}

void testResourcesWithStaticImport(const Framework& framework,
                                   const Bundle& bundle)
{
  BundleResource resource = bundle.GetResource("res.txt");
  US_TEST_CONDITION_REQUIRED(resource.IsValid(), "Check valid res.txt resource")
  std::string line = GetResourceContent(resource);
  US_TEST_CONDITION(line == "dynamic resource",
                    "Check dynamic resource content")

  resource = bundle.GetResource("dynamic.txt");
  US_TEST_CONDITION_REQUIRED(resource.IsValid(),
                             "Check valid dynamic.txt resource")
  line = GetResourceContent(resource);
  US_TEST_CONDITION(line == "dynamic", "Check dynamic resource content")

  resource = bundle.GetResource("static.txt");
  US_TEST_CONDITION_REQUIRED(!resource.IsValid(),
                             "Check in-valid static.txt resource")

  auto importedByBBundle =
    testing::GetBundle("TestBundleImportedByB", framework.GetBundleContext());
  US_TEST_CONDITION_REQUIRED(importedByBBundle, "Check valid static bundle")
  resource = importedByBBundle.GetResource("static.txt");
  US_TEST_CONDITION_REQUIRED(resource.IsValid(),
                             "Check valid static.txt resource")
  line = GetResourceContent(resource);
  US_TEST_CONDITION(line == "static", "Check static resource content")

  std::vector<BundleResource> resources =
    bundle.FindResources("", "*.txt", false);
  std::stable_sort(resources.begin(), resources.end(), ResourceComparator());
  std::vector<BundleResource> importedResources =
    importedByBBundle.FindResources("", "*.txt", false);
  std::stable_sort(
    importedResources.begin(), importedResources.end(), ResourceComparator());

  US_TEST_CONDITION(resources.size() == 2, "Check resource count")
  US_TEST_CONDITION(importedResources.size() == 2, "Check resource count")
  line = GetResourceContent(resources[0]);
  US_TEST_CONDITION(line == "dynamic", "Check dynamic.txt resource content")
  line = GetResourceContent(resources[1]);
  US_TEST_CONDITION(line == "dynamic resource",
                    "Check res.txt (from importing bundle) resource content")
  line = GetResourceContent(importedResources[0]);
  US_TEST_CONDITION(line == "static resource",
                    "Check res.txt (from imported bundle) resource content")
  line = GetResourceContent(importedResources[1]);
  US_TEST_CONDITION(line == "static",
                    "Check static.txt (from importing bundle) resource content")
}

} // end unnamed namespace

int StaticBundleResourceTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("StaticBundleResourceTest");

  FrameworkFactory factory;
  auto framework = factory.NewFramework();
  framework.Start();

  assert(framework.GetBundleContext());

  auto bundle =
    testing::InstallLib(framework.GetBundleContext(), "TestBundleB");
  US_TEST_CONDITION_REQUIRED(bundle, "Test for existing bundle TestBundleB")
  US_TEST_CONDITION(bundle.GetSymbolicName() == "TestBundleB",
                    "Test bundle name")

  auto importedBundle =
    testing::GetBundle("TestBundleImportedByB", framework.GetBundleContext());
  US_TEST_CONDITION_REQUIRED(
    importedBundle, "Test installation of bundle TestBundleImportedByB")

  testResourceOperators(bundle);
  testResourcesWithStaticImport(framework, bundle);

  BundleResource resource = importedBundle.GetResource("static.txt");
  bundle.Start();
  US_TEST_CONDITION_REQUIRED(resource.IsValid(),
                             "Check valid static.txt resource")

  bundle.Stop();
  US_TEST_CONDITION_REQUIRED(resource.IsValid() == true,
                             "Check still valid static.txt resource")

  US_TEST_END()
}
