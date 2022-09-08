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
//     AES encryption/decryption (OFB mode)
// 
//  Contents:
//        cpProcessAES_ofb8()
//
*/

#include "owndefs.h"
#include "owncp.h"

#if !defined(_PCP_AES_OFB_H)
#define _PCP_AES_OFB_H

/*
// AES-OFB ecnryption/decryption
//
// Parameters:
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    dataLen     input/output buffer length (in bytes)
//    ofbBlkSize  ofb block size (in bytes)
//    pCtx        pointer to the AES context
//    pIV         pointer to the initialization vector
*/
#define cpProcessAES_ofb8 OWNAPI(cpProcessAES_ofb8)
    IPP_OWN_DECL (void, cpProcessAES_ofb8, (const Ipp8u *pSrc, Ipp8u *pDst, int dataLen, int ofbBlkSize, const IppsAESSpec* pCtx, Ipp8u* pIV))

#endif /* #if !defined(_PCP_AES_OFB_H) */
