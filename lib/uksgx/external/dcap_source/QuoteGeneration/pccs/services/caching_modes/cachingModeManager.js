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
class CachingModeManager {
  constructor() {
    this._mode = null;
    if (this.instance) {
      return this.instance;
    }
    this.instance = this;
  }
  set cachingMode(mode) {
    this._mode = mode;
  }
  get cachingMode() {
    return this._mode;
  }

  async getPckCertFromPCS(
    qeid,
    cpusvn,
    pcesvn,
    pceid,
    enc_ppid,
    platform_manifest
  ) {
    return this._mode.getPckCertFromPCS(
      qeid,
      cpusvn,
      pcesvn,
      pceid,
      enc_ppid,
      platform_manifest
    );
  }

  async getEnclaveIdentityFromPCS(enclave_id, version) {
    return this._mode.getEnclaveIdentityFromPCS(enclave_id, version);
  }

  async getPckCrlFromPCS(ca) {
    return this._mode.getPckCrlFromPCS(ca);
  }

  async getRootCACrlFromPCS(rootca) {
    return this._mode.getRootCACrlFromPCS(rootca);
  }

  async getTcbInfoFromPCS(type, fmspc, version) {
    return this._mode.getTcbInfoFromPCS(type, fmspc, version);
  }

  isRefreshable() {
    return this._mode.isRefreshable();
  }

  async registerPlatforms(isCached, regDataJson) {
    return this._mode.registerPlatforms(isCached, regDataJson);
  }

  async processNotAvailableTcbs(
    qeid,
    pceid,
    enc_ppid,
    platform_manifest,
    pckcerts_not_available
  ) {
    return this._mode.processNotAvailableTcbs(
      qeid,
      pceid,
      enc_ppid,
      platform_manifest,
      pckcerts_not_available
    );
  }

  needUpdatePlatformTcbs(hasNotAvailableCerts) {
    return this._mode.needUpdatePlatformTcbs(hasNotAvailableCerts);
  }

  async getCrlFromPCS(uri) {
    return this._mode.getCrlFromPCS(uri);
  }
}

export const cachingModeManager = new CachingModeManager();
