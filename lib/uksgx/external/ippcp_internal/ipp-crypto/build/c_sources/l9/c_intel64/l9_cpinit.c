/*******************************************************************************
* Copyright 2001-2021 Intel Corporation
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

//
//  Intel® Integrated Performance Primitives Cryptography (Intel® IPP Cryptography)
//

#include "owndefs.h"
#include "ippcpdefs.h"
#include "ippcp.h"
#include "dispatcher.h"

#if defined( _IPP_DATA )

static Ipp64u cpFeatures = 0;
static Ipp64u cpFeaturesMask = 0;

static int cpGetFeatures( Ipp64u* pFeaturesMask );

extern IPP_OWN_DECL (void, cpGetReg, ( int* buf, int valEAX, int valECX ))
extern IPP_OWN_DECL (int,  cp_is_avx_extension, ( void ))
extern IPP_OWN_DECL (int,  cp_is_avx512_extension, ( void ))
extern IPP_OWN_DECL (int,  cp_issue_avx512_instruction, ( void ))

IppStatus owncpSetCpuFeaturesAndIdx( Ipp64u cpuFeatures, int* index );

IPPFUN( Ipp64u, ippcpGetEnabledCpuFeatures, ( void ))
{
    return cpFeaturesMask;
}

/*===================================================================*/
IPPFUN( IppStatus, ippcpGetCpuFeatures, ( Ipp64u* pFeaturesMask ))
{
  IPP_BAD_PTR1_RET( pFeaturesMask )
  {
    if( 0 != cpFeatures){
        *pFeaturesMask = cpFeatures;// & cpFeaturesMask;
    } else {
        int ret = cpGetFeatures( pFeaturesMask );
        if( !ret ) return ippStsNotSupportedCpu;
    }
    return ippStsNoErr;
  }
}

/*===================================================================*/

int cpGetFeature( Ipp64u Feature )
{
  if(( cpFeaturesMask & Feature ) == Feature ){
    return 1;
  } else {
    return 0;
  }
}

/*===================================================================*/
#define BIT00 0x00000001
#define BIT01 0x00000002
#define BIT02 0x00000004
#define BIT03 0x00000008
#define BIT04 0x00000010
#define BIT05 0x00000020
#define BIT06 0x00000040
#define BIT07 0x00000080
#define BIT08 0x00000100
#define BIT09 0x00000200
#define BIT10 0x00000400
#define BIT11 0x00000800
#define BIT12 0x00001000
#define BIT13 0x00002000
#define BIT14 0x00004000
#define BIT15 0x00008000
#define BIT16 0x00010000
#define BIT17 0x00020000
#define BIT18 0x00040000
#define BIT19 0x00080000
#define BIT20 0x00100000
#define BIT21 0x00200000
#define BIT22 0x00400000
#define BIT23 0x00800000
#define BIT24 0x01000000
#define BIT25 0x02000000
#define BIT26 0x04000000
#define BIT27 0x08000000
#define BIT28 0x10000000
#define BIT29 0x20000000
#define BIT30 0x40000000
#define BIT31 0x80000000


