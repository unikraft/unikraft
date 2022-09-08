/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
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

/*!
 *
 *  \file
 *
 *  \brief RSASSA-PSS signature generation scheme usage example.
 *
 *  This example demonstrates message signature generation according to
 *  RSASSA-PSS scheme with 3072-bit RSA modulus and SHA-384 hash function.
 *  It uses Reduced Memory Footprint (_rmf) version of the function.
 *
 *  The RSASSA-PSS scheme is implemented according to the PKCS#1 v2.1: RSA Cryptography Standard (June 2002),
 *  available at:
 *
 *  https://tools.ietf.org/html/rfc3447.
 *
 */

#include <cstring>

#include "ippcp.h"
#include "examples_common.h"
#include "bignum.h"

/*! 3072-bit RSA Modulus N = P*Q */
BigNumber N("0xA7A1882A7FB896786034D07FB1B9F6327C27BDD7CE6FE39C285AE3B6C34259ADC0DC4F7B9C7DEC3CA4A20D3407339EEDD\
7A12A421DA18F5954673CAC2FF059156ECC73C6861EC761E6A0F2A5A033A6768C6A42D8B459E1B4932349E84EFD92DF59B45935F3D0E3081\
7C66201AA99D07AE36C5D74F408D69CC08F044151FF4960E531360CB19077833ADF7BCE77ECFAA133C0CCC63C93B856814569E0B9884EE55\
4061B9A20AB46C38263C094DAE791AA61A17F8D16F0E85B7E5CE3B067ECE89E20BC4E8F1AE814B276D234E04F4E766F501DA74EA7E3817C2\
4EA35D016676CECE652B823B051625573CA92757FC720D254ECF1DCBBFD21D98307561ECAAB545480C7C52AD7E9FA6B597F5FE550559C2FE\
923205AC1761A99737CA02D7B19822E008A8969349C87FB874C81620E38F613C8521F0381FE5BA55B74827DAD3E1CF2AA29C6933629F2B28\
6AD11BE88FA6436E7E3F64A75E3595290DC0D1CD5EEE7AAAC54959CC53BD5A934A365E72DD81A2BD4FB9A67821BFFEDF2EF2BD94913DE8B");

/*! Public exponent */
BigNumber E("0x1415a7");

/*! Private exponent */
BigNumber D("0x073A5FC4CD642F6113DFFC4F84035CEE3A2B8ACC549703751A1D6A5EAA13487229A58EF7D7A522BB9F4F25510F1AA0F74\
C6A8FC8A5C5BE8B91A674EDE50E92F7E34A90A3C9DA999FFFB1D695E4588F451256C163484C151350CB9C7825A7D910845EE5CF826FECF9A\
7C0FBBBBA22BB4A531C131D2E7761BA898F002EBEF8AB87218511F81D3266E1EC07A7CA8622514C6DFDC86C67679A2C8F5F031DE9A0C22B5\
A88060B46EE0C64D3B9AF3C0A379BCD9C6A1B51CF6480456D3FD6DEF94CD2A6C171DD3F010E3C9D662BC857208248C94EBCB9FD997B9FF4A\
7E5FD95558569906525E741D78344F6F6CFDBD59D4FAA52EE3FA964FB7CCCB2D6BE1935D211FE1498217716273939A946081FD8509913FD4\
7747C5C2F03EFD4D6FC9C6FCFD8402E9F40A0A5B3DE3CA2B3C0FAC9456938FAA6CF2C20E3912E5981C9876D8CA1FF29B87A15EEAE0CCCE3F\
8A8F1E405091C083B98BCC5FE0D0DEAAE33C67C0394437F0ECCB385B7EFB17AEEBBA8AFAECCA30A2F63EAC8F0AC8F1EACAD85BBCAF3960B");

