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
//
//   Intel® Integrated Performance Primitives Cryptography (Intel® IPP Cryptography)
//
*/

#if !defined( IPPCP_H__ ) || defined( _OWN_BLDPCS )
#define IPPCP_H__


#ifndef IPPCPDEFS_H__
  #include "ippcpdefs.h"
#endif


#ifdef  __cplusplus
extern "C" {
#endif

#if !defined( IPP_NO_DEFAULT_LIB )
  #if defined( _IPP_SEQUENTIAL_DYNAMIC )
    #pragma comment( lib, __FILE__ "/../../lib/" INTEL_PLATFORM "ippcp" )
  #elif defined( _IPP_SEQUENTIAL_STATIC )
    #pragma comment( lib, __FILE__ "/../../lib/" INTEL_PLATFORM "ippcpmt" )
  #endif
#endif

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning(push)
#pragma warning(disable : 4100) // for MSVC, unreferenced param
#endif

/* /////////////////////////////////////////////////////////////////////////////
//  Name:       ippcpGetLibVersion
//  Purpose:    getting of the library version
//  Returns:    the structure of information about version of ippCP library
//  Parameters:
//
//  Notes:      not necessary to release the returned structure
*/
IPPAPI( const IppLibraryVersion*, ippcpGetLibVersion, (void) )


/*
// =========================================================
// Symmetric Ciphers
// =========================================================
*/

/* TDES */

#define TDES_DEPRECATED "This algorithm is considered weak due to known attacks on it. \
The functionality remains in the library, but the implementation will no be longer \
optimized and no security patches will be applied. A more secure alternative is available: AES"

IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsDESGetSize,(int *size))
IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsDESInit,(const Ipp8u* pKey, IppsDESSpec* pCtx))

IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsDESPack,(const IppsDESSpec* pCtx, Ipp8u* pBuffer))
IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsDESUnpack,(const Ipp8u* pBuffer, IppsDESSpec* pCtx))

IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsTDESEncryptECB,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsDESSpec* pCtx1, const IppsDESSpec* pCtx2, const IppsDESSpec* pCtx3,
                                      IppsCPPadding padding))
IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsTDESDecryptECB,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsDESSpec* pCtx1, const IppsDESSpec* pCtx2, const IppsDESSpec* pCtx3,
                                      IppsCPPadding padding))

IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsTDESEncryptCBC,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsDESSpec* pCtx1, const IppsDESSpec* pCtx2, const IppsDESSpec* pCtx3,
                                      const Ipp8u* pIV,
                                      IppsCPPadding padding))
IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsTDESDecryptCBC,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsDESSpec* pCtx1, const IppsDESSpec* pCtx2, const IppsDESSpec* pCtx3,
                                      const Ipp8u* pIV,
                                      IppsCPPadding padding))

IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsTDESEncryptCFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int cfbBlkSize,
                                      const IppsDESSpec* pCtx1, const IppsDESSpec* pCtx2, const IppsDESSpec* pCtx3,
                                      const Ipp8u* pIV,
                                      IppsCPPadding padding))
IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsTDESDecryptCFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int cfbBlkSize,
                                      const IppsDESSpec* pCtx1, const IppsDESSpec* pCtx2, const IppsDESSpec* pCtx3,
                                      const Ipp8u* pIV,
                                      IppsCPPadding padding))

IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsTDESEncryptOFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int ofbBlkSize,
                                     const IppsDESSpec* pCtx1,
                                     const IppsDESSpec* pCtx2,
                                     const IppsDESSpec* pCtx3,
                                     Ipp8u* pIV))
IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsTDESDecryptOFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int ofbBlkSize,
                                     const IppsDESSpec* pCtx1,
                                     const IppsDESSpec* pCtx2,
                                     const IppsDESSpec* pCtx3,
                                     Ipp8u* pIV))

IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsTDESEncryptCTR,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsDESSpec* pCtx1,
                                      const IppsDESSpec* pCtx2,
                                      const IppsDESSpec* pCtx3,
                                      Ipp8u* pCtrValue, int ctrNumBitSize))
IPP_DEPRECATED(TDES_DEPRECATED) \
IPPAPI(IppStatus, ippsTDESDecryptCTR,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsDESSpec* pCtx1,
                                      const IppsDESSpec* pCtx2,
                                      const IppsDESSpec* pCtx3,
                                      Ipp8u* pCtrValue, int ctrNumBitSize))

/* AES */
IPPAPI(IppStatus, ippsAESGetSize,(int *pSize))
IPPAPI(IppStatus, ippsAESInit,(const Ipp8u* pKey, int keyLen, IppsAESSpec* pCtx, int ctxSize))
IPPAPI(IppStatus, ippsAESSetKey,(const Ipp8u* pKey, int keyLen, IppsAESSpec* pCtx))

IPPAPI(IppStatus, ippsAESPack,(const IppsAESSpec* pCtx, Ipp8u* pBuffer, int bufSize))
IPPAPI(IppStatus, ippsAESUnpack,(const Ipp8u* pBuffer, IppsAESSpec* pCtx, int ctxSize))

#define ECB_DEPRECATED "ECB functionality remains in the library, but it is not safe when used as is. \
It is recommended to use any other mode, for example CBC."

IPP_DEPRECATED(ECB_DEPRECATED) \
IPPAPI(IppStatus, ippsAESEncryptECB,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx))
IPP_DEPRECATED(ECB_DEPRECATED) \
IPPAPI(IppStatus, ippsAESDecryptECB,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx))

IPPAPI(IppStatus, ippsAESEncryptCBC,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx,
                                     const Ipp8u* pIV))
IPPAPI(IppStatus, ippsAESEncryptCBC_CS1,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx,
                                     const Ipp8u* pIV))
IPPAPI(IppStatus, ippsAESEncryptCBC_CS2,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx,
                                     const Ipp8u* pIV))
IPPAPI(IppStatus, ippsAESEncryptCBC_CS3,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx,
                                     const Ipp8u* pIV))
IPPAPI(IppStatus, ippsAESDecryptCBC,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx,
                                     const Ipp8u* pIV))
IPPAPI(IppStatus, ippsAESDecryptCBC_CS1,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx,
                                     const Ipp8u* pIV))
IPPAPI(IppStatus, ippsAESDecryptCBC_CS2,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx,
                                     const Ipp8u* pIV))
IPPAPI(IppStatus, ippsAESDecryptCBC_CS3,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx,
                                     const Ipp8u* pIV))

IPPAPI(IppStatus, ippsAESEncryptCFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int cfbBlkSize,
                                     const IppsAESSpec* pCtx,
                                     const Ipp8u* pIV))
IPPAPI(IppStatus, ippsAESDecryptCFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int cfbBlkSize,
                                     const IppsAESSpec* pCtx,
                                     const Ipp8u* pIV))

IPPAPI(IppStatus, ippsAESEncryptOFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int ofbBlkSize,
                                     const IppsAESSpec* pCtx,
                                     Ipp8u* pIV))
IPPAPI(IppStatus, ippsAESDecryptOFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int ofbBlkSize,
                                     const IppsAESSpec* pCtx,
                                     Ipp8u* pIV))

IPPAPI(IppStatus, ippsAESEncryptCTR,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx,
                                     Ipp8u* pCtrValue, int ctrNumBitSize))
IPPAPI(IppStatus, ippsAESDecryptCTR,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx,
                                     Ipp8u* pCtrValue, int ctrNumBitSize))

IPPAPI(IppStatus, ippsAESEncryptXTS_Direct,(const Ipp8u* pSrc, Ipp8u* pDst, int encBitsize, int aesBlkNo,
                                     const Ipp8u* pTweakPT,
                                     const Ipp8u* pKey, int keyBitsize,
                                     int dataUnitBitsize))
IPPAPI(IppStatus, ippsAESDecryptXTS_Direct,(const Ipp8u* pSrc, Ipp8u* pDst, int encBitsize, int aesBlkNo,
                                     const Ipp8u* pTweakPT,
                                     const Ipp8u* pKey, int keyBitsize,
                                     int dataUnitBitsize))

/* AES multi-buffer functions */
IPPAPI(IppStatus, ippsAES_EncryptCFB16_MB, (const Ipp8u* pSrc[], Ipp8u* pDst[], int len[],
                                            const IppsAESSpec* pCtx[],
                                            const Ipp8u* pIV[],
                                            IppStatus status[],
                                            int numBuffers))

/* SMS4 */
IPPAPI(IppStatus, ippsSMS4GetSize,(int *pSize))
IPPAPI(IppStatus, ippsSMS4Init,(const Ipp8u* pKey, int keyLen, IppsSMS4Spec* pCtx, int ctxSize))
IPPAPI(IppStatus, ippsSMS4SetKey,(const Ipp8u* pKey, int keyLen, IppsSMS4Spec* pCtx))

IPP_DEPRECATED(ECB_DEPRECATED) \
IPPAPI(IppStatus, ippsSMS4EncryptECB,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx))
IPP_DEPRECATED(ECB_DEPRECATED) \
IPPAPI(IppStatus, ippsSMS4DecryptECB,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx))

IPPAPI(IppStatus, ippsSMS4EncryptCBC,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx,
                                      const Ipp8u* pIV))
IPPAPI(IppStatus, ippsSMS4EncryptCBC_CS1,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx,
                                      const Ipp8u* pIV))
IPPAPI(IppStatus, ippsSMS4EncryptCBC_CS2,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx,
                                      const Ipp8u* pIV))
IPPAPI(IppStatus, ippsSMS4EncryptCBC_CS3,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx,
                                      const Ipp8u* pIV))
IPPAPI(IppStatus, ippsSMS4DecryptCBC,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx,
                                      const Ipp8u* pIV))
IPPAPI(IppStatus, ippsSMS4DecryptCBC_CS1,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx,
                                      const Ipp8u* pIV))
IPPAPI(IppStatus, ippsSMS4DecryptCBC_CS2,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx,
                                      const Ipp8u* pIV))