static int cpGetFeatures( Ipp64u* pFeaturesMask )
{
    Ipp32u  buf[4];
    Ipp32u  eax_, ebx_, ecx_, edx_, tmp;
    Ipp64u  mask;
    int flgFMA=0, flgINT=0, flgGPR=0;   // for avx2
    Ipp32u idBaseMax, idExtdMax;

    cpGetReg((int*)buf, 0, 0);          //get max value for basic info.
    idBaseMax = buf[0];
    cpGetReg((int*)buf, (Ipp32s)0x80000000, 0); //get max value for extended info.
    idExtdMax = buf[0];

    cpGetReg( (int*)buf, 1, 0 );
    eax_ = (Ipp32u)buf[0];
    ecx_ = (Ipp32u)buf[2];
    edx_ = (Ipp32u)buf[3];
    mask = 0;
    if( edx_ & BIT23 ) mask |= ippCPUID_MMX;          // edx[23] - MMX(TM) Technology
    if( edx_ & BIT25 ) mask |= ippCPUID_SSE;          // edx[25] - Intel® Streaming SIMD Extensions (Intel® SSE)
    if( edx_ & BIT26 ) mask |= ippCPUID_SSE2;         // edx[26] - Intel® Streaming SIMD Extensions 2 (Intel® SSE2)
    if( ecx_ & BIT00 ) mask |= ippCPUID_SSE3;         // ecx[0]  - Intel® Streaming SIMD Extensions 3 (Intel® SSE3) (formerly codenamed Prescott)
    if( ecx_ & BIT09 ) mask |= ippCPUID_SSSE3;        // ecx[9]  - Supplemental Streaming SIMD Extensions 3 (SSSE3) (formerly codenamed Merom)
    if( ecx_ & BIT22 ) mask |= ippCPUID_MOVBE;        // ecx[22] - Intel® instruction MOVBE (Intel Atom® processor)
    if( ecx_ & BIT19 ) mask |= ippCPUID_SSE41;        // ecx[19] - Intel® Streaming SIMD Extensions 4.1 (Intel® SSE4.1) (formerly codenamed Penryn)
    if( ecx_ & BIT20 ) mask |= ippCPUID_SSE42;        // ecx[20] - Intel® Streaming SIMD Extensions 4.2 (Intel® SSE4.2) (formerly codenamed Nenalem)
    if( ecx_ & BIT28 ) mask |= ippCPUID_AVX;          // ecx[28] - Intel® Advanced Vector Extensions (Intel® AVX) (formerly codenamed Sandy Bridge)
    if(( ecx_ & 0x18000000 ) == 0x18000000 ){
        tmp = (Ipp32u)cp_is_avx_extension();
        if( tmp & BIT00 ) mask |= ippAVX_ENABLEDBYOS; // Intel® AVX is supported by OS
    }
    if( ecx_ & BIT25 ) mask |= ippCPUID_AES;          // ecx[25] - Intel® AES New Instructions
    if( ecx_ & BIT01 ) mask |= ippCPUID_CLMUL;        // ecx[1]  - Intel® instruction PCLMULQDQ
    if( ecx_ & BIT30 ) mask |= ippCPUID_RDRAND;       // ecx[30] - Intel® instruction RDRRAND
    if( ecx_ & BIT29 ) mask |= ippCPUID_F16C;         // ecx[29] - Intel® instruction F16C
         // Intel® AVX2 instructions extention: only if 3 features are enabled at once:
         // FMA, Intel® AVX 256 int & GPR BMI (bit-manipulation);
    if( ecx_ & BIT12 ) flgFMA = 1; else flgFMA = 0;   // ecx[12] - FMA 128 & 256 bit
    if( idBaseMax >= 7 ){                             // get CPUID.eax = 7
       cpGetReg( (int*)buf, 0x7, 0 );
       ebx_ = (Ipp32u)buf[1];
       ecx_ = (Ipp32u)buf[2];
       edx_ = (Ipp32u)buf[3];
       if( ebx_ & BIT05 ) flgINT = 1;
       else flgINT = 0;                               //ebx[5], Intel® Advanced Vector Extensions 2 (Intel® AVX2) (int 256bits)
           // ebx[3] - enabled ANDN, BEXTR, BLSI, BLSMK, BLSR, TZCNT
           // ebx[8] - enabled BZHI, MULX, PDEP, PEXT, RORX, SARX, SHLX, SHRX
       if(( ebx_ & BIT03 )&&( ebx_ & BIT08 )) flgGPR = 1; 
       else flgGPR = 0;                               // VEX-encoded GPR instructions (GPR BMI)
           // Intel® architecture formerly codenamed Broadwell instructions extention
       if( ebx_ & BIT19 ) mask |= ippCPUID_ADCOX;     // eax[0x7] -->> ebx:: Bit 19: Intel® instructions ADOX/ADCX
       if( ebx_ & BIT18 ) mask |= ippCPUID_RDSEED;    // eax[0x7] -->> ebx:: Bit 18: Intel® instruction RDSEED
       if( ebx_ & BIT29 ) mask |= ippCPUID_SHA;       // eax[0x7] -->> ebx:: Bit 29: Intel® Secure Hash Algorithm Extensions

       // Intel® Advanced Vector Extensions 512 (Intel® AVX-512) extention
       if( ebx_ & BIT16 ) mask |= ippCPUID_AVX512F;   // ebx[16] - Intel® AVX-512 Foundation
       if( ebx_ & BIT26 ) mask |= ippCPUID_AVX512PF;  // ebx[26] - Intel® AVX-512 Pre Fetch Instructions (PFI)
       if( ebx_ & BIT27 ) mask |= ippCPUID_AVX512ER;  // ebx[27] - Intel® AVX-512 Exponential and Reciprocal Instructions (ERI)
       if( ebx_ & BIT28 ) mask |= ippCPUID_AVX512CD;  // ebx[28] - Intel® AVX-512 Conflict Detection
       if( ebx_ & BIT17 ) mask |= ippCPUID_AVX512DQ;  // ebx[17] - Intel® AVX-512 Dword & Quadword
       if( ebx_ & BIT30 ) mask |= ippCPUID_AVX512BW;  // ebx[30] - Intel® AVX-512 Byte & Word
       if( ebx_ & BIT31 ) mask |= ippCPUID_AVX512VL;  // ebx[31] - Intel® AVX-512 Vector Length Extensions (VLE)
       if( ecx_ & BIT01 ) mask |= ippCPUID_AVX512VBMI; // ecx[01] - Intel® AVX-512 Vector Bit Manipulation Instructions
       if( ecx_ & BIT06 ) mask |= ippCPUID_AVX512VBMI2; // ecx[06] - Intel® AVX-512 Vector Bit Manipulation Instructions 2
       if( edx_ & BIT02 ) mask |= ippCPUID_AVX512_4VNNIW; // edx[02] - Intel® AVX-512 Vector instructions for deep learning enhanced word variable precision
       if( edx_ & BIT03 ) mask |= ippCPUID_AVX512_4FMADDPS; // edx[03] - Intel® AVX-512 Vector instructions for deep learning floating-point single precision
       // bitwise OR between ippCPUID_MPX & ippCPUID_AVX flags can be used to define that arch is GE than formerly codenamed Skylake
       if( ebx_ & BIT14 ) mask |= ippCPUID_MPX;       // ebx[14] - Intel® Memory Protection Extensions (Intel® MPX)
       if( ebx_ & BIT21 ) mask |= ippCPUID_AVX512IFMA;  // ebx[21] - Intel® AVX-512 IFMA PMADD52

       if (ecx_ & BIT08) mask |= ippCPUID_AVX512GFNI;    // test bit ecx[08]
       if (ecx_ & BIT09) mask |= ippCPUID_AVX512VAES;    // test bit ecx[09]
       if (ecx_ & BIT10) mask |= ippCPUID_AVX512VCLMUL;  // test bit ecx[10]

       if (mask & ippCPUID_AVX512F) {
          /* test if Intel® AVX-512 is supported by OS */
          if (cp_is_avx512_extension())
            mask |= ippAVX512_ENABLEDBYOS;

          #if defined(OSXEM64T)
          else {
             /* submit some avx512f instructions, returns 0 */
             mask |= cp_issue_avx512_instruction();
             /* and check OS support again */
             if (cp_is_avx512_extension())
                mask |= ippAVX512_ENABLEDBYOS;
          }
          #endif
       }
    }
    mask = ( flgFMA && flgINT && flgGPR ) ? (mask | ippCPUID_AVX2) : mask; // to separate Intel® AVX2 flags here

    if( idExtdMax >= 0x80000001 ){ // get CPUID.eax=0x80000001
       cpGetReg( (int*)buf, (Ipp32s)0x80000001, 0 );
       ecx_ = (Ipp32u)buf[2];
           // Intel® architecture formerly codenamed Broadwell instructions extention
       if( ecx_ & BIT08 ) mask |= ippCPUID_PREFETCHW; // eax[0x80000001] -->> ecx:: Bit 8: Intel® instruction PREFETCHW
    }
       // Intel® architecture formerly codenamed Knights Corner
    if(((( eax_ << 20 ) >> 24 ) ^ 0xb1 ) == 0 ){
        mask = mask | ippCPUID_KNC;
    }
    cpFeatures = mask;
    cpFeaturesMask = mask; /* all CPU features are enabled by default */
    *pFeaturesMask = cpFeatures;
    return 1; /* if somebody need to check for cpuid support - do it at the top of function and return 0 if it's not supported */
}

