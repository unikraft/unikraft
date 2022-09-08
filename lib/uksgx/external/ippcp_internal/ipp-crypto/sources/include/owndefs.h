/*******************************************************************************
* Copyright 1999-2021 Intel Corporation
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
//  Purpose:
//     Internal definitions
//
//

#ifndef __OWNDEFS_H__
#define __OWNDEFS_H__

#if defined( _VXWORKS )
  #include <vxWorks.h>
  #undef NONE
#endif

#ifndef IPPCPDEFS_H__
  #include "ippcpdefs.h"
#endif

#if !defined(__INLINE)
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
  #define __INLINE static __inline
#elif defined( __GNUC__ )
  #define __INLINE static __inline__
#else
  #define __INLINE static
#endif
#endif /*__INLINE*/

#if !defined(__FORCEINLINE)
#if defined(_MSC_VER)
  #define __FORCEINLINE __forceinline
#elif defined(__INTEL_COMPILER) || defined( __GNUC__ )
  #define __FORCEINLINE __INLINE __attribute__((always_inline))
#else
  #define __FORCEINLINE
#endif
#endif /*__FORCEINLINE*/

#if defined(__INTEL_COMPILER)
 #define __RESTRICT restrict
#elif !defined( __RESTRICT )
 #define __RESTRICT
#endif

#if defined( IPP_W32DLL )
  #if defined( _MSC_VER ) || defined( __INTEL_COMPILER )
    #define IPPDEF(type) __declspec(dllexport) type
  #else
    #define IPPDEF(type) type
  #endif
#else
  #define   IPPDEF(type) type
#endif

/* ia32 */
#define _IPP_PX 0    /* pure C-code                                                                                                            */
#define _IPP_M5 1    /* Intel® Quark(TM) processor                                                                                             */
#define _IPP_W7 8    /* Intel® Streaming SIMD Extensions 2 (Intel® SSE2)                                                                       */
#define _IPP_T7 16   /* Intel® Streaming SIMD Extensions 3 (Intel® SSE3)                                                                       */
#define _IPP_V8 32   /* Supplemental Streaming SIMD Extensions 3 (SSSE3)                                                                       */
#define _IPP_S8 33   /* Supplemental Streaming SIMD Extensions 3 (SSSE3) + MOVBE instruction                                                   */
#define _IPP_P8 64   /* Intel® Streaming SIMD Extensions 4.2 (Intel® SSE4.2)                                                                   */
#define _IPP_G9 128  /* Intel® Advanced Vector Extensions (Intel® AVX)                                                                         */
#define _IPP_H9 256  /* Intel® Advanced Vector Extensions 2 (Intel® AVX2)                                                                      */
#define _IPP_I0 512  /* Intel® Advanced Vector Extensions 512 (Intel® AVX512) - Intel® Xeon Phi(TM) Processor (formerly Knights Landing)       */
#define _IPP_S0 1024 /* Intel® Advanced Vector Extensions 512 (Intel® AVX512) - Intel® Xeon® Processor (formerly codenamed Skylake)            */

/* intel64 */
#define _IPP32E_PX _IPP_PX /* pure C-code                                                                                                      */
#define _IPP32E_M7 32      /* Intel® Streaming SIMD Extensions 3 (Intel® SSE3)                                                                 */
#define _IPP32E_U8 64      /* Supplemental Streaming SIMD Extensions 3 (SSSE3)                                                                 */
#define _IPP32E_N8 65      /* Supplemental Streaming SIMD Extensions 3 (SSSE3) + MOVBE instruction                                             */
#define _IPP32E_Y8 128     /* Intel® Streaming SIMD Extensions 4.2 (Intel® SSE4.2)                                                             */
#define _IPP32E_E9 256     /* Intel® Advanced Vector Extensions (Intel® AVX)                                                                   */
#define _IPP32E_L9 512     /* Intel® Advanced Vector Extensions 2 (Intel® AVX2)                                                                */
#define _IPP32E_N0 1024    /* Intel® Advanced Vector Extensions 512 (Intel® AVX512) - Intel® Xeon Phi(TM) Processor (formerly Knights Landing) */
#define _IPP32E_K0 2048    /* Intel® Advanced Vector Extensions 512 (Intel® AVX512) - Intel® Xeon® Processor (formerly codenamed Skylake)      */
#define _IPP32E_K1 4096    /* Intel® Advanced Vector Extensions 512 (Intel® AVX512) - Intel® Xeon® Processor (formerly codenamed Icelake)      */


