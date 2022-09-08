/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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

/*
//  Purpose:
//     Cryptography Primitive.
//     Hash Message Authentication Code
//     Internal Definitions and Internal Functions Prototypes
*/

#if !defined(_PCP_HMAC_RMF_H)
#define _PCP_HMAC_RMF_H

#include "pcphash_rmf.h"

/*
// HMAC context
*/
struct _cpHMAC_rmf {
   Ipp32u   idCtx;               /* HMAC identifier   */
   Ipp8u ipadKey[MBS_HASH_MAX];  /* inner padding key */
   Ipp8u opadKey[MBS_HASH_MAX];  /* outer padding key */
   IppsHashState_rmf hashCtx;    /* hash context      */
};


#endif /* _PCP_HMAC_RMF_H */
