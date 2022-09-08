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
import { FmspcTcbs, sequelize } from './models/index.js';
import Sequelize from 'sequelize';

// Update or insert a record in JSON format
export async function upsertFmspcTcb(tcbinfoJson) {
  return await FmspcTcbs.upsert({
    type: tcbinfoJson.type,
    fmspc: tcbinfoJson.fmspc,
    version: tcbinfoJson.version,
    tcbinfo: tcbinfoJson.tcbinfo,
    root_cert_id: Constants.PROCESSOR_ROOT_CERT_ID,
    signing_cert_id: Constants.PROCESSOR_SIGNING_CERT_ID,
  });
}

//Query TCBInfo by fmspc
export async function getTcbInfo(type, fmspc, version) {
  if (typeof type == 'undefined' || typeof version == 'undefined') {
    throw new PccsError(PccsStatus.PCCS_STATUS_INTERNAL_ERROR);
  }

  const sql =
    'select a.*,' +
    ' (select cert from pcs_certificates where id=a.root_cert_id) as root_cert,' +
    ' (select cert from pcs_certificates where id=a.signing_cert_id) as signing_cert' +
    ' from fmspc_tcbs a ' +
    ' where a.type=$type' +
    ' and a.fmspc=$fmspc' +
    ' and a.version=$version';
  const tcbinfo = await sequelize.query(sql, {
    type: sequelize.QueryTypes.SELECT,
    bind: {
      type: type,
      fmspc: fmspc,
      version: version,
    },
  });
  if (tcbinfo.length == 0) return null;
  else if (tcbinfo.length == 1) {
    if (tcbinfo[0].root_cert != null && tcbinfo[0].signing_cert != null)
      return tcbinfo[0];
    else return null;
  } else throw new PccsError(PccsStatus.PCCS_STATUS_INTERNAL_ERROR);
}

//Query all TCBInfos
export async function getAllTcbs() {
  return await FmspcTcbs.findAll({
    where: {
      type: {
        [Sequelize.Op.not]: null,
      },
    },
  });
}

//Delete TCBInfos whose type is null
export async function deleteInvalidTcbs() {
  return await FmspcTcbs.destroy({
    where: {
      type: {
        [Sequelize.Op.is]: null,
      },
    },
  });
}
