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
//     TRNG Functions
// 
//  Contents:
//     ippsTRNGenHW()
//     ippsTRNGenHW_BN()
// 
// 
*/

#include "owndefs.h"

#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"

#if ((_IPP>=_IPP_G9) || (_IPP32E>=_IPP32E_E9))
static int cpSeed_hw_sample(BNU_CHUNK_T* pSample)
{
#define LOCAL_COUNTER (320) /* this constant has been tuned manually */
   int n;
   int success = 0;
   for(n=0; n<LOCAL_COUNTER && !success; n++)
      #if (_IPP_ARCH == _IPP_ARCH_IA32)
      success = _rdseed32_step(pSample);
      #elif (_IPP_ARCH == _IPP_ARCH_EM64T)
      #pragma warning(push) // temporary, compiler bug workaround
      #pragma warning(disable : 167)
      success = _rdseed64_step(pSample);
      #pragma warning(pop)
      #else
      #error Unknown CPU arch
      #endif
   return success;
#undef LOCAL_COUNTER
}

#if (_IPP32E>=_IPP32E_E9)
static int cpSeed_hw_sample32(Ipp32u* pSample)
{
#define LOCAL_COUNTER (320) /* this constant has been tuned manually */
   int n;
   int success = 0;
   for(n=0; n<LOCAL_COUNTER && !success; n++)
      success = _rdseed32_step(pSample);
   return success;
#undef LOCAL_COUNTER
}
#endif

static int cpSeedHW_buffer(Ipp32u* pRand, int bufLen)
{
   int nSamples = bufLen/((Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)));

   int n;
   /* collect nSamples randoms */
   for(n=0; n<nSamples; n++, pRand+=((Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)))) {
      if( !cpSeed_hw_sample((BNU_CHUNK_T*)pRand))
         return 0;
   }

   #if (_IPP32E>=_IPP32E_E9)
   if( bufLen%((Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u))) ) {
      if( !cpSeed_hw_sample32(pRand)) {
         return 0;
      }
   }
   #endif
   return 1;
}
#endif

/*F*
// Name: ippsTRNGenRDSEED
//
// Purpose: Generates a true random bit sequence
//          based on Intel® instruction RDSEED of the specified nBits length.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pRand
//
//    ippStsLengthErr            1 > nBits
//
//    ippStsNotSupportedModeErr  unsupported rdrand instruction
//
//    ippStsErr                  random bit sequence can't be generated
//
//    ippStsNoErr                no error
//
// Parameters:
//    pRand    pointer to the buffer
//    nBits    number of bits be requested
//    pCtx     pointer to the context
*F*/
IPPFUN(IppStatus, ippsTRNGenRDSEED,(Ipp32u* pRand, int nBits, void* pCtx))
{
   /* test PRNG buffer */
   IPP_BAD_PTR1_RET(pRand);

   /* test sizes */
   IPP_BADARG_RET(nBits< 1, ippStsLengthErr);

   IPP_UNREFERENCED_PARAMETER(pCtx);

   #if ((_IPP>=_IPP_G9) || (_IPP32E>=_IPP32E_E9))
   if( IsFeatureEnabled(ippCPUID_RDSEED) ) {
      cpSize rndSize = BITS2WORD32_SIZE(nBits);
      Ipp32u rndMask = MAKEMASK32(nBits);

      if(cpSeedHW_buffer(pRand, rndSize)) {
         pRand[rndSize-1] &= rndMask;
         return ippStsNoErr;
      }
      else
         return ippStsErr;
   }
   /* unsupported rdrand instruction */
   else
   #endif
      IPP_ERROR_RET(ippStsNotSupportedModeErr);
}


/*F*
// Name: ippsTRNGenRDSEED_BN
//
// Purpose: Generates a true random big number
//          based on Intel® instruction RDSEED of the specified nBits length.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pRand
//
//    ippStsLengthErr            1 > nBits
//                               nBits > BN_ROOM(pRand)
//
//    ippStsNotSupportedModeErr  unsupported rdrand instruction
//
//    ippStsErr                  random big number can't be generated
//
//    ippStsNoErr                no error
//
// Parameters:
//    pRand  pointer to the big number
//    nBits    number of bits be requested
//    pCtx  pointer to the context
*F*/
IPPFUN(IppStatus, ippsTRNGenRDSEED_BN,(IppsBigNumState* pRand, int nBits, void* pCtx))
{
   /* test random BN */
   IPP_BAD_PTR1_RET(pRand);
   IPP_BADARG_RET(!BN_VALID_ID(pRand), ippStsContextMatchErr);

   /* test sizes */
   IPP_BADARG_RET(nBits< 1, ippStsLengthErr);
   IPP_BADARG_RET(nBits> BN_ROOM(pRand)*BNU_CHUNK_BITS, ippStsLengthErr);

   IPP_UNREFERENCED_PARAMETER(pCtx);

   #if ((_IPP>=_IPP_G9) || (_IPP32E>=_IPP32E_E9))
   if( IsFeatureEnabled(ippCPUID_RDSEED) ) {
      BNU_CHUNK_T* pRandBN = BN_NUMBER(pRand);
      cpSize rndSize = BITS_BNU_CHUNK(nBits);
      BNU_CHUNK_T rndMask = MASK_BNU_CHUNK(nBits);

      if(cpSeedHW_buffer((Ipp32u*)pRandBN, rndSize*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)))) {
         pRandBN[rndSize-1] &= rndMask;

         FIX_BNU(pRandBN, rndSize);
         BN_SIZE(pRand) = rndSize;
         BN_SIGN(pRand) = ippBigNumPOS;

         return ippStsNoErr;
      }
      else
         return ippStsErr;
   }

   /* unsupported rdrand instruction */
   else
   #endif
      IPP_ERROR_RET(ippStsNotSupportedModeErr);
}
