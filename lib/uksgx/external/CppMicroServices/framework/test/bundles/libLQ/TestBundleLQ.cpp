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

#include "TestBundleLQService.h"

#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/GlobalConfig.h"

#include <iostream>

namespace cppmicroservices {

struct TestBundleLQ : public TestBundleLQService
{
  TestBundleLQ() {}
  virtual ~TestBundleLQ() {}
};

class TestBundleLQActivator : public BundleActivator
{
public:
  TestBundleLQActivator() {}
  ~TestBundleLQActivator() {}

  void Start(BundleContext context)
  {
    s = std::make_shared<TestBundleLQ>();

    ServiceProperties props;
    props["service.testproperty"] = std::string("YES");
    std::cout << "Registering TestBundleLQService";
    sr = context.RegisterService<TestBundleLQService>(s, props);
  }

  void Stop(BundleContext) { sr.Unregister(); }

private:
  std::shared_ptr<TestBundleLQ> s;
  ServiceRegistration<TestBundleLQService> sr;
};
}

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(
  cppmicroservices::TestBundleLQActivator)
