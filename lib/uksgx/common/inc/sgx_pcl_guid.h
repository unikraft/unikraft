/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef SGX_PCL_GUID_H
#define SGX_PCL_GUID_H
/*
 * GUID enables coupling of PCL lib (e.g. libsgx_pcl.a) and sealed blob
 * Before the PCL unseals the sealed blob, the PCL must verify the AAD 
 * portion of the sealed blob equals the GUID embedded in enclave binary. 
 */
// SGX_PCL_GUID_SIZE must equal 16 Bytes
#ifndef SGX_PCL_GUID_SIZE

#define SGX_PCL_GUID_SIZE (16)

#else // SGX_PCL_GUID_SIZE is defined: // #ifndef SGX_PCL_GUID_SIZE

#if (16 != SGX_PCL_GUID_SIZE)
#error SGX_PCL_GUID_SIZE != 16
#endif // #if (16 != SGX_PCL_GUID_SIZE) 

#endif // #ifndef SGX_PCL_GUID_SIZE

/* g_pcl_guid is used by: 
 * 1. Sealing enclave (decryption-key provisioning enclave) 
 *    as AAD for the sealed key blob.
 * 2. Encryption tool which embeds the GUID into the PCL enclave binary. 
 */
uint8_t g_pcl_guid[SGX_PCL_GUID_SIZE] = 
{0x95, 0x48, 0x6e, 0x8f, 0x8f, 0x4a, 0x41, 0x4f, 0xb1, 0x27, 0x46, 0x21, 0xa8, 0x59, 0xa8, 0xac};

#endif // #ifndef SGX_PCL_GUID_H
