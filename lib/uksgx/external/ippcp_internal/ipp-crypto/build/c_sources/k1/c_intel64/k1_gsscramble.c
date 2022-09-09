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
//     gsScramblePut()
//     gsScrambleGet()
//
*/

#include "owncp.h"
#include "gsscramble.h"

IPP_OWN_DEFN (int, gsGetScrambleBufferSize, (int modulusLen, int w))
{
   /* size of resource to store 2^w values of modulusLen*sizeof(BNU_CHUNK_T) each */
   int size = (1<<w) * modulusLen * (Ipp32s)sizeof(BNU_CHUNK_T);
   /* padd it up to CACHE_LINE_SIZE */
   size += (CACHE_LINE_SIZE - (size % CACHE_LINE_SIZE)) %CACHE_LINE_SIZE;
   return size/(Ipp32s)sizeof(BNU_CHUNK_T);
}

IPP_OWN_DEFN (void, gsScramblePut, (BNU_CHUNK_T* tbl, int idx, const BNU_CHUNK_T* val, int vLen, int w))
{
   int width = 1 << w;
   int i, j;
   for(i=0, j=idx; i<vLen; i++, j+= width) {
      tbl[j] = val[i];
   }
}

IPP_OWN_DEFN (void, gsScrambleGet, (BNU_CHUNK_T* val, int vLen, const BNU_CHUNK_T* tbl, int idx, int w))
{
   int width = 1 << w;
   int i, j;
   for(i=0, j=idx; i<vLen; i++, j+= width) {
      val[i] = tbl[j];
   }
}
