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

#include "cppmicroservices/GlobalConfig.h"

US_MSVC_DISABLE_WARNING(4355)

#include "CoreBundleContext.h"

#include "cppmicroservices/BundleInitialization.h"
#include "cppmicroservices/Constants.h"

#include "cppmicroservices/util/FileSystem.h"
#include "cppmicroservices/util/String.h"

#include "BundleStorageMemory.h"
#include "BundleThread.h"
#include "BundleUtils.h"
#include "FrameworkPrivate.h"

#include <iomanip>

CPPMICROSERVICES_INITIALIZE_BUNDLE

namespace cppmicroservices {

std::atomic<int> CoreBundleContext::globalId{ 0 };

std::unordered_map<std::string, Any> InitProperties(
  std::unordered_map<std::string, Any> configuration)
{
  // Framework internal diagnostic logging is off by default
  configuration.insert(std::make_pair(Constants::FRAMEWORK_LOG, Any(false)));

  // Framework::PROP_THREADING_SUPPORT is a read-only property whose value is based off of a compile-time switch.
  // Run-time modification of the property should be ignored as it is irrelevant.
#ifdef US_ENABLE_THREADING_SUPPORT
  configuration[Constants::FRAMEWORK_THREADING_SUPPORT] = std::string("multi");
#else
  configuration[Constants::FRAMEWORK_THREADING_SUPPORT] = std::string("single");
#endif

  if (configuration.find(Constants::FRAMEWORK_WORKING_DIR) ==
      configuration.end()) {
    configuration.insert(std::make_pair(Constants::FRAMEWORK_WORKING_DIR,
                                        util::GetCurrentWorkingDirectory()));
  }

  configuration.insert(
    std::make_pair(Constants::FRAMEWORK_STORAGE, Any(FWDIR_DEFAULT)));

  configuration[Constants::FRAMEWORK_VERSION] =
    std::string(CppMicroServices_VERSION_STR);
  configuration[Constants::FRAMEWORK_VENDOR] = std::string("CppMicroServices");

  return configuration;
}

CoreBundleContext::CoreBundleContext(
  const std::unordered_map<std::string, Any>& props,
  std::ostream* logger)
  : id(globalId++)
  , frameworkProperties(InitProperties(props))
  , workingDir(ref_any_cast<std::string>(
      frameworkProperties.at(Constants::FRAMEWORK_WORKING_DIR)))
  , listeners(this)
  , services(this)
  , serviceHooks(this)
  , bundleHooks(this)
  , bundleRegistry(this)
  , firstInit(true)
  , initCount(0)
{
  bool enableDiagLog =
    any_cast<bool>(frameworkProperties.at(Constants::FRAMEWORK_LOG));
  std::ostream* diagnosticLogger = (logger) ? logger : &std::clog;
  sink = std::make_shared<detail::LogSink>(diagnosticLogger, enableDiagLog);
  systemBundle = std::shared_ptr<FrameworkPrivate>(new FrameworkPrivate(this));
  DIAG_LOG(*sink) << "created";
}

CoreBundleContext::~CoreBundleContext() {}

std::shared_ptr<CoreBundleContext> CoreBundleContext::shared_from_this() const
{
  return self.Lock(), self.v.lock();
}

void CoreBundleContext::SetThis(const std::shared_ptr<CoreBundleContext>& self)
{
  this->self.Lock(), this->self.v = self;
}

void CoreBundleContext::Init()
{
  DIAG_LOG(*sink) << "initializing";
  initCount++;

  auto storageCleanProp =
    frameworkProperties.find(Constants::FRAMEWORK_STORAGE_CLEAN);
  if (firstInit && storageCleanProp != frameworkProperties.end() &&
      storageCleanProp->second ==
        Constants::FRAMEWORK_STORAGE_CLEAN_ONFIRSTINIT) {
    // DeleteFWDir();
    firstInit = false;
  }

  // We use a "pseudo" random UUID.
  const std::string sid_base = "04f4f884-31bb-45c0-b176-";
  std::stringstream ss;
  ss << sid_base << std::setfill('0') << std::setw(8) << std::hex
     << static_cast<int32_t>(id * 65536 + initCount);

  frameworkProperties[Constants::FRAMEWORK_UUID] = ss.str();

  // $TODO we only support non-persistent (main memory) storage yet
  storage.reset(new BundleStorageMemory());
  //  if (frameworkProperties[FWProps::READ_ONLY_PROP] == true)
  //  {
  //    dataStorage.clear();
  //  }
  //  else
  //  {
  //    dataStorage = GetFileStorage(this, "data");
  //  }
  try {
    dataStorage = GetPersistentStoragePath(this, "data", /*create=*/false);
  } catch (const std::exception& e) {
    DIAG_LOG(*sink) << "Ignored runtime exception with message'" << e.what()
                    << "' from the GetPersistentStoragePath function.\n";
  }

  systemBundle->InitSystemBundle();
  _us_set_bundle_context_instance_system_bundle(
    systemBundle->bundleContext.Load().get());

  bundleRegistry.Init();

  serviceHooks.Open();
  //resolverHooks.Open();

  bundleRegistry.Load();

  std::string execPath;
  try {
    execPath = util::GetExecutablePath();
  } catch (const std::exception& e) {
    DIAG_LOG(*sink) << e.what();
    // Let the exception propagate all the way up to the
    // call site of Framework::Init().
    throw;
  }

  if (IsBundleFile(execPath) && bundleRegistry.GetBundles(execPath).empty()) {
    // Auto-install all embedded bundles inside the executable.
    // Same here: If an embedded bundle cannot be installed,
    // an exception is thrown and we will let it propagate all
    // the way up to the call site of Framework::Init().
    bundleRegistry.Install(execPath, systemBundle.get());
  }

  DIAG_LOG(*sink) << "inited\nInstalled bundles: ";
  for (auto b : bundleRegistry.GetBundles()) {
    DIAG_LOG(*sink) << " #" << b->id << " " << b->symbolicName << ":"
                    << b->version << " location:" << b->location;
  }
}

void CoreBundleContext::Uninit0()
{
  DIAG_LOG(*sink) << "uninit";
  serviceHooks.Close();
  systemBundle->UninitSystemBundle();
}

void CoreBundleContext::Uninit1()
{
  bundleRegistry.Clear();
  services.Clear();
  listeners.Clear();
  resolver.Clear();

  // Do not hold the bundleThreads lock while calling
  // BundleThread::Quit() because this will join the bundle
  // thread which may need to acquire the bundleThreads lock
  // itself.
  //
  // At this point, all bundles are stopped and all
  // bundle context instances invalidated. So no new bundle
  // threads can be created and it is sufficient to get
  // the current list once and not check for new bundle
  // threads again.
  std::list<std::shared_ptr<BundleThread>> threads;
  bundleThreads.Lock(), std::swap(threads, bundleThreads.value);

  while (!threads.empty()) {
    // Quit the bundle thread. This joins the bundle thread
    // with this thread and puts it into the zombies list.
    threads.front()->Quit();
    threads.pop_front();
  }

  // Clear up all bundle threads that have been quit. At this
  // point, we do not need to lock the bundleTheads structure
  // any more.
  bundleThreads.zombies.clear();

  dataStorage.clear();
  storage->Close();
}

std::string CoreBundleContext::GetDataStorage(long id) const
{
  if (!dataStorage.empty()) {
    return dataStorage + util::DIR_SEP + util::ToString(id);
  }
  return std::string();
}
}
