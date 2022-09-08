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

import Constants from '../constants/index.js';
import PccsError from '../utils/PccsError.js';
import PccsStatus from '../constants/pccs_status_code.js';
import { PckCertchain, sequelize } from './models/index.js';

export async function upsertPckCertchain(ca) {
  if (typeof ca == 'undefined') {
    throw new PccsError(PccsStatus.PCCS_STATUS_INTERNAL_ERROR);
  }
  if (ca != Constants.CA_PROCESSOR && ca != Constants.CA_PLATFORM) {
    throw new PccsError(PccsStatus.PCCS_STATUS_INTERNAL_ERROR);
  }

  return await PckCertchain.upsert({
    ca: ca,
    root_cert_id: Constants.PROCESSOR_ROOT_CERT_ID,
    intmd_cert_id:
      ca == Constants.CA_PROCESSOR
        ? Constants.PROCESSOR_INTERMEDIATE_CERT_ID
        : Constants.PLATFORM_INTERMEDIATE_CERT_ID,
  });
}

export async function getPckCertChain(ca) {
  const sql =
    'select a.*,' +
    ' (select cert from pcs_certificates where id=a.root_cert_id) as root_cert,' +
    ' (select cert from pcs_certificates where id=a.intmd_cert_id) as intmd_cert' +
    ' from pck_certchain a ' +
    ' where a.ca=$ca';
  const certchain = await sequelize.query(sql, {
    type: sequelize.QueryTypes.SELECT,
    bind: { ca: ca },
  });
  if (certchain.length == 0) return null;
  else if (certchain.length == 1) {
    return certchain[0];
  } else throw new PccsError(PccsStatus.PCCS_STATUS_INTERNAL_ERROR);
}
