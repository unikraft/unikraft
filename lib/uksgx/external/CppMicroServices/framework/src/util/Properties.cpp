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

#include "Properties.h"

#include <limits>
#include <stdexcept>
#ifdef US_PLATFORM_WINDOWS
#  include <string.h>
#  define ci_compare strnicmp
#else
#  include <strings.h>
#  define ci_compare strncasecmp
#endif

namespace cppmicroservices {

const Any Properties::emptyAny;

Properties::Properties(const AnyMap& p)
{
  if (p.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    throw std::runtime_error("Properties contain too many keys");
  }

  keys.reserve(p.size());
  values.reserve(p.size());

  for (auto& iter : p) {
    if (Find_unlocked(iter.first) > -1) {
      std::string msg("Properties contain case variants of the key: ");
      msg += iter.first;
      throw std::runtime_error(msg.c_str());
    }
    keys.push_back(iter.first);
    values.push_back(iter.second);
  }
}

Properties::Properties(Properties&& o)
  : keys(std::move(o.keys))
  , values(std::move(o.values))
{}

Properties& Properties::operator=(Properties&& o)
{
  keys = std::move(o.keys);
  values = std::move(o.values);
  return *this;
}

Any Properties::Value_unlocked(const std::string& key) const
{
  int i = Find_unlocked(key);
  if (i < 0) {
    return emptyAny;
  }
  return values[i];
}

Any Properties::Value_unlocked(int index) const
{
  if (index < 0 || static_cast<std::size_t>(index) >= values.size()) {
    return emptyAny;
  }
  return values[static_cast<std::size_t>(index)];
}

int Properties::Find_unlocked(const std::string& key) const
{
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (key.size() == keys[i].size() &&
        ci_compare(key.c_str(), keys[i].c_str(), key.size()) == 0) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int Properties::FindCaseSensitive_unlocked(const std::string& key) const
{
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (key == keys[i]) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

std::vector<std::string> Properties::Keys_unlocked() const
{
  return keys;
}

void Properties::Clear_unlocked()
{
  keys.clear();
  values.clear();
}
}
