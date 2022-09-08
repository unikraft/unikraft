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

/*
//
//  Purpose:
//     Cryptography Primitive.
//     AES decryption (CBC mode)
//     AES decryption (CBC-CS mode)
//
//  Contents:
//     cpDecryptAES_cbc()
//
*/

#include "owndefs.h"
#include "owncp.h"

#if !defined(_PCP_AES_CBC_DECRYPT_H_)
#define _PCP_AES_CBC_DECRYPT_H_

#define cpDecryptAES_cbc OWNAPI(cpDecryptAES_cbc)
    IPP_OWN_DECL (void, cpDecryptAES_cbc, (const Ipp8u* pIV, const Ipp8u* pSrc, Ipp8u* pDst, int nBlocks, const IppsAESSpec* pCtx))

#endif /* #if !defined(_PCP_AES_CBC_DECRYPT_H_) */
