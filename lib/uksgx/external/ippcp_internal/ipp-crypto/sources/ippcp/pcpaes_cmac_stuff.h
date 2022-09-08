/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
// 
//  Purpose:
//     Cryptography Primitive.
//     AES-CMAC Functions
// 
//  Contents:
//        init()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpcmac.h"
#include "pcpaesm.h"
#include "pcptool.h"

#if !defined(_PCP_AES_CMAC_STUFF_H_)
#define _PCP_AES_CMAC_STUFF_H_

__INLINE int cpSizeofCtx_AESCMAC(void)
{
   return sizeof(IppsAES_CMACState);
}

static void init(IppsAES_CMACState* pCtx)
{
   /* buffer is empty */
   CMAC_INDX(pCtx) = 0;
   /* zeros MAC */
   PadBlock(0, CMAC_MAC(pCtx), MBS_RIJ128);
}

#endif /* #if !defined(_PCP_AES_CMAC_STUFF_H_) */
