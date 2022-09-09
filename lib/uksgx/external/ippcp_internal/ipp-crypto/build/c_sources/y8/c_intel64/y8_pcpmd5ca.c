/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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
//     Digesting message according to MD5
//     (derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm)
// 
//     Equivalent code is available from RFC 1321.
// 
//  Contents:
//        cpFinalizeMD5()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpmd5stuff.h"
#include "pcpmd5stuff.h"

/*
// Compute digest
*/
IPP_OWN_DEFN (void, cpFinalizeMD5, (DigestMD5 pHash, const Ipp8u* inpBuffer, int inpLen, Ipp64u processedMsgLen))
{
   /* local buffer and it length */
   Ipp8u buffer[MBS_MD5*2];
   int bufferLen = inpLen < (MBS_MD5-(int)MLR_MD5)? MBS_MD5 : MBS_MD5*2; 

   /* copy rest of message into internal buffer */
   CopyBlock(inpBuffer, buffer, (cpSize)inpLen);

   /* padd message */
   buffer[inpLen++] = 0x80;
   PadBlock(0, buffer+inpLen, (cpSize)(bufferLen-inpLen-(int)MLR_MD5));

   /* put processed message length in bits */
   processedMsgLen <<= 3;
   ((Ipp64u*)(buffer+bufferLen))[-1] = processedMsgLen;

   /* copmplete hash computation */
   UpdateMD5(pHash, buffer, bufferLen, MD5_cnt);
}
