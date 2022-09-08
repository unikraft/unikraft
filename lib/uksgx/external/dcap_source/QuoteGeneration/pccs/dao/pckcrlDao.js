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
import { PckCrl, sequelize } from './models/index.js';

//Query a PCK CRL by ca
export async function getPckCrl(ca) {
  const sql =
    'select a.*,' +
    ' (select cert from pcs_certificates where id=a.root_cert_id) as root_cert,' +
    ' (select cert from pcs_certificates where id=a.intmd_cert_id) as intmd_cert' +
    ' from pck_crl a ' +
    ' where a.ca=$ca';
  const pckcrl = await sequelize.query(sql, {
    type: sequelize.QueryTypes.SELECT,
    bind: { ca: ca },
  });
  if (pckcrl.length == 0) return null;
  else if (pckcrl.length == 1) {
    if (pckcrl[0].root_cert != null && pckcrl[0].intmd_cert != null)
      return pckcrl[0];
    else return null;
  } else throw new PccsError(PccsStatus.PCCS_STATUS_INTERNAL_ERROR);
}

//Query all PCK CRLs from table
export async function getAllPckCrls() {
  const sql =
    'select a.*,' +
    ' (select cert from pcs_certificates where id=a.root_cert_id) as root_cert,' +
    ' (select cert from pcs_certificates where id=a.intmd_cert_id) as intmd_cert' +
    ' from pck_crl a ';
  return await sequelize.query(sql, {
    type: sequelize.QueryTypes.SELECT,
  });
}

//Update or Insert a PCK CRL
export async function upsertPckCrl(ca, crl) {
  if (ca != Constants.CA_PROCESSOR && ca != Constants.CA_PLATFORM) {
    throw new PccsError(PccsStatus.PCCS_STATUS_INTERNAL_ERROR);
  }

  return await PckCrl.upsert({
    ca: ca,
    pck_crl: crl,
    root_cert_id: Constants.PROCESSOR_ROOT_CERT_ID,
    intmd_cert_id:
      ca == Constants.CA_PROCESSOR
        ? Constants.PROCESSOR_INTERMEDIATE_CERT_ID
        : Constants.PLATFORM_INTERMEDIATE_CERT_ID,
  });
}
