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

import { PlatformTcbs, sequelize } from './models/index.js';

export async function upsertPlatformTcbs(
  qe_id,
  pce_id,
  cpu_svn,
  pce_svn,
  tcbm
) {
  return await PlatformTcbs.upsert({
    qe_id: qe_id,
    pce_id: pce_id,
    cpu_svn: cpu_svn,
    pce_svn: pce_svn,
    tcbm: tcbm,
  });
}

// Query all platform TCBs that has the same fmspc
export async function getPlatformTcbs(fmspc) {
  let sql;
  if (fmspc == null || fmspc == '') {
    sql =
      'select a.*,b.enc_ppid as enc_ppid from platform_tcbs a, platforms b ' +
      ' where a.qe_id=b.qe_id and a.pce_id=b.pce_id';
    return await sequelize.query(sql, { type: sequelize.QueryTypes.SELECT });
  } else {
    sql =
      'select a.*,b.enc_ppid as enc_ppid from platform_tcbs a, platforms b ' +
      ' where a.qe_id=b.qe_id and a.pce_id=b.pce_id and b.fmspc in(:FMSPC)';
    return await sequelize.query(sql, {
      type: sequelize.QueryTypes.SELECT,
      replacements: { FMSPC: fmspc.toUpperCase().split(',') },
    });
  }
}

// Query all TCBs for a platform
export async function getPlatformTcbsById(qe_id, pce_id) {
  return await PlatformTcbs.findAll({
    where: {
      qe_id: qe_id,
      pce_id: pce_id,
    },
  });
}

export async function deletePlatformTcbsById(qe_id, pce_id) {
  return await PlatformTcbs.destroy({
    where: {
      qe_id: qe_id,
      pce_id: pce_id,
    },
  });
}
