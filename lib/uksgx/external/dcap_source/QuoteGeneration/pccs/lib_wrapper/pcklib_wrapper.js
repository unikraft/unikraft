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

import refNAPI from 'ref-napi';
import ffi from 'ffi-napi';
import StructType from 'ref-struct-di';
import ArrayType from 'ref-array-di';
import * as path from 'path';
import { fileURLToPath } from 'url';
import logger from '../utils/Logger.js';

const NAPIStructType = StructType(refNAPI);
const NAPIArrayType = ArrayType(refNAPI);
const intPtr = refNAPI.refType('int');
const StringArray = NAPIArrayType('string');
const ByteArray = NAPIArrayType('byte', 16);
const cpu_svn_t = NAPIStructType({
  bytes: ByteArray,
});
const cpu_svn_ptr = refNAPI.refType(cpu_svn_t);

const __dirname = path.dirname(fileURLToPath(import.meta.url));

////////////// Load library ////////////////////////////////
let dllpath = 'PCKCertSelectionLib.dll';
if (process.platform === 'linux') {
  dllpath = path.join(__dirname, '../lib/libPCKCertSelection.so');
}
const pcklib = ffi.Library(dllpath, {
  pck_cert_select: [
    'int',
    [
      cpu_svn_ptr,
      'uint16',
      'uint16',
      'string',
      StringArray,
      'uint32',
      intPtr,
    ],
  ],
});

export function pck_cert_select(
  cpu_svn,
  pce_svn,
  pce_id,
  tcb_info,
  pem_certs,
  ncerts
) {
  let my_cpu_svn = new cpu_svn_t();
  let buf = Buffer.from(cpu_svn, 'hex');
  my_cpu_svn.bytes = new ByteArray();
  for (let i = 0; i < buf.length; i++) my_cpu_svn.bytes[i] = buf[i];

  let my_pce_svn = Buffer.from(pce_svn, 'hex').readInt16LE();
  let my_pce_id = Buffer.from(pce_id, 'hex').readInt16LE();
  let best_index_ptr = refNAPI.alloc('int');
  let ret = pcklib.pck_cert_select(
    my_cpu_svn.ref(),
    my_pce_svn,
    my_pce_id,
    tcb_info,
    pem_certs,
    ncerts,
    best_index_ptr
  );
  if (ret == 0) {
    let best_index = best_index_ptr.deref();
    return best_index;
  } else {
    logger.error('PCK selection library returned ' + ret);
    return -1;
  }
}
