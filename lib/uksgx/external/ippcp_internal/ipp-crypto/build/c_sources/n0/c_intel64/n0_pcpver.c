/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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
//               Intel® Integrated Performance Primitives
//                   Cryptographic Primitives (ippcp)
//
*/

#include "owndefs.h"
#include "ippcpdefs.h"
#include "owncp.h"
#include "pcpver.h"
#include "pcpname.h"

#define GET_LIBRARY_NAME( cpu, is ) #cpu, IPP_LIB_SHORTNAME() " " is " (" #cpu ")"

static const IppLibraryVersion ippcpLibVer = {
    /* major, minor, update (ex-majorBuild) */
    BASE_VERSION(),
#if defined IPP_REVISION
    IPP_REVISION,
#else
    -1,
#endif /* IPP_REVISION */
       /* targetCpu[4] */
#if ( _IPP_ARCH == _IPP_ARCH_IA32 ) || ( _IPP_ARCH == _IPP_ARCH_LP32 )
    #if ( _IPP == _IPP_M5 )             /* Intel® Quark(TM) processor - ia32 */
        GET_LIBRARY_NAME( m5, "586" )
    #elif ( _IPP == _IPP_H9 )           /* Intel® Advanced Vector Extensions 2 - ia32 */
        GET_LIBRARY_NAME( h9, "AVX2" )
    #elif ( _IPP == _IPP_G9 )           /* Intel® Advanced Vector Extensions - ia32 */
        GET_LIBRARY_NAME( g9, "AVX" )
    #elif ( _IPP == _IPP_P8 )           /* Intel® Streaming SIMD Extensions 4.2 (Intel® SSE4.2) - ia32 */
        GET_LIBRARY_NAME( p8, "SSE4.2" )
    #elif ( _IPP == _IPP_S8 )           /* Supplemental Streaming SIMD Extensions 3 + Intel® instruction MOVBE - ia32 */
        GET_LIBRARY_NAME( s8, "Atom" )
    #elif ( _IPP == _IPP_V8 )           /* Supplemental Streaming SIMD Extensions 3 - ia32 */
        GET_LIBRARY_NAME( v8, "SSSE3" )
    #elif ( _IPP == _IPP_W7 )           /* Intel® Streaming SIMD Extensions 2 - ia32 */
        GET_LIBRARY_NAME( w7, "SSE2" )
    #else
        GET_LIBRARY_NAME( px, "PX" )
    #endif

#elif ( _IPP_ARCH == _IPP_ARCH_EM64T ) || ( _IPP_ARCH == _IPP_ARCH_LP64 )
    #if ( _IPP32E == _IPP32E_K1 )       /* Intel® Advanced Vector Extensions 512 (formerly Icelake) - intel64 */
        GET_LIBRARY_NAME( k1, "AVX-512F/CD/BW/DQ/VL/SHA/VBMI/VBMI2/IFMA/GFNI/VAES/VCLMUL" )
    #elif ( _IPP32E == _IPP32E_K0 )       /* Intel® Advanced Vector Extensions 512 (formerly Skylake) - intel64 */
        GET_LIBRARY_NAME( k0, "AVX-512F/CD/BW/DQ/VL" )
    #elif ( _IPP32E == _IPP32E_N0 )     /* Intel® Advanced Vector Extensions 512 (formerly codenamed Knights Landing) - intel64 */
        GET_LIBRARY_NAME( n0, "AVX-512F/CD/ER/PF" )
    #elif ( _IPP32E == _IPP32E_E9 )     /* Intel® Advanced Vector Extensions - intel64 */
        GET_LIBRARY_NAME( e9, "AVX" )
    #elif ( _IPP32E == _IPP32E_L9 )     /* Intel® Advanced Vector Extensions 2 - intel64 */
        GET_LIBRARY_NAME( l9, "AVX2" )
    #elif ( _IPP32E == _IPP32E_Y8 )     /* Intel® Streaming SIMD Extensions 4.2 - intel64 */
        GET_LIBRARY_NAME( y8, "SSE4.2" )
    #elif ( _IPP32E == _IPP32E_N8 )     /* Supplemental Streaming SIMD Extensions 3 + Intel® instruction MOVBE - intel64 */
        GET_LIBRARY_NAME( n8, "Atom" )
    #elif ( _IPP32E == _IPP32E_U8 )     /* Supplemental Streaming SIMD Extensions 3 - intel64 */
        GET_LIBRARY_NAME( u8, "SSSE3" )
    #elif ( _IPP32E == _IPP32E_M7 )     /* Intel® Streaming SIMD Extensions 3 (Intel® SSE3) */
        GET_LIBRARY_NAME( m7, "SSE3" )
    #else
        GET_LIBRARY_NAME( mx, "PX" )
    #endif
#endif
    ,STR_VERSION() /* release Version */
    ,__DATE__ //BuildDate
};

IPPFUN( const IppLibraryVersion*, ippcpGetLibVersion, ( void )){
    return &ippcpLibVer;
};

/*////////////////////////// End of file "pcpver.c" ////////////////////////// */
