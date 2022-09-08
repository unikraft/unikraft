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
import PccsError from '../utils/PccsError.js';
import PccsStatus from '../constants/pccs_status_code.js';
import Constants from '../constants/index.js';
import Ajv from 'ajv';
import * as platformsRegDao from '../dao/platformsRegDao.js';
import * as platformsDao from '../dao/platformsDao.js';
import * as pckcertDao from '../dao/pckcertDao.js';
import { PLATFORM_REG_SCHEMA } from './pccs_schemas.js';
import { cachingModeManager } from './caching_modes/cachingModeManager.js';

const ajv = new Ajv();

async function checkPCKCertCacheStatus(regDataJson) {
  let isCached = false;
  do {
    const platform = await platformsDao.getPlatform(
      regDataJson.qe_id,
      regDataJson.pce_id
    );
    if (platform == null) {
      break;
    }
    if (!Boolean(regDataJson.platform_manifest)) {
      // * treat the absence of the PLATFORMMANIFEST in the API while
      // there is a PLATFORM_MANIFEST in the cache as a 'match' *
      regDataJson.platform_manifest = platform.platform_manifest;
      let pckcert = await pckcertDao.getCert(
        regDataJson.qe_id,
        regDataJson.cpu_svn,
        regDataJson.pce_svn,
        regDataJson.pce_id
      );
      if (pckcert == null) {
        break;
      }
    } else if (platform.platform_manifest != regDataJson.platform_manifest) {
      // cached status is false
      break;
    }
    isCached = true;
  } while (false);

  return isCached;
}

function normalizeRegData(regDataJson) {
  // normalize the registration data
  regDataJson.qe_id = regDataJson.qe_id.toUpperCase();
  regDataJson.pce_id = regDataJson.pce_id.toUpperCase();
  if (regDataJson.platform_manifest) {
    // other parameters are useless
    regDataJson.cpu_svn = '';
    regDataJson.pce_svn = '';
    regDataJson.enc_ppid = '';
  } else {
    if (!regDataJson.cpu_svn || !regDataJson.pce_svn || !regDataJson.enc_ppid)
      throw new PccsError(PccsStatus.PCCS_STATUS_INVALID_REQ);
    regDataJson.platform_manifest = '';
    regDataJson.cpu_svn = regDataJson.cpu_svn.toUpperCase();
    regDataJson.pce_svn = regDataJson.pce_svn.toUpperCase();
  }
}

export async function registerPlatforms(regDataJson) {
  //check parameters
  let valid = ajv.validate(PLATFORM_REG_SCHEMA, regDataJson);
  if (!valid) {
    throw new PccsError(PccsStatus.PCCS_STATUS_INVALID_REQ);
  }

  // normalize registration data
  normalizeRegData(regDataJson);

  // Get cache status
  let isCached = await checkPCKCertCacheStatus(regDataJson);

  await cachingModeManager.registerPlatforms(isCached, regDataJson);
}

export async function getRegisteredPlatforms() {
  return await platformsRegDao.findRegisteredPlatforms(Constants.PLATF_REG_NEW);
}

export async function getRegisteredNaPlatforms() {
  return await platformsRegDao.findRegisteredPlatforms(
    Constants.PLATF_REG_NOT_AVAILABLE
  );
}

export async function deleteRegisteredPlatforms(state) {
  await platformsRegDao.deleteRegisteredPlatforms(state);
}
