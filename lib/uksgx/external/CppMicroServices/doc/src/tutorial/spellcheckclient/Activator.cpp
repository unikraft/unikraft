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
#include "ISpellCheckService.h"

#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/ServiceTracker.h"

#include <cstring>
#include <iostream>

using namespace cppmicroservices;

namespace {

/**
 * This class implements a bundle that uses a spell checker
 * service to check the spelling of a passage. This bundle
 * is essentially identical to Example 5, in that it uses the
 * Service Tracker to monitor the dynamic availability of the
 * spell checker service. When starting this bundle, the thread
 * calling the Start() method is used to read passages from
 * standard input. You can stop spell checking passages by
 * entering an empty line, but to start spell checking again
 * you must un-load and then load the bundle again.
**/
class US_ABI_LOCAL Activator : public BundleActivator
{

public:
  Activator()
    : m_context()
    , m_tracker(nullptr)
  {}

  /**
   * Implements BundleActivator::Start(). Creates a service
   * tracker object to monitor spell checker services. Enters
   * a spell check loop where it reads passages from standard
   * input and checks their spelling using the spell checker service.
   *
   * \note It is very bad practice to use the calling thread to perform a
   *       lengthy process like this; this is only done for the purpose of
   *       the tutorial.
   *
   * @param context the bundle context for this bundle.
   */
  void Start(BundleContext context)
  {
    m_context = context;

    // Create a service tracker to monitor spell check services.
    m_tracker = new ServiceTracker<ISpellCheckService>(m_context);
    m_tracker->Open();

    //std::cout << "Tracker count is :" << m_tracker->GetTrackingCount() << std::endl;
    std::cout << "Enter a blank line to exit." << std::endl;

    // Loop endlessly until the user enters a blank line
    while (std::cin) {
      // Ask the user to enter a passage.
      std::cout << "Enter passage: ";

      std::string passage;
      std::getline(std::cin, passage);

      // Get the selected spell check service, if available.
      std::shared_ptr<ISpellCheckService> checker = m_tracker->GetService();

      // If the user entered a blank line, then
      // exit the loop.
      if (passage.empty()) {
        break;
      }
      // If there is no spell checker, then say so.
      else if (checker == nullptr) {
        std::cout << "No spell checker available." << std::endl;
      }
      // Otherwise check passage and print misspelled words.
      else {
        std::vector<std::string> errors = checker->Check(passage);

        if (errors.empty()) {
          std::cout << "Passage is correct." << std::endl;
        } else {
          std::cout << "Incorrect word(s):" << std::endl;
          for (std::size_t i = 0; i < errors.size(); ++i) {
            std::cout << "    " << errors[i] << std::endl;
          }
        }
      }
    }

    // This automatically closes the tracker
    delete m_tracker;
  }

  /**
   * Implements BundleActivator::Stop(). Does nothing since
   * the C++ Micro Services library will automatically unget any used services.
   * @param context the context for the bundle.
   */
  void Stop(BundleContext /*context*/) {}

private:
  // Bundle context
  BundleContext m_context;

  // The service tracker
  ServiceTracker<ISpellCheckService>* m_tracker;
};
}

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(Activator)
//![Activator]
