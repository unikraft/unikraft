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
//     Security Hash Standard
//     Internal Definitions and Internal Functions Prototypes
//
*/

#if !defined(_CP_HASH_RMF_H)
#define _CP_HASH_RMF_H

#include "pcphash.h"
#include "pcphashmethod_rmf.h"

struct _cpHashCtx_rmf {
   Ipp32u      idCtx;                     /* hash identifier   */
   const cpHashMethod_rmf* pMethod;       /* hash methods      */
   int         msgBuffIdx;                /* buffer index      */
   Ipp8u       msgBuffer[MBS_HASH_MAX];   /* buffer            */
   Ipp64u      msgLenLo;                  /* processed message */
   Ipp64u      msgLenHi;                  /* length (bytes)    */
   cpHash      msgHash;                   /* hash value        */
};

/* accessors (see others in pcphash.h) */
#define HASH_METHOD(stt)   ((stt)->pMethod)

#define cpFinalize_rmf OWNAPI(cpFinalize_rmf)
   IPP_OWN_DECL (void, cpFinalize_rmf, (DigestSHA512 pHash, const Ipp8u* inpBuffer, int inpLen, Ipp64u lenLo, Ipp64u lenHi, const IppsHashMethod* method))

#endif /* _CP_HASH_RMF_H */
