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

import { tcbinfoService } from '../services/index.js';
import PccsError from '../utils/PccsError.js';
import PccsStatus from '../constants/pccs_status_code.js';
import Constants from '../constants/index.js';
import * as appUtil from '../utils/apputil.js';

async function getTcbInfo(req, res, next, type) {
  const FMSPC_SIZE = 12;

  try {
    // validate request parameters
    let version = appUtil.get_api_version_from_url(req.originalUrl);
    let fmspc = req.query.fmspc;
    if (fmspc == null || fmspc.length != FMSPC_SIZE) {
      throw new PccsError(PccsStatus.PCCS_STATUS_INVALID_REQ);
    }

    // normalize request parameters
    fmspc = fmspc.toUpperCase();

    // call service
    let tcbinfoJson = await tcbinfoService.getTcbInfo(type, fmspc, version);
    let issuerChainName = appUtil.getTcbInfoIssuerChainName(version);

    // send response
    res
      .status(PccsStatus.PCCS_STATUS_SUCCESS[0])
      .header(
        issuerChainName,
        tcbinfoJson[issuerChainName]
      )
      .header('Content-Type', 'application/json')
      .send(tcbinfoJson['tcbinfo']);
  } catch (err) {
    next(err);
  }
}

export async function getSgxTcbInfo(req, res, next) {
  await getTcbInfo(req, res, next, Constants.PROD_TYPE_SGX);
}

export async function getTdxTcbInfo(req, res, next) {
  await getTcbInfo(req, res, next, Constants.PROD_TYPE_TDX);
}
