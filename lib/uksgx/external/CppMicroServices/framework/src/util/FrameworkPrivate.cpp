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

#include "FrameworkPrivate.h"

#include "cppmicroservices/BundleEvent.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkEvent.h"

#include "BundleContextPrivate.h"
#include "BundleStorage.h"

#include <chrono>

namespace cppmicroservices {

FrameworkPrivate::FrameworkPrivate(CoreBundleContext* fwCtx)
  : BundlePrivate(fwCtx)
{
  // default the internal framework event to what should be
  // returned if a client calls WaitForStop while this
  // framework is not in an active, starting or stopping state.
  stopEvent = FrameworkEventInternal{
    true, FrameworkEvent::Type::FRAMEWORK_ERROR, std::string(), nullptr
  };
}

void FrameworkPrivate::DoInit()
{
  state = Bundle::STATE_STARTING;
  coreCtx->Init();
}

void FrameworkPrivate::Init()
{
  auto l = Lock();
  WaitOnOperation(*this, l, "Framework::Init", true);

  switch (static_cast<Bundle::State>(state.load())) {
    case Bundle::STATE_INSTALLED:
    case Bundle::STATE_RESOLVED:
      break;
    case Bundle::STATE_STARTING:
    case Bundle::STATE_ACTIVE:
      return;
    default:
      std::stringstream ss;
      ss << state;
      throw std::logic_error("INTERNAL ERROR, Illegal state, " + ss.str());
  }
  this->DoInit();
}

void FrameworkPrivate::InitSystemBundle()
{
  bundleContext.Store(std::make_shared<BundleContextPrivate>(this));

  // TODO Capabilities
  /*
      std::string sp;
      sp.append(coreCtx->frameworkProperties.getProperty(Constants::FRAMEWORK_SYSTEMCAPABILITIES));
      // Add in extra system capabilities
      std::string epc = coreCtx->frameworkProperties.getProperty(Constants::FRAMEWORK_SYSTEMCAPABILITIES_EXTRA);
      if (!epc.empty())
      {
        if (!sp.empty())
        {
          sp.append(',');
        }
        sp.append(epc);
      }
      provideCapabilityString = sp;
      */

  // TODO Wiring
  /*
      BundleGeneration* gen = new BundleGeneration(this, exportPackageString,
                                                        provideCapabilityString);
      generations.add(gen);
      gen->SetWired();
      fwWiring = new FrameworkWiringImpl(coreCtx);
      */

  timeStamp = std::chrono::steady_clock::now();
}

void FrameworkPrivate::UninitSystemBundle()
{
  auto bc = bundleContext.Exchange(std::shared_ptr<BundleContextPrivate>());
  if (bc)
    bc->Invalidate();
}

FrameworkEvent FrameworkPrivate::WaitForStop(
  const std::chrono::milliseconds& timeout)
{
  auto l = Lock();
  // Already stopped?
  if (((Bundle::STATE_INSTALLED | Bundle::STATE_RESOLVED) & state) == 0) {
    stopEvent = FrameworkEventInternal{ false,
                                        FrameworkEvent::Type::FRAMEWORK_ERROR,
                                        std::string(),
                                        std::exception_ptr() };
    if (timeout == std::chrono::milliseconds::zero()) {
      Wait(l, [&] { return stopEvent.valid; });
    } else {
      WaitFor(l, timeout, [&] { return stopEvent.valid; });
    }
    if (!stopEvent.valid) {
      return FrameworkEvent(FrameworkEvent::Type::FRAMEWORK_WAIT_TIMEDOUT,
                            MakeBundle(this->shared_from_this()),
                            std::string(),
                            std::exception_ptr());
    }
  } else if (!stopEvent.valid) {
    // Return this if stop or update have not been called and framework is
    // stopped.
    stopEvent = FrameworkEventInternal{ true,
                                        FrameworkEvent::Type::FRAMEWORK_STOPPED,
                                        std::string(),
                                        std::exception_ptr() };
  }
  if (shutdownThread.joinable())
    shutdownThread.join();
  return FrameworkEvent(stopEvent.type,
                        MakeBundle(this->shared_from_this()),
                        stopEvent.msg,
                        stopEvent.excPtr);
}

void FrameworkPrivate::Shutdown(bool restart)
{
  auto l = Lock();
  US_UNUSED(l);
  bool wasActive = false;
  switch (static_cast<Bundle::State>(state.load())) {
    case Bundle::STATE_INSTALLED:
    case Bundle::STATE_RESOLVED:
      ShutdownDone_unlocked(false);
      break;
    case Bundle::STATE_ACTIVE:
      wasActive = true;
      // Fall through
    case Bundle::STATE_STARTING: {
      const bool wa = wasActive;
#ifdef US_ENABLE_THREADING_SUPPORT
      if (!shutdownThread.joinable()) {
        shutdownThread = std::thread(
          std::bind(&FrameworkPrivate::Shutdown0, this, restart, wa));
      }
#else
      Shutdown0(restart, wa);
#endif
      break;
    }
    case Bundle::STATE_UNINSTALLED:
    case Bundle::STATE_STOPPING:
      // Shutdown already inprogress
      break;
  }
}

void FrameworkPrivate::Start(uint32_t)
{
  std::vector<long> bundlesToStart;
  {
    auto l = Lock();
    WaitOnOperation(*this, l, "Framework::Start", true);

    switch (state.load()) {
      case Bundle::STATE_INSTALLED:
      case Bundle::STATE_RESOLVED:
        DoInit();
        // Fall through
      case Bundle::STATE_STARTING:
        operation = BundlePrivate::OP_ACTIVATING;
        break;
      case Bundle::STATE_ACTIVE:
        return;
      default:
        std::stringstream ss;
        ss << state;
        throw std::runtime_error("INTERNAL ERROR, Illegal state, " + ss.str());
    }
    bundlesToStart = coreCtx->storage->GetStartOnLaunchBundles();
  }

  // Start bundles according to their autostart setting.
  for (auto i : bundlesToStart) {
    auto b = coreCtx->bundleRegistry.GetBundle(i);
    try {
      const int32_t autostartSetting = b->barchive->GetAutostartSetting();
      // Launch must not change the autostart setting of a bundle
      int option = Bundle::START_TRANSIENT;
      if (Bundle::START_ACTIVATION_POLICY == autostartSetting) {
        // Transient start according to the bundles activation policy.
        option |= Bundle::START_ACTIVATION_POLICY;
      }
      b->Start(option);
    } catch (...) {
      coreCtx->listeners.SendFrameworkEvent(
        FrameworkEvent(FrameworkEvent::Type::FRAMEWORK_ERROR,
                       MakeBundle(b->shared_from_this()),
                       std::string(),
                       std::current_exception()));
    }
  }

  {
    auto l = Lock();
    US_UNUSED(l);
    state = Bundle::STATE_ACTIVE;
    operation = BundlePrivate::OP_IDLE;
  }
  NotifyAll();
  coreCtx->listeners.SendFrameworkEvent(
    FrameworkEvent(FrameworkEvent::Type::FRAMEWORK_STARTED,
                   MakeBundle(shared_from_this()),
                   std::string()));
}

void FrameworkPrivate::Stop(uint32_t)
{
  Shutdown(false);
}

void FrameworkPrivate::Uninstall()
{
  throw std::runtime_error("Cannot uninstall a system bundle.");
}

std::string FrameworkPrivate::GetLocation() const
{
  // OSGi Core release 6, section 4.6:
  //  The system bundle GetLocation method returns the string: "System Bundle"
  return std::string("System Bundle");
}

AnyMap FrameworkPrivate::GetHeaders() const
{
  AnyMap headers(AnyMap::UNORDERED_MAP_CASEINSENSITIVE_KEYS);
  headers[Constants::BUNDLE_SYMBOLICNAME] = symbolicName;
  headers[Constants::BUNDLE_NAME] = location;
  headers[Constants::BUNDLE_VERSION] = version.ToString();
  headers[Constants::BUNDLE_MANIFESTVERSION] = std::string("2");
  //headers.put("Bundle-Icon", "icon.png;size=32,icon64.png;size=64");
  headers[Constants::BUNDLE_VENDOR] = std::string("C++ Micro Services");
  headers[Constants::BUNDLE_DESCRIPTION] =
    std::string("C++ Micro Services System Bundle");
  //headers.put(Constants::PROVIDE_CAPABILITY, provideCapabilityString);
  return headers;
}

void FrameworkPrivate::Shutdown0(bool restart, bool wasActive)
{
  try {
    {
      auto l = Lock();
      WaitOnOperation(*this,
                      l,
                      std::string("Framework::") +
                        (restart ? "Update" : "Stop"),
                      true);
      operation = OP_DEACTIVATING;
      state = Bundle::STATE_STOPPING;
    }
    coreCtx->listeners.BundleChanged(BundleEvent(
      BundleEvent::BUNDLE_STOPPING, MakeBundle(this->shared_from_this())));
    if (wasActive) {
      StopAllBundles();
    }
    coreCtx->Uninit0();
    {
      auto l = Lock();
      US_UNUSED(l);
      coreCtx->Uninit1();
      ShutdownDone_unlocked(restart);
    }
    if (restart) {
      if (wasActive) {
        Start(0);
      } else {
        Init();
      }
    }
  } catch (...) {
    auto l = Lock();
    US_UNUSED(l);
    SystemShuttingdownDone_unlocked(
      FrameworkEventInternal{ true,
                              FrameworkEvent::Type::FRAMEWORK_ERROR,
                              std::string(),
                              std::current_exception() });
  }
}

void FrameworkPrivate::ShutdownDone_unlocked(bool restart)
{
  auto t = restart ? FrameworkEvent::FRAMEWORK_STOPPED_UPDATE
                   : FrameworkEvent::FRAMEWORK_STOPPED;
  SystemShuttingdownDone_unlocked(
    FrameworkEventInternal{ true, t, std::string(), std::exception_ptr() });
}

void FrameworkPrivate::StopAllBundles()
{
  // Stop all active bundles, in reverse bundle ID order
  auto activeBundles = coreCtx->bundleRegistry.GetActiveBundles();
  for (auto iter = activeBundles.rbegin(); iter != activeBundles.rend();
       ++iter) {
    auto b = *iter;
    try {
      if (((Bundle::STATE_ACTIVE | Bundle::STATE_STARTING) & b->state) != 0) {
        // Stop bundle without changing its autostart setting.
        b->Stop(Bundle::StopOptions::STOP_TRANSIENT);
      }
    } catch (...) {
      coreCtx->listeners.SendFrameworkEvent(
        FrameworkEvent(FrameworkEvent(FrameworkEvent::Type::FRAMEWORK_ERROR,
                                      MakeBundle(b),
                                      std::string(),
                                      std::current_exception())));
    }
  }

  auto allBundles = coreCtx->bundleRegistry.GetBundles();

  // Set state to BUNDLE_INSTALLED
  for (auto b : allBundles) {
    if (b->id != 0) {
      auto l = coreCtx->resolver.Lock();
      b->SetStateInstalled(false, l);
    }
  }
}

void FrameworkPrivate::SystemShuttingdownDone_unlocked(
  const FrameworkEventInternal& fe)
{
  if (state != Bundle::STATE_INSTALLED) {
    state = Bundle::STATE_RESOLVED;
    operation = OP_IDLE;
    NotifyAll();
  }
  stopEvent = fe;
}
}
