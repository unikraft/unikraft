/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include "owncp.h"

#define cpHashMessage_MB8_rmf OWNAPI(cpHashMessage_MB8_rmf)
    IPP_OWN_DECL (IppStatus, cpHashMessage_MB8_rmf, (const Ipp8u* const pSrc[8], int const lens[8], Ipp8u* const pDst[8], const IppsHashMethod* pMethod))
#define cpMGF1_MB8_rmf OWNAPI(cpMGF1_MB8_rmf)
    IPP_OWN_DECL (IppStatus, cpMGF1_MB8_rmf, (const Ipp8u* const pSeeds[8], int const seedLens[8], Ipp8u* const pMasks[8], int const maskLens[8], const IppsHashMethod* pMethod))