IPPAPI(IppStatus, ippsSMS4DecryptCBC_CS3,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx,
                                      const Ipp8u* pIV))

IPPAPI(IppStatus, ippsSMS4EncryptCFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int cfbBlkSize,
                                      const IppsSMS4Spec* pCtx,
                                      const Ipp8u* pIV))
IPPAPI(IppStatus, ippsSMS4DecryptCFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int cfbBlkSize,
                                      const IppsSMS4Spec* pCtx,
                                      const Ipp8u* pIV))

IPPAPI(IppStatus, ippsSMS4EncryptOFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int ofbBlkSize,
                                      const IppsSMS4Spec* pCtx,
                                      Ipp8u* pIV))
IPPAPI(IppStatus, ippsSMS4DecryptOFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int ofbBlkSize,
                                      const IppsSMS4Spec* pCtx,
                                      Ipp8u* pIV))

IPPAPI(IppStatus, ippsSMS4EncryptCTR,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx,
                                      Ipp8u* pCtrValue, int ctrNumBitSize))
IPPAPI(IppStatus, ippsSMS4DecryptCTR,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsSMS4Spec* pCtx,
                                      Ipp8u* pCtrValue, int ctrNumBitSize))

/* SMS4-CCM */
IPPAPI(IppStatus, ippsSMS4_CCMGetSize,(int* pSize))
IPPAPI(IppStatus, ippsSMS4_CCMInit,(const Ipp8u* pKey, int keyLen, IppsSMS4_CCMState* pCtx, int ctxSize))

IPPAPI(IppStatus, ippsSMS4_CCMMessageLen,(Ipp64u msgLen, IppsSMS4_CCMState* pCtx))
IPPAPI(IppStatus, ippsSMS4_CCMTagLen,(int tagLen, IppsSMS4_CCMState* pCtx))

IPPAPI(IppStatus, ippsSMS4_CCMStart,(const Ipp8u* pIV, int ivLen, const Ipp8u* pAD, int adLen, IppsSMS4_CCMState* pCtx))
IPPAPI(IppStatus, ippsSMS4_CCMEncrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int len, IppsSMS4_CCMState* pCtx))
IPPAPI(IppStatus, ippsSMS4_CCMDecrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int len, IppsSMS4_CCMState* pCtx))
IPPAPI(IppStatus, ippsSMS4_CCMGetTag,(Ipp8u* pTag, int tagLen, const IppsSMS4_CCMState* pCtx))

/*
// =========================================================
// AES based  authentication & confidence Primitives
// =========================================================
*/

/* AES-CCM */
IPPAPI(IppStatus, ippsAES_CCMGetSize,(int* pSize))
IPPAPI(IppStatus, ippsAES_CCMInit,(const Ipp8u* pKey, int keyLen, IppsAES_CCMState* pState, int ctxSize))

IPPAPI(IppStatus, ippsAES_CCMMessageLen,(Ipp64u msgLen, IppsAES_CCMState* pState))
IPPAPI(IppStatus, ippsAES_CCMTagLen,(int tagLen, IppsAES_CCMState* pState))

IPPAPI(IppStatus, ippsAES_CCMStart,(const Ipp8u* pIV, int ivLen, const Ipp8u* pAD, int adLen, IppsAES_CCMState* pState))
IPPAPI(IppStatus, ippsAES_CCMEncrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int len, IppsAES_CCMState* pState))
IPPAPI(IppStatus, ippsAES_CCMDecrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int len, IppsAES_CCMState* pState))
IPPAPI(IppStatus, ippsAES_CCMGetTag,(Ipp8u* pTag, int tagLen, const IppsAES_CCMState* pState))

/* AES-GCM */
IPPAPI(IppStatus, ippsAES_GCMGetSize,(int * pSize))
IPPAPI(IppStatus, ippsAES_GCMInit,(const Ipp8u* pKey, int keyLen, IppsAES_GCMState* pState, int ctxSize))

IPPAPI(IppStatus, ippsAES_GCMReset,(IppsAES_GCMState* pState))
IPPAPI(IppStatus, ippsAES_GCMProcessIV,(const Ipp8u* pIV, int ivLen,
                                        IppsAES_GCMState* pState))
IPPAPI(IppStatus, ippsAES_GCMProcessAAD,(const Ipp8u* pAAD, int ivAAD,
                                        IppsAES_GCMState* pState))
IPPAPI(IppStatus, ippsAES_GCMStart,(const Ipp8u* pIV, int ivLen,
                                    const Ipp8u* pAAD, int aadLen,
                                    IppsAES_GCMState* pState))
IPPAPI(IppStatus, ippsAES_GCMEncrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int len, IppsAES_GCMState* pState))
IPPAPI(IppStatus, ippsAES_GCMDecrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int len, IppsAES_GCMState* pState))
IPPAPI(IppStatus, ippsAES_GCMGetTag,(Ipp8u* pDstTag, int tagLen, const IppsAES_GCMState* pState))

/* AES-XTS */
IPPAPI(IppStatus, ippsAES_XTSGetSize,(int * pSize))
IPPAPI(IppStatus, ippsAES_XTSInit,(const Ipp8u* pKey, int keyLen,
                                   int duBitsize,
                                   IppsAES_XTSSpec* pCtx,int ctxSize))
IPPAPI(IppStatus, ippsAES_XTSEncrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int bitSizeLen,
                                      const IppsAES_XTSSpec* pCtx,
                                      const Ipp8u* pTweak,
                                      int startCipherBlkNo))
IPPAPI(IppStatus, ippsAES_XTSDecrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int bitSizeLen,
                                      const IppsAES_XTSSpec* pCtx,
                                      const Ipp8u* pTweak,
                                      int startCipherBlkNo))

/* AES-SIV (RFC 5297) */
IPPAPI(IppStatus, ippsAES_S2V_CMAC,(const Ipp8u* pKey, int keyLen,
                                    const Ipp8u* pAD[], const int pADlen[], int numAD,
                                          Ipp8u* pV))
IPPAPI(IppStatus, ippsAES_SIVEncrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                            Ipp8u* pSIV,
                                      const Ipp8u* pAuthKey, const Ipp8u* pConfKey, int keyLen,
                                      const Ipp8u* pAD[], const int pADlen[], int numAD))
IPPAPI(IppStatus, ippsAES_SIVDecrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      int* pAuthPassed,
                                      const Ipp8u* pAuthKey, const Ipp8u* pConfKey, int keyLen,
                                      const Ipp8u* pAD[], const int pADlen[], int numAD,
                                      const Ipp8u* pSIV))

/* AES-CMAC */
IPPAPI(IppStatus, ippsAES_CMACGetSize,(int* pSize))
IPPAPI(IppStatus, ippsAES_CMACInit,(const Ipp8u* pKey, int keyLen, IppsAES_CMACState* pState, int ctxSize))

IPPAPI(IppStatus, ippsAES_CMACUpdate,(const Ipp8u* pSrc, int len, IppsAES_CMACState* pState))
IPPAPI(IppStatus, ippsAES_CMACFinal,(Ipp8u* pMD, int mdLen, IppsAES_CMACState* pState))
IPPAPI(IppStatus, ippsAES_CMACGetTag,(Ipp8u* pMD, int mdLen, const IppsAES_CMACState* pState))


/*
// =========================================================
// RC4 Stream Ciphers
// =========================================================
*/

#define RC4_DEPRECATED "is deprecated. This function is obsolete and will be removed in one of the future Intel IPP Cryptography releases"

IPP_DEPRECATED(RC4_DEPRECATED) \
IPPAPI(IppStatus, ippsARCFourCheckKey, (const Ipp8u *pKey, int keyLen, IppBool* pIsWeak))

IPP_DEPRECATED(RC4_DEPRECATED) \
IPPAPI(IppStatus, ippsARCFourGetSize, (int* pSize))
IPP_DEPRECATED(RC4_DEPRECATED) \
IPPAPI(IppStatus, ippsARCFourInit, (const Ipp8u *pKey, int keyLen, IppsARCFourState *pCtx))
IPP_DEPRECATED(RC4_DEPRECATED) \
IPPAPI(IppStatus, ippsARCFourReset, (IppsARCFourState* pCtx))

IPP_DEPRECATED(RC4_DEPRECATED) \
IPPAPI(IppStatus, ippsARCFourPack,(const IppsARCFourState* pCtx, Ipp8u* pBuffer))
IPP_DEPRECATED(RC4_DEPRECATED) \
IPPAPI(IppStatus, ippsARCFourUnpack,(const Ipp8u* pBuffer, IppsARCFourState* pCtx))

IPP_DEPRECATED(RC4_DEPRECATED) \
IPPAPI(IppStatus, ippsARCFourEncrypt, (const Ipp8u *pSrc, Ipp8u *pDst, int length, IppsARCFourState *pCtx))
IPP_DEPRECATED(RC4_DEPRECATED) \
IPPAPI(IppStatus, ippsARCFourDecrypt, (const Ipp8u *pSrc, Ipp8u *pDst, int length, IppsARCFourState *pCtx))


/*
// =========================================================
// One-Way Hash Functions
// =========================================================
*/

#define OBSOLETE_API "is deprecated. This API is considered obsolete and will be removed in one of future Intel IPP Cryptography releases. \
Use the following link for opening a ticket and providing feedback: https://supporttickets.intel.com/ if you have concerns."

