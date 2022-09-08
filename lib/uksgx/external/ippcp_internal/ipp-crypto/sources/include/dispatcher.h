/*******************************************************************************
* Copyright 2009-2021 Intel Corporation
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
// Intel® Integrated Performance Primitives Cryptography (Intel® IPP Cryptography)
//

#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#if defined( __cplusplus )
extern "C" {
#endif

/*
  Intel IPP Cryptography libraries and CPU features mask fitness. Implemented only for IA32 and Intel64 (emt)
*/

#if defined( _ARCH_IA32 )
#define M5_FM ( 0 )
#ifdef _IPP_QUARK
#define PX_FM ( 0 )
#else
#define PX_FM ( ippCPUID_MMX | ippCPUID_SSE )
#endif
#define W7_FM ( PX_FM | ippCPUID_SSE2 )
#define V8_FM ( W7_FM | ippCPUID_SSE3 | ippCPUID_SSSE3 )
#define S8_FM ( V8_FM | ippCPUID_MOVBE )
#define P8_FM ( V8_FM | ippCPUID_SSE41 | ippCPUID_SSE42 | ippCPUID_AES | ippCPUID_CLMUL | ippCPUID_SHA )
#define G9_FM ( P8_FM | ippCPUID_AVX | ippAVX_ENABLEDBYOS | ippCPUID_RDRAND | ippCPUID_F16C )
#define H9_FM ( G9_FM | ippCPUID_MOVBE | ippCPUID_AVX2 | ippCPUID_ADCOX | ippCPUID_RDSEED | ippCPUID_PREFETCHW )
#define I0_FM ( H9_FM | ippCPUID_AVX512F | ippCPUID_AVX512CD | ippCPUID_AVX512PF | ippCPUID_AVX512ER | ippAVX512_ENABLEDBYOS )
#define S0_FM ( H9_FM | ippCPUID_AVX512F | ippCPUID_AVX512CD | ippCPUID_AVX512VL | ippCPUID_AVX512BW | ippCPUID_AVX512DQ | ippAVX512_ENABLEDBYOS )

#elif defined (_ARCH_EM64T)

#define PX_FM ( ippCPUID_MMX | ippCPUID_SSE | ippCPUID_SSE2 )
#define M7_FM ( PX_FM | ippCPUID_SSE3 )
#define U8_FM ( M7_FM | ippCPUID_SSSE3 )
#define N8_FM ( U8_FM | ippCPUID_MOVBE )
#define Y8_FM ( U8_FM | ippCPUID_SSE41 | ippCPUID_SSE42 | ippCPUID_AES | ippCPUID_CLMUL | ippCPUID_SHA )
#define E9_FM ( Y8_FM | ippCPUID_AVX | ippAVX_ENABLEDBYOS | ippCPUID_RDRAND | ippCPUID_F16C )
#define L9_FM ( E9_FM | ippCPUID_MOVBE | ippCPUID_AVX2 | ippCPUID_ADCOX | ippCPUID_RDSEED | ippCPUID_PREFETCHW )
#define N0_FM ( L9_FM | ippCPUID_AVX512F | ippCPUID_AVX512CD | ippCPUID_AVX512PF | ippCPUID_AVX512ER | ippAVX512_ENABLEDBYOS )
#define K0_FM ( L9_FM | ippCPUID_AVX512F | ippCPUID_AVX512CD | ippCPUID_AVX512VL | ippCPUID_AVX512BW | ippCPUID_AVX512DQ | ippAVX512_ENABLEDBYOS )

#elif defined (_ARCH_LRB2)
  #define PX_FM ( ippCPUID_KNC )
#else
  #error undefined architecture
#endif

