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
#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/GlobalConfig.h"

#include <iostream>
#include <thread>

namespace cppmicroservices {

class TestBundleWaitOnOperationActivator : public BundleActivator
{
public:
  TestBundleWaitOnOperationActivator() {}
  ~TestBundleWaitOnOperationActivator() {}

  // Perform a bundle state change while one is already happening,
  // since this is called from a bundle's activator start/stop method.
  // This is designed to cause a bundle wait operation to fail.
  //
  // @param context This bundle's bundle context
  // @param prop framework property controlling which bundle operation to perform.
  // @throw std::runtime_error if the bundle operation timed out.
  void FailWaitOnOperation(BundleContext context, Any prop)
  {
    if (prop.Empty())
      return;

    if (prop.ToString() == "start") {
      context.GetBundle().Start();
    } else if (prop.ToString() == "stop") {
      context.GetBundle().Stop();
    } else if (prop.ToString() == "uninstall") {
      context.GetBundle().Uninstall();
    } else {
      // unknown option - FAIL TEST
      throw std::runtime_error("unknown framework property value used in "
                               "TestBundleWaitOnOperationActivator");
    }
  }

  void Start(BundleContext context)
  {
    FailWaitOnOperation(
      context,
      Any(context.GetProperty(
        "org.cppmicroservices.framework.testing.waitonoperation.start")));
  }

  void Stop(BundleContext context)
  {
    FailWaitOnOperation(
      context,
      Any(context.GetProperty(
        "org.cppmicroservices.framework.testing.waitonoperation.stop")));
  }
};
}

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(
  cppmicroservices::TestBundleWaitOnOperationActivator)