/* SHA1 Hash Primitives */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA1GetSize,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA1Init,(IppsSHA1State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA1Duplicate,(const IppsSHA1State* pSrcState, IppsSHA1State* pDstState))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA1Pack,(const IppsSHA1State* pState, Ipp8u* pBuffer))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA1Unpack,(const Ipp8u* pBuffer, IppsSHA1State* pState))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA1Update,(const Ipp8u* pSrc, int len, IppsSHA1State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA1GetTag,(Ipp8u* pTag, Ipp32u tagLen, const IppsSHA1State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA1Final,(Ipp8u* pMD, IppsSHA1State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA1MessageDigest,(const Ipp8u* pMsg, int len, Ipp8u* pMD))

/* SHA224 Hash Primitives */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA224GetSize,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA224Init,(IppsSHA224State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA224Duplicate,(const IppsSHA224State* pSrcState, IppsSHA224State* pDstState))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA224Pack,(const IppsSHA224State* pState, Ipp8u* pBuffer))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA224Unpack,(const Ipp8u* pBuffer, IppsSHA224State* pState))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA224Update,(const Ipp8u* pSrc, int len, IppsSHA224State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA224GetTag,(Ipp8u* pTag, Ipp32u tagLen, const IppsSHA224State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA224Final,(Ipp8u* pMD, IppsSHA224State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA224MessageDigest,(const Ipp8u* pMsg, int len, Ipp8u* pMD))

/* SHA256 Hash Primitives */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA256GetSize,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA256Init,(IppsSHA256State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA256Duplicate,(const IppsSHA256State* pSrcState, IppsSHA256State* pDstState))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA256Pack,(const IppsSHA256State* pState, Ipp8u* pBuffer))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA256Unpack,(const Ipp8u* pBuffer, IppsSHA256State* pState))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA256Update,(const Ipp8u* pSrc, int len, IppsSHA256State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA256GetTag,(Ipp8u* pTag, Ipp32u tagLen, const IppsSHA256State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA256Final,(Ipp8u* pMD, IppsSHA256State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA256MessageDigest,(const Ipp8u* pMsg, int len, Ipp8u* pMD))

/* SHA384 Hash Primitives */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA384GetSize,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA384Init,(IppsSHA384State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA384Duplicate,(const IppsSHA384State* pSrcState, IppsSHA384State* pDstState))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA384Pack,(const IppsSHA384State* pState, Ipp8u* pBuffer))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA384Unpack,(const Ipp8u* pBuffer, IppsSHA384State* pState))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA384Update,(const Ipp8u* pSrc, int len, IppsSHA384State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA384GetTag,(Ipp8u* pTag, Ipp32u tagLen, const IppsSHA384State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA384Final,(Ipp8u* pMD, IppsSHA384State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA384MessageDigest,(const Ipp8u* pMsg, int len, Ipp8u* pMD))

/* SHA512 Hash Primitives */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA512GetSize,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA512Init,(IppsSHA512State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA512Duplicate,(const IppsSHA512State* pSrcState, IppsSHA512State* pDstState))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA512Pack,(const IppsSHA512State* pState, Ipp8u* pBuffer))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA512Unpack,(const Ipp8u* pBuffer, IppsSHA512State* pState))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA512Update,(const Ipp8u* pSrc, int len, IppsSHA512State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA512GetTag,(Ipp8u* pTag, Ipp32u tagLen, const IppsSHA512State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA512Final,(Ipp8u* pMD, IppsSHA512State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSHA512MessageDigest,(const Ipp8u* pMsg, int len, Ipp8u* pMD))

/* MD5 Hash Primitives */

#define MD5_DEPRECATED "This algorithm is considered weak due to known attacks on it. \
The functionality remains in the library, but the implementation will no be longer \
optimized and no security patches will be applied. A more secure alternative is available: SHA-2"

IPP_DEPRECATED(MD5_DEPRECATED) \
IPPAPI(IppStatus, ippsMD5GetSize,(int* pSize))
IPP_DEPRECATED(MD5_DEPRECATED) \
IPPAPI(IppStatus, ippsMD5Init,(IppsMD5State* pState))
IPP_DEPRECATED(MD5_DEPRECATED) \
IPPAPI(IppStatus, ippsMD5Duplicate,(const IppsMD5State* pSrcState, IppsMD5State* pDstState))

IPP_DEPRECATED(MD5_DEPRECATED) \
IPPAPI(IppStatus, ippsMD5Pack,(const IppsMD5State* pState, Ipp8u* pBuffer))
IPP_DEPRECATED(MD5_DEPRECATED) \
IPPAPI(IppStatus, ippsMD5Unpack,(const Ipp8u* pBuffer, IppsMD5State* pState))

IPP_DEPRECATED(MD5_DEPRECATED) \
IPPAPI(IppStatus, ippsMD5Update,(const Ipp8u* pSrc, int len, IppsMD5State* pState))
IPP_DEPRECATED(MD5_DEPRECATED) \
IPPAPI(IppStatus, ippsMD5GetTag,(Ipp8u* pTag, Ipp32u tagLen, const IppsMD5State* pState))
IPP_DEPRECATED(MD5_DEPRECATED) \
IPPAPI(IppStatus, ippsMD5Final,(Ipp8u* pMD, IppsMD5State* pState))
IPP_DEPRECATED(MD5_DEPRECATED) \
IPPAPI(IppStatus, ippsMD5MessageDigest,(const Ipp8u* pMsg, int len, Ipp8u* pMD))

/* SM3 Hash Primitives */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSM3GetSize,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSM3Init,(IppsSM3State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSM3Duplicate,(const IppsSM3State* pSrcState, IppsSM3State* pDstState))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSM3Pack,(const IppsSM3State* pState, Ipp8u* pBuffer))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSM3Unpack,(const Ipp8u* pBuffer, IppsSM3State* pState))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSM3Update,(const Ipp8u* pSrc, int len, IppsSM3State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSM3GetTag,(Ipp8u* pTag, Ipp32u tagLen, const IppsSM3State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSM3Final,(Ipp8u* pMD, IppsSM3State* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsSM3MessageDigest,(const Ipp8u* pMsg, int len, Ipp8u* pMD))

/* generalized Hash Primitives */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHashGetSize,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHashInit,(IppsHashState* pState, IppHashAlgId hashAlg))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHashPack,(const IppsHashState* pState, Ipp8u* pBuffer, int bufSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHashUnpack,(const Ipp8u* pBuffer, IppsHashState* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHashDuplicate,(const IppsHashState* pSrcState, IppsHashState* pDstState))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHashUpdate,(const Ipp8u* pSrc, int len, IppsHashState* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHashGetTag,(Ipp8u* pTag, int tagLen, const IppsHashState* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHashFinal,(Ipp8u* pMD, IppsHashState* pState))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHashMessage,(const Ipp8u* pMsg, int len, Ipp8u* pMD, IppHashAlgId hashAlg))

/* method based generalized (reduced memory footprint) Hash Primitives */
IPP_DEPRECATED(MD5_DEPRECATED) \
IPPAPI( const IppsHashMethod*, ippsHashMethod_MD5, (void) )
IPPAPI( const IppsHashMethod*, ippsHashMethod_SM3, (void) )

IPPAPI( const IppsHashMethod*, ippsHashMethod_SHA1, (void) )
IPPAPI( const IppsHashMethod*, ippsHashMethod_SHA1_NI, (void) )
IPPAPI( const IppsHashMethod*, ippsHashMethod_SHA1_TT, (void) )

IPPAPI( const IppsHashMethod*, ippsHashMethod_SHA256, (void) )
IPPAPI( const IppsHashMethod*, ippsHashMethod_SHA256_NI, (void) )
IPPAPI( const IppsHashMethod*, ippsHashMethod_SHA256_TT, (void) )

IPPAPI( const IppsHashMethod*, ippsHashMethod_SHA224, (void) )
IPPAPI( const IppsHashMethod*, ippsHashMethod_SHA224_NI, (void) )
IPPAPI( const IppsHashMethod*, ippsHashMethod_SHA224_TT, (void) )

IPPAPI( const IppsHashMethod*, ippsHashMethod_SHA512, (void) )
IPPAPI( const IppsHashMethod*, ippsHashMethod_SHA384, (void) )
IPPAPI( const IppsHashMethod*, ippsHashMethod_SHA512_256, (void) )
IPPAPI( const IppsHashMethod*, ippsHashMethod_SHA512_224, (void) )

IPPAPI( IppStatus, ippsHashMethodGetSize, (int* pSize) )
IPP_DEPRECATED(MD5_DEPRECATED) \
IPPAPI( IppStatus, ippsHashMethodSet_MD5, (IppsHashMethod* pMethod) )
IPPAPI( IppStatus, ippsHashMethodSet_SM3, (IppsHashMethod* pMethod) )

IPPAPI( IppStatus, ippsHashMethodSet_SHA1, (IppsHashMethod* pMethod) )
IPPAPI( IppStatus, ippsHashMethodSet_SHA1_NI, (IppsHashMethod* pMethod) )
IPPAPI( IppStatus, ippsHashMethodSet_SHA1_TT, (IppsHashMethod* pMethod) )

IPPAPI( IppStatus, ippsHashMethodSet_SHA256, (IppsHashMethod* pMethod) )
IPPAPI( IppStatus, ippsHashMethodSet_SHA256_NI, (IppsHashMethod* pMethod) )
IPPAPI( IppStatus, ippsHashMethodSet_SHA256_TT, (IppsHashMethod* pMethod) )

IPPAPI( IppStatus, ippsHashMethodSet_SHA224, (IppsHashMethod* pMethod) )
IPPAPI( IppStatus, ippsHashMethodSet_SHA224_NI, (IppsHashMethod* pMethod) )
IPPAPI( IppStatus, ippsHashMethodSet_SHA224_TT, (IppsHashMethod* pMethod) )

IPPAPI( IppStatus, ippsHashMethodSet_SHA512, (IppsHashMethod* pMethod) )
IPPAPI( IppStatus, ippsHashMethodSet_SHA384, (IppsHashMethod* pMethod) )
IPPAPI( IppStatus, ippsHashMethodSet_SHA512_256, (IppsHashMethod* pMethod) )
IPPAPI( IppStatus, ippsHashMethodSet_SHA512_224, (IppsHashMethod* pMethod) )

IPPAPI(IppStatus, ippsHashGetSize_rmf,(int* pSize))
IPPAPI(IppStatus, ippsHashInit_rmf,(IppsHashState_rmf* pState, const IppsHashMethod* pMethod))

IPPAPI(IppStatus, ippsHashPack_rmf,(const IppsHashState_rmf* pState, Ipp8u* pBuffer, int bufSize))
IPPAPI(IppStatus, ippsHashUnpack_rmf,(const Ipp8u* pBuffer, IppsHashState_rmf* pState))
IPPAPI(IppStatus, ippsHashDuplicate_rmf,(const IppsHashState_rmf* pSrcState, IppsHashState_rmf* pDstState))

IPPAPI(IppStatus, ippsHashUpdate_rmf,(const Ipp8u* pSrc, int len, IppsHashState_rmf* pState))
IPPAPI(IppStatus, ippsHashGetTag_rmf,(Ipp8u* pMD, int tagLen, const IppsHashState_rmf* pState))
IPPAPI(IppStatus, ippsHashFinal_rmf,(Ipp8u* pMD, IppsHashState_rmf* pState))
IPPAPI(IppStatus, ippsHashMessage_rmf,(const Ipp8u* pMsg, int len, Ipp8u* pMD, const IppsHashMethod* pMethod))

IPPAPI(IppStatus, ippsHashMethodGetInfo,(IppsHashInfo* pInfo, const IppsHashMethod* pMethod))
IPPAPI(IppStatus, ippsHashGetInfo_rmf,(IppsHashInfo* pInfo, const IppsHashState_rmf* pState))

/* general MGF Primitives*/
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsMGF,(const Ipp8u* pSeed, int seedLen, Ipp8u* pMask, int maskLen, IppHashAlgId hashAlg))
IPPAPI(IppStatus, ippsMGF1_rmf,(const Ipp8u* pSeed, int seedLen, Ipp8u* pMask, int maskLen, const IppsHashMethod* pMethod))
IPPAPI(IppStatus, ippsMGF2_rmf,(const Ipp8u* pSeed, int seedLen, Ipp8u* pMask, int maskLen, const IppsHashMethod* pMethod))


/*
// =========================================================
// Keyed-Hash Message Authentication Codes
// =========================================================
*/

/* generalized Keyed HMAC primitives */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHMAC_GetSize,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHMAC_Init,(const Ipp8u* pKey, int keyLen, IppsHMACState* pCtx, IppHashAlgId hashAlg))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHMAC_Pack,(const IppsHMACState* pCtx, Ipp8u* pBuffer, int bufSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHMAC_Unpack,(const Ipp8u* pBuffer, IppsHMACState* pCtx))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHMAC_Duplicate,(const IppsHMACState* pSrcCtx, IppsHMACState* pDstCtx))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHMAC_Update,(const Ipp8u* pSrc, int len, IppsHMACState* pCtx))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHMAC_Final,(Ipp8u* pMD, int mdLen, IppsHMACState* pCtx))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHMAC_GetTag,(Ipp8u* pMD, int mdLen, const IppsHMACState* pCtx))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsHMAC_Message,(const Ipp8u* pMsg, int msgLen,
                                    const Ipp8u* pKey, int keyLen,
                                    Ipp8u* pMD, int mdLen,
                                    IppHashAlgId hashAlg))

