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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     GF(p^d) methods
//
*/
#if !defined(_CP_GFP_METHOD_COM_H)
#define _CP_GFP_METHOD_COM_H

#include "owncp.h"
#include "pcpgfpstuff.h"

#define cpGFpxAdd_com OWNAPI(cpGFpxAdd_com)
    IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpxAdd_com, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFEx))
#define cpGFpxSub_com OWNAPI(cpGFpxSub_com)
    IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpxSub_com, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFEx))
#define cpGFpxNeg_com OWNAPI(cpGFpxNeg_com)
    IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpxNeg_com, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx))

#define cpGFpxMul_com OWNAPI(cpGFpxMul_com)
    IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpxMul_com, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFEx))
#define cpGFpxSqr_com OWNAPI(cpGFpxSqr_com)
    IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpxSqr_com, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx))

#define cpGFpxDiv2_com OWNAPI(cpGFpxDiv2_com)
    IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpxDiv2_com, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx))
#define cpGFpxMul2_com OWNAPI(cpGFpxMul2_com)
    IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpxMul2_com, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx))
#define cpGFpxMul3_com OWNAPI(cpGFpxMul3_com)
    IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpxMul3_com, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx))

#define cpGFpxEncode_com OWNAPI(cpGFpxEncode_com)
    IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpxEncode_com, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx))
#define cpGFpxDecode_com OWNAPI(cpGFpxDecode_com)
    IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpxDecode_com, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFEx))

#endif /* _CP_GFP_METHOD_COM_H */
