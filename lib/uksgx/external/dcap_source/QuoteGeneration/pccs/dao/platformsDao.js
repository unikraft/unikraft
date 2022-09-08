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

import { Platforms, sequelize } from './models/index.js';

export async function upsertPlatform(
  qe_id,
  pce_id,
  platform_manifest,
  enc_ppid,
  fmspc,
  ca
) {
  return await Platforms.upsert({
    qe_id: qe_id,
    pce_id: pce_id,
    platform_manifest: platform_manifest,
    enc_ppid: enc_ppid,
    fmspc: fmspc,
    ca: ca,
  });
}

export async function getPlatform(qe_id, pce_id) {
  return await Platforms.findOne({ where: { qe_id: qe_id, pce_id: pce_id } });
}

export async function updatePlatform(
  qe_id,
  pce_id,
  platform_manifest,
  enc_ppid
) {
  return await Platforms.update(
    { platform_manifest: platform_manifest, enc_ppid: enc_ppid },
    {
      where: {
        qe_id: qe_id,
        pce_id: pce_id,
      },
    }
  );
}

export async function getCachedPlatformsByFmspc(fmspc_arr) {
  let sql =
    'select a.qe_id, a.pce_id, b.cpu_svn, b.pce_svn, hex(a.enc_ppid) as enc_ppid,' +
    ' hex(a.platform_manifest) as platform_manifest' +
    ' from platforms a, platform_tcbs b ' +
    ' where a.qe_id=b.qe_id and a.pce_id = b.pce_id ';
  if (fmspc_arr.length > 0) {
    sql += ' and a.fmspc in (:FMSPC)';
  }

  return await sequelize.query(sql, {
    replacements: {
      FMSPC: fmspc_arr,
    },
    type: sequelize.QueryTypes.SELECT,
  });
}