/* method based generalized (reduced memory footprint) Keyed HMAC primitives */
IPPAPI(IppStatus, ippsHMACGetSize_rmf,(int* pSize))
IPPAPI(IppStatus, ippsHMACInit_rmf,(const Ipp8u* pKey, int keyLen,
                                 IppsHMACState_rmf* pCtx,
                                 const IppsHashMethod* pMethod))

IPPAPI(IppStatus, ippsHMACPack_rmf,(const IppsHMACState_rmf* pCtx, Ipp8u* pBuffer, int bufSize))
IPPAPI(IppStatus, ippsHMACUnpack_rmf,(const Ipp8u* pBuffer, IppsHMACState_rmf* pCtx))
IPPAPI(IppStatus, ippsHMACDuplicate_rmf,(const IppsHMACState_rmf* pSrcCtx, IppsHMACState_rmf* pDstCtx))

IPPAPI(IppStatus, ippsHMACUpdate_rmf,(const Ipp8u* pSrc, int len, IppsHMACState_rmf* pCtx))
IPPAPI(IppStatus, ippsHMACFinal_rmf,(Ipp8u* pMD, int mdLen, IppsHMACState_rmf* pCtx))
IPPAPI(IppStatus, ippsHMACGetTag_rmf,(Ipp8u* pMD, int mdLen, const IppsHMACState_rmf* pCtx))
IPPAPI(IppStatus, ippsHMACMessage_rmf,(const Ipp8u* pMsg, int msgLen,
                                       const Ipp8u* pKey, int keyLen,
                                       Ipp8u* pMD, int mdLen,
                                       const IppsHashMethod* pMethod))


/*
// =========================================================
// Big Number Integer Arithmetic
// =========================================================
*/

/* Signed BigNum Operations */
IPPAPI(IppStatus, ippsBigNumGetSize,(int length, int* pSize))
IPPAPI(IppStatus, ippsBigNumInit,(int length, IppsBigNumState* pBN))

IPPAPI(IppStatus, ippsCmpZero_BN,(const IppsBigNumState* pBN, Ipp32u* pResult))
IPPAPI(IppStatus, ippsCmp_BN,(const IppsBigNumState* pA, const IppsBigNumState* pB, Ipp32u* pResult))

IPPAPI(IppStatus, ippsGetSize_BN,(const IppsBigNumState* pBN, int* pSize))
IPPAPI(IppStatus, ippsSet_BN,(IppsBigNumSGN sgn,
                              int length, const Ipp32u* pData,
                              IppsBigNumState* pBN))
IPPAPI(IppStatus, ippsGet_BN,(IppsBigNumSGN* pSgn,
                              int* pLength, Ipp32u* pData,
                              const IppsBigNumState* pBN))
IPPAPI(IppStatus, ippsRef_BN,(IppsBigNumSGN* pSgn, int* bitSize, Ipp32u** const ppData,
                              const IppsBigNumState* pBN))
IPPAPI(IppStatus, ippsExtGet_BN,(IppsBigNumSGN* pSgn,
                              int* pBitSize, Ipp32u* pData,
                              const IppsBigNumState* pBN))

IPPAPI(IppStatus, ippsAdd_BN,   (IppsBigNumState* pA, IppsBigNumState* pB, IppsBigNumState* pR))
IPPAPI(IppStatus, ippsSub_BN,   (IppsBigNumState* pA, IppsBigNumState* pB, IppsBigNumState* pR))
IPPAPI(IppStatus, ippsMul_BN,   (IppsBigNumState* pA, IppsBigNumState* pB, IppsBigNumState* pR))
IPPAPI(IppStatus, ippsMAC_BN_I, (IppsBigNumState* pA, IppsBigNumState* pB, IppsBigNumState* pR))
IPPAPI(IppStatus, ippsDiv_BN,   (IppsBigNumState* pA, IppsBigNumState* pB, IppsBigNumState* pQ, IppsBigNumState* pR))
IPPAPI(IppStatus, ippsMod_BN,   (IppsBigNumState* pA, IppsBigNumState* pM, IppsBigNumState* pR))
IPPAPI(IppStatus, ippsGcd_BN,   (IppsBigNumState* pA, IppsBigNumState* pB, IppsBigNumState* pGCD))
IPPAPI(IppStatus, ippsModInv_BN,(IppsBigNumState* pA, IppsBigNumState* pM, IppsBigNumState* pInv))

IPPAPI(IppStatus, ippsSetOctString_BN,(const Ipp8u* pStr, int strLen, IppsBigNumState* pBN))
IPPAPI(IppStatus, ippsGetOctString_BN,(Ipp8u* pStr, int strLen, const IppsBigNumState* pBN))

/* Montgomery Operations */
IPPAPI(IppStatus, ippsMontGetSize,(IppsExpMethod method, int length, int* pSize))
IPPAPI(IppStatus, ippsMontInit,(IppsExpMethod method, int length, IppsMontState* pCtx))

IPPAPI(IppStatus, ippsMontSet,(const Ipp32u* pModulo, int size, IppsMontState* pCtx))
IPPAPI(IppStatus, ippsMontGet,(Ipp32u* pModulo, int* pSize, const IppsMontState* pCtx))

IPPAPI(IppStatus, ippsMontForm,(const IppsBigNumState* pA, IppsMontState* pCtx, IppsBigNumState* pR))
IPPAPI(IppStatus, ippsMontMul, (const IppsBigNumState* pA, const IppsBigNumState* pB, IppsMontState* m, IppsBigNumState* pR))
IPPAPI(IppStatus, ippsMontExp, (const IppsBigNumState* pA, const IppsBigNumState* pE, IppsMontState* m, IppsBigNumState* pR))

/* Pseudo-Random Number Generation */
IPPAPI(IppStatus, ippsPRNGGetSize,(int* pSize))
IPPAPI(IppStatus, ippsPRNGInit,   (int seedBits, IppsPRNGState* pCtx))

IPPAPI(IppStatus, ippsPRNGSetModulus,(const IppsBigNumState* pMod, IppsPRNGState* pCtx))
IPPAPI(IppStatus, ippsPRNGSetH0,     (const IppsBigNumState* pH0,  IppsPRNGState* pCtx))
IPPAPI(IppStatus, ippsPRNGSetAugment,(const IppsBigNumState* pAug, IppsPRNGState* pCtx))
IPPAPI(IppStatus, ippsPRNGSetSeed,   (const IppsBigNumState* pSeed,IppsPRNGState* pCtx))
IPPAPI(IppStatus, ippsPRNGGetSeed,   (const IppsPRNGState* pCtx,IppsBigNumState* pSeed))

IPPAPI(IppStatus, ippsPRNGen,     (Ipp32u* pRand, int nBits, void* pCtx))
IPPAPI(IppStatus, ippsPRNGen_BN,  (IppsBigNumState* pRand, int nBits, void* pCtx))
IPPAPI(IppStatus, ippsPRNGenRDRAND,   (Ipp32u* pRand, int nBits, void* pCtx))
IPPAPI(IppStatus, ippsPRNGenRDRAND_BN,(IppsBigNumState* pRand, int nBits, void* pCtx))
IPPAPI(IppStatus, ippsTRNGenRDSEED,   (Ipp32u* pRand, int nBits, void* pCtx))
IPPAPI(IppStatus, ippsTRNGenRDSEED_BN,(IppsBigNumState* pRand, int nBits, void* pCtx))