int ippcpJumpIndexForMergedLibs = -1;
static int cpthreads_omp_of_n_ipp = 1;

IPPFUN( int, ippcpGetEnabledNumThreads,( void ))
{
    return cpthreads_omp_of_n_ipp;
}


#define AVX3X_FEATURES ( ippCPUID_AVX512F|ippCPUID_AVX512CD|ippCPUID_AVX512VL|ippCPUID_AVX512BW|ippCPUID_AVX512DQ )
#define AVX3M_FEATURES ( ippCPUID_AVX512F|ippCPUID_AVX512CD|ippCPUID_AVX512PF|ippCPUID_AVX512ER )
// AVX3X_FEATURES means Intel® Xeon® processor
// AVX3M_FEATURES means Intel® Many Integrated Core Architecture
#define AVX3I_FEATURES ( AVX3X_FEATURES| \
                         ippCPUID_SHA|ippCPUID_AVX512VBMI|ippCPUID_AVX512VBMI2| \
                         ippCPUID_AVX512IFMA| \
                         ippCPUID_AVX512GFNI|ippCPUID_AVX512VAES|ippCPUID_AVX512VCLMUL)


IppStatus owncpFeaturesToIdx(  Ipp64u* cpuFeatures, int* index )
{
   IppStatus ownStatus = ippStsNoErr;
   Ipp64u    mask = 0;

   *index = 0;

   if(( AVX3I_FEATURES  == ( *cpuFeatures & AVX3I_FEATURES  ))&&
      ( ippAVX512_ENABLEDBYOS & cpFeatures )){                         /* Intel® architecture formerly codenamed Icelake ia32=S0, x64=K0 */
         mask = AVX3I_MSK;
         *index = LIB_AVX3I;
   } else
   if(( AVX3X_FEATURES  == ( *cpuFeatures & AVX3X_FEATURES  ))&&
      ( ippAVX512_ENABLEDBYOS & cpFeatures )){                         /* Intel® architecture formerly codenamed Skylake ia32=S0, x64=K0 */
         mask = AVX3X_MSK;
         *index = LIB_AVX3X;
   } else 
   if(( AVX3M_FEATURES  == ( *cpuFeatures & AVX3M_FEATURES  ))&&
      ( ippAVX512_ENABLEDBYOS & cpFeatures )){                         /* Intel® architecture formerly codenamed Knights Landing ia32=i0, x64=N0 */
       mask = AVX3M_MSK;
       *index = LIB_AVX3M;
   } else 
   if(( ippCPUID_AVX2  == ( *cpuFeatures & ippCPUID_AVX2  ))&&
      ( ippAVX_ENABLEDBYOS & cpFeatures )){                            /* Intel® architecture formerly codenamed Haswell ia32=H9, x64=L9 */
       mask = AVX2_MSK;
       *index = LIB_AVX2;
   } else 
   if(( ippCPUID_AVX   == ( *cpuFeatures & ippCPUID_AVX   ))&&
      ( ippAVX_ENABLEDBYOS & cpFeatures )){                            /* Intel® architecture formerly codenamed Sandy Bridge ia32=G9, x64=E9 */
       mask = AVX_MSK;
       *index = LIB_AVX;
   } else 
   if( ippCPUID_SSE42 == ( *cpuFeatures & ippCPUID_SSE42 )){           /* Intel® microarchitecture code name Nehalem or Intel® architecture formerly codenamed Westmer = Intel® architecture formerly codenamed Penryn + Intel® SSE4.2 + ?Intel® instruction PCLMULQDQ + ?(Intel® AES New Instructions) + ?(Intel® Secure Hash Algorithm Extensions) */ 
       mask = SSE42_MSK;                                               /* or new Intel Atom® processor formerly codenamed Silvermont */
       *index = LIB_SSE42;
   } else 
   if( ippCPUID_SSE41 == ( *cpuFeatures & ippCPUID_SSE41 )){           /* Intel® architecture formerly codenamed Penryn ia32=P8, x64=Y8 */
       mask = SSE41_MSK;
       *index = LIB_SSE41;
   } else 
   if( ippCPUID_MOVBE == ( *cpuFeatures & ippCPUID_MOVBE )) {          /* Intel Atom® processor formerly codenamed Silverthorne ia32=S8, x64=N8 */
       mask = ATOM_MSK;
       *index = LIB_ATOM;
   } else 
   if( ippCPUID_SSSE3 == ( *cpuFeatures & ippCPUID_SSSE3 )) {          /* Intel® architecture formerly codenamed Merom ia32=V8, x64=U8 (letters etymology is unknown) */
       mask = SSSE3_MSK;
       *index = LIB_SSSE3;
   } else 
   if( ippCPUID_SSE3  == ( *cpuFeatures & ippCPUID_SSE3  )) {          /* Intel® architecture formerly codenamed Prescott ia32=W7, x64=M7 */
       mask = SSE3_MSK;
       *index = LIB_SSE3;
   } else 
   if( ippCPUID_SSE2  == ( *cpuFeatures & ippCPUID_SSE2  )) {          /* Intel® architecture formerly codenamed Willamette ia32=W7, x64=PX */
       mask = SSE2_MSK;
       *index = LIB_SSE2;
   } else 
   if( ippCPUID_SSE   == ( *cpuFeatures & ippCPUID_SSE   )) {          /* Intel® Pentium® processor III ia32=PX only */
       mask = SSE_MSK;
       *index = LIB_SSE;
#if (defined( WIN32E ) || defined( LINUX32E ) || defined( OSXEM64T )) && !(defined( _ARCH_LRB2 ))
       ownStatus = ippStsNotSupportedCpu;                              /* the lowest CPU supported by Intel IPP Cryptography must at least support Intel® SSE2 for x64 */
#endif
   } else 
   if( ippCPUID_MMX   >= ( *cpuFeatures & ippCPUID_MMX   )) {          /* not supported, PX dispatched */
       mask = MMX_MSK;
       *index = LIB_MMX;
       ownStatus = ippStsNotSupportedCpu; /* the lowest CPU supported by Intel IPP Cryptography must at least support Intel® SSE for ia32 or Intel® SSE2 for x64 */
   } 
#if defined ( _IPP_QUARK)
     else {
       mask = PX_MSK;
       *index = LIB_PX;
       ownStatus = ippStsNoErr; /* the lowest CPU supported by Intel IPP Cryptography must at least support Intel® SSE for ia32 or Intel® SSE2 for x64 */
   }
#endif

    if(( mask != ( *cpuFeatures & mask ))&&( ownStatus == ippStsNoErr )) 
        ownStatus = ippStsFeaturesCombination; /* warning if combination of features is incomplete */
   *cpuFeatures |= mask;
   return ownStatus;
}


