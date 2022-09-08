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

import { PlatformsRegistered } from './models/index.js';
import Constants from '../constants/index.js';

export async function findRegisteredPlatform(regDataJson) {
  return await PlatformsRegistered.findOne({
    where: {
      qe_id: regDataJson.qe_id,
      pce_id: regDataJson.pce_id,
      cpu_svn: regDataJson.cpu_svn,
      pce_svn: regDataJson.pce_svn,
      platform_manifest: regDataJson.platform_manifest,
      state: Constants.PLATF_REG_NEW,
    },
  });
}

export async function findRegisteredPlatforms(state) {
  return await PlatformsRegistered.findAll({
    attributes: [
      'qe_id',
      'pce_id',
      'cpu_svn',
      'pce_svn',
      'platform_manifest',
      'enc_ppid',
    ],
    where: { state: state },
  });
}

export async function registerPlatform(regDataJson, state) {
  return await PlatformsRegistered.upsert({
    qe_id: regDataJson.qe_id,
    pce_id: regDataJson.pce_id,
    cpu_svn: regDataJson.cpu_svn,
    pce_svn: regDataJson.pce_svn,
    enc_ppid: regDataJson.enc_ppid,
    platform_manifest: regDataJson.platform_manifest,
    state: state,
  });
}

export async function deleteRegisteredPlatforms(state) {
  await PlatformsRegistered.update(
    { state: Constants.PLATF_REG_DELETED },
    { where: { state: state } }
  );
}