/* Probable Prime Number Generation */
IPPAPI(IppStatus, ippsPrimeGetSize,(int nMaxBits, int* pSize))
IPPAPI(IppStatus, ippsPrimeInit,   (int nMaxBits, IppsPrimeState* pCtx))

IPPAPI(IppStatus, ippsPrimeGen, (int nBits, int nTrials, IppsPrimeState* pCtx,
                                 IppBitSupplier rndFunc, void* pRndParam))
IPPAPI(IppStatus, ippsPrimeTest,(int nTrials, Ipp32u* pResult, IppsPrimeState* pCtx,
                                 IppBitSupplier rndFunc, void* pRndParam))
IPPAPI(IppStatus, ippsPrimeGen_BN,(IppsBigNumState* pPrime, int nBits, int nTrials, IppsPrimeState* pCtx,
                                 IppBitSupplier rndFunc, void* pRndParam))
IPPAPI(IppStatus, ippsPrimeTest_BN,(const IppsBigNumState* pPrime, int nTrials, Ipp32u* pResult, IppsPrimeState* pCtx,
                                 IppBitSupplier rndFunc, void* pRndParam))

IPPAPI(IppStatus, ippsPrimeGet,   (Ipp32u* pPrime, int* pLen, const IppsPrimeState* pCtx))
IPPAPI(IppStatus, ippsPrimeGet_BN,(IppsBigNumState* pPrime, const IppsPrimeState* pCtx))

IPPAPI(IppStatus, ippsPrimeSet,   (const Ipp32u* pPrime, int nBits, IppsPrimeState* pCtx))
IPPAPI(IppStatus, ippsPrimeSet_BN,(const IppsBigNumState* pPrime, IppsPrimeState* pCtx))


/*
// =========================================================
// RSA Cryptography
// =========================================================
*/
IPPAPI(IppStatus, ippsRSA_GetSizePublicKey,(int rsaModulusBitSize, int pubicExpBitSize, int* pKeySize))
IPPAPI(IppStatus, ippsRSA_InitPublicKey,(int rsaModulusBitSize, int publicExpBitSize,
                                         IppsRSAPublicKeyState* pKey, int keyCtxSize))
IPPAPI(IppStatus, ippsRSA_SetPublicKey,(const IppsBigNumState* pModulus,
                                        const IppsBigNumState* pPublicExp,
                                        IppsRSAPublicKeyState* pKey))
IPPAPI(IppStatus, ippsRSA_GetPublicKey,(IppsBigNumState* pModulus,
                                        IppsBigNumState* pPublicExp,
                                  const IppsRSAPublicKeyState* pKey))

IPPAPI(IppStatus, ippsRSA_GetSizePrivateKeyType1,(int rsaModulusBitSize, int privateExpBitSize, int* pKeySize))
IPPAPI(IppStatus, ippsRSA_InitPrivateKeyType1,(int rsaModulusBitSize, int privateExpBitSize,
                                               IppsRSAPrivateKeyState* pKey, int keyCtxSize))
IPPAPI(IppStatus, ippsRSA_SetPrivateKeyType1,(const IppsBigNumState* pModulus,
                                              const IppsBigNumState* pPrivateExp,
                                              IppsRSAPrivateKeyState* pKey))
IPPAPI(IppStatus, ippsRSA_GetPrivateKeyType1,(IppsBigNumState* pModulus,
                                              IppsBigNumState* pPrivateExp,
                                        const IppsRSAPrivateKeyState* pKey))

IPPAPI(IppStatus, ippsRSA_GetSizePrivateKeyType2,(int factorPbitSize, int factorQbitSize, int* pKeySize))
IPPAPI(IppStatus, ippsRSA_InitPrivateKeyType2,(int factorPbitSize, int factorQbitSize,
                                               IppsRSAPrivateKeyState* pKey, int keyCtxSize))
IPPAPI(IppStatus, ippsRSA_SetPrivateKeyType2,(const IppsBigNumState* pFactorP,
                                              const IppsBigNumState* pFactorQ,
                                              const IppsBigNumState* pCrtExpP,
                                              const IppsBigNumState* pCrtExpQ,
                                              const IppsBigNumState* pInverseQ,
                                              IppsRSAPrivateKeyState* pKey))
IPPAPI(IppStatus, ippsRSA_GetPrivateKeyType2,(IppsBigNumState* pFactorP,
                                              IppsBigNumState* pFactorQ,
                                              IppsBigNumState* pCrtExpP,
                                              IppsBigNumState* pCrtExpQ,
                                              IppsBigNumState* pInverseQ,
                                              const IppsRSAPrivateKeyState* pKey))

IPPAPI(IppStatus, ippsRSA_GetBufferSizePublicKey,(int* pBufferSize, const IppsRSAPublicKeyState* pKey))
IPPAPI(IppStatus, ippsRSA_GetBufferSizePrivateKey,(int* pBufferSize, const IppsRSAPrivateKeyState* pKey))

IPPAPI(IppStatus, ippsRSA_Encrypt,(const IppsBigNumState* pPtxt,
                                         IppsBigNumState* pCtxt,
                                   const IppsRSAPublicKeyState* pKey,
                                         Ipp8u* pScratchBuffer))
IPPAPI(IppStatus, ippsRSA_Decrypt,(const IppsBigNumState* pCtxt,
                                         IppsBigNumState* pPtxt,
                                   const IppsRSAPrivateKeyState* pKey,
                                         Ipp8u* pScratchBuffer))

IPPAPI(IppStatus, ippsRSA_GenerateKeys,(const IppsBigNumState* pSrcPublicExp,
                                    IppsBigNumState* pModulus,
                                    IppsBigNumState* pPublicExp,
                                    IppsBigNumState* pPrivateExp,
                                    IppsRSAPrivateKeyState* pPrivateKeyType2,
                                    Ipp8u* pScratchBuffer,
                                    int nTrials,
                                    IppsPrimeState* pPrimeGen,
                                    IppBitSupplier rndFunc, void* pRndParam))

IPPAPI(IppStatus, ippsRSA_ValidateKeys,(int* pResult,
                                 const IppsRSAPublicKeyState* pPublicKey,
                                 const IppsRSAPrivateKeyState* pPrivateKeyType2,
                                 const IppsRSAPrivateKeyState* pPrivateKeyType1,
                                 Ipp8u* pScratchBuffer,
                                 int nTrials,
                                 IppsPrimeState* pPrimeGen,
                                 IppBitSupplier rndFunc, void* pRndParam))

/* encryption scheme: RSAES-OAEP */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsRSAEncrypt_OAEP,(const Ipp8u* pSrc, int srcLen,
                                       const Ipp8u* pLabel, int labLen,
                                       const Ipp8u* pSeed,
                                             Ipp8u* pDst,
                                       const IppsRSAPublicKeyState* pKey,
                                             IppHashAlgId hashAlg,
                                             Ipp8u* pBuffer))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsRSADecrypt_OAEP,(const Ipp8u* pSrc,
                                       const Ipp8u* pLab, int labLen,
                                             Ipp8u* pDst, int* pDstLen,
                                       const IppsRSAPrivateKeyState* pKey,
                                             IppHashAlgId hashAlg,
                                             Ipp8u* pBuffer))

IPPAPI(IppStatus, ippsRSAEncrypt_OAEP_rmf,(const Ipp8u* pSrc, int srcLen,
                                       const Ipp8u* pLabel, int labLen,
                                       const Ipp8u* pSeed,
                                             Ipp8u* pDst,
                                       const IppsRSAPublicKeyState* pKey,
                                       const IppsHashMethod* pMethod,
                                             Ipp8u* pBuffer))

IPPAPI(IppStatus, ippsRSADecrypt_OAEP_rmf,(const Ipp8u* pSrc,
                                       const Ipp8u* pLab, int labLen,
                                             Ipp8u* pDst, int* pDstLen,
                                       const IppsRSAPrivateKeyState* pKey,
                                       const IppsHashMethod* pMethod,
                                             Ipp8u* pBuffer))

/* encryption scheme: RSAES-PKCS_v1_5 */

#define PKCS_DEPRECATED "This algorithm is considered weak due to known attacks on it. \
The functionality remains in the library, but the implementation will no be longer \
optimized and no security patches will be applied. A more secure alternative is available: RSA-OAEP"

IPP_DEPRECATED(PKCS_DEPRECATED) \
IPPAPI(IppStatus, ippsRSAEncrypt_PKCSv15,(const Ipp8u* pSrc, int srcLen,
                                          const Ipp8u* pRndPS,
                                                Ipp8u* pDst,
                                          const IppsRSAPublicKeyState* pKey,
                                                Ipp8u* pBuffer))

IPP_DEPRECATED(PKCS_DEPRECATED) \
IPPAPI(IppStatus, ippsRSADecrypt_PKCSv15,(const Ipp8u* pSrc,
                                                Ipp8u* pDst, int* pDstLen,
                                          const IppsRSAPrivateKeyState* pKey,
                                                Ipp8u* pBuffer))

/* signature scheme : RSA-SSA-PSS */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsRSASign_PSS,(const Ipp8u* pMsg,  int msgLen,
                                   const Ipp8u* pSalt, int saltLen,
                                         Ipp8u* pSign,
                                   const IppsRSAPrivateKeyState* pPrvKey,
                                   const IppsRSAPublicKeyState*  pPubKey,
                                         IppHashAlgId hashAlg,
                                         Ipp8u* pBuffer))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsRSAVerify_PSS,(const Ipp8u* pMsg,  int msgLen,
                                     const Ipp8u* pSign,
                                           int* pIsValid,
                                     const IppsRSAPublicKeyState*  pKey,
                                           IppHashAlgId hashAlg,
                                           Ipp8u* pBuffer))