#if defined(__INTEL_COMPILER) || (_MSC_VER >= 1300)
    #define __ALIGN8  __declspec (align(8))
    #define __ALIGN16 __declspec (align(16))
    #define __ALIGN32 __declspec (align(32))
    #if !defined(__ALIGN64)
        #define __ALIGN64 __declspec (align(64))
    #endif/*__ALIGN64*/
#elif defined(__GNUC__)
    #define __ALIGN8  __attribute__((aligned(8)))
    #define __ALIGN16 __attribute__((aligned(16)))
    #define __ALIGN32 __attribute__((aligned(32)))
    #if !defined(__ALIGN64)
        #define __ALIGN64 __attribute__((aligned(64)))
    #endif/*__ALIGN64*/
#else
   #error Intel, MS or GNU C compiler required
#endif

/* ia32 */
#if defined ( _M5 ) /* Intel® Quark(TM) processor */
  #define _IPP    _IPP_M5
  #define _IPP32E _IPP32E_PX
  #define OWNAPI(name) m5_##name

#elif defined( _W7 ) /* Intel® SSE2 */
  #define _IPP    _IPP_W7
  #define _IPP32E _IPP32E_PX
  #define OWNAPI(name) w7_##name

#elif defined( _T7 ) /* Intel® SSE3 */
  #define _IPP    _IPP_T7
  #define _IPP32E _IPP32E_PX
  #define OWNAPI(name) t7_##name

#elif defined( _V8 ) /* SSSE3 */
  #define _IPP    _IPP_V8
  #define _IPP32E _IPP32E_PX
  #define OWNAPI(name) v8_##name

#elif defined( _S8 ) /* SSSE3 + MOVBE instruction */
  #define _IPP    _IPP_S8
  #define _IPP32E _IPP32E_PX
  #define OWNAPI(name) s8_##name

#elif defined( _P8 ) /* Intel® SSE4.2 */
  #define _IPP    _IPP_P8
  #define _IPP32E _IPP32E_PX
  #define OWNAPI(name) p8_##name

#elif defined( _G9 ) /* Intel® AVX */
  #define _IPP    _IPP_G9
  #define _IPP32E _IPP32E_PX
  #define OWNAPI(name) g9_##name

#elif defined( _H9 ) /* Intel® AVX2 */
  #define _IPP    _IPP_H9
  #define _IPP32E _IPP32E_PX
  #define OWNAPI(name) h9_##name

/* intel64 */
#elif defined( _M7 ) /* Intel® SSE3 */
  #define _IPP    _IPP_PX
  #define _IPP32E _IPP32E_M7
  #define OWNAPI(name) m7_##name

#elif defined( _U8 ) /* SSSE3 */
  #define _IPP    _IPP_PX
  #define _IPP32E _IPP32E_U8
  #define OWNAPI(name) u8_##name

#elif defined( _N8 ) /* SSSE3 + MOVBE instruction */
  #define _IPP    _IPP_PX
  #define _IPP32E _IPP32E_N8
  #define OWNAPI(name) n8_##name

#elif defined( _Y8 ) /* Intel® SSE4.2 */
  #define _IPP    _IPP_PX
  #define _IPP32E _IPP32E_Y8
  #define OWNAPI(name) y8_##name

#elif defined( _E9 ) /* Intel® AVX */
  #define _IPP    _IPP_PX
  #define _IPP32E _IPP32E_E9
  #define OWNAPI(name) e9_##name

#elif defined( _L9 ) /* Intel® AVX2 */
  #define _IPP    _IPP_PX
  #define _IPP32E _IPP32E_L9
  #define OWNAPI(name) l9_##name

#elif defined( _N0 ) /* Intel® AVX512 - formerly Knights Landing */
  #define _IPP    _IPP_PX
  #define _IPP32E _IPP32E_N0
  #define OWNAPI(name) n0_##name

#elif defined( _K0 ) /* Intel® AVX512 - formerly codenamed Skylake */
  #define _IPP    _IPP_PX
  #define _IPP32E _IPP32E_K0
  #define OWNAPI(name) k0_##name

#elif defined( _K1 ) /* Intel® AVX512 - formerly codenamed Icelake */
#define _IPP    _IPP_PX
#define _IPP32E _IPP32E_K1
#define OWNAPI(name) k1_##name

