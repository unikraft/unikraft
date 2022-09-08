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
/// TPM log error prints
/*! \file */

#ifndef EPID_MEMBER_TPM2_IBM_TSS_PRINTTSS_H_
#define EPID_MEMBER_TPM2_IBM_TSS_PRINTTSS_H_

#if !defined(EPID_ENABLE_DEBUG_PRINT)
/// Do not print tpm2 response error if EPID_ENABLE_DEBUG_PRINT is undefined
#define print_tpm2_response_code(...)
#else
#include <tss2/TPM_Types.h>
#include <tss2/tss.h>
/// Print TPM 2.0 response code as human readable message
/*!
 * \param[in] operation The operation that returned the code
 * \param[in] rc The response code
 */
void print_tpm2_response_code(char const* operation, TPM_RC rc);
#endif

#endif  // EPID_MEMBER_TPM2_IBM_TSS_PRINTTSS_H_