IPPAPI(IppStatus, ippsRSASign_PSS_rmf,(const Ipp8u* pMsg,  int msgLen,
                                       const Ipp8u* pSalt, int saltLen,
                                             Ipp8u* pSign,
                                       const IppsRSAPrivateKeyState* pPrvKey,
                                       const IppsRSAPublicKeyState*  pPubKey,
                                       const IppsHashMethod* pMethod,
                                             Ipp8u* pBuffer))

IPPAPI(IppStatus, ippsRSAVerify_PSS_rmf,(const Ipp8u* pMsg,  int msgLen,
                                         const Ipp8u* pSign,
                                          int* pIsValid,
                                         const IppsRSAPublicKeyState*  pKey,
                                         const IppsHashMethod* pMethod,
                                               Ipp8u* pBuffer))

/* signature scheme : RSA-SSA-PKCS1-v1_5 */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsRSASign_PKCS1v15,(const Ipp8u* pMsg, int msgLen,
                                              Ipp8u* pSign,
                                        const IppsRSAPrivateKeyState* pPrvKey,
                                        const IppsRSAPublicKeyState*  pPubKey,
                                              IppHashAlgId hashAlg,
                                              Ipp8u* pBuffer))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsRSAVerify_PKCS1v15,(const Ipp8u* pMsg, int msgLen,
                                          const Ipp8u* pSign, int* pIsValid,
                                          const IppsRSAPublicKeyState* pKey,
                                                IppHashAlgId hashAlg,
                                                Ipp8u* pBuffer))

IPPAPI(IppStatus, ippsRSASign_PKCS1v15_rmf,(const Ipp8u* pMsg, int msgLen,
                                                  Ipp8u* pSign,
                                            const IppsRSAPrivateKeyState* pPrvKey,
                                            const IppsRSAPublicKeyState*  pPubKey,
                                            const IppsHashMethod* pMethod,
                                                  Ipp8u* pBuffer))

IPPAPI(IppStatus, ippsRSAVerify_PKCS1v15_rmf,(const Ipp8u* pMsg, int msgLen,
                                              const Ipp8u* pSign, int* pIsValid,
                                              const IppsRSAPublicKeyState* pKey,
                                              const IppsHashMethod* pMethod,
                                                    Ipp8u* pBuffer))

/*
// =========================================================
// DL Cryptography
// =========================================================
*/
IPPAPI( const char*, ippsDLGetResultString, (IppDLResult code))

/* Initialization */
IPPAPI(IppStatus, ippsDLPGetSize,(int bitSizeP, int bitSizeR, int* pSize))
IPPAPI(IppStatus, ippsDLPInit,   (int bitSizeP, int bitSizeR, IppsDLPState* pCtx))

IPPAPI(IppStatus, ippsDLPPack,(const IppsDLPState* pCtx, Ipp8u* pBuffer))
IPPAPI(IppStatus, ippsDLPUnpack,(const Ipp8u* pBuffer, IppsDLPState* pCtx))

/* Set Up and Retrieve Domain Parameters */
IPPAPI(IppStatus, ippsDLPSet,(const IppsBigNumState* pP,
                              const IppsBigNumState* pR,
                              const IppsBigNumState* pG,
                              IppsDLPState* pCtx))
IPPAPI(IppStatus, ippsDLPGet,(IppsBigNumState* pP,
                              IppsBigNumState* pR,
                              IppsBigNumState* pG,
                              IppsDLPState* pCtx))
IPPAPI(IppStatus, ippsDLPSetDP,(const IppsBigNumState* pDP, IppDLPKeyTag tag, IppsDLPState* pCtx))
IPPAPI(IppStatus, ippsDLPGetDP,(IppsBigNumState* pDP, IppDLPKeyTag tag, const IppsDLPState* pCtx))

/* Key Generation, Validation and Set Up */
IPPAPI(IppStatus, ippsDLPGenKeyPair,(IppsBigNumState* pPrvKey, IppsBigNumState* pPubKey,
                                     IppsDLPState* pCtx,
                                     IppBitSupplier rndFunc, void* pRndParam))
IPPAPI(IppStatus, ippsDLPPublicKey, (const IppsBigNumState* pPrvKey,
                                     IppsBigNumState* pPubKey,
                                     IppsDLPState* pCtx))
IPPAPI(IppStatus, ippsDLPValidateKeyPair,(const IppsBigNumState* pPrvKey,
                                     const IppsBigNumState* pPubKey,
                                     IppDLResult* pResult,
                                     IppsDLPState* pCtx))

IPPAPI(IppStatus, ippsDLPSetKeyPair,(const IppsBigNumState* pPrvKey,
                                     const IppsBigNumState* pPubKey,
                                     IppsDLPState* pCtx))

/* Singing/Verifying (DSA version) */
IPPAPI(IppStatus, ippsDLPSignDSA,  (const IppsBigNumState* pMsgDigest,
                                    const IppsBigNumState* pPrvKey,
                                    IppsBigNumState* pSignR, IppsBigNumState* pSignS,
                                    IppsDLPState* pCtx))
IPPAPI(IppStatus, ippsDLPVerifyDSA,(const IppsBigNumState* pMsgDigest,
                                    const IppsBigNumState* pSignR, const IppsBigNumState* pSignS,
                                    IppDLResult* pResult,
                                    IppsDLPState* pCtx))

/* Shared Secret Element (DH version) */
IPPAPI(IppStatus, ippsDLPSharedSecretDH,(const IppsBigNumState* pPrvKeyA,
                                         const IppsBigNumState* pPubKeyB,
                                         IppsBigNumState* pShare,
                                         IppsDLPState* pCtx))

/* DSA's parameter Generation and Validation */
IPPAPI(IppStatus, ippsDLPGenerateDSA,(const IppsBigNumState* pSeedIn,
                                      int nTrials, IppsDLPState* pCtx,
                                      IppsBigNumState* pSeedOut, int* pCounter,
                                      IppBitSupplier rndFunc, void* pRndParam))
IPPAPI(IppStatus, ippsDLPValidateDSA,(int nTrials, IppDLResult* pResult, IppsDLPState* pCtx,
                                      IppBitSupplier rndFunc, void* pRndParam))

/* DH parameter's Generation and Validation */
IPPAPI(IppStatus, ippsDLPGenerateDH,(const IppsBigNumState* pSeedIn,
                                     int nTrials, IppsDLPState* pCtx,
                                     IppsBigNumState* pSeedOut, int* pCounter,
                                     IppBitSupplier rndFunc, void* pRndParam))
IPPAPI(IppStatus, ippsDLPValidateDH,(int nTrials, IppDLResult* pResult, IppsDLPState* pCtx,
                                     IppBitSupplier rndFunc, void* pRndParam))


/*
// =========================================================
// EC Cryptography
// =========================================================
*/
IPPAPI( const char*, ippsECCGetResultString, (IppECResult code))

/*
// EC over Prime Fields
*/
/* general EC initialization */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPGetSize,(int feBitSize, int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPGetSizeStd128r1,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPGetSizeStd128r2,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPGetSizeStd192r1,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPGetSizeStd224r1,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPGetSizeStd256r1,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPGetSizeStd384r1,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPGetSizeStd521r1,(int* pSize))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPGetSizeStdSM2,  (int* pSize))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPInit,(int feBitSize, IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPInitStd128r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPInitStd128r2,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPInitStd192r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPInitStd224r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPInitStd256r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPInitStd384r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPInitStd521r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPInitStdSM2,  (IppsECCPState* pEC))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSet,(const IppsBigNumState* pPrime,
                               const IppsBigNumState* pA, const IppsBigNumState* pB,
                               const IppsBigNumState* pGX,const IppsBigNumState* pGY,const IppsBigNumState* pOrder,
                               int cofactor,
                               IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSetStd,(IppECCType flag, IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSetStd128r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSetStd128r2,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSetStd192r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSetStd224r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSetStd256r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSetStd384r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSetStd521r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSetStdSM2,  (IppsECCPState* pEC))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPBindGxyTblStd192r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPBindGxyTblStd224r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPBindGxyTblStd256r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPBindGxyTblStd384r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPBindGxyTblStd521r1,(IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPBindGxyTblStdSM2,  (IppsECCPState* pEC))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPGet,(IppsBigNumState* pPrime,
                               IppsBigNumState* pA, IppsBigNumState* pB,
                               IppsBigNumState* pGX,IppsBigNumState* pGY,IppsBigNumState* pOrder,
                               int* cofactor,
                               IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPGetOrderBitSize,(int* pBitSize, IppsECCPState* pEC))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPValidate,(int nTrials, IppECResult* pResult, IppsECCPState* pEC,
                                    IppBitSupplier rndFunc, void* pRndParam))

/* EC Point */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPPointGetSize,(int feBitSize, int* pSize))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPPointInit,(int feBitSize, IppsECCPPointState* pPoint))

/* Setup/retrieve point's coordinates */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSetPoint,(const IppsBigNumState* pX, const IppsBigNumState* pY,
                                    IppsECCPPointState* pPoint, IppsECCPState* pEC))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSetPointAtInfinity,(IppsECCPPointState* pPoint, IppsECCPState* pEC))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPGetPoint,(IppsBigNumState* pX, IppsBigNumState* pY,
                                    const IppsECCPPointState* pPoint, IppsECCPState* pEC))

