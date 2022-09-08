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
#include "cppmicroservices/Constants.h"
#include "cppmicroservices/ServiceEvent.h"

#include <iostream>

using namespace cppmicroservices;

namespace {

/**
 * This class implements a bundle activator that uses a dictionary service to check for
 * the proper spelling of a word by checking for its existence in the
 * dictionary. This bundle is more complex than the bundle in Example 3 because
 * it monitors the dynamic availability of the dictionary services. In other
 * words, if the service it is using departs, then it stops using it gracefully,
 * or if it needs a service and one arrives, then it starts using it
 * automatically. As before, the bundle uses the first service that it finds and
 * uses the calling thread of the Start() method to read words from standard
 * input. You can stop checking words by entering an empty line, but to start
 * checking words again you must unload and then load the bundle again.
 */
class US_ABI_LOCAL Activator : public BundleActivator
{

public:
  Activator()
    : m_context()
    , m_dictionary(nullptr)
  {}

  /**
   * Implements BundleActivator::Start(). Adds itself as a listener for service
   * events, then queries for available dictionary services. If any
   * dictionaries are found it gets a reference to the first one available and
   * then starts its "word checking loop". If no dictionaries are found, then
   * it just goes directly into its "word checking loop", but it will not be
   * able to check any words until a dictionary service arrives; any arriving
   * dictionary service will be automatically used by the client if a
   * dictionary is not already in use. Once it has dictionary, it reads words
   * from standard input and checks for their existence in the dictionary that
   * it is using.
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

    {
      // Use your favorite thread library to synchronize member
      // variable access within this scope while registering
      // the service listener and performing our initial
      // dictionary service lookup since we
      // don't want to receive service events when looking up the
      // dictionary service, if one exists.
      // MutexLocker lock(&m_mutex);

      // Listen for events pertaining to dictionary services.
      m_context.AddServiceListener(
        std::bind(&Activator::ServiceChanged, this, std::placeholders::_1),
        std::string("(&(") + Constants::OBJECTCLASS + "=" +
          us_service_interface_iid<IDictionaryService>() + ")" +
          "(Language=*))");

      // Query for any service references matching any language.
      std::vector<ServiceReference<IDictionaryService>> refs =
        context.GetServiceReferences<IDictionaryService>("(Language=*)");

      // If we found any dictionary services, then just get
      // a reference to the first one so we can use it.
      if (!refs.empty()) {
        m_ref = refs.front();
        m_dictionary = m_context.GetService(m_ref);
      }
    }

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
      // If there is no dictionary, then say so.
      else if (m_dictionary == nullptr) {
        std::cout << "No dictionary available." << std::endl;
      }
      // Otherwise print whether the word is correct or not.
      else if (m_dictionary->CheckWord(word)) {
        std::cout << "Correct." << std::endl;
      } else {
        std::cout << "Incorrect." << std::endl;
      }
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

  /**
   * Implements ServiceListener.serviceChanged(). Checks to see if the service
   * we are using is leaving or tries to get a service if we need one.
   *
   * @param event the fired service event.
   */
  void ServiceChanged(const ServiceEvent& event)
  {
    // Use your favorite thread library to synchronize this
    // method with the Start() method.
    // MutexLocker lock(&m_mutex);

    // If a dictionary service was registered, see if we
    // need one. If so, get a reference to it.
    if (event.GetType() == ServiceEvent::SERVICE_REGISTERED) {
      if (!m_ref) {
        // Get a reference to the service object.
        m_ref = event.GetServiceReference();
        m_dictionary = m_context.GetService(m_ref);
      }
    }
    // If a dictionary service was unregistered, see if it
    // was the one we were using. If so, unget the service
    // and try to query to get another one.
    else if (event.GetType() == ServiceEvent::SERVICE_UNREGISTERING) {
      if (event.GetServiceReference() == m_ref) {
        // Unget service object and null references.
        m_ref = nullptr;
        m_dictionary.reset();

        // Query to see if we can get another service.
        std::vector<ServiceReference<IDictionaryService>> refs;
        try {
          refs =
            m_context.GetServiceReferences<IDictionaryService>("(Language=*)");
        } catch (const std::invalid_argument& e) {
          std::cout << e.what() << std::endl;
        }

        if (!refs.empty()) {
          // Get a reference to the first service object.
          m_ref = refs.front();
          m_dictionary = m_context.GetService(m_ref);
        }
      }
    }
  }

private:
  // Bundle context
  BundleContext m_context;

  // The service reference being used
  ServiceReference<IDictionaryService> m_ref;

  // The service object being used
  std::shared_ptr<IDictionaryService> m_dictionary;
};
}

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(Activator)
//![Activator]
