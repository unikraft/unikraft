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

#include "TestBundleBA_01Service.h"

#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/GlobalConfig.h"

#include <iostream>

namespace cppmicroservices {

struct TestBundleBA_01 : public TestBundleBA_01Service
{

  TestBundleBA_01() {}
  virtual ~TestBundleBA_01() {}
};

class TestBundleAActivator : public BundleActivator
{
public:
  TestBundleAActivator() {}
  ~TestBundleAActivator() {}

  void Start(BundleContext context)
  {
    std::cout << "Registering TestBundleAService";
    sr = context.RegisterService<TestBundleBA_01Service>(
      std::make_shared<TestBundleBA_01>());
  }

  void Stop(BundleContext) { sr.Unregister(); }

private:
  ServiceRegistration<TestBundleBA_01Service> sr;
};
}

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(cppmicroservices::TestBundleAActivator)
