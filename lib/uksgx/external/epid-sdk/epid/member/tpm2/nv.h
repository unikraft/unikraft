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
/// SDK TPM non volatile memory API.
/*! \file */

#ifndef EPID_MEMBER_TPM2_NV_H_
#define EPID_MEMBER_TPM2_NV_H_

#include <stddef.h>

#include "epid/common/errors.h"
#include "epid/common/stdtypes.h"

/// \cond
typedef struct Tpm2Ctx Tpm2Ctx;
/// \endcond

/*!
 \addtogroup Tpm2Module tpm2
 \ingroup EpidMemberModule
 @{
*/

/// Performs TPM2_NV_DefineSpace TPM command.
/*!
 \param[in] ctx
 The TPM context.
 \param[in] nv_index
 Handle of the data area.
 \param[in] size
 Size of the data area.

 \returns ::EpidStatus

 \see Tpm2NvRead
 \see Tpm2NvWrite
*/
EpidStatus Tpm2NvDefineSpace(Tpm2Ctx* ctx, uint32_t nv_index, size_t size);

/// Performs TPM2_NV_UndefineSpace TPM command.
/*!
 \param[in] ctx
 The TPM context.
 \param[in] nv_index
 Handle of the data area to undefine.

 \returns ::EpidStatus

 \see Tpm2NvDefineSpace
*/
EpidStatus Tpm2NvUndefineSpace(Tpm2Ctx* ctx, uint32_t nv_index);

/// Performs TPM2_NV_Write TPM command.
/*!
 An area in NV memory must be defined prior writing.

 \param[in] ctx
 The TPM context.
 \param[in] nv_index
 NV Index to be write.
 \param[in] size
 Number of bytes to write.
 \param[in] offset
 Offset into the area.
 \param[in] data
 Data to write.

 \returns ::EpidStatus

 \see Tpm2NvDefineSpace
*/
EpidStatus Tpm2NvWrite(Tpm2Ctx* ctx, uint32_t nv_index, size_t size,
                       uint16_t offset, void const* data);

/// Performs TPM2_NV_Read TPM command.
/*!
 \param[in] ctx
 The TPM context.
 \param[in] nv_index
 NV Index to be read.
 \param[in] size
 Number of bytes to read.
 \param[in] offset
 Offset into the area.
 \param[out] data
 Data read.

 \returns ::EpidStatus

 \see Tpm2NvWrite
*/
EpidStatus Tpm2NvRead(Tpm2Ctx* ctx, uint32_t nv_index, size_t size,
                      uint16_t offset, void* data);

/*! @} */

#endif  // EPID_MEMBER_TPM2_NV_H_