IPPFUN(IppStatus, ippcpSetNumThreads, (int numThr))
{
   IppStatus status = ippStsNoErr;

   IPP_UNREFERENCED_PARAMETER(numThr);
   status = ippStsNoOperation;

   return status;
}

IPPFUN(IppStatus, ippcpGetNumThreads, (int* pNumThr))
{
   IppStatus status = ippStsNoErr;
   IPP_BAD_PTR1_RET(pNumThr)

      *pNumThr = 1;
   status = ippStsNoOperation;

   return status;
}

IPPFUN( IppStatus, ippcpInit,( void ))
{
    Ipp64u     cpuFeatures;

    ippcpGetCpuFeatures( &cpuFeatures );
    return ippcpSetCpuFeatures( cpuFeatures );
}


IPPFUN( IppStatus, ippcpSetCpuFeatures,( Ipp64u cpuFeatures ))
{
   IppStatus ownStatus;
   int       index = 0;

    ownStatus = owncpSetCpuFeaturesAndIdx( cpuFeatures, &index );
    ippcpJumpIndexForMergedLibs = index; 
    cpFeaturesMask = cpuFeatures;
    return ownStatus;
}

IppStatus owncpSetCpuFeaturesAndIdx(Ipp64u cpuFeatures, int* index)
{
   Ipp64u    tmp;
   IppStatus tmpStatus;
   *index = 0;

   if (ippCPUID_NOCHECK & cpuFeatures) {
      // if NOCHECK is set - static variable cpFeatures is initialized unconditionally and real CPU features from CPUID are ignored;
      // the one who uses this method of initialization must understand what and why it does and the possible unpredictable consequences.
      // the only one known purpose for this approach - environments where CPUID instruction is disabled (for example Intel® Software Guard Extensions).
      cpuFeatures &= (IPP_MAX_64U ^ ippCPUID_NOCHECK);
      cpFeatures = cpuFeatures;
   }
   else
      /* read cpu features unconditionally */
      cpGetFeatures(&tmp);

   tmpStatus = owncpFeaturesToIdx(&cpuFeatures, index);
   cpFeaturesMask = cpuFeatures;

   return tmpStatus;
}

