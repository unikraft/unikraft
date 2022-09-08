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

#include "BundleManifest.h"

#include "json/json.h"

#include <stdexcept>

namespace cppmicroservices {

namespace {

typedef std::map<std::string, Any> AnyOrderedMap;
typedef std::vector<Any> AnyVector;

void ParseJsonObject(const Json::Value& jsonObject, AnyMap& anyMap);
void ParseJsonObject(const Json::Value& jsonObject, AnyOrderedMap& anyMap);
void ParseJsonArray(const Json::Value& jsonArray,
                    AnyVector& anyVector,
                    bool ci);

Any ParseJsonValue(const Json::Value& jsonValue, bool ci)
{
  if (jsonValue.isObject()) {
    if (ci) {
      Any any = AnyMap(AnyMap::UNORDERED_MAP_CASEINSENSITIVE_KEYS);
      ParseJsonObject(jsonValue, ref_any_cast<AnyMap>(any));
      return any;
    } else {
      Any any = AnyOrderedMap();
      ParseJsonObject(jsonValue, ref_any_cast<AnyOrderedMap>(any));
      return any;
    }
  } else if (jsonValue.isArray()) {
    Any any = AnyVector();
    ParseJsonArray(jsonValue, ref_any_cast<AnyVector>(any), ci);
    return any;
  } else if (jsonValue.isString()) {
    // We do not support attribute localization yet, so we just
    // always remove the leading '%' character.
    std::string val = jsonValue.asString();
    if (!val.empty() && val[0] == '%')
      val = val.substr(1);

    return Any(val);
  } else if (jsonValue.isBool()) {
    return Any(jsonValue.asBool());
  } else if (jsonValue.isIntegral()) {
    return Any(jsonValue.asInt());
  } else if (jsonValue.isDouble()) {
    return Any(jsonValue.asDouble());
  }

  return Any();
}

void ParseJsonObject(const Json::Value& jsonObject, AnyOrderedMap& anyMap)
{
  for (Json::Value::const_iterator it = jsonObject.begin();
       it != jsonObject.end();
       ++it) {
    const Json::Value& jsonValue = *it;
    Any anyValue = ParseJsonValue(jsonValue, false);
    if (!anyValue.Empty()) {
      anyMap.insert(std::make_pair(it.name(), anyValue));
    }
  }
}

void ParseJsonObject(const Json::Value& jsonObject, AnyMap& anyMap)
{
  for (Json::Value::const_iterator it = jsonObject.begin();
       it != jsonObject.end();
       ++it) {
    const Json::Value& jsonValue = *it;
    Any anyValue = ParseJsonValue(jsonValue, true);
    if (!anyValue.Empty()) {
      anyMap.insert(std::make_pair(it.name(), anyValue));
    }
  }
}

void ParseJsonArray(const Json::Value& jsonArray, AnyVector& anyVector, bool ci)
{
  for (Json::Value::const_iterator it = jsonArray.begin();
       it != jsonArray.end();
       ++it) {
    const Json::Value& jsonValue = *it;
    Any anyValue = ParseJsonValue(jsonValue, ci);
    if (!anyValue.Empty()) {
      anyVector.push_back(anyValue);
    }
  }
}
}

BundleManifest::BundleManifest()
  : m_Headers(AnyMap::UNORDERED_MAP_CASEINSENSITIVE_KEYS)
{}

void BundleManifest::Parse(std::istream& is)
{
  Json::Value root;
  Json::Reader jsonReader(Json::Features::strictMode());
  if (!jsonReader.parse(is, root, false)) {
    throw std::runtime_error(jsonReader.getFormattedErrorMessages());
  }

  if (!root.isObject()) {
    throw std::runtime_error("The Json root element must be an object.");
  }

  // This is deprecated in 3.0
  ParseJsonObject(root, m_PropertiesDeprecated);

  ParseJsonObject(root, m_Headers);
}

AnyMap BundleManifest::GetHeaders() const
{
  return m_Headers;
}

bool BundleManifest::Contains(const std::string& key) const
{
  return m_Headers.count(key) > 0;
}

Any BundleManifest::GetValue(const std::string& key) const
{
  auto iter = m_Headers.find(key);
  if (iter != m_Headers.end()) {
    return iter->second;
  }
  return Any();
}

Any BundleManifest::GetValueDeprecated(const std::string& key) const
{
  auto iter = m_PropertiesDeprecated.find(key);
  if (iter != m_PropertiesDeprecated.end()) {
    return iter->second;
  }
  return Any();
}

std::vector<std::string> BundleManifest::GetKeysDeprecated() const
{
  std::vector<std::string> keys;
  for (AnyMap::const_iterator iter = m_PropertiesDeprecated.begin();
       iter != m_PropertiesDeprecated.end();
       ++iter) {
    keys.push_back(iter->first);
  }
  return keys;
}

std::map<std::string, Any> BundleManifest::GetPropertiesDeprecated() const
{
  return m_PropertiesDeprecated;
}
}
