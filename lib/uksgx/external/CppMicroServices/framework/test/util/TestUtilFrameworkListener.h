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

#ifndef CPPMICROSERVICES_TESTUTILFRAMEWORKLISTENER_H
#define CPPMICROSERVICES_TESTUTILFRAMEWORKLISTENER_H

#include "cppmicroservices/FrameworkEvent.h"

#include <vector>

namespace cppmicroservices {

class TestFrameworkListener
{
public:
  TestFrameworkListener();
  virtual ~TestFrameworkListener();

  std::size_t events_received() const;
  bool CheckEvents(const std::vector<FrameworkEvent>& events);
  void frameworkEvent(const FrameworkEvent& evt);
  void throwOnFrameworkEvent(const FrameworkEvent&);

private:
  std::vector<FrameworkEvent> _events;
};
}

#endif // CPPMICROSERVICES_TESTUTILFRAMEWORKLISTENER_H