/*! Reference value of signature */
static const Ipp8u signatureRef[] =
"\x96\x87\x11\x5b\xe4\x78\xe4\xb6\x42\xcd\x36\x93\x92\xb9\xdd\x0f\x35\x76\xe7\x04\xaf\x72\x18\xb1\xf9\x4d\x7f\x8f\
\xe7\xf0\x70\x73\xe3\xe8\xe1\x18\x6f\xa7\x68\x97\x7d\x6b\x51\x4e\x51\x34\x59\xf2\x37\x3d\xf6\xec\x52\xe3\xde\x9b\
\xd8\x3f\xcc\x5c\xc3\xe6\xb9\x7f\x8b\x3f\xb5\x34\x16\x3c\x64\xf5\x26\x76\x20\x70\x0e\x9d\x8c\x52\xb3\xdf\x61\xa7\
\xc3\x74\x8e\xf1\x59\xd6\xb3\x90\x89\x5a\xfa\x3a\xf5\x91\x09\xa5\x47\x8d\x01\x6d\x96\xc4\x9f\x68\xdf\xc7\x35\xba\
\x2a\xaf\xd5\x01\x2c\x13\x51\x5e\xd6\x64\x4f\x0d\x41\x09\xc4\x55\x56\xe1\x4a\x38\x21\xe1\xaa\x24\xbe\xb8\xa8\x1a\
\x48\xda\x27\xf1\x31\xde\x84\xf7\xba\x51\x58\x1d\x81\xb8\xff\x31\xba\x92\xb8\xa1\xfd\xe8\x67\xf0\x7e\x32\xe6\xc2\
\x70\x92\x53\x44\x81\x74\xdd\x31\x32\x4d\xbc\x32\xb0\x5f\x07\x58\x7f\x76\xa9\x99\x7d\xec\xb8\x0f\x38\xd8\xc1\x3d\
\x0f\x6e\xb3\xc1\x0e\x3d\x96\xa2\x29\x3f\x74\x64\xf1\xe0\x46\x02\xef\x6e\x84\xc2\xd0\x24\x5d\x7d\xb2\x56\xa6\x7d\
\x13\x2a\x47\xca\xe9\xab\xe0\x6b\x61\xa8\x96\x8f\x50\xa1\x74\x99\x95\xdc\x15\xef\x0d\xcb\x1d\x5f\x59\x59\xe4\xd4\
\x54\xc8\x54\x7b\xbb\x4d\x19\x56\x98\xf4\x84\x61\x7b\xfd\x12\x2a\xca\xae\x2d\x0e\x8c\x76\xd2\x8b\x24\x00\x5a\xb0\
\x3c\xaa\x78\x1e\xa9\x7b\x1c\x4d\x93\x96\xa1\x6f\x79\x98\xee\xe7\xdd\xd9\xde\x4c\xab\xe5\x70\x32\xd9\x43\x8a\x5d\
\x99\xc6\xb3\x4a\x95\x61\x22\x35\x02\x63\xc7\xe9\x98\xbc\x61\xde\xc9\x13\x81\x01\x2e\x68\x6d\x07\x9e\x39\xe9\x6b\
\x1e\xa4\xbf\xdb\x7c\xdf\x63\x0d\xdb\x42\x2c\x6b\x58\x0e\x55\x06\xc9\xcc\x3d\x6c\x10\x0f\x20\x41\xd1\x7c\xea\xaa\
\xa5\x45\x89\x24\x9f\x04\xa1\x37\x0f\xfa\x3b\xf3\xff\x1a\xde\xb8\x90\x68\x86\x98";

/*! Message to be signed */
static const Ipp8u sourceMessage[] =
"\x92\x21\xf0\xfe\x91\x15\x84\x35\x54\xd5\x68\x5d\x9f\xe6\x9d\xc4\x9e\x95\xce\xb5\x79\x39\x86\xe4\x28\xb8\xa1\x0b\
\x89\x4c\x01\xd6\xaf\x87\x82\xfd\x7d\x95\x2f\xaf\x74\xc2\xb6\x37\xca\x3b\x19\xda\xbc\x19\xa7\xfe\x25\x9b\x2b\x92\
\x4e\xb3\x63\xa9\x08\xc5\xb3\x68\xf8\xab\x1b\x23\x33\xfc\x67\xc3\x0b\x8e\xa5\x6b\x28\x39\xdc\x5b\xda\xde\xfb\x14\
\xad\xa8\x10\xbc\x3e\x92\xba\xc5\x4e\x2a\xe1\xca\x15\x94\xa4\xb9\xd8\xd1\x93\x37\xbe\x42\x1f\x40\xe0\x67\x4e\x0e\
\x9f\xed\xb4\x3d\x3a\xe8\x9e\x2c\xa0\x5d\x90\xa6\x82\x03\xf2\xc2";

