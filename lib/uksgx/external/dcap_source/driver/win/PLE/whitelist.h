// This file is provided under a dual BSD/GPLv2 license.  When using or
// redistributing this file, you may do so under either license.
//
// GPL LICENSE SUMMARY
//
// Copyright(c) 2016-2018 Intel Corporation.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of version 2 of the GNU General Public License as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact Information:
// Jarkko Sakkinen <jarkko.sakkinen@linux.intel.com>
// Intel Finland Oy - BIC 0357606-4 - Westendinkatu 7, 02160 Espoo
//
// BSD LICENSE
//
// Copyright(c) 2016-2018 Intel Corporation.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in
//     the documentation and/or other materials provided with the
//     distribution.
//   * Neither the name of Intel Corporation nor the names of its
//     contributors may be used to endorse or promote products derived
//     from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Authors:
//

#ifndef _WHITELIST_H_
#define _WHITELIST_H_


uint8_t G_SERVICE_ENCLAVE_MRSIGNER[][32] =
{
    {//MR_SIGNER of PvE provided
        0XEC, 0X15, 0XB1, 0X07, 0X87, 0XD2, 0XF8, 0X46,
        0X67, 0XCE, 0XB0, 0XB5, 0X98, 0XFF, 0XC4, 0X4A,
        0X1F, 0X1C, 0XB8, 0X0F, 0X67, 0X0A, 0XAE, 0X5D,
        0XF9, 0XE8, 0XFA, 0X9F, 0X63, 0X76, 0XE1, 0XF8
    },
    {//MR_SIGNER of PCE provided
        0xC5, 0x4A, 0x62, 0xF2, 0xBE, 0x9E, 0xF7, 0x6E,
        0xFB, 0x1F, 0x39, 0x30, 0xAD, 0x81, 0xEA, 0x7F,
        0x60, 0xDE, 0xFC, 0x1F, 0x5F, 0x25, 0xE0, 0x9B,
        0x7C, 0x06, 0x7A, 0x81, 0x5A, 0xE0, 0xC6, 0xCB
    },
    {//MR_SIGNER of ECDSA/QE
        0X8C, 0X4F, 0X57, 0X75, 0XD7, 0X96, 0X50, 0X3E,
        0X96, 0X13, 0X7F, 0X77, 0XC6, 0X8A, 0X82, 0X9A,
        0X00, 0X56, 0XAC, 0X8D, 0XED, 0X70, 0X14, 0X0B,
        0X08, 0X1B, 0X09, 0X44, 0X90, 0XC5, 0X7B, 0XFF
    }
};


#endif //_WHITELIST_H_