#else
  #define _IPP    _IPP_PX
  #define _IPP32E _IPP32E_PX
 #if defined (_M_AMD64) || defined (__x86_64__) || defined ( _ARCH_EM64T )
  #define OWNAPI(name) mx_##name
 #else
  #define OWNAPI(name) px_##name
 #endif
#endif

#if defined(_MERGED_BLD)
#define _OWN_MERGED_BLD
#endif

#ifndef _OWN_MERGED_BLD
  #undef OWNAPI
  #define OWNAPI(name) name
#endif

/* Force __cdecl calling convention for internal functions declarations */
#define IPP_OWN_DECL(type,name,param) type IPP_CDECL name param ;
#define IPP_OWN_DEFN(type,name,param) type IPP_CDECL name param
#define IPP_OWN_FUNPTR(type,name,param) typedef type (IPP_CDECL *name) param ;

#if defined( IPP_W32DLL )
  #if defined( _MSC_VER ) || defined( __INTEL_COMPILER )
    #define IPPFUN(type,name,arg) __declspec(dllexport) type IPP_CALL name arg
  #else
    #define IPPFUN(type,name,arg)                extern type IPP_CALL name arg
  #endif
#else
  #if defined(LINUX32E) && !defined(IPP_PIC)
    #define IPPFUN(type,name,arg) __attribute__((force_align_arg_pointer)) extern type IPP_CALL name arg
  #else
    #define   IPPFUN(type,name,arg)                extern type IPP_CALL name arg
  #endif
#endif

#define _IPP_ARCH_IA32    1
#define _IPP_ARCH_EM64T   4
#define _IPP_ARCH_LRB     16
#define _IPP_ARCH_LRB2    128
#define _IPP_ARCH_LP64    0

#if defined ( _ARCH_IA32 )
  #define _IPP_ARCH    _IPP_ARCH_IA32

#elif defined( _ARCH_EM64T )
  #define _IPP_ARCH    _IPP_ARCH_EM64T

#else
  #if defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__)
    #define _IPP_ARCH    _IPP_ARCH_EM64T

  #else
    #define _IPP_ARCH    _IPP_ARCH_IA32

  #endif
#endif

