/*******************************************************************************
* Copyright 2005-2021 Intel Corporation
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
//     DL over Prime Field (initialization)
// 
//  Contents:
//        ippsDLPInit()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"
#include "pcptool.h"

#include "pcpmont_exp_bufsize.h"

/*F*
//    Name: ippsDLPInit
//
// Purpose: Init DL context.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pDSA
//
//    ippStsSizeErr              MIN_DLP_BITSIZE  > feBitSize
//                               MIN_DLP_BITSIZER > ordBitSize
//                               ordBitSize >= feBitSize
//
//    ippStsNoErr                no errors
//
// Parameters:
//    feBitSize   size (bits) of field element (GF(p)) of DL system
//    ordBitSize  size (bits) of subgroup (GF(p)) generator order
//    pDL         pointer to the DL context
//
*F*/
IPPFUN(IppStatus, ippsDLPInit,(int feBitSize, int ordBitSize, IppsDLPState* pDL))
{
   /* test DSA context */
   IPP_BAD_PTR1_RET(pDL);

   /* test sizes of DL system */
   IPP_BADARG_RET(MIN_DLP_BITSIZE >feBitSize,  ippStsSizeErr);
   IPP_BADARG_RET(MIN_DLP_BITSIZER>ordBitSize, ippStsSizeErr);
   IPP_BADARG_RET(ordBitSize>=feBitSize,       ippStsSizeErr);

   DLP_SET_ID(pDL);
   DLP_FLAG(pDL)     = 0;
   DLP_BITSIZEP(pDL) = feBitSize;
   DLP_BITSIZER(pDL) = ordBitSize;
   DLP_EXPMETHOD(pDL)= BINARY;

   /*
   // init other context fields
   */
   {
      int bnSizeP;
      int bnSizeR;
      int montSizeP;
      int montSizeR;
      int primeGenSize;
      int bn_resourceSize;

      Ipp8u* ptr = (Ipp8u*)pDL;

      /* size of GF(P) element */
      int sizeP = BITS2WORD32_SIZE(feBitSize);
      /* size of GF(R) element */
      int sizeR = BITS2WORD32_SIZE(ordBitSize);
      /* size of pre-computed multi-exp table */
      int sizeMeTable = cpMontExpScratchBufferSize(feBitSize, ordBitSize, 2);

      /* size of window for exponentiation */
      #if defined(_USE_WINDOW_EXP_)
      int window = cpMontExp_WinSize(ordBitSize);
      DLP_EXPMETHOD(pDL) = window>1? WINDOW : BINARY;
      #endif

      /* size of BigNum over GF(P) */
      ippsBigNumGetSize(sizeP, &bnSizeP);
      /* size of BigNum over GF(R) */
      ippsBigNumGetSize(sizeR, &bnSizeR);

      /* size of montgomery engine over GF(P) */
      gsModEngineGetSize(feBitSize, DLP_MONT_POOL_LENGTH, &montSizeP);

      /* size of montgomery engine over GF(R) */
      gsModEngineGetSize(ordBitSize, DLP_MONT_POOL_LENGTH, &montSizeR);

      /* size of prime engine */
      ippsPrimeGetSize(feBitSize, &primeGenSize);

      /* size of big num list (big num in the list preserve 32 bit word) */
      bn_resourceSize = cpBigNumListGetSize(feBitSize+1, BNLISTSIZE);

      /* allocate buffers */
      ptr += sizeof(IppsDLPState);
      DLP_MONTP0(pDL)  = (gsModEngine*)(ptr);
      ptr += montSizeP;
      DLP_MONTP1(pDL)  = 0;
      DLP_MONTR(pDL)   = (gsModEngine*)(ptr);
      ptr += montSizeR;

      DLP_GENC(pDL)    = (IppsBigNumState*)(ptr);
      ptr += bnSizeP;
      DLP_X(pDL)       = (IppsBigNumState*)(ptr);
      ptr += bnSizeR;
      DLP_YENC(pDL)    = (IppsBigNumState*)(ptr);
      ptr += bnSizeP;

      DLP_PRIMEGEN(pDL)= (IppsPrimeState*)(ptr);
      ptr += primeGenSize;

      DLP_METBL(pDL)   = (BNU_CHUNK_T*)( IPP_ALIGNED_PTR(ptr, CACHE_LINE_SIZE) );
      ptr += sizeMeTable;

      DLP_BNCTX(pDL)   = (BigNumNode*)(ptr);
      ptr += bn_resourceSize;

      #if defined(_USE_WINDOW_EXP_)
      DLP_BNUCTX0(pDL) = 0;
      DLP_BNUCTX1(pDL) = 0;
      DLP_BNUCTX0(pDL) = window>1? (BNU_CHUNK_T*)( IPP_ALIGNED_PTR(ptr, (int)sizeof(BNU_CHUNK_T)) ) : 0;
      #endif

      /* init buffers */
      gsModEngineInit(DLP_MONTP0(pDL), NULL, feBitSize, DLP_MONT_POOL_LENGTH, gsModArithDLP());

      gsModEngineInit(DLP_MONTR(pDL), NULL, ordBitSize, DLP_MONT_POOL_LENGTH, gsModArithDLP());

      ippsBigNumInit(sizeP,  DLP_GENC(pDL));
      ippsBigNumInit(sizeP,  DLP_YENC(pDL));
      ippsBigNumInit(sizeR,  DLP_X(pDL));

      ippsPrimeInit(feBitSize, DLP_PRIMEGEN(pDL));

      cpBigNumListInit(feBitSize+1, BNLISTSIZE, DLP_BNCTX(pDL));

      return ippStsNoErr;
   }
}