#define PX_MSK    ( 0 )
#define MMX_MSK   ( ippCPUID_MMX )
#define SSE_MSK   ( MMX_MSK   | ippCPUID_SSE     )
#define SSE2_MSK  ( SSE_MSK   | ippCPUID_SSE2    )
#define SSE3_MSK  ( SSE2_MSK  | ippCPUID_SSE3    )
#define SSSE3_MSK ( SSE3_MSK  | ippCPUID_SSSE3   )
#define ATOM_MSK  ( SSE3_MSK  | ippCPUID_SSSE3 | ippCPUID_MOVBE )
#define SSE41_MSK ( SSSE3_MSK | ippCPUID_SSE41   )
#define SSE42_MSK ( SSE41_MSK | ippCPUID_SSE42   )
#define AVX_MSK   ( SSE42_MSK | ippCPUID_AVX     )
#define AVX2_MSK  ( AVX_MSK   | ippCPUID_AVX2    )
#define AVX3X_MSK ( AVX2_MSK  | ippCPUID_AVX512F | ippCPUID_AVX512CD | ippCPUID_AVX512VL | ippCPUID_AVX512BW | ippCPUID_AVX512DQ )
#define AVX3M_MSK ( AVX2_MSK  | ippCPUID_AVX512F | ippCPUID_AVX512CD | ippCPUID_AVX512PF | ippCPUID_AVX512ER )
#define AVX3I_MSK ( AVX3X_MSK | ippCPUID_SHA | ippCPUID_AVX512VBMI | ippCPUID_AVX512VBMI2 | ippCPUID_AVX512IFMA | ippCPUID_AVX512GFNI | ippCPUID_AVX512VAES | ippCPUID_AVX512VCLMUL )

#if defined( _ARCH_IA32 ) && !defined( OSX32 )
  enum lib_enum {
     LIB_W7=0, LIB_S8=1, LIB_P8=2, LIB_G9=3, LIB_H9=4, LIB_NOMORE
  };
  #define LIB_PX LIB_W7
#elif defined( OSX32 )
  enum lib_enum {
     LIB_S8=0, LIB_P8=1, LIB_G9=2, LIB_H9=3, LIB_NOMORE
  };
  #define LIB_PX LIB_S8
  #define LIB_W7 LIB_S8
#elif defined( _ARCH_EM64T ) && !defined( OSXEM64T ) && !defined( WIN32E ) /* Linux* OS Intel64 supports N0 */
  enum lib_enum {
     LIB_M7=0, LIB_N8=1, LIB_Y8=2, LIB_E9=3, LIB_L9=4, LIB_N0=5, LIB_K0=6, LIB_K1=7,LIB_NOMORE
  };
  #define LIB_PX LIB_M7
#elif defined( _ARCH_EM64T ) && !defined( OSXEM64T ) /* Windows* OS Intel64 doesn't support N0 */
  enum lib_enum {
     LIB_M7=0, LIB_N8=1, LIB_Y8=2, LIB_E9=3, LIB_L9=4, LIB_K0=5, LIB_K1=6, LIB_NOMORE
  };
  #define LIB_PX LIB_M7
  #define LIB_N0 LIB_L9
#elif defined( OSXEM64T )
  enum lib_enum {
     LIB_Y8=0, LIB_E9=1, LIB_L9=2, LIB_K0=3, LIB_K1=4, LIB_NOMORE
  };
  #define LIB_PX LIB_Y8
  #define LIB_M7 LIB_Y8
  #define LIB_N8 LIB_Y8
  #define LIB_N0 LIB_L9
#elif defined( _ARCH_LRB2 )
  enum lib_enum {
     LIB_PX=0, LIB_B2=1, LIB_NOMORE
  };
   #define LIB_MIC LIB_B2
#else
  #error "lib_enum isn't defined!"
#endif

#if defined( _ARCH_IA32 )
  #if defined( OSX32 )       /* OSX supports starting with Intel® architecture formerly codenamed Penryn only */
    #define LIB_MMX   LIB_S8
    #define LIB_SSE   LIB_S8
    #define LIB_SSE2  LIB_S8
    #define LIB_SSE3  LIB_S8
    #define LIB_ATOM  LIB_S8
    #define LIB_SSSE3 LIB_S8
    #define LIB_SSE41 LIB_S8
    #define LIB_SSE42 LIB_P8
    #define LIB_AVX   LIB_G9
    #define LIB_AVX2  LIB_H9
    #define LIB_AVX3M LIB_H9 /* no ia32 library for Intel® Xeon Phi(TM) processor (formerly Knight Landing) */
    #define LIB_AVX3X LIB_H9 /* no ia32 library for Intel® Xeon® processor (formerly Skylake) */
    #define LIB_AVX3I LIB_H9 /* no ia32 library for Intel® Xeon® processor (formerly Icelake) */