#if ((_IPP_ARCH == _IPP_ARCH_IA32))
__INLINE Ipp32s IPP_INT_PTR ( const void* ptr )
{
    union {
        void*   Ptr;
        Ipp32s  Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}

__INLINE Ipp32u IPP_UINT_PTR( const void* ptr )
{
    union {
        void*   Ptr;
        Ipp32u  Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}
#elif ((_IPP_ARCH == _IPP_ARCH_EM64T) || (_IPP_ARCH == _IPP_ARCH_LRB2))
__INLINE Ipp64s IPP_INT_PTR( const void* ptr )
{
    union {
        void*   Ptr;
        Ipp64s  Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}

__INLINE Ipp64u IPP_UINT_PTR( const void* ptr )
{
    union {
        void*    Ptr;
        Ipp64u   Int;
    } dd;
    dd.Ptr = (void*)ptr;
    return dd.Int;
}
#else
  #define IPP_INT_PTR( ptr )  ( (long)(ptr) )
  #define IPP_UINT_PTR( ptr ) ( (unsigned long)(ptr) )
#endif

#define IPP_ALIGN_TYPE(type, align) ((align)/sizeof(type)-1)
#define IPP_BYTES_TO_ALIGN(ptr, align) ((~(IPP_INT_PTR(ptr)&((align)-1))+1)&((align)-1))
#define IPP_ALIGNED_PTR(ptr, align) (void*)( (unsigned char*)(ptr) + (IPP_BYTES_TO_ALIGN( ptr, align )) )

#define IPP_ALIGNED_SIZE(size, align) (((size)+(align)-1)&~((align)-1))

#define IPP_MALLOC_ALIGNED_BYTES   64
#define IPP_MALLOC_ALIGNED_8BYTES   8
#define IPP_MALLOC_ALIGNED_16BYTES 16
#define IPP_MALLOC_ALIGNED_32BYTES 32

#define IPP_ALIGNED_ARRAY(align,arrtype,arrname,arrlength)\
 char arrname##AlignedArrBuff[sizeof(arrtype)*(arrlength)+IPP_ALIGN_TYPE(char, align)];\
 arrtype *arrname = (arrtype*)IPP_ALIGNED_PTR(arrname##AlignedArrBuff,align)

#if defined( __cplusplus )
extern "C" {
#endif

/* /////////////////////////////////////////////////////////////////////////////

           Intel IPP Cryptography Context Identification

  /////////////////////////////////////////////////////////////////////////// */

#define IPP_CONTEXT( a, b, c, d) \
            (int)(((unsigned)(a) << 24) | ((unsigned)(b) << 16) | \
            ((unsigned)(c) << 8) | (unsigned)(d))

typedef enum {
    idCtxUnknown = 0,
    idCtxDES            = IPP_CONTEXT( ' ', 'D', 'E', 'S'),
    idCtxRijndael       = IPP_CONTEXT( ' ', 'R', 'I', 'J'),
    idCtxSMS4           = IPP_CONTEXT( 'S', 'M', 'S', '4'),
    idCtxARCFOUR        = IPP_CONTEXT( ' ', 'R', 'C', '4'),
    idCtxSHA1           = IPP_CONTEXT( 'S', 'H', 'S', '1'),
    idCtxSHA224         = IPP_CONTEXT( 'S', 'H', 'S', '3'),
    idCtxSHA256         = IPP_CONTEXT( 'S', 'H', 'S', '2'),
    idCtxSHA384         = IPP_CONTEXT( 'S', 'H', 'S', '4'),
    idCtxSHA512         = IPP_CONTEXT( 'S', 'H', 'S', '5'),
    idCtxMD5            = IPP_CONTEXT( ' ', 'M', 'D', '5'),
    idCtxHMAC           = IPP_CONTEXT( 'H', 'M', 'A', 'C'),
    idCtxBigNum         = IPP_CONTEXT( 'B', 'I', 'G', 'N'),
    idCtxMontgomery     = IPP_CONTEXT( 'M', 'O', 'N', 'T'),
    idCtxPrimeNumber    = IPP_CONTEXT( 'P', 'R', 'I', 'M'),
    idCtxPRNG           = IPP_CONTEXT( 'P', 'R', 'N', 'G'),
    idCtxRSA            = IPP_CONTEXT( ' ', 'R', 'S', 'A'),
    idCtxRSA_PubKey     = IPP_CONTEXT( 'R', 'S', 'A', '0'),
    idCtxRSA_PrvKey1    = IPP_CONTEXT( 'R', 'S', 'A', '1'),
    idCtxRSA_PrvKey2    = IPP_CONTEXT( 'R', 'S', 'A', '2'),
    idCtxDSA            = IPP_CONTEXT( ' ', 'D', 'S', 'A'),
    idCtxECCP           = IPP_CONTEXT( ' ', 'E', 'C', 'P'),
    idCtxECCB           = IPP_CONTEXT( ' ', 'E', 'C', 'B'),
    idCtxECCPPoint      = IPP_CONTEXT( 'P', 'E', 'C', 'P'),
    idCtxECCBPoint      = IPP_CONTEXT( 'P', 'E', 'C', 'B'),
    idCtxDH             = IPP_CONTEXT( ' ', ' ', 'D', 'H'),
    idCtxDLP            = IPP_CONTEXT( ' ', 'D', 'L', 'P'),
    idCtxCMAC           = IPP_CONTEXT( 'C', 'M', 'A', 'C'),
    idCtxAESXCBC,
    idCtxAESCCM,
    idCtxAESGCM,
    idCtxGFP,
    idCtxGFPE,
    idCtxGFPX,
    idCtxGFPXE,
    idCtxGFPXQX,
    idCtxGFPXQXE,
    idCtxGFPEC,
    idCtxGFPPoint,
    idCtxGFPXEC,
    idCtxGFPXECPoint,
    idCtxHash,
    idCtxSM3,
    idCtxAESXTS,
    idxCtxECES_SM2
} IppCtxId;




/* /////////////////////////////////////////////////////////////////////////////
           Helpers
  /////////////////////////////////////////////////////////////////////////// */

#define IPP_NOERROR_RET()  return ippStsNoErr
#define IPP_ERROR_RET( ErrCode )  return (ErrCode)

#define IPP_BADARG_RET( expr, ErrCode )\
            {if (expr) { IPP_ERROR_RET( ErrCode ); }}

#define IPP_BAD_SIZE_RET( n )\
            IPP_BADARG_RET( (n)<=0, ippStsSizeErr )

#define IPP_BAD_STEP_RET( n )\
            IPP_BADARG_RET( (n)<=0, ippStsStepErr )

#define IPP_BAD_PTR1_RET( ptr )\
            IPP_BADARG_RET( NULL==(ptr), ippStsNullPtrErr )

#define IPP_BAD_PTR2_RET( ptr1, ptr2 )\
            {IPP_BAD_PTR1_RET( ptr1 ); IPP_BAD_PTR1_RET( ptr2 )}

#define IPP_BAD_PTR3_RET( ptr1, ptr2, ptr3 )\
            {IPP_BAD_PTR2_RET( ptr1, ptr2 ); IPP_BAD_PTR1_RET( ptr3 )}

#define IPP_BAD_PTR4_RET( ptr1, ptr2, ptr3, ptr4 )\
            {IPP_BAD_PTR2_RET( ptr1, ptr2 ); IPP_BAD_PTR2_RET( ptr3, ptr4 )}

#define IPP_BAD_ISIZE_RET(roi) \
            IPP_BADARG_RET( ((roi).width<=0 || (roi).height<=0), ippStsSizeErr)

/* ////////////////////////////////////////////////////////////////////////// */
/*                              internal messages                             */

#define MSG_LOAD_DLL_ERR (-9700) /* Error at loading of %s library */
#define MSG_NO_DLL       (-9701) /* No DLLs were found in the Waterfall procedure */
#define MSG_NO_SHARED    (-9702) /* No shared libraries were found in the Waterfall procedure */

/* ////////////////////////////////////////////////////////////////////////// */


typedef union { /* double precision */
    Ipp64s  hex;
    Ipp64f   fp;
} IppFP_64f;

typedef union { /* single precision */
    Ipp32s  hex;
    Ipp32f   fp;
} IppFP_32f;


/* ////////////////////////////////////////////////////////////////////////// */

/* Define NULL pointer value */
#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#define IPP_UNREFERENCED_PARAMETER(p) (void)(p)

#if defined( _IPP_MARK_LIBRARY )
static char G[] = {73, 80, 80, 71, 101, 110, 117, 105, 110, 101, 243, 193, 210, 207, 215};
#endif


/*
// endian definition
*/
#define IPP_LITTLE_ENDIAN  (0)
#define IPP_BIG_ENDIAN     (1)

#if defined( _IPP_LE )
   #define IPP_ENDIAN IPP_LITTLE_ENDIAN

#elif defined( _IPP_BE )
   #define IPP_ENDIAN IPP_BIG_ENDIAN

#else
   #if defined( __ARMEB__ )
     #define IPP_ENDIAN IPP_BIG_ENDIAN

   #else
     #define IPP_ENDIAN IPP_LITTLE_ENDIAN

   #endif
#endif


/* ////////////////////////////////////////////////////////////////////////// */

/* intrinsics */
#if (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7)
    #if defined(__INTEL_COMPILER) || (_MSC_VER >= 1300)
        #if (_IPP == _IPP_W7)
            #if defined(__INTEL_COMPILER)
              #include "emmintrin.h"
            #else
              #undef _W7
              #include "emmintrin.h"
              #define _W7
            #endif
            #define _mm_loadu _mm_loadu_si128
        #elif (_IPP32E == _IPP32E_M7)
            #if defined(__INTEL_COMPILER)
                #include "pmmintrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER >= 140050110)
                #include "intrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER < 140050110)
                #include "emmintrin.h"
                #define _mm_loadu _mm_loadu_si128
            #endif
        #elif ((_IPP == _IPP_V8) || (_IPP32E == _IPP32E_U8) || (_IPP == _IPP_S8) || (_IPP32E == _IPP32E_N8))
            #if defined(__INTEL_COMPILER)
                #include "tmmintrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER >= 140050110)
                #include "intrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER < 140050110)
                #include "emmintrin.h"
                #define _mm_loadu _mm_loadu_si128
            #endif
        #elif (_IPP == _IPP_P8) || (_IPP32E == _IPP32E_Y8)
            #if defined(__INTEL_COMPILER)
                #include "smmintrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER >= 140050110)
                #include "intrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER < 140050110)
                #include "emmintrin.h"
                #define _mm_loadu _mm_loadu_si128
            #endif
        #elif (_IPP >= _IPP_G9) || (_IPP32E >= _IPP32E_E9)
            #if defined(__INTEL_COMPILER)
                #include "immintrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #elif (_MSC_FULL_VER >= 160021003)
                #include "immintrin.h"
                #define _mm_loadu _mm_lddqu_si128
            #endif
        #endif
    #endif
    #if defined(__GNUC__) && !defined(__INTEL_COMPILER)
      #include "x86intrin.h"
    #endif
#endif

// **** intrinsics for bit casting ****
#if defined(__INTEL_COMPILER)
extern unsigned int      __intel_castf32_u32(float val);
extern float             __intel_castu32_f32(unsigned int val);
extern unsigned __int64  __intel_castf64_u64(double val);
extern double            __intel_castu64_f64(unsigned __int64 val);
 #define __CAST_32f32u(val) __intel_castf32_u32((Ipp32f)val)
 #define __CAST_32u32f(val) __intel_castu32_f32((Ipp32u)val)
 #define __CAST_64f64u(val) __intel_castf64_u64((Ipp64f)val)
 #define __CAST_64u64f(val) __intel_castu64_f64((Ipp64u)val)
#else
 #define __CAST_32f32u(val) ( *((Ipp32u*)&val) )
 #define __CAST_32u32f(val) ( *((Ipp32f*)&val) )
 #define __CAST_64f64u(val) ( *((Ipp64u*)&val) )
 #define __CAST_64u64f(val) ( *((Ipp64f*)&val) )
#endif


// short names for vector registers casting
#define _pd2ps _mm_castpd_ps
#define _ps2pd _mm_castps_pd
#define _pd2pi _mm_castpd_si128
#define _pi2pd _mm_castsi128_pd
#define _ps2pi _mm_castps_si128
#define _pi2ps _mm_castsi128_ps

#define _ypd2ypi _mm256_castpd_si256
#define _ypi2ypd _mm256_castsi256_pd
#define _yps2ypi _mm256_castps_si256
#define _ypi2yps _mm256_castsi256_ps
#define _ypd2yps _mm256_castpd_ps
#define _yps2ypd _mm256_castps_pd

#define _yps2ps _mm256_castps256_ps128
#define _ypi2pi _mm256_castsi256_si128
#define _ypd2pd _mm256_castpd256_pd128
#define _ps2yps _mm256_castps128_ps256
#define _pi2ypi _mm256_castsi128_si256
#define _pd2ypd _mm256_castpd128_pd256

#define _zpd2zpi _mm512_castpd_si512
#define _zpi2zpd _mm512_castsi512_pd
#define _zps2zpi _mm512_castps_si512
#define _zpi2zps _mm512_castsi512_ps
#define _zpd2zps _mm512_castpd_ps
#define _zps2zpd _mm512_castps_pd

#define _zps2ps _mm512_castps512_ps128
#define _zpi2pi _mm512_castsi512_si128
#define _zpd2pd _mm512_castpd512_pd128
#define _ps2zps _mm512_castps128_ps512
#define _pi2zpi _mm512_castsi128_si512
#define _pd2zpd _mm512_castpd128_pd512

#define _zps2yps _mm512_castps512_ps256
#define _zpi2ypi _mm512_castsi512_si256
#define _zpd2ypd _mm512_castpd512_pd256
#define _yps2zps _mm512_castps256_ps512
#define _ypi2zpi _mm512_castsi256_si512
#define _ypd2zpd _mm512_castpd256_pd512


#if defined(__INTEL_COMPILER)
#define __IVDEP ivdep
#else
#define __IVDEP message("message :: 'ivdep' is not defined")
#endif

#if defined( _MERGED_BLD )
   #if !defined( _IPP_DYNAMIC )
      /* WIN-32, WIN-64 */
      #if defined(WIN32) || defined(WIN32E)
         #if ( defined(_W7) || defined(_M7) )
         #define _IPP_DATA 1
         #endif

      #elif defined(linux)
         /* LIN-32, LIN-64 */
         #if ( defined(_W7) || defined(_M7) )
         #define _IPP_DATA 1
         #endif


      /* OSX-32, OSX-64 */
      #elif defined(OSX32) || defined(OSXEM64T)
         #if ( defined(_Y8) )
         #define _IPP_DATA 1
         #endif
      #endif
   #endif
#else
   /* compile data unconditionally */
   #define _IPP_DATA 1
#endif


#if defined( __cplusplus )
}
#endif

#endif /* __OWNDEFS_H__ */

