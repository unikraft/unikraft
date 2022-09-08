/*############################################################################
  # Copyright 2017 Intel Corporation
  #
  # Licensed under the Apache License, Version 2.0 (the "License");
  # you may not use this file except in compliance with the License.
  # You may obtain a copy of the License at
  #
  #     http://www.apache.org/licenses/LICENSE-2.0
  #
  # Unless required by applicable law or agreed to in writing, software
  # distributed under the License is distributed on an "AS IS" BASIS,
  # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  # See the License for the specific language governing permissions and
  # limitations under the License.
  ############################################################################*/

/*!
 * \file
 * \brief Implementation of Intel(R) EPID specific predicates for gtest
 */
#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include <string>

/// Record mapping status code to string
struct EpidStatusTextEntry {
  /// error code
  EpidStatus value;
  /// name of error code
  const char* value_name;
};
#define EPID_STATUS_TEXT_ENTRY_VALUE(sts) \
  { sts, #sts }
/// Mapping of status codes to strings
static const struct EpidStatusTextEntry kEnumToText[] = {
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidNoErr),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidErr),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidSigInvalid),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidSigRevokedInGroupRl),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidSigRevokedInPrivRl),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidSigRevokedInSigRl),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidSigRevokedInVerifierRl),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidNotImpl),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidBadArgErr),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidNoMemErr),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidMemAllocErr),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidMathErr),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidDivByZeroErr),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidUnderflowErr),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidHashAlgorithmNotSupported),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidRandMaxIterErr),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidDuplicateErr),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidInconsistentBasenameSetErr),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidMathQuadraticNonResidueError),
    EPID_STATUS_TEXT_ENTRY_VALUE(kEpidOutOfSequenceError)};

const char* EpidStatusToName(EpidStatus e) {
  size_t i = 0;
  const size_t num_entries = sizeof(kEnumToText) / sizeof(kEnumToText[0]);
  for (i = 0; i < num_entries; i++) {
    if (e == kEnumToText[i].value) {
      return kEnumToText[i].value_name;
    }
  }
  return "unknown";
}
std::ostream& operator<<(std::ostream& os, EpidStatus e) {
  const char* enum_name = EpidStatusToName(e);
  return os << enum_name << " (" << std::to_string(e) << ")";
}