#else
    #define LIB_MMX   LIB_W7
    #define LIB_SSE   LIB_W7
    #define LIB_SSE2  LIB_W7
    #define LIB_SSE3  LIB_W7
    #define LIB_ATOM  LIB_S8
    #define LIB_SSSE3 LIB_S8
    #define LIB_SSE41 LIB_S8 /* P8 is oriented for new Intel Atom® processor (formerly Silvermont) */
    #define LIB_SSE42 LIB_P8
    #define LIB_AVX   LIB_G9
    #define LIB_AVX2  LIB_H9
    #define LIB_AVX3M LIB_H9 /* no ia32 library for Intel® Xeon Phi(TM) processor (formerly Knight Landing) */
    #define LIB_AVX3X LIB_H9 /* no ia32 library for Intel® Xeon® processor (formerly Skylake) */
    #define LIB_AVX3I LIB_H9 /* no ia32 library for Intel® Xeon® processor (formerly Icelake) */
#endif
#elif defined (_ARCH_EM64T)
  #if defined( OSXEM64T )    /* OSX supports starting PNR only */
    #define LIB_MMX   LIB_N8
    #define LIB_SSE   LIB_N8
    #define LIB_SSE2  LIB_N8
    #define LIB_SSE3  LIB_N8
    #define LIB_ATOM  LIB_N8
    #define LIB_SSSE3 LIB_N8
    #define LIB_SSE41 LIB_N8
    #define LIB_SSE42 LIB_Y8
    #define LIB_AVX   LIB_E9
    #define LIB_AVX2  LIB_L9
    #define LIB_AVX3M LIB_L9
    #define LIB_AVX3X LIB_K0
    #define LIB_AVX3I LIB_K1
#else
    #define LIB_MMX   LIB_M7
    #define LIB_SSE   LIB_M7
    #define LIB_SSE2  LIB_M7
    #define LIB_SSE3  LIB_M7
    #define LIB_ATOM  LIB_N8
    #define LIB_SSSE3 LIB_N8
    #define LIB_SSE41 LIB_N8 /* Y8 is oriented for new Intel Atom® processor (formerly Silvermont) */
    #define LIB_SSE42 LIB_Y8
    #define LIB_AVX   LIB_E9
    #define LIB_AVX2  LIB_L9
    #define LIB_AVX3M LIB_N0
    #define LIB_AVX3X LIB_K0
    #define LIB_AVX3I LIB_K1
#endif
#elif defined (_ARCH_LRB2)
    #define LIB_MIC   LIB_B2
#endif

//gres: #if defined( _IPP_DYNAMIC )
#if defined( _PCS )
#if defined( _ARCH_IA32 )

/* Describe Intel CPUs and libraries */
typedef enum{CPU_W7=0, CPU_S8, CPU_P8, CPU_G9, CPU_H9, CPU_NOMORE} cpu_enum;
typedef enum{DLL_W7=0, DLL_S8, DLL_P8, DLL_G9, DLL_H9, DLL_NOMORE} dll_enum;

/* New cpu can use some libraries for old cpu */
static const dll_enum dllUsage[][DLL_NOMORE+1] = {
         /*  DLL_H9, DLL_G9, DLL_P8, DLL_S8, DLL_W7, DLL_NOMORE */
/*CPU_W7*/ {                                 DLL_W7, DLL_NOMORE },
/*CPU_S8*/ {                         DLL_S8, DLL_W7, DLL_NOMORE },
/*CPU_P8*/ {                 DLL_P8, DLL_S8, DLL_W7, DLL_NOMORE },
/*CPU_G9*/ {         DLL_G9, DLL_P8, DLL_S8, DLL_W7, DLL_NOMORE },
/*CPU_H9*/ { DLL_H9, DLL_G9, DLL_P8, DLL_S8, DLL_W7, DLL_NOMORE }
};

#elif defined (_ARCH_EM64T)
/* Describe Intel CPUs and libraries */
typedef enum{CPU_M7=0, CPU_N8, CPU_Y8, CPU_E9, CPU_L9, CPU_N0, CPU_K0, CPU_K1, CPU_NOMORE} cpu_enum;
typedef enum{DLL_M7=0, DLL_N8, DLL_Y8, DLL_E9, DLL_L9, DLL_N0, DLL_K0, DLL_K1, DLL_NOMORE} dll_enum;

