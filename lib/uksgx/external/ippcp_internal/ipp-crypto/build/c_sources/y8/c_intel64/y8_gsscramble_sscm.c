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
//     gsScrambleGet_sscm()
//
*/
#include "owncp.h"
#include "gsscramble.h"
#include "pcpmask_ct.h"

IPP_OWN_DEFN (void, gsScrambleGet_sscm, (BNU_CHUNK_T* val, int vLen, const BNU_CHUNK_T* tbl, int idx, int w))
{
   BNU_CHUNK_T mask[1<<MAX_W];

   int width = 1 << w;

   int n, i;
   switch (w) {
   case 6:
      for(n=0; n<(1<<6); n++)
         mask[n] = cpIsEqu_ct((BNU_CHUNK_T)n, (BNU_CHUNK_T)idx);
      break;
   case 5:
      for(n=0; n<(1<<5); n++)
         mask[n] = cpIsEqu_ct((BNU_CHUNK_T)n, (BNU_CHUNK_T)idx);
      break;
   case 4:
      for(n=0; n<(1<<4); n++)
         mask[n] = cpIsEqu_ct((BNU_CHUNK_T)n, (BNU_CHUNK_T)idx);
      break;
   case 3:
      for(n=0; n<(1<<3); n++)
         mask[n] = cpIsEqu_ct((BNU_CHUNK_T)n, (BNU_CHUNK_T)idx);
      break;
   case 2:
      for(n=0; n<(1<<2); n++)
         mask[n] = cpIsEqu_ct((BNU_CHUNK_T)n, (BNU_CHUNK_T)idx);
      break;
   default:
      mask[0] = cpIsEqu_ct(0, (BNU_CHUNK_T)idx);
      mask[1] = cpIsEqu_ct(1, (BNU_CHUNK_T)idx);
      break;
   }

   for(i=0; i<vLen; i++, tbl += width) {
      BNU_CHUNK_T acc = 0;

      switch (w) {
      case 6:
         for(n=0; n<(1<<6); n++)
            acc |= tbl[n] & mask[n];
         break;
      case 5:
         for(n=0; n<(1<<5); n++)
            acc |= tbl[n] & mask[n];
         break;
      case 4:
         for(n=0; n<(1<<4); n++)
            acc |= tbl[n] & mask[n];
         break;
      case 3:
         for(n=0; n<(1<<3); n++)
            acc |= tbl[n] & mask[n];
         break;
      case 2:
         for(n=0; n<(1<<2); n++)
            acc |= tbl[n] & mask[n];
         break;
      default:
         acc |= tbl[0] & mask[0];
         acc |= tbl[1] & mask[1];
         break;
      }

      val[i] = acc;
   }
}
