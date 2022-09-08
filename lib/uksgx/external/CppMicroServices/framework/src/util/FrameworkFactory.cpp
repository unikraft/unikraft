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

#include "cppmicroservices/FrameworkFactory.h"

#include "cppmicroservices/Framework.h"

#include "FrameworkPrivate.h"

namespace cppmicroservices {

// A helper class to shut down the framework when all external
// references (Bundle, BundleContext, etc.) are destroyed. It
// also manages the lifetime of the CoreBundleContext instance.
struct CoreBundleContextHolder
{
  CoreBundleContextHolder(std::unique_ptr<CoreBundleContext> ctx)
    : ctx(std::move(ctx))
  {}

  ~CoreBundleContextHolder()
  {
    auto const state = ctx->systemBundle->state.load();
    if (((Bundle::STATE_STARTING | Bundle::STATE_ACTIVE) & state) == 0) {
      // Call WaitForStop in case someone did call Framework::Stop()
      // but didn't wait for it. This joins with a potentially
      // running framework shut down thread.
      ctx->systemBundle->WaitForStop(std::chrono::milliseconds::zero());
      return;
    }

    // Create a new CoreBundleContext holder, in case some event listener
    // gets the system bundle and starts it again.
    auto fwCtx = ctx.get();
    std::shared_ptr<CoreBundleContext> holder(
      std::make_shared<CoreBundleContextHolder>(std::move(ctx)), fwCtx);
    holder->SetThis(holder);
    holder->systemBundle->Shutdown(false);
    holder->systemBundle->WaitForStop(std::chrono::milliseconds::zero());
  }

  std::unique_ptr<CoreBundleContext> ctx;
};

Framework FrameworkFactory::NewFramework(
  const FrameworkConfiguration& configuration,
  std::ostream* logger)
{
  std::unique_ptr<CoreBundleContext> ctx(
    new CoreBundleContext(configuration, logger));
  auto fwCtx = ctx.get();
  std::shared_ptr<CoreBundleContext> holder(
    std::make_shared<CoreBundleContextHolder>(std::move(ctx)), fwCtx);
  holder->SetThis(holder);
  return Framework(holder->systemBundle);
}

Framework FrameworkFactory::NewFramework()
{
  return NewFramework(FrameworkConfiguration());
}

Framework FrameworkFactory::NewFramework(
  const std::map<std::string, Any>& configuration,
  std::ostream* logger)
{
  FrameworkConfiguration fwConfig;
  for (auto& c : configuration) {
    fwConfig.insert(c);
  }

  return NewFramework(fwConfig, logger);
}
}
