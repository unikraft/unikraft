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
//     Fixed window exponentiation scramble/unscramble
// 
//  Contents:
//    gsGetScrambleBufferSize()
//    gsScramblePut()
//    gsScrambleGet()
//    gsScrambleGet_sscm()
// 
// 
*/

#if !defined(_GS_SCRAMBLE_H)
#define _GS_SCRAMBLE_H

#include "pcpbnuimpl.h"

#define MAX_W  (6)

#define gsGetScrambleBufferSize OWNAPI(gsGetScrambleBufferSize)
    IPP_OWN_DECL (int, gsGetScrambleBufferSize, (int modulusLen, int w))
#define gsScramblePut OWNAPI(gsScramblePut)
    IPP_OWN_DECL (void, gsScramblePut, (BNU_CHUNK_T* tbl, int idx, const BNU_CHUNK_T* val, int vLen, int w))
#define gsScrambleGet OWNAPI(gsScrambleGet)
    IPP_OWN_DECL (void, gsScrambleGet, (BNU_CHUNK_T* val, int vLen, const BNU_CHUNK_T* tbl, int idx, int w))
#define gsScrambleGet_sscm OWNAPI(gsScrambleGet_sscm)
    IPP_OWN_DECL (void, gsScrambleGet_sscm, (BNU_CHUNK_T* val, int vLen, const BNU_CHUNK_T* tbl, int idx, int w))

#endif /* _GS_SCRAMBLE_H */
