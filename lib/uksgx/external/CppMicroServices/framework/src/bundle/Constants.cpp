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

#include "cppmicroservices/Constants.h"

namespace cppmicroservices {

namespace Constants {

const std::string SYSTEM_BUNDLE_LOCATION = "System Bundle";
const std::string SYSTEM_BUNDLE_SYMBOLICNAME = "system_bundle";

const std::string BUNDLE_ACTIVATOR = "bundle.activator";
const std::string BUNDLE_CATEGORY = "bundle.category";
const std::string BUNDLE_COPYRIGHT = "bundle.copyright";
const std::string BUNDLE_DESCRIPTION = "bundle.description";
const std::string BUNDLE_NAME = "bundle.name";
const std::string BUNDLE_VENDOR = "bundle.vendor";
const std::string BUNDLE_VERSION = "bundle.version";
const std::string BUNDLE_DOCURL = "bundle.doc_url";
const std::string BUNDLE_CONTACTADDRESS = "bundle.contact_address";
const std::string BUNDLE_SYMBOLICNAME = "bundle.symbolic_name";
const std::string BUNDLE_MANIFESTVERSION = "bundle.manifest_version";
const std::string BUNDLE_ACTIVATIONPOLICY = "bundle.activation_policy";
const std::string ACTIVATION_LAZY = "lazy";
const std::string FRAMEWORK_VERSION = "org.cppmicroservices.framework.version";
const std::string FRAMEWORK_VENDOR = "org.cppmicroservices.framework.vendor";
const std::string FRAMEWORK_STORAGE = "org.cppmicroservices.framework.storage";
const std::string FRAMEWORK_STORAGE_CLEAN =
  "org.cppmicroservices.framework.storage.clean";
const std::string FRAMEWORK_STORAGE_CLEAN_ONFIRSTINIT = "onFirstInit";
const std::string FRAMEWORK_THREADING_SUPPORT =
  "org.cppmicroservices.framework.threading.support";
const std::string FRAMEWORK_THREADING_SINGLE = "single";
const std::string FRAMEWORK_THREADING_MULTI = "multi";
const std::string FRAMEWORK_LOG = "org.cppmicroservices.framework.log";
const std::string FRAMEWORK_UUID = "org.cppmicroservices.framework.uuid";
const std::string FRAMEWORK_WORKING_DIR =
  "org.cppmicroservices.framework.working.dir";
const std::string OBJECTCLASS = "objectclass";
const std::string SERVICE_ID = "service.id";
const std::string SERVICE_PID = "service.pid";
const std::string SERVICE_RANKING = "service.ranking";
const std::string SERVICE_VENDOR = "service.vendor";
const std::string SERVICE_DESCRIPTION = "service.description";
const std::string SERVICE_SCOPE = "service.scope";
const std::string SCOPE_SINGLETON = "singleton";
const std::string SCOPE_BUNDLE = "bundle";
const std::string SCOPE_PROTOTYPE = "prototype";
}
}
