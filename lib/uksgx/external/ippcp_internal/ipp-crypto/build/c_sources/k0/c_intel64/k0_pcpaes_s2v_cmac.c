/*******************************************************************************
* Copyright 2015-2021 Intel Corporation
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
//     AES-SIV Functions (RFC 5297)
// 
//  Contents:
//        ippsAES_S2V_CMAC()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpcmac.h"
#include "pcpaesm.h"
#include "pcptool.h"
#include "pcpaes_sivstuff.h"

/*F*
//    Name: ippsAES_S2V_CMAC
//
// Purpose: Converts strings to vector  -
//          performs S2V operation as defined RFC 5297.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pV == NULL
//                            pAD== NULL
//                            pADlen==NULL
//                            pADlen[i]!=0 && pAD[i]==0
//    ippStsLengthErr         keyLen != 16
//                            keyLen != 24
//                            keyLen != 32
//                            pADlen[i]<0
//                            0 > numAD
//    ippStsNoErr             no errors
//
// Parameters:
//    pKey     pointer to the secret key
//    keyLen   length of secret key
//    pAD[]    array of pointers to input strings
//    pADlen[] array of input string lengths
//    numAD    number of pAD[] and pADlen[] terms
//    pV       pointer to output vector
//
*F*/
IPPFUN(IppStatus, ippsAES_S2V_CMAC,(const Ipp8u* pKey, int keyLen,
                                    const Ipp8u* pAD[], const int pADlen[], int numAD,
                                          Ipp8u  V[MBS_RIJ128]))
{
   /* test output vector */
   IPP_BAD_PTR1_RET(V);

   /* make sure that number of input string is legal */
   IPP_BADARG_RET(0>numAD, ippStsLengthErr);

   /* test arrays of input */
   IPP_BAD_PTR2_RET(pAD, pADlen);

   int n;
   for(n=0; n<numAD; n++) {
      /* test input message and it's length */
      IPP_BADARG_RET((pADlen[n]<0), ippStsLengthErr);
      /* test source pointer */
      IPP_BADARG_RET((pADlen[n] && !pAD[n]), ippStsNullPtrErr);
   }

   {
      Ipp8u ctxBlob[sizeof(IppsAES_CMACState)];
      IppsAES_CMACState* pCtx = (IppsAES_CMACState*)ctxBlob;
      IppStatus sts = cpAES_S2V_init(V, pKey, keyLen, pCtx, sizeof(ctxBlob));

      if(ippStsNoErr==sts) {
         if(0==numAD) {
            PadBlock(0, V, MBS_RIJ128);
            V[MBS_RIJ128-1] = 0x1;
            cpAES_CMAC(V, V, MBS_RIJ128, pCtx);
         }

         else {
            int i;
            for(i=0, numAD--; i<numAD; i++)
               cpAES_S2V_update(V, pAD[i], pADlen[i], pCtx);
             cpAES_S2V_final(V, pAD[numAD], pADlen[numAD], pCtx);
         }
      }

      PurgeBlock(&ctxBlob, sizeof(ctxBlob));
      return sts;
   }
}
