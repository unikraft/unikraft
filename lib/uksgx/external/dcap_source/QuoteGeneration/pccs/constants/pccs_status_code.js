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

class PccsStatus {}

function define(name, value) {
  Object.defineProperty(PccsStatus, name, {
    value: value,
    enumerable: true,
  });
}

// REST API return codes
define('PCCS_STATUS_SUCCESS', [200, 'Operation successful.']);
define('PCCS_STATUS_INVALID_REQ', [400, 'Invalid request parameters.']);
define('PCCS_STATUS_UNAUTHORIZED', [401, 'Authentication failed.']);
define('PCCS_STATUS_NO_CACHE_DATA', [404, 'No cache data for this platform.']);
define('PCCS_STATUS_INTEGRITY_ERROR', [
  460,
  `The integrity of the data can't be verified.`,
]);
define('PCCS_STATUS_PLATFORM_UNKNOWN', [
  461,
  'The platform was not found in the cache.',
]);
define('PCCS_STATUS_CERTS_UNAVAILABLE', [
  462,
  'Certificates are not available for certain TCBs.',
]);
define('PCCS_STATUS_INTERNAL_ERROR', [500, 'Internal server error occurred.']);
define('PCCS_STATUS_SERVICE_UNAVAILABLE', [
  503,
  'Server is currently unable to process the request.',
]);
define('PCCS_STATUS_PCS_ACCESS_FAILURE', [
  502,
  'Unable to retrieve the collateral from the Intel SGX PCS.',
]);

export default PccsStatus;