/*! Salt */
static const Ipp8u salt[] =
"\x61\xa7\x62\xf8\x96\x8d\x5f\x36\x7e\x2d\xbc\xac\xb4\x02\x16\x53\xdc\x75\x43\x7d\x90\x00\xe3\x16\x9d\x94\x37\x29\
\x70\x38\x37\xa5\xcb\xf4\xde\x62\xbd\xed\xc9\x5f\xd0\xd1\x00\x4e\x84\x75\x14\x52";


/*! Main function  */
int main(void)
{
    /* RSA Modulus N = P*Q in bits */
    const int RSA_MODULUS = 3072;

    /* Internal function status */
    IppStatus status = ippStsNoErr;

    /* Size in bits of RSA modulus */
    const int bitSizeN = N.BitSize();
    /* Size in bits of RSA private exponent */
    const int bitSizeD = D.BitSize();

    /* Allocate memory for signature.
     * Size shall be equal to the RSA modulus size.
     */
    const int signatureLen = bitSizeInBytes(RSA_MODULUS);
    Ipp8u* pSignature = new Ipp8u[signatureLen];

    /* Allocate memory private key.
     * There are two types of private keys that are supported: Type1 and Type2.
     * You can choose any of them, depending on your private key representation.
     * This example uses Type1 key.
     * For more information, see https://software.intel.com/en-us/ipp-crypto-reference-2019-rsa-getsizepublickey-rsa-getsizeprivatekeytype1-rsa-getsizeprivatekeytype2
     */
    int keySize = 0;
    ippsRSA_GetSizePrivateKeyType1(bitSizeN, bitSizeD, &keySize);
    IppsRSAPrivateKeyState* pPrvKeyType1 = (IppsRSAPrivateKeyState*)(new Ipp8u[keySize]);
    ippsRSA_InitPrivateKeyType1(bitSizeN, bitSizeD, pPrvKeyType1, keySize);

    if (pPrvKeyType1) {
        do {
            /* Set private key */
            status = ippsRSA_SetPrivateKeyType1(N, D, pPrvKeyType1);
            if (!checkStatus("ippsRSA_SetPrivateKeyType1", ippStsNoErr, status))
                break;

            /* Calculate temporary buffer size */
            int bufSize = 0;
            status = ippsRSA_GetBufferSizePrivateKey(&bufSize, pPrvKeyType1);
            if (!checkStatus("ippsRSA_GetBufferSizePrivateKey", ippStsNoErr, status))
                break;

            Ipp8u* pScratchBuffer = new Ipp8u[bufSize];

            /* Sign message with use of SHA384 hash function */
            status = ippsRSASign_PSS_rmf(sourceMessage,  sizeof(sourceMessage)-1,
                                         salt, sizeof(salt)-1,
                                         pSignature,
                                         pPrvKeyType1, NULL /* public key */,
                                         ippsHashMethod_SHA384(),
                                         pScratchBuffer);

            if (pScratchBuffer) delete [] pScratchBuffer;

            if (!checkStatus("ippsRSASign_PSS_rmf", ippStsNoErr, status))
                break;

            /* Compare signature with expected value */
            if (0 != memcmp(signatureRef, pSignature, signatureLen)) {
                printf("ERROR: Signature and reference value do not match\n");
                status = ippStsErr;
            }
        } while (0);
    }

    if (pPrvKeyType1) delete [] (Ipp8u*)pPrvKeyType1;
    if (pSignature) delete [] pSignature;

    PRINT_EXAMPLE_STATUS("ippsRSASign_PSS_rmf", "RSA-PSS 3072 (SHA-384) Type1 Signature", ippStsNoErr == status);

    return status;
}
