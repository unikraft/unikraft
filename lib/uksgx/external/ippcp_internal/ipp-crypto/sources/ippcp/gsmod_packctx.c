/*******************************************************************************
* Copyright 2017-2021 Intel Corporation
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
//     Cryptography Primitive. Modular Arithmetic Engine. General Functionality
// 
//  Contents:
//        gsPackModEngineCtx()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpbnuarith.h"
#include "gsmodstuff.h"
#include "pcptool.h"

/*
// Pack/Unpack methods
*/
IPP_OWN_DEFN (void, gsPackModEngineCtx, (const gsModEngine* pCtx, Ipp8u* pBuffer))
{
   gsModEngine* pB = (gsModEngine*)pBuffer;

   /* max modulus length */
   int modSize = MOD_LEN(pCtx);
   /* size of context (bytes) without cube and pool buffers */
   int ctxSize = (Ipp32s)sizeof(gsModEngine)
                +(Ipp32s)sizeof(BNU_CHUNK_T)*(modSize*3);

   CopyBlock(pCtx, pB, ctxSize);
   MOD_MODULUS(pB) = (BNU_CHUNK_T*)((Ipp8u*)NULL + IPP_UINT_PTR(MOD_MODULUS(pCtx))-IPP_UINT_PTR(pCtx));
   MOD_MNT_R(pB)   = (BNU_CHUNK_T*)((Ipp8u*)NULL + IPP_UINT_PTR(MOD_MNT_R(pCtx))-IPP_UINT_PTR(pCtx));
   MOD_MNT_R2(pB)  = (BNU_CHUNK_T*)((Ipp8u*)NULL + IPP_UINT_PTR(MOD_MNT_R2(pCtx))-IPP_UINT_PTR(pCtx));
}
