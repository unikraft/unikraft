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

import { pckcrlService } from '../services/index.js';
import PccsError from '../utils/PccsError.js';
import PccsStatus from '../constants/pccs_status_code.js';
import Constants from '../constants/index.js';

export async function getPckCrl(req, res, next) {
  try {
    // validate request parameters
    let ca = req.query.ca;
    if (ca) ca = ca.toUpperCase();
    if (ca != Constants.CA_PROCESSOR && ca != Constants.CA_PLATFORM) {
      throw new PccsError(PccsStatus.PCCS_STATUS_INVALID_REQ);
    }

    let encoding = req.query.encoding;

    // call service
    let pckcrlJson = await pckcrlService.getPckCrl(ca, encoding);

    // send response
    res
      .status(PccsStatus.PCCS_STATUS_SUCCESS[0])
      .header(
        Constants.SGX_PCK_CRL_ISSUER_CHAIN,
        pckcrlJson[Constants.SGX_PCK_CRL_ISSUER_CHAIN]
      )
      .header('Content-Type', (!encoding || encoding.toUpperCase() != 'DER')? 'application/x-pem-file' : 'application/pkix-crl')
      .send(pckcrlJson['pckcrl']);
  } catch (err) {
    next(err);
  }
}