/* New cpu can use some libraries for old cpu */
static const dll_enum dllUsage[][DLL_NOMORE+1] = {
         /*  DLL_K1, DLL_K0, DLL_N0, DLL_L9, DLL_E9, DLL_Y8, DLL_N8, DLL_M7, DLL_NOMORE */
/*CPU_M7*/ {                                                         DLL_M7, DLL_NOMORE },
/*CPU_N8*/ {                                                 DLL_N8, DLL_M7, DLL_NOMORE },
/*CPU_Y8*/ {                                         DLL_Y8, DLL_N8, DLL_M7, DLL_NOMORE },
/*CPU_E9*/ {                                 DLL_E9, DLL_Y8, DLL_N8, DLL_M7, DLL_NOMORE },
/*CPU_L9*/ {                         DLL_L9, DLL_E9, DLL_Y8, DLL_N8, DLL_M7, DLL_NOMORE },
/*CPU_N0*/ {                 DLL_N0, DLL_L9, DLL_E9, DLL_Y8, DLL_N8, DLL_M7, DLL_NOMORE },
/*CPU_K0*/ {         DLL_K0, DLL_N0, DLL_L9, DLL_E9, DLL_Y8, DLL_N8, DLL_M7, DLL_NOMORE }
/*CPU_K1*/ { DLL_K1, DLL_K0, DLL_N0, DLL_L9, DLL_E9, DLL_Y8, DLL_N8, DLL_M7, DLL_NOMORE }
};

#endif

#if defined( _PCS )

/* Names of the Intel libraries which can be loaded */
#if defined ( WIN32 )
static const _TCHAR* dllNames[DLL_NOMORE] = {
    _T(IPP_LIB_PREFIX()) _T("w7") _T(".dll"),
    _T(IPP_LIB_PREFIX()) _T("s8") _T(".dll"),
    _T(IPP_LIB_PREFIX()) _T("p8") _T(".dll"),
    _T(IPP_LIB_PREFIX()) _T("g9") _T(".dll"),
    _T(IPP_LIB_PREFIX()) _T("h9") _T(".dll")
};
#elif defined(LINUX32)
static const _TCHAR* dllNames[DLL_NOMORE] = {
    _T("lib") _T(IPP_LIB_PREFIX()) _T("w7.so"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("s8.so"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("p8.so"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("g9.so"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("h9.so")
};
#elif defined( OSX32 )
static const _TCHAR* dllNames[DLL_NOMORE] = {
    _T("lib") _T(IPP_LIB_PREFIX()) _T("s8") _T(".dylib"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("p8") _T(".dylib"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("g9") _T(".dylib"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("h9") _T(".dylib")
};
#elif defined( WIN32E )
static const _TCHAR* dllNames[DLL_NOMORE] = {
    _T(IPP_LIB_PREFIX()) _T("m7") _T(".dll"),
    _T(IPP_LIB_PREFIX()) _T("n8") _T(".dll"),
    _T(IPP_LIB_PREFIX()) _T("y8") _T(".dll"),
    _T(IPP_LIB_PREFIX()) _T("e9") _T(".dll"),
    _T(IPP_LIB_PREFIX()) _T("l9") _T(".dll"), /* no support for N0 on win */
    _T(IPP_LIB_PREFIX()) _T("k0") _T(".dll"),
    _T(IPP_LIB_PREFIX()) _T("k1") _T(".dll")
};
#elif defined( OSXEM64T )
static const _TCHAR* dllNames[DLL_NOMORE] = {
    _T("lib") _T(IPP_LIB_PREFIX()) _T("n8") _T(".dylib"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("y8") _T(".dylib"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("e9") _T(".dylib"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("l9") _T(".dylib"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("k0") _T(".dylib"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("k1") _T(".dylib")
};
#elif defined( LINUX32E )
static const _TCHAR* dllNames[DLL_NOMORE] = {
    _T("lib") _T(IPP_LIB_PREFIX()) _T("m7.so"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("n8.so"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("y8.so"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("e9.so"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("l9.so"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("n0.so"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("k0.so"),
    _T("lib") _T(IPP_LIB_PREFIX()) _T("k1.so")
};
#endif

#endif /* _PCS */

#else /*_IPP_DYNAMIC */


#endif


#if defined( __cplusplus )
}
#endif

#endif /* __DISPATCHER_H__ */
