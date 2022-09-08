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
#include "cppmicroservices/ServiceTracker.h"

using namespace cppmicroservices;

namespace {

/**
 * This class implements a bundle activator that uses a dictionary
 * service to check for the proper spelling of a word by
 * checking for its existence in the dictionary. This bundle
 * uses a service tracker to dynamically monitor the availability
 * of a dictionary service, instead of providing a custom service
 * listener as in Example 4. The bundle uses the service returned
 * by the service tracker, which is selected based on a ranking
 * algorithm defined by the C++ Micro Services library.
 * Again, the calling thread of the Start() method is used to read
 * words from standard input, checking its existence in the dictionary.
 * You can stop checking words by entering an empty line, but
 * to start checking words again you must unload and then load
 * the bundle again.
 */
class US_ABI_LOCAL Activator : public BundleActivator
{

public:
  Activator()
    : m_context()
    , m_tracker(nullptr)
  {}

  /**
   * Implements BundleActivator::Start(). Creates a service
   * tracker to monitor dictionary services and starts its "word
   * checking loop". It will not be able to check any words until
   * the service tracker finds a dictionary service; any discovered
   * dictionary service will be automatically used by the client.
   * It reads words from standard input and checks for their
   * existence in the discovered dictionary.
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

    // Create a service tracker to monitor dictionary services.
    m_tracker = new ServiceTracker<IDictionaryService>(
      m_context,
      LDAPFilter(std::string("(&(") + Constants::OBJECTCLASS + "=" +
                 us_service_interface_iid<IDictionaryService>() + ")" +
                 "(Language=*))"));
    m_tracker->Open();

    std::cout << "Enter a blank line to exit." << std::endl;

    // Loop endlessly until the user enters a blank line
    while (std::cin) {
      // Ask the user to enter a word.
      std::cout << "Enter word: ";

      std::string word;
      std::getline(std::cin, word);

      // Get the selected dictionary, if available.
      std::shared_ptr<IDictionaryService> dictionary = m_tracker->GetService();

      // If the user entered a blank line, then
      // exit the loop.
      if (word.empty()) {
        break;
      }
      // If there is no dictionary, then say so.
      else if (!dictionary) {
        std::cout << "No dictionary available." << std::endl;
      }
      // Otherwise print whether the word is correct or not.
      else if (dictionary->CheckWord(word)) {
        std::cout << "Correct." << std::endl;
      } else {
        std::cout << "Incorrect." << std::endl;
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
  ServiceTracker<IDictionaryService>* m_tracker;
};
}

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(Activator)
//![Activator]
