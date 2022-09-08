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
//        ippsDLPGetSize()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdlp.h"
#include "pcptool.h"

#include "pcpmont_exp_bufsize.h"



/*F*
//    Name: ippsDLPGetSize
//
// Purpose: Returns size of DL context (bytes).
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pSize
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
//    pSize       pointer to the size of DLP context (bytes)
//
*F*/
IPPFUN(IppStatus, ippsDLPGetSize,(int feBitSize, int ordBitSize, int *pSize))
{
   /* test size's pointer */
   IPP_BAD_PTR1_RET(pSize);

   /* test sizes of DL system */
   IPP_BADARG_RET(MIN_DLP_BITSIZE >feBitSize,  ippStsSizeErr);
   IPP_BADARG_RET(MIN_DLP_BITSIZER>ordBitSize, ippStsSizeErr);
   IPP_BADARG_RET(ordBitSize>=feBitSize,       ippStsSizeErr);

   {
      int bnSizeP;
      int bnSizeR;
      int montSizeP;
      int montSizeR;
      int primeGenSize;
      int bn_resourceSize;

      #if defined(_USE_WINDOW_EXP_)
      int window = cpMontExp_WinSize(ordBitSize);
      int bnu_resourceSize = window==1? 0 : cpMontExpScratchBufferSize(feBitSize, ordBitSize, 1);
      #endif

      /* size of GF(P) element */
      int sizeP = BITS2WORD32_SIZE(feBitSize);
      /* size of GF(R) element */
      int sizeR = BITS2WORD32_SIZE(ordBitSize);
      /* sizeof multi-exp table */
      int sizeMeTable = cpMontExpScratchBufferSize(feBitSize, ordBitSize, 2);

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

      *pSize = (Ipp32s)sizeof(IppsDLPState)
              +montSizeP         /* montgomery(P) */
              +montSizeR         /* montgomery(Q) */

              +bnSizeP           /* Genc          */
              +bnSizeR           /* X             */
              +bnSizeP           /* Y             */

              +primeGenSize      /* prime engine  */

              +sizeMeTable       /* pre-computed multi-exp table */

              +bn_resourceSize   /* BN resource   */
              #if defined(_USE_WINDOW_EXP_)
              +bnu_resourceSize  /* BNU resource  */
              #endif
              ;

      return ippStsNoErr;
   }
}
