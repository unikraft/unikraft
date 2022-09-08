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
#include "cppmicroservices/BundleImport.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkEvent.h"
#include "cppmicroservices/FrameworkFactory.h"

#include "CppMicroServicesExamplesDriverConfig.h"

#if defined(US_PLATFORM_POSIX)
#  include <dlfcn.h>
#elif defined(US_PLATFORM_WINDOWS)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
// clang-format off
// Do not re-order include directives, it would break MinGW builds.
  #include <windows.h>
  #include <strsafe.h>
// clang-format on
#else
#  error Unsupported platform
#endif

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

using namespace cppmicroservices;

#if !defined(US_BUILD_SHARED_LIBS)
static const std::string BUNDLE_PATH = US_RUNTIME_OUTPUT_DIRECTORY;
#else
#  ifdef US_PLATFORM_WINDOWS
static const std::string BUNDLE_PATH = US_RUNTIME_OUTPUT_DIRECTORY;
#  else
static const std::string BUNDLE_PATH = US_LIBRARY_OUTPUT_DIRECTORY;
#  endif
#endif // US_BUILD_SHARED_LIBS

std::vector<std::string> GetExampleBundles()
{
  std::vector<std::string> names;
  names.push_back("eventlistener");
  names.push_back("dictionaryservice");
  names.push_back("frenchdictionary");
  names.push_back("dictionaryclient");
  names.push_back("dictionaryclient2");
  names.push_back("dictionaryclient3");
  names.push_back("spellcheckservice");
  names.push_back("spellcheckclient");
  return names;
}

int main(int /*argc*/, char** /*argv*/)
{
  char cmd[256];

  std::unordered_map<std::string, long> symbolicNameToId;

  FrameworkFactory factory;
  auto framework = factory.NewFramework();

  auto get_bundle = [&framework, &symbolicNameToId](const std::string& str) {
    std::stringstream ss(str);

    long int id = -1;
    ss >> id;
    if (!ss) {
      id = -1;
      auto it = symbolicNameToId.find(str);
      if (it != symbolicNameToId.end()) {
        id = it->second;
      }
    }

    return framework.GetBundleContext().GetBundle(id);
  };

  try {
    framework.Start();

    /* install all available bundles for this example */
#if defined(US_BUILD_SHARED_LIBS)
    for (auto name : GetExampleBundles()) {
      framework.GetBundleContext().InstallBundles(
        BUNDLE_PATH + PATH_SEPARATOR + US_LIB_PREFIX + name + US_LIB_EXT);
    }
#endif

    for (auto b : framework.GetBundleContext().GetBundles()) {
      symbolicNameToId[b.GetSymbolicName()] = b.GetBundleId();
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  std::cout << "> ";
  while (std::cin.getline(cmd, sizeof(cmd))) {
    /*
     The user can stop the framework and we handle this like
     a regular shutdown command.
    */
    if (framework.GetState() != Bundle::STATE_ACTIVE) {
      break;
    }

    std::string strCmd(cmd);
    if (strCmd == "shutdown") {
      break;
    } else if (strCmd == "h") {
      std::cout << std::left << std::setw(20) << "h"
                << " This help text\n"
                << std::setw(20) << "start <id | name>"
                << " Start the bundle with id <id> or name <name>\n"
                << std::setw(20) << "stop <id | name>"
                << " Stop the bundle with id <id> or name <name>\n"
                << std::setw(20) << "status"
                << " Print status information\n"
                << std::setw(20) << "shutdown"
                << " Shut down the framework\n"
                << std::flush;
    } else if (strCmd.find("start ") != std::string::npos) {
      auto bundle = get_bundle(strCmd.substr(6));
      if (bundle) {
        try {
          /* starting an already started bundle does nothing.
             There is no harm in doing it. */
          if (bundle.GetState() == Bundle::STATE_ACTIVE) {
            std::cout << "Info: bundle already active" << std::endl;
          }
          bundle.Start();
        } catch (const std::exception& e) {
          std::cerr << e.what() << std::endl;
        }
      } else {
        std::cerr << "Error: unknown id or symbolic name" << std::endl;
      }
    } else if (strCmd.find("stop ") != std::string::npos) {
      auto bundle = get_bundle(strCmd.substr(5));
      if (bundle) {
        try {
          bundle.Stop();
          if (bundle.GetBundleId() == 0) {
            break;
          }
        } catch (const std::exception& e) {
          std::cerr << e.what() << std::endl;
        }
      } else {
        std::cerr << "Error: unknown id or symbolic name" << std::endl;
      }
    } else if (strCmd == "status") {
      std::map<long, Bundle> bundles;
      for (auto& b : framework.GetBundleContext().GetBundles()) {
        bundles.insert(std::make_pair(b.GetBundleId(), b));
      }

      std::cout << std::left;

      std::cout << "Id | " << std::setw(20) << "Symbolic Name"
                << " | " << std::setw(9) << "State" << std::endl;
      std::cout << "-----------------------------------\n";

      for (auto& bundle : bundles) {
        std::cout << std::right << std::setw(2) << bundle.first << std::left
                  << " | ";
        std::cout << std::setw(20) << bundle.second.GetSymbolicName() << " | ";
        std::cout << std::setw(9) << (bundle.second.GetState());
        std::cout << std::endl;
      }
    } else {
      std::cout << "Unknown command: " << strCmd << " (type 'h' for help)"
                << std::endl;
    }
    std::cout << "> ";
  }

  framework.Stop();
  framework.WaitForStop(std::chrono::seconds(2));

  return 0;
}

#ifndef US_BUILD_SHARED_LIBS
CPPMICROSERVICES_INITIALIZE_STATIC_BUNDLE(system_bundle)
CPPMICROSERVICES_IMPORT_BUNDLE(eventlistener)
CPPMICROSERVICES_IMPORT_BUNDLE(dictionaryservice)
CPPMICROSERVICES_IMPORT_BUNDLE(spellcheckservice)
CPPMICROSERVICES_IMPORT_BUNDLE(frenchdictionary)
CPPMICROSERVICES_IMPORT_BUNDLE(dictionaryclient)
CPPMICROSERVICES_IMPORT_BUNDLE(dictionaryclient2)
CPPMICROSERVICES_IMPORT_BUNDLE(dictionaryclient3)
CPPMICROSERVICES_IMPORT_BUNDLE(spellcheckclient)
#endif
