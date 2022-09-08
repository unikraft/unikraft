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
//     Internal Definitions and
//     Internal AES Function Prototypes
// 
// 
*/

#if !defined(_PCP_AES_H)
#define _PCP_AES_H

#include "pcprij.h"

/* Intel(R) AES New Instructions (Intel(R) AES-NI) flag */
#define AES_NI_ENABLED        (ippCPUID_AES)

/* alignment of AES context */
#define AES_ALIGNMENT   (RIJ_ALIGNMENT)

/* valid AES context ID */
#define VALID_AES_ID(ctx)   ((((ctx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((ctx))) == (Ipp32u)idCtxRijndael)

/* number of rounds (use [NK] for access) */
static int rij128nRounds[3] = {NR128_128, NR128_192, NR128_256};

/*
// number of keys (estimation only!)  (use [NK] for access)
//
// accurate number of keys necassary for encrypt/decrypt are:
//    nKeys = NB * (NR+1)
//       where NB - data block size (32-bit words)
//             NR - number of rounds (depend on NB and keyLen)
//
// but the estimation
//    estnKeys = (NK*n) >= nKeys
// or
//    estnKeys = ( (NB*(NR+1) + (NK-1)) / NK) * NK
//       where NK - key length (words)
//             NB - data block size (word)
//             NR - number of rounds (depend on NB and keyLen)
//             nKeys - accurate numner of keys
// is more convinient when calculates key extension
*/
static int rij128nKeys[3] = {44,  52,  60 };

/*
// helper for nRounds[] and estnKeys[] access
// note: x is length in 32-bits words
*/
__INLINE int rij_index(int x)
{
   return (x-NB(128))>>1;
}

/* size of AES context */
__INLINE int cpSizeofCtx_AES(void)
{
   return sizeof(IppsAESSpec);
}

#endif /* _PCP_AES_H */
