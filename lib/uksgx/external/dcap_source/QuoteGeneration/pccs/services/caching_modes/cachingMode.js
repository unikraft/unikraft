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
import PccsError from '../../utils/PccsError.js';
import PccsStatus from '../../constants/pccs_status_code.js';
import Constants from '../../constants/index.js';
import * as CommonCacheLogic from '../logic/commonCacheLogic.js';
import * as platformsRegDao from '../../dao/platformsRegDao.js';
import { checkQuoteVerificationCollateral } from '../logic/qvCollateralLogic.js';

function intToHex(i) {
  return ('00' + i.toString(16)).slice(-2);
}

function getRawCpuSvnFromTcb(tcb) {
  return (
    intToHex(tcb.sgxtcbcomp01svn) +
    intToHex(tcb.sgxtcbcomp02svn) +
    intToHex(tcb.sgxtcbcomp03svn) +
    intToHex(tcb.sgxtcbcomp04svn) +
    intToHex(tcb.sgxtcbcomp05svn) +
    intToHex(tcb.sgxtcbcomp06svn) +
    intToHex(tcb.sgxtcbcomp07svn) +
    intToHex(tcb.sgxtcbcomp08svn) +
    intToHex(tcb.sgxtcbcomp09svn) +
    intToHex(tcb.sgxtcbcomp10svn) +
    intToHex(tcb.sgxtcbcomp11svn) +
    intToHex(tcb.sgxtcbcomp12svn) +
    intToHex(tcb.sgxtcbcomp13svn) +
    intToHex(tcb.sgxtcbcomp14svn) +
    intToHex(tcb.sgxtcbcomp15svn) +
    intToHex(tcb.sgxtcbcomp16svn)
  );
}

class CachingMode {
  async getPckCertFromPCS(
    qeid,
    cpusvn,
    pcesvn,
    pceid,
    enc_ppid,
    platform_manifest
  ) {
    throw new PccsError(PccsStatus.PCCS_STATUS_PLATFORM_UNKNOWN);
  }

  async getEnclaveIdentityFromPCS(enclave_id, version) {
    throw new PccsError(PccsStatus.PCCS_STATUS_NO_CACHE_DATA);
  }

  async getPckCrlFromPCS(ca) {
    throw new PccsError(PccsStatus.PCCS_STATUS_NO_CACHE_DATA);
  }

  async getRootCACrlFromPCS(rootca) {
    throw new PccsError(PccsStatus.PCCS_STATUS_NO_CACHE_DATA);
  }

  async getTcbInfoFromPCS(type, fmspc, version) {
    throw new PccsError(PccsStatus.PCCS_STATUS_NO_CACHE_DATA);
  }

  isRefreshable() {
    return false;
  }

  async registerPlatforms(regDataJson) {}

  async processNotAvailableTcbs(
    qeid,
    pceid,
    enc_ppid,
    platform_manifest,
    pckcerts_not_available
  ) {
    // OFFLINE mode won't reach here
  }

  needUpdatePlatformTcbs(hasNotAvailableCerts) {
    // OFFLINE mode won't reach here
  }

  async getCrlFromPCS(uri) {
    throw new PccsError(PccsStatus.PCCS_STATUS_NO_CACHE_DATA);
  }
}

//////////////////////////////////////////////////////////////////////
export class LazyCachingMode extends CachingMode {
  async getPckCertFromPCS(
    qeid,
    cpusvn,
    pcesvn,
    pceid,
    enc_ppid,
    platform_manifest
  ) {
    return await CommonCacheLogic.getPckCertFromPCS(
      qeid,
      cpusvn,
      pcesvn,
      pceid,
      enc_ppid,
      platform_manifest
    );
  }

  async getEnclaveIdentityFromPCS(enclave_id, version) {
    return await CommonCacheLogic.getEnclaveIdentityFromPCS(enclave_id, version);
  }

  async getPckCrlFromPCS(ca) {
    return await CommonCacheLogic.getPckCrlFromPCS(ca);
  }

  async getRootCACrlFromPCS(rootca) {
    return await CommonCacheLogic.getRootCACrlFromPCS(rootca);
  }

  async getCrlFromPCS(uri) {
    return await CommonCacheLogic.getCrlFromPCS(uri);
  }

  async getTcbInfoFromPCS(type, fmspc, version) {
    return await CommonCacheLogic.getTcbInfoFromPCS(type, fmspc, version);
  }

  isRefreshable() {
    return true;
  }

  async registerPlatforms(isCached, regDataJson) {
    if (!isCached) {
      // Get PCK certs from Intel PCS if not cached
      await CommonCacheLogic.getPckCertFromPCS(
        regDataJson.qe_id,
        regDataJson.cpu_svn,
        regDataJson.pce_svn,
        regDataJson.pce_id,
        regDataJson.enc_ppid,
        regDataJson.platform_manifest
      );
    }
    // Get other collaterals if not cached
    await checkQuoteVerificationCollateral();
  }

  async processNotAvailableTcbs(
    qeid,
    pceid,
    enc_ppid,
    platform_manifest,
    pckcerts_not_available
  ) {}

  needUpdatePlatformTcbs(hasNotAvailableCerts) {
    if (hasNotAvailableCerts) return false;
    else return true;
  }
}

//////////////////////////////////////////////////////////////////////
export class ReqCachingMode extends CachingMode {
  isRefreshable() {
    return true;
  }

  async registerPlatforms(isCached, regDataJson) {
    if (!isCached) {
      // For REQ mode, add registration entry first, and delete it after the collaterals are retrieved
      await platformsRegDao.registerPlatform(
        regDataJson,
        Constants.PLATF_REG_NEW
      );

      // Get PCK certs from Intel PCS if not cached
      await CommonCacheLogic.getPckCertFromPCS(
        regDataJson.qe_id,
        regDataJson.cpu_svn,
        regDataJson.pce_svn,
        regDataJson.pce_id,
        regDataJson.enc_ppid,
        regDataJson.platform_manifest
      );

      // For REQ mode, add registration entry first, and delete it after the collaterals are retrieved
      await platformsRegDao.registerPlatform(
        regDataJson,
        Constants.PLATF_REG_DELETED
      );
    }
    // Get other collaterals if not cached
    await checkQuoteVerificationCollateral();
  }

  async processNotAvailableTcbs(
    qeid,
    pceid,
    enc_ppid,
    platform_manifest,
    pckcerts_not_available
  ) {
    // save 'Not available' platform TCBs in platform registration queue
    for (const pckcert of pckcerts_not_available) {
      await platformsRegDao.registerPlatform(
        {
          qe_id: qeid,
          pce_id: pceid,
          cpu_svn: getRawCpuSvnFromTcb(pckcert.tcb),
          pce_svn: pckcert.tcb.pcesvn,
          enc_ppid: enc_ppid,
          platform_manifest: platform_manifest,
        },
        Constants.PLATF_REG_NOT_AVAILABLE
      );
    }
  }

  needUpdatePlatformTcbs(hasNotAvailableCerts) {
    return true;
  }
}

//////////////////////////////////////////////////////////////////////
export class OfflineCachingMode extends CachingMode {
  async registerPlatforms(isCached, regDataJson) {
    if (!isCached) {
      // add to registration table
      await platformsRegDao.registerPlatform(
        regDataJson,
        Constants.PLATF_REG_NEW
      );
    }
  }
}
