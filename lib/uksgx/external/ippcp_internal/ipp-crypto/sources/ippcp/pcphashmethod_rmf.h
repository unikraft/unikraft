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
#if !defined(_PCP_HASH_METHOD_RMF_H)
#define _PCP_HASH_METHOD_RMF_H

/* hash alg methods */
IPP_OWN_FUNPTR (void, hashInitF, (void* pHash))
IPP_OWN_FUNPTR (void, hashUpdateF, (void* pHash, const Ipp8u* pMsg, int msgLen))
IPP_OWN_FUNPTR (void, hashOctStrF, (Ipp8u* pDst, void* pHash))
IPP_OWN_FUNPTR (void, msgLenRepF, (Ipp8u* pDst, Ipp64u lenLo, Ipp64u lenHi))

typedef struct _cpHashMethod_rmf {
   IppHashAlgId   hashAlgId;     /* algorithm ID */
   int            hashLen;       /* hash length in bytes */
   int            msgBlkSize;    /* message blkock size in bytes */
   int            msgLenRepSize; /* length of processed msg length representation in bytes */
   hashInitF      hashInit;      /* set initial hash value */
   hashUpdateF    hashUpdate;    /* hash compressor */
   hashOctStrF    hashOctStr;    /* convert hash into oct string */
   msgLenRepF     msgLenRep;     /* processed mgs length representation */
} cpHashMethod_rmf;

#endif /* _PCP_HASH_METHOD_RMF_H */
