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

#include "cppmicroservices/BundleVersion.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace cppmicroservices {

const char BundleVersion::SEPARATOR = '.';

bool IsInvalidQualifier(char c)
{
  return !(std::isalnum(c) || c == '_' || c == '-');
}

BundleVersion BundleVersion::EmptyVersion()
{
  static BundleVersion emptyV(false);
  return emptyV;
}

BundleVersion BundleVersion::UndefinedVersion()
{
  static BundleVersion undefinedV(true);
  return undefinedV;
}

BundleVersion& BundleVersion::operator=(const BundleVersion& v)
{
  majorVersion = v.majorVersion;
  minorVersion = v.minorVersion;
  microVersion = v.microVersion;
  qualifier = v.qualifier;
  undefined = v.undefined;
  return *this;
}

BundleVersion::BundleVersion(bool undefined)
  : majorVersion(0)
  , minorVersion(0)
  , microVersion(0)
  , qualifier("")
  , undefined(undefined)
{}

void BundleVersion::Validate()
{
  if (std::find_if(qualifier.begin(), qualifier.end(), IsInvalidQualifier) !=
      qualifier.end())
    throw std::invalid_argument(std::string("invalid qualifier: ") + qualifier);

  undefined = false;
}

BundleVersion::BundleVersion(unsigned int majorVersion,
                             unsigned int minorVersion,
                             unsigned int microVersion)
  : majorVersion(majorVersion)
  , minorVersion(minorVersion)
  , microVersion(microVersion)
  , qualifier("")
  , undefined(false)
{}

BundleVersion::BundleVersion(unsigned int majorVersion,
                             unsigned int minorVersion,
                             unsigned int microVersion,
                             const std::string& qualifier)
  : majorVersion(majorVersion)
  , minorVersion(minorVersion)
  , microVersion(microVersion)
  , qualifier(qualifier)
  , undefined(true)
{
  this->Validate();
}

BundleVersion::BundleVersion(const std::string& version)
  : majorVersion(0)
  , minorVersion(0)
  , microVersion(0)
  , undefined(true)
{
  unsigned int maj = 0;
  unsigned int min = 0;
  unsigned int mic = 0;
  std::string qual("");

  std::vector<std::string> st;
  std::stringstream ss(version);
  std::string token;
  while (std::getline(ss, token, SEPARATOR)) {
    st.push_back(token);
  }

  if (st.empty())
    return;

  bool ok = true;
  ss.clear();
  ss.str(st[0]);
  ss >> maj;
  ok = !ss.fail();

  if (st.size() > 1) {
    ss.clear();
    ss.str(st[1]);
    ss >> min;
    ok = !ss.fail();
    if (st.size() > 2) {
      ss.clear();
      ss.str(st[2]);
      ss >> mic;
      ok = !ss.fail();
      if (st.size() > 3) {
        qual = st[3];
        if (st.size() > 4) {
          ok = false;
        }
      }
    }
  }

  if (!ok)
    throw std::invalid_argument("invalid format");

  majorVersion = maj;
  minorVersion = min;
  microVersion = mic;
  qualifier = qual;
  this->Validate();
}

BundleVersion::BundleVersion(const BundleVersion& version)
  : majorVersion(version.majorVersion)
  , minorVersion(version.minorVersion)
  , microVersion(version.microVersion)
  , qualifier(version.qualifier)
  , undefined(version.undefined)
{}

BundleVersion BundleVersion::ParseVersion(const std::string& version)
{
  if (version.empty()) {
    return EmptyVersion();
  }

  std::string version2(version);
  version2.erase(0, version2.find_first_not_of(' '));
  version2.erase(version2.find_last_not_of(' ') + 1);
  if (version2.empty()) {
    return EmptyVersion();
  }

  return BundleVersion(version2);
}

bool BundleVersion::IsUndefined() const
{
  return undefined;
}

unsigned int BundleVersion::GetMajor() const
{
  if (undefined)
    throw std::logic_error("Version undefined");
  return majorVersion;
}

unsigned int BundleVersion::GetMinor() const
{
  if (undefined)
    throw std::logic_error("Version undefined");
  return minorVersion;
}

unsigned int BundleVersion::GetMicro() const
{
  if (undefined)
    throw std::logic_error("Version undefined");
  return microVersion;
}

std::string BundleVersion::GetQualifier() const
{
  if (undefined)
    throw std::logic_error("Version undefined");
  return qualifier;
}

std::string BundleVersion::ToString() const
{
  if (undefined)
    return "undefined";

  std::stringstream ss;
  ss << majorVersion << SEPARATOR << minorVersion << SEPARATOR << microVersion;
  if (!qualifier.empty()) {
    ss << SEPARATOR << qualifier;
  }
  return ss.str();
}

bool BundleVersion::operator==(const BundleVersion& other) const
{
  if (&other == this) { // quicktest
    return true;
  }

  if (other.undefined && this->undefined)
    return true;
  if (this->undefined)
    throw std::logic_error("Version undefined");
  if (other.undefined)
    return false;

  return (majorVersion == other.majorVersion) &&
         (minorVersion == other.minorVersion) &&
         (microVersion == other.microVersion) && qualifier == other.qualifier;
}

int BundleVersion::Compare(const BundleVersion& other) const
{
  if (&other == this) { // quicktest
    return 0;
  }

  if (this->undefined || other.undefined)
    throw std::logic_error("Cannot compare undefined version");

  if (majorVersion < other.majorVersion) {
    return -1;
  }

  if (majorVersion == other.majorVersion) {

    if (minorVersion < other.minorVersion) {
      return -1;
    }

    if (minorVersion == other.minorVersion) {

      if (microVersion < other.microVersion) {
        return -1;
      }

      if (microVersion == other.microVersion) {
        return qualifier.compare(other.qualifier);
      }
    }
  }
  return 1;
}

std::ostream& operator<<(std::ostream& os, const BundleVersion& v)
{
  return os << v.ToString();
}
}
