/*******************************************************************************
* Copyright 2017-2021 Intel Corporation
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
//     Cryptography Primitive. Modular Arithmetic Engine. General Functionality
// 
//  Contents:
//        alm_mont_inv()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpbnuarith.h"
#include "gsmodstuff.h"
#include "pcpmask_ct.h"

/*
// almost Montgomery Inverse
//
// returns (k,r), r = (1/a)*(2^k) mod m
//
*/
IPP_OWN_DEFN (int, alm_mont_inv, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pME))
{
   const BNU_CHUNK_T* pm = MOD_MODULUS(pME);
   int mLen = MOD_LEN(pME);

   const int polLength  = 4;
   BNU_CHUNK_T* pBuffer = gsModPoolAlloc(pME, polLength);

   BNU_CHUNK_T* pu = pBuffer;
   BNU_CHUNK_T* ps = pu+mLen;
   BNU_CHUNK_T* pv = ps+mLen;
   BNU_CHUNK_T* pt = pv+mLen;

   int k = 0;
   BNU_CHUNK_T ext = 0;

   //gres: temporary excluded: assert(NULL!=pBuffer);

   // u=modulus, v=a, t=0, s=1
   COPY_BNU(pu, pm, mLen);
   ZEXPAND_BNU(ps, 0, mLen); ps[0] = 1;
   COPY_BNU(pv, pa, mLen);
   ZEXPAND_BNU(pt, 0, mLen);

   while(!cpEqu_BNU_CHUNK(pv, mLen, 0)) {             // while(v>0) {
      if(0==(pu[0]&1)) {                              //    if(isEven(u)) {
         cpLSR_BNU(pu, pu, mLen, 1);                  //       u = u/2;
         cpAdd_BNU(ps, ps, ps, mLen);                 //       s = 2*s;
      }                                               //    }
      else if(0==(pv[0]&1)) {                         //    else if(isEven(v)) {
         cpLSR_BNU(pv, pv, mLen, 1);                  //       v = v/2;
         /*ext +=*/ cpAdd_BNU(pt, pt, pt, mLen);      //       t = 2*t;
      }                                               //    }
      else {
         int cmpRes = cpCmp_BNU(pu, mLen, pv, mLen);
         if(cmpRes>0) {                               //    else if (u>v) {
            cpSub_BNU(pu, pu, pv, mLen);              //       u = (u-v);
            cpLSR_BNU(pu, pu, mLen, 1);               //       u = u/2;
            /*ext +=*/ cpAdd_BNU(pt, pt, ps, mLen);   //       t = t+s;
            cpAdd_BNU(ps, ps, ps, mLen);              //       s = 2*s;
         }                                            //    }
         else {                                       //    else if(v>=u) {
            cpSub_BNU(pv, pv, pu, mLen);              //       v = (v-u);
            cpLSR_BNU(pv, pv, mLen, 1);               //       v = v/2;
            cpAdd_BNU(ps, ps, pt, mLen);              //       s = s+t;
            ext += cpAdd_BNU(pt, pt, pt, mLen);       //       t = 2*t;
         }                                            //    }
      }
      k++;                                            //    k += 1;
   }                                                  // }

   // test
   if(1!=cpEqu_BNU_CHUNK(pu, mLen, 1)) {
      k = 0; /* inversion not found */
   }

   else {
      ext -= cpSub_BNU(pr, pt, pm, mLen);             // if(t>mod) r = t-mod;
      cpMaskMove_gs(pr, pt, mLen, cpIsNonZero(ext));  // else r = t;
      cpSub_BNU(pr, pm, pr, mLen);                    // return  r= (mod - r) and k
   }

   gsModPoolFree(pME, polLength);
   return k;
}
