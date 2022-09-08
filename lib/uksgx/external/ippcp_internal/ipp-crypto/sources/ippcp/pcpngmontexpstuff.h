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
//     Internal Definitions and
//     Internal ng RSA Function Prototypes
// 
// 
*/

#if !defined(_CP_NG_MONT_EXP_STUFF_H)
#define _CP_NG_MONT_EXP_STUFF_H

#include "pcpbnuimpl.h"
#include "pcpbn.h"
#include "gsmodstuff.h"


/*
// optimal size of fixed window exponentiation
*/
__INLINE cpSize gsMontExp_WinSize(cpSize bitsize)
{
   #if defined(_USE_WINDOW_EXP_)
   // new computations
   return
      #if (_IPP !=_IPP_M5)     /*limited by 6 or 4 (LOG_CACHE_LINE_SIZE); we use it for windowing-exp imtigation */
         bitsize> 4096? 6 :    /* 4096- .. .  */
         bitsize> 2666? 5 :    /* 2666 - 4095 */
      #endif
         bitsize>  717? 4 :    /*  717 - 2665 */
         bitsize>  178? 3 :    /*  178 - 716  */
         bitsize>   41? 2 : 1; /*   41 - 177  */
   #else
   IPP_UNREFERENCED_PARAMETER(bitsize);
   return 1;
   #endif
}

/*
// Montgomery encoding/decoding
*/
__INLINE cpSize gsMontEnc_BNU(BNU_CHUNK_T* pR,
                        const BNU_CHUNK_T* pXreg, cpSize nsX,
                        const gsModEngine* pMont)
{
   cpSize nsM = MOD_LEN( pMont );
   ZEXPAND_COPY_BNU(pR, nsM, pXreg, nsX);
   MOD_METHOD( pMont )->encode(pR, pR, (gsModEngine*)pMont);
   return nsM;
}

__INLINE cpSize gsMontDec_BNU(BNU_CHUNK_T* pR,
                        const BNU_CHUNK_T* pXmont,
                              gsModEngine* pMont)
{
   cpSize nsM = MOD_LEN(pMont);
   MOD_METHOD( pMont )->decode(pR, pXmont, (gsModEngine*)pMont);
   return nsM;
}

__INLINE void gsMontEnc_BN(IppsBigNumState* pRbn,
                     const IppsBigNumState* pXbn,
                           gsModEngine* pMont)
{
   BNU_CHUNK_T* pR = BN_NUMBER(pRbn);
   cpSize nsM = MOD_LEN(pMont);

   gsMontEnc_BNU(pR, BN_NUMBER(pXbn), BN_SIZE(pXbn), pMont);

   FIX_BNU(pR, nsM);
   BN_SIZE(pRbn) = nsM;
   BN_SIGN(pRbn) = ippBigNumPOS;
}

/* exponentiation buffer size */
#define gsMontExpBinBuffer OWNAPI(gsMontExpBinBuffer)
   IPP_OWN_DECL (cpSize, gsMontExpBinBuffer, (int modulusBits))
#define gsMontExpWinBuffer OWNAPI(gsMontExpWinBuffer)
   IPP_OWN_DECL (cpSize, gsMontExpWinBuffer, (int modulusBits))

/* exponentiation prototype */
IPP_OWN_FUNPTR (cpSize, ngMontExp, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nbitsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
IPP_OWN_FUNPTR (cpSize, ngMontDualExp, (BNU_CHUNK_T* dataY[2], const BNU_CHUNK_T* dataX[2], cpSize nsX[2], const BNU_CHUNK_T* dataE[2], gsModEngine* pMont[2], BNU_CHUNK_T* pBuffer))

/*
// "fast" and "safe" binary montgomery exponentiation ("fast" version)
*/
#define gsMontExpBin_BNU OWNAPI(gsMontExpBin_BNU)
   IPP_OWN_DECL (cpSize, gsMontExpBin_BNU, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nbitsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
#define gsModExpBin_BNU OWNAPI(gsModExpBin_BNU)
   IPP_OWN_DECL (cpSize, gsModExpBin_BNU, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nbitsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))

#define gsMontExpBin_BNU_sscm OWNAPI(gsMontExpBin_BNU_sscm)
   IPP_OWN_DECL (cpSize, gsMontExpBin_BNU_sscm, (BNU_CHUNK_T* pY, const BNU_CHUNK_T* pX, cpSize nsX, const BNU_CHUNK_T* pE, cpSize nbitsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
#define gsModExpBin_BNU_sscm OWNAPI(gsModExpBin_BNU_sscm)
   IPP_OWN_DECL (cpSize, gsModExpBin_BNU_sscm, (BNU_CHUNK_T* pY, const BNU_CHUNK_T* pX, cpSize nsX, const BNU_CHUNK_T* pE, cpSize nbitsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))

/*
// "fast" and "safe" fixed-size window montgomery exponentiation
*/
#define gsMontExpWin_BNU OWNAPI(gsMontExpWin_BNU)
   IPP_OWN_DECL (cpSize, gsMontExpWin_BNU, (BNU_CHUNK_T* pY, const BNU_CHUNK_T* pX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nbitsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
#define gsModExpWin_BNU OWNAPI(gsModExpWin_BNU)
   IPP_OWN_DECL (cpSize, gsModExpWin_BNU, (BNU_CHUNK_T* pY, const BNU_CHUNK_T* pX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nbitsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))

#define gsMontExpWin_BNU_sscm OWNAPI(gsMontExpWin_BNU_sscm)
   IPP_OWN_DECL (cpSize, gsMontExpWin_BNU_sscm, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nbitsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
#define gsModExpWin_BNU_sscm OWNAPI(gsModExpWin_BNU_sscm)
   IPP_OWN_DECL (cpSize, gsModExpWin_BNU_sscm, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize nbitsE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))

#endif /* _CP_NG_MONT_EXP_STUFF_H */
