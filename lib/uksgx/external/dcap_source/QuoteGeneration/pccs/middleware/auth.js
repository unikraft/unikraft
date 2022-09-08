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
import Config from 'config';
import Crypto from 'crypto';
import PccsStatus from '../constants/pccs_status_code.js';
import PccsError from '../utils/PccsError.js';

export function validateUser(req, res, next) {
  const HTTP_HEADER_USER_TOKEN = 'user-token';
  const token = req.headers[HTTP_HEADER_USER_TOKEN];
  if (token) {
    let hash = Crypto.createHash('sha512');
    hash.update(token);
    let user_token_hash = hash.digest('hex');

    if (user_token_hash != Config.get('UserTokenHash')) {
      throw new PccsError(PccsStatus.PCCS_STATUS_UNAUTHORIZED);
    } else {
      next();
    }
  } else {
    throw new PccsError(PccsStatus.PCCS_STATUS_UNAUTHORIZED);
  }
}

export function validateAdmin(req, res, next) {
  const HTTP_HEADER_ADMIN_TOKEN = 'admin-token';
  const token = req.headers[HTTP_HEADER_ADMIN_TOKEN];
  if (token) {
    let hash = Crypto.createHash('sha512');
    hash.update(token);
    let admin_token_hash = hash.digest('hex');

    if (admin_token_hash != Config.get('AdminTokenHash')) {
      throw new PccsError(PccsStatus.PCCS_STATUS_UNAUTHORIZED);
    } else {
      next();
    }
  } else {
    throw new PccsError(PccsStatus.PCCS_STATUS_UNAUTHORIZED);
  }
}