static struct {
   int sts;
   const char *msg;
} ippcpMsg[] = {
/* ippStatus */
/* -9999 */ ippStsCpuNotSupportedErr, "ippStsCpuNotSupportedErr: The target CPU is not supported",
/* -9702 */ MSG_NO_SHARED, "No shared libraries were found in the Waterfall procedure",
/* -9701 */ MSG_NO_DLL, "No DLLs were found in the Waterfall procedure",
/* -9700 */ MSG_LOAD_DLL_ERR, "Error at loading of %s library",
/* -1016 */ ippStsQuadraticNonResidueErr, "ippStsQuadraticNonResidueErr: SQRT operation on quadratic non-residue value",
/* -1015 */ ippStsPointAtInfinity, "ippStsPointAtInfinity: Point at infinity is detected",
/* -1014 */ ippStsOFBSizeErr, "ippStsOFBSizeErr: Incorrect value for crypto OFB block size",
/* -1013 */ ippStsIncompleteContextErr, "ippStsIncompleteContextErr: Crypto: set up of context is not complete",
/* -1012 */ ippStsCTRSizeErr, "ippStsCTRSizeErr: Incorrect value for crypto CTR block size",
/* -1011 */ ippStsEphemeralKeyErr, "ippStsEphemeralKeyErr: ECC: Invalid ephemeral key", 
/* -1010 */ ippStsMessageErr, "ippStsMessageErr: ECC: Invalid message digest",
/* -1009 */ ippStsShareKeyErr, "ippStsShareKeyErr: ECC: Invalid share key", 
/* -1008 */ ippStsIvalidPrivateKey, "ippStsIvalidPrivateKey ECC: Invalid private key",
/* -1007 */ ippStsOutOfECErr, "ippStsOutOfECErr: ECC: Point out of EC",
/* -1006 */ ippStsECCInvalidFlagErr, "ippStsECCInvalidFlagErr: ECC: Invalid Flag",
/* -1005 */ ippStsUnderRunErr, "ippStsUnderRunErr: Error in data under run",
/* -1004 */ ippStsPaddingErr, "ippStsPaddingErr: Detected padding error indicates the possible data corruption",
/* -1003 */ ippStsCFBSizeErr, "ippStsCFBSizeErr: Incorrect value for crypto CFB block size",
/* -1002 */ ippStsPaddingSchemeErr, "ippStsPaddingSchemeErr: Invalid padding scheme",
/* -1001 */ ippStsBadModulusErr, "ippStsBadModulusErr: Bad modulus caused a failure in module inversion",
/*  -216 */ ippStsUnknownStatusCodeErr, "ippStsUnknownStatusCodeErr: Unknown status code",
/*  -221 */ ippStsLoadDynErr, "ippStsLoadDynErr: Error when loading the dynamic library",
/*   -15 */ ippStsLengthErr, "ippStsLengthErr: Incorrect value for string length",
/*   -14 */ ippStsNotSupportedModeErr, "ippStsNotSupportedModeErr: The requested mode is currently not supported",
/*   -13 */ ippStsContextMatchErr, "ippStsContextMatchErr: Context parameter does not match the operation",
/*   -12 */ ippStsScaleRangeErr, "ippStsScaleRangeErr: Scale bounds are out of range",
/*   -11 */ ippStsOutOfRangeErr, "ippStsOutOfRangeErr: Argument is out of range, or point is outside the image",
/*   -10 */ ippStsDivByZeroErr, "ippStsDivByZeroErr: An attempt to divide by zero",
/*    -9 */ ippStsMemAllocErr, "ippStsMemAllocErr: Memory allocated for the operation is not enough",
/*    -8 */ ippStsNullPtrErr, "ippStsNullPtrErr: Null pointer error",
/*    -7 */ ippStsRangeErr, "ippStsRangeErr: Incorrect values for bounds: the lower bound is greater than the upper bound",
/*    -6 */ ippStsSizeErr, "ippStsSizeErr: Incorrect value for data size",
/*    -5 */ ippStsBadArgErr, "ippStsBadArgErr: Incorrect arg/param of the function",
/*    -4 */ ippStsNoMemErr, "ippStsNoMemErr: Not enough memory for the operation",
/*    -2 */ ippStsErr, "ippStsErr: Unknown/unspecified error, -2",
/*     0 */ ippStsNoErr, "ippStsNoErr: No errors",
/*     1 */ ippStsNoOperation, "ippStsNoOperation: No operation has been executed",
/*     2 */ ippStsDivByZero, "ippStsDivByZero: Zero value(s) for the divisor in the Div function",
/*    25 */ ippStsInsufficientEntropy, "ippStsInsufficientEntropy: Generation of the prime/key failed due to insufficient entropy in the random seed and stimulus bit string",
/*    36 */ ippStsNotSupportedCpu, "The CPU is not supported",
/*    51 */ ippStsFeaturesCombination, "Wrong combination of features",
/*    53 */ ippStsMbWarning, "ippStsMbWarning: Error(s) in statuses array",
};

/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippcpGetStatusString
//  Purpose:    transformation of a code of a status Intel IPP Cryptography to string
//  Returns:
//  Parameters:
//    StsCode   Intel IPP Cryptography status code
//
//  Notes:      not necessary to release the returned string
*/
IPPFUN( const char*, ippcpGetStatusString, ( IppStatus StsCode ) )
{
   unsigned int i;
   for( i=0; i<IPP_COUNT_OF( ippcpMsg ); i++ ) {
      if( StsCode == ippcpMsg[i].sts ) { 
         return ippcpMsg[i].msg;
      }
   }
   return ippcpGetStatusString( ippStsUnknownStatusCodeErr );
}

extern IPP_OWN_DECL (Ipp64u, cp_get_pentium_counter, (void))

/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippcpGetCpuClocks
//  Purpose:    time stamp counter (TSC) register reading
//  Returns:    TSC value
//
//  Note:      An hardware exception is possible if TSC reading is not supported by
//             the current chipset
*/
IPPFUN( Ipp64u, ippcpGetCpuClocks, (void) )
{
   return (Ipp64u)cp_get_pentium_counter();
}

#endif /* _IPP_DATA */
