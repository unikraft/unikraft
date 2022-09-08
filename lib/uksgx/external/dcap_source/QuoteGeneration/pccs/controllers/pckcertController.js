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

import { pckcertService } from '../services/index.js';
import PccsError from '../utils/PccsError.js';
import PccsStatus from '../constants/pccs_status_code.js';
import Constants from '../constants/index.js';

export async function getPckCert(req, res, next) {
  const QEID_MAX_SIZE = 260;
  const CPUSVN_SIZE = 32;
  const PCESVN_SIZE = 4;
  const PCEID_SIZE = 4;
  const ENC_PPID_SIZE = 768;

  try {
    let qeid = req.query.qeid;
    let cpusvn = req.query.cpusvn;
    let pcesvn = req.query.pcesvn;
    let pceid = req.query.pceid;
    let enc_ppid = req.query.encrypted_ppid;

    // validate request parameters
    if (!qeid || !cpusvn || !pcesvn || !pceid) {
      throw new PccsError(PccsStatus.PCCS_STATUS_INVALID_REQ);
    }
    if (
      qeid.length > QEID_MAX_SIZE ||
      cpusvn.length != CPUSVN_SIZE ||
      pcesvn.length != PCESVN_SIZE ||
      pceid.length != PCEID_SIZE
    ) {
      throw new PccsError(PccsStatus.PCCS_STATUS_INVALID_REQ);
    }
    if (enc_ppid != null && enc_ppid.length != ENC_PPID_SIZE) {
      throw new PccsError(PccsStatus.PCCS_STATUS_INVALID_REQ);
    }

    // normalize the request parameters
    qeid = qeid.toUpperCase();
    cpusvn = cpusvn.toUpperCase();
    pcesvn = pcesvn.toUpperCase();
    pceid = pceid.toUpperCase();
    if (enc_ppid)
      // enc_ppid can be null
      enc_ppid = enc_ppid.toUpperCase();

    // call service
    const pckcertJson = await pckcertService.getPckCert(
      qeid,
      cpusvn,
      pcesvn,
      pceid,
      enc_ppid
    );

    // send response
    res
      .status(PccsStatus.PCCS_STATUS_SUCCESS[0])
      .header(Constants.SGX_TCBM, pckcertJson[Constants.SGX_TCBM])
      .header(Constants.SGX_FMSPC, pckcertJson[Constants.SGX_FMSPC])
      .header(
        Constants.SGX_PCK_CERTIFICATE_CA_TYPE,
        pckcertJson[Constants.SGX_PCK_CERTIFICATE_CA_TYPE]
      )
      .header(
        Constants.SGX_PCK_CERTIFICATE_ISSUER_CHAIN,
        pckcertJson[Constants.SGX_PCK_CERTIFICATE_ISSUER_CHAIN]
      )
      .header('Content-Type', 'application/x-pem-file')
      .send(pckcertJson['cert']);
  } catch (err) {
    next(err);
  }
}