/* EC Point Operations */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPCheckPoint,(const IppsECCPPointState* pP,
                                      IppECResult* pResult, IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPComparePoint,(const IppsECCPPointState* pP, const IppsECCPPointState* pQ,
                                        IppECResult* pResult, IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPNegativePoint,(const IppsECCPPointState* pP,
                                         IppsECCPPointState* pR, IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPAddPoint,(const IppsECCPPointState* pP, const IppsECCPPointState* pQ,
                                    IppsECCPPointState* pR, IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPMulPointScalar,(const IppsECCPPointState* pP, const IppsBigNumState* pK,
                                          IppsECCPPointState* pR, IppsECCPState* pEC))

/* Key Generation, Setup and Validation */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPGenKeyPair,(IppsBigNumState* pPrivate, IppsECCPPointState* pPublic,
                                      IppsECCPState* pEC,
                                      IppBitSupplier rndFunc, void* pRndParam))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPPublicKey,(const IppsBigNumState* pPrivate,
                                     IppsECCPPointState* pPublic,
                                     IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPValidateKeyPair,(const IppsBigNumState* pPrivate, const IppsECCPPointState* pPublic,
                                           IppECResult* pResult,
                                           IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSetKeyPair,(const IppsBigNumState* pPrivate, const IppsECCPPointState* pPublic,
                                      IppBool regular,
                                      IppsECCPState* pEC))

/* Shared Secret (DH scheme ) */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSharedSecretDH,(const IppsBigNumState* pPrivateA,
                                          const IppsECCPPointState* pPublicB,
                                          IppsBigNumState* pShare,
                                          IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSharedSecretDHC,(const IppsBigNumState* pPrivateA,
                                           const IppsECCPPointState* pPublicB,
                                           IppsBigNumState* pShare,
                                           IppsECCPState* pEC))

/* Sing/Verify */
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSignDSA,(const IppsBigNumState* pMsgDigest,
                        const IppsBigNumState* pPrivate,
                        IppsBigNumState* pSignX, IppsBigNumState* pSignY,
                        IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPVerifyDSA,(const IppsBigNumState* pMsgDigest,
                        const IppsBigNumState* pSignX, const IppsBigNumState* pSignY,
                        IppECResult* pResult,
                        IppsECCPState* pEC))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSignNR,(const IppsBigNumState* pMsgDigest,
                        const IppsBigNumState* pPrivate,
                        IppsBigNumState* pSignX, IppsBigNumState* pSignY,
                        IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPVerifyNR,(const IppsBigNumState* pMsgDigest,
                        const IppsBigNumState* pSignX, const IppsBigNumState* pSignY,
                        IppECResult* pResult,
                        IppsECCPState* pEC))

IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPSignSM2,(const IppsBigNumState* pMsgDigest,
                        const IppsBigNumState* pRegPrivate,
                        IppsBigNumState* pEphPrivate,
                        IppsBigNumState* pSignR, IppsBigNumState* pSignS,
                        IppsECCPState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsECCPVerifySM2,(const IppsBigNumState* pMsgDigest,
                        const IppsECCPPointState* pRegPublic,
                        const IppsBigNumState* pSignR, const IppsBigNumState* pSignS,
                        IppECResult* pResult,
                        IppsECCPState* pEC))

/*
// GF over prime and its extension
*/
IPPAPI(IppStatus, ippsGFpGetSize, (int feBitSize, int* pSize))
IPPAPI(IppStatus, ippsGFpInitArbitrary,(const IppsBigNumState* pPrime, int primeBitSize, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpInitFixed,(int primeBitSize, const IppsGFpMethod* pGFpMethod, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpInit,    (const IppsBigNumState* pPrime, int primeBitSize, const IppsGFpMethod* pGFpMethod, IppsGFpState* pGFp))
IPPAPI( const IppsGFpMethod*, ippsGFpMethod_p192r1, (void) )
IPPAPI( const IppsGFpMethod*, ippsGFpMethod_p224r1, (void) )
IPPAPI( const IppsGFpMethod*, ippsGFpMethod_p256r1, (void) )
IPPAPI( const IppsGFpMethod*, ippsGFpMethod_p384r1, (void) )
IPPAPI( const IppsGFpMethod*, ippsGFpMethod_p521r1, (void) )
IPPAPI( const IppsGFpMethod*, ippsGFpMethod_p256sm2,(void) )
IPPAPI( const IppsGFpMethod*, ippsGFpMethod_p256bn, (void) )
IPPAPI( const IppsGFpMethod*, ippsGFpMethod_p256,   (void) )
IPPAPI( const IppsGFpMethod*, ippsGFpMethod_pArb,   (void) )

IPPAPI(IppStatus, ippsGFpxGetSize,(const IppsGFpState* pGroundGF, int degree, int* pSize))
IPPAPI(IppStatus, ippsGFpxInit,   (const IppsGFpState* pGroundGF, int extDeg, const IppsGFpElement* const ppGroundElm[], int nElm, const IppsGFpMethod* pGFpMethod, IppsGFpState* pGFpx))
IPPAPI(IppStatus, ippsGFpxInitBinomial,(const IppsGFpState* pGroundGF, int extDeg, const IppsGFpElement* pGroundElm, const IppsGFpMethod* pGFpMethod, IppsGFpState* pGFpx))
IPPAPI( const IppsGFpMethod*, ippsGFpxMethod_binom2_epid2,(void) )
IPPAPI( const IppsGFpMethod*, ippsGFpxMethod_binom3_epid2,(void) )
IPPAPI( const IppsGFpMethod*, ippsGFpxMethod_binom2,      (void) )
IPPAPI( const IppsGFpMethod*, ippsGFpxMethod_binom3,      (void) )
IPPAPI( const IppsGFpMethod*, ippsGFpxMethod_binom,       (void) )
IPPAPI( const IppsGFpMethod*, ippsGFpxMethod_com,         (void) )

IPPAPI(IppStatus, ippsGFpScratchBufferSize,(int nExponents, int ExpBitSize, const IppsGFpState* pGFp, int* pBufferSize))

IPPAPI(IppStatus, ippsGFpElementGetSize,(const IppsGFpState* pGFp, int* pElementSize))
IPPAPI(IppStatus, ippsGFpElementInit,   (const Ipp32u* pA, int lenA, IppsGFpElement* pR, IppsGFpState* pGFp))

IPPAPI(IppStatus, ippsGFpSetElement,      (const Ipp32u* pA, int lenA, IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpSetElementRegular,(const IppsBigNumState* pBN, IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpSetElementOctString,(const Ipp8u* pStr, int strSize, IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpSetElementRandom,(IppsGFpElement* pR, IppsGFpState* pGFp, IppBitSupplier rndFunc, void* pRndParam))
IPPAPI(IppStatus, ippsGFpSetElementHash,(const Ipp8u* pMsg, int msgLen, IppsGFpElement* pElm, IppsGFpState* pGFp, IppHashAlgId hashID))
IPPAPI(IppStatus, ippsGFpSetElementHash_rmf,(const Ipp8u* pMsg, int msgLen, IppsGFpElement* pElm, IppsGFpState* pGFp, const IppsHashMethod* pMethod))
IPPAPI(IppStatus, ippsGFpCpyElement,(const IppsGFpElement* pA, IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpGetElement,(const IppsGFpElement* pA, Ipp32u* pDataA, int lenA, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpGetElementOctString,(const IppsGFpElement* pA, Ipp8u* pStr, int strSize, IppsGFpState* pGFp))

IPPAPI(IppStatus, ippsGFpCmpElement,(const IppsGFpElement* pA, const IppsGFpElement* pB, int* pResult, const IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpIsZeroElement,(const IppsGFpElement* pA, int* pResult, const IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpIsUnityElement,(const IppsGFpElement* pA, int* pResult, const IppsGFpState* pGFp))

IPPAPI(IppStatus, ippsGFpConj,(const IppsGFpElement* pA, IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpNeg, (const IppsGFpElement* pA, IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpInv, (const IppsGFpElement* pA, IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpSqrt,(const IppsGFpElement* pA, IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpSqr, (const IppsGFpElement* pA, IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpAdd, (const IppsGFpElement* pA, const IppsGFpElement* pB,  IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpSub, (const IppsGFpElement* pA, const IppsGFpElement* pB,  IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpMul, (const IppsGFpElement* pA, const IppsGFpElement* pB,  IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpExp, (const IppsGFpElement* pA, const IppsBigNumState* pE, IppsGFpElement* pR, IppsGFpState* pGFp, Ipp8u* pScratchBuffer))
IPPAPI(IppStatus, ippsGFpMultiExp,(const IppsGFpElement* const ppElmA[], const IppsBigNumState* const ppE[], int nItems, IppsGFpElement* pR, IppsGFpState* pGFp, Ipp8u* pScratchBuffer))

IPPAPI(IppStatus, ippsGFpAdd_PE,(const IppsGFpElement* pA, const IppsGFpElement* pParentB, IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpSub_PE,(const IppsGFpElement* pA, const IppsGFpElement* pParentB, IppsGFpElement* pR, IppsGFpState* pGFp))
IPPAPI(IppStatus, ippsGFpMul_PE,(const IppsGFpElement* pA, const IppsGFpElement* pParentB, IppsGFpElement* pR, IppsGFpState* pGFp))

IPPAPI(IppStatus, ippsGFpGetInfo, (IppsGFpInfo* pInfo, const IppsGFpState* pGFp))

/* ================== */
IPPAPI(IppStatus, ippsGFpECGetSize,(const IppsGFpState* pGFp, int* pSize))
IPPAPI(IppStatus, ippsGFpECInit,   (const IppsGFpState* pGFp,
                                    const IppsGFpElement* pA, const IppsGFpElement* pB,
                                    IppsGFpECState* pEC))

IPPAPI(IppStatus, ippsGFpECSet,(const IppsGFpElement* pA, const IppsGFpElement* pB,
                                IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECSetSubgroup,(const IppsGFpElement* pX, const IppsGFpElement* pY,
                                        const IppsBigNumState* pOrder,
                                        const IppsBigNumState* pCofactor,
                                        IppsGFpECState* pEC))

IPPAPI(IppStatus, ippsGFpECInitStd128r1,(const IppsGFpState* pGFp, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECInitStd128r2,(const IppsGFpState* pGFp, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECInitStd192r1,(const IppsGFpState* pGFp, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECInitStd224r1,(const IppsGFpState* pGFp, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECInitStd256r1,(const IppsGFpState* pGFp, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECInitStd384r1,(const IppsGFpState* pGFp, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECInitStd521r1,(const IppsGFpState* pGFp, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECInitStdSM2,  (const IppsGFpState* pGFp, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECInitStdBN256,(const IppsGFpState* pGFp, IppsGFpECState* pEC))

IPPAPI(IppStatus, ippsGFpECBindGxyTblStd192r1,(IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECBindGxyTblStd224r1,(IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECBindGxyTblStd256r1,(IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECBindGxyTblStd384r1,(IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECBindGxyTblStd521r1,(IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECBindGxyTblStdSM2,  (IppsGFpECState* pEC))

IPPAPI(IppStatus, ippsGFpECGet,(IppsGFpState** const ppGFp,
                                IppsGFpElement* pA, IppsGFpElement* pB,
                                const IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECGetSubgroup,(IppsGFpState** const ppGFp,
                                     IppsGFpElement* pX, IppsGFpElement* pY,
                                     IppsBigNumState* pOrder,IppsBigNumState* pCofactor,
                                     const IppsGFpECState* pEC))

IPPAPI(IppStatus, ippsGFpECScratchBufferSize,(int nScalars, const IppsGFpECState* pEC, int* pBufferSize))

IPPAPI(IppStatus, ippsGFpECVerify,(IppECResult* pResult, IppsGFpECState* pEC, Ipp8u* pScratchBuffer))

IPPAPI(IppStatus, ippsGFpECPointGetSize,(const IppsGFpECState* pEC, int* pSize))
IPPAPI(IppStatus, ippsGFpECPointInit,   (const IppsGFpElement* pX, const IppsGFpElement* pY, IppsGFpECPoint* pPoint, IppsGFpECState* pEC))

IPPAPI(IppStatus, ippsGFpECSetPointAtInfinity,(IppsGFpECPoint* pPoint, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECSetPoint,(const IppsGFpElement* pX, const IppsGFpElement* pY, IppsGFpECPoint* pPoint, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECSetPointRegular,(const IppsBigNumState* pX, const IppsBigNumState* pY, IppsGFpECPoint* pPoint, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECSetPointRandom,(IppsGFpECPoint* pPoint, IppsGFpECState* pEC, IppBitSupplier rndFunc, void* pRndParam, Ipp8u* pScratchBuffer))
IPPAPI(IppStatus, ippsGFpECMakePoint,(const IppsGFpElement* pX, IppsGFpECPoint* pPoint, IppsGFpECState* pEC))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsGFpECSetPointHash,(Ipp32u hdr, const Ipp8u* pMsg, int msgLen, IppsGFpECPoint* pPoint, IppsGFpECState* pEC, IppHashAlgId hashID, Ipp8u* pScratchBuffer))
IPP_DEPRECATED(OBSOLETE_API) \
IPPAPI(IppStatus, ippsGFpECSetPointHashBackCompatible,(Ipp32u hdr, const Ipp8u* pMsg, int msgLen, IppsGFpECPoint* pPoint, IppsGFpECState* pEC, IppHashAlgId hashID, Ipp8u* pScratchBuffer))
IPPAPI(IppStatus, ippsGFpECSetPointHash_rmf,(Ipp32u hdr, const Ipp8u* pMsg, int msgLen, IppsGFpECPoint* pPoint, IppsGFpECState* pEC, const IppsHashMethod* pMethod, Ipp8u* pScratchBuffer))
IPPAPI(IppStatus, ippsGFpECSetPointHashBackCompatible_rmf,(Ipp32u hdr, const Ipp8u* pMsg, int msgLen, IppsGFpECPoint* pPoint, IppsGFpECState* pEC, const IppsHashMethod* pMethod, Ipp8u* pScratchBuffer))
IPPAPI(IppStatus, ippsGFpECGetPoint,(const IppsGFpECPoint* pPoint, IppsGFpElement* pX, IppsGFpElement* pY, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECGetPointRegular,(const IppsGFpECPoint* pPoint, IppsBigNumState* pX, IppsBigNumState* pY, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECSetPointOctString,(const Ipp8u* pStr, int strLen, IppsGFpECPoint* pPoint, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECGetPointOctString,(const IppsGFpECPoint* pPoint, Ipp8u* pStr, int strLen, IppsGFpECState* pEC))

IPPAPI(IppStatus, ippsGFpECTstPoint,(const IppsGFpECPoint* pP, IppECResult* pResult, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECTstPointInSubgroup,(const IppsGFpECPoint* pP, IppECResult* pResult, IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
IPPAPI(IppStatus, ippsGFpECCpyPoint,(const IppsGFpECPoint* pA, IppsGFpECPoint* pR, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECCmpPoint,(const IppsGFpECPoint* pP, const IppsGFpECPoint* pQ, IppECResult* pResult, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECNegPoint,(const IppsGFpECPoint* pP, IppsGFpECPoint* pR, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECAddPoint,(const IppsGFpECPoint* pP, const IppsGFpECPoint* pQ, IppsGFpECPoint* pR, IppsGFpECState* pEC))
IPPAPI(IppStatus, ippsGFpECMulPoint,(const IppsGFpECPoint* pP, const IppsBigNumState* pN, IppsGFpECPoint* pR, IppsGFpECState* pEC, Ipp8u* pScratchBuffer))

/* keys */
IPPAPI(IppStatus, ippsGFpECPrivateKey,(IppsBigNumState* pPrivate, IppsGFpECState* pEC,
                                       IppBitSupplier rndFunc, void* pRndParam))
IPPAPI(IppStatus, ippsGFpECPublicKey,(const IppsBigNumState* pPrivate, IppsGFpECPoint* pPublic,
                        IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
IPPAPI(IppStatus, ippsGFpECTstKeyPair,(const IppsBigNumState* pPrivate, const IppsGFpECPoint* pPublic, IppECResult* pResult,
                        IppsGFpECState* pEC, Ipp8u* pScratchBuffer))

/* DH shared secret */
IPPAPI(IppStatus, ippsGFpECSharedSecretDH,(const IppsBigNumState* pPrivateA, const IppsGFpECPoint* pPublicB,
                        IppsBigNumState* pShare,
                        IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
IPPAPI(IppStatus, ippsGFpECSharedSecretDHC,(const IppsBigNumState* pPrivateA,
                        const IppsGFpECPoint* pPublicB,
                        IppsBigNumState* pShare,
                        IppsGFpECState* pEC, Ipp8u* pScratchBuffer))

/* sign generation/verification of DSA, NR, SM2 */
IPPAPI(IppStatus, ippsGFpECSignDSA,(const IppsBigNumState* pMsgDigest,
                        const IppsBigNumState* pRegPrivate,
                        IppsBigNumState* pEphPrivate,
                        IppsBigNumState* pSignR, IppsBigNumState* pSignS,
                        IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
IPPAPI(IppStatus, ippsGFpECVerifyDSA,(const IppsBigNumState* pMsgDigest,
                        const IppsGFpECPoint* pRegPublic,
                        const IppsBigNumState* pSignR, const IppsBigNumState* pSignS,
                        IppECResult* pResult,
                        IppsGFpECState* pEC, Ipp8u* pScratchBuffer))

IPPAPI(IppStatus, ippsGFpECSignNR,(const IppsBigNumState* pMsgDigest,
                        const IppsBigNumState* pRegPrivate,
                        IppsBigNumState* pEphPrivate,
                        IppsBigNumState* pSignR, IppsBigNumState* pSignS,
                        IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
IPPAPI(IppStatus, ippsGFpECVerifyNR,(const IppsBigNumState* pMsgDigest,
                        const IppsGFpECPoint* pRegPublic,
                        const IppsBigNumState* pSignR, const IppsBigNumState* pSignS,
                        IppECResult* pResult,
                        IppsGFpECState* pEC, Ipp8u* pScratchBuffer))

IPPAPI(IppStatus, ippsGFpECSignSM2,(const IppsBigNumState* pMsgDigest,
                        const IppsBigNumState* pRegPrivate,
                        IppsBigNumState* pEphPrivate,
                        IppsBigNumState* pSignR, IppsBigNumState* pSignS,
                        IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
IPPAPI(IppStatus, ippsGFpECVerifySM2,(const IppsBigNumState* pMsgDigest,
                        const IppsGFpECPoint* pRegPublic,
                        const IppsBigNumState* pSignR, const IppsBigNumState* pSignS,
                        IppECResult* pResult,
                        IppsGFpECState* pEC, Ipp8u* pScratchBuffer))

IPPAPI(IppStatus, ippsGFpECGetInfo_GF,(IppsGFpInfo* pInfo, const IppsGFpECState* pEC))

IPPAPI(IppStatus, ippsGFpECESGetSize_SM2, (const IppsGFpECState* pEC, int* pSize))
IPPAPI(IppStatus, ippsGFpECESInit_SM2, (IppsGFpECState* pEC,
                        IppsECESState_SM2* pState, int avaliableCtxSize))
IPPAPI(IppStatus, ippsGFpECESSetKey_SM2, (const IppsBigNumState* pPrivate,
                        const IppsGFpECPoint* pPublic,
                        IppsECESState_SM2* pState,
                        IppsGFpECState* pEC,
                        Ipp8u* pEcScratchBuffer))
IPPAPI(IppStatus, ippsGFpECESStart_SM2, (IppsECESState_SM2* pState))
IPPAPI(IppStatus, ippsGFpECESEncrypt_SM2, (const Ipp8u* pInput, Ipp8u* pOutput,
                        int dataLen, IppsECESState_SM2* pState))
IPPAPI(IppStatus, ippsGFpECESDecrypt_SM2, (const Ipp8u* pInput, Ipp8u* pOutput,
                        int dataLen, IppsECESState_SM2* pState))
IPPAPI(IppStatus, ippsGFpECESFinal_SM2, (Ipp8u* pTag, int tagLen, IppsECESState_SM2* pState))
IPPAPI(IppStatus, ippsGFpECESGetBuffersSize_SM2, (int* pPublicKeySize,
                        int* pMaximumTagSize, const IppsECESState_SM2* pState))

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning(pop)
#endif
#ifdef  __cplusplus
}
#endif


#endif /* IPPCP_H__ */
