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

//! [Activator]

#include "IDictionaryService.h"

#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/BundleContext.h"

#include <iostream>

using namespace cppmicroservices;

namespace {

/**
 * This class implements a bundle activator that uses a dictionary service to check for
 * the proper spelling of a word by check for its existence in the dictionary.
 * This bundles uses the first service that it finds and does not monitor the
 * dynamic availability of the service (i.e., it does not listen for the arrival
 * or departure of dictionary services). When starting this bundle, the thread
 * calling the Start() method is used to read words from standard input. You can
 * stop checking words by entering an empty line, but to start checking words
 * again you must unload and then load the bundle again.
 */
class US_ABI_LOCAL Activator : public BundleActivator
{

public:
  /**
   * Implements BundleActivator::Start(). Queries for all available dictionary
   * services. If none are found it simply prints a message and returns,
   * otherwise it reads words from standard input and checks for their
   * existence from the first dictionary that it finds.
   *
   * \note It is very bad practice to use the calling thread to perform a lengthy
   *       process like this; this is only done for the purpose of the tutorial.
   *
   * @param context the bundle context for this bundle.
   */
  void Start(BundleContext context)
  {
    // Query for all service references matching any language.
    std::vector<ServiceReference<IDictionaryService>> refs =
      context.GetServiceReferences<IDictionaryService>("(Language=*)");

    if (!refs.empty()) {
      std::cout << "Enter a blank line to exit." << std::endl;

      // Loop endlessly until the user enters a blank line
      while (std::cin) {
        // Ask the user to enter a word.
        std::cout << "Enter word: ";

        std::string word;
        std::getline(std::cin, word);

        // If the user entered a blank line, then
        // exit the loop.
        if (word.empty()) {
          break;
        }

        // First, get a dictionary service and then check
        // if the word is correct.
        std::shared_ptr<IDictionaryService> dictionary =
          context.GetService<IDictionaryService>(refs.front());
        if (dictionary->CheckWord(word)) {
          std::cout << "Correct." << std::endl;
        } else {
          std::cout << "Incorrect." << std::endl;
        }
      }
    } else {
      std::cout << "Couldn't find any dictionary service..." << std::endl;
    }
  }

  /**
   * Implements BundleActivator::Stop(). Does nothing since
   * the C++ Micro Services library will automatically unget any used services.
   * @param context the context for the bundle.
   */
  void Stop(BundleContext /*context*/)
  {
    // NOTE: The service is automatically released.
  }
};
}

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(Activator)
//![Activator]
