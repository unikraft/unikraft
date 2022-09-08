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
import logger from '../utils/Logger.js';

async function up(sequelize) {
  await sequelize.transaction(async (t) => {
    logger.info('DB Migration (Ver.1 -> 2) -- Start');

    // update pcs_version table
    logger.debug('DB Migration -- Update pcs_version table');
    let sql = 'UPDATE pcs_version SET db_version=2,api_version=3';
    await sequelize.query(sql);

    // update fmspc_tcbs table
    // this is done by 1.Create new table 2.Copy data 3.Drop old table 4.Rename new into old
    logger.debug('DB Migration -- update fmspc_tcbs');
    sql =
      'CREATE TABLE IF NOT EXISTS fmspc_tcbs_temp (fmspc VARCHAR(255) NOT NULL, type INTEGER NOT NULL, ' +
      ' tcbinfo BLOB, root_cert_id INTEGER, signing_cert_id INTEGER, ' +
      ' created_time DATETIME NOT NULL, updated_time DATETIME NOT NULL, PRIMARY KEY(fmspc, type));';
    await sequelize.query(sql);

    sql =
      'INSERT INTO fmspc_tcbs_temp (fmspc, type, tcbinfo, root_cert_id, signing_cert_id, created_time, updated_time) ' +
      ' SELECT fmspc, 0 as type, tcbinfo, root_cert_id, signing_cert_id, created_time, updated_time ' +
      ' FROM fmspc_tcbs ';
    await sequelize.query(sql);

    sql = 'DROP TABLE fmspc_tcbs';
    await sequelize.query(sql);

    sql = 'ALTER TABLE fmspc_tcbs_temp RENAME TO fmspc_tcbs';
    await sequelize.query(sql);

    // add enclave_identities table
    logger.debug('DB Migration -- create enclave_identities table');
    sql =
      'CREATE TABLE IF NOT EXISTS enclave_identities (id INTEGER PRIMARY KEY, identity BLOB, root_cert_id INTEGER, ' +
      ' signing_cert_id INTEGER, created_time DATETIME NOT NULL, updated_time DATETIME NOT NULL);';
    await sequelize.query(sql);

    sql =
      'INSERT INTO enclave_identities (id, identity, root_cert_id, signing_cert_id, created_time, updated_time) ' +
      ' SELECT 1 as id, qe_identity, root_cert_id, signing_cert_id, created_time, updated_time ' +
      ' FROM qe_identities ';
    await sequelize.query(sql);
    sql =
      'INSERT INTO enclave_identities (id, identity, root_cert_id, signing_cert_id, created_time, updated_time) ' +
      ' SELECT 2 as id, qve_identity, root_cert_id, signing_cert_id, created_time, updated_time ' +
      ' FROM qve_identities ';
    await sequelize.query(sql);

    sql = 'DROP TABLE qe_identities';
    await sequelize.query(sql);

    sql = 'DROP TABLE qve_identities';
    await sequelize.query(sql);

    // add crl_cache table
    logger.debug('DB Migration -- create crl_cache table');
    sql =
      'CREATE TABLE IF NOT EXISTS crl_cache (cdp_url VARCHAR(255) PRIMARY KEY, crl BLOB, ' +
      ' created_time DATETIME NOT NULL, updated_time DATETIME NOT NULL);';
    await sequelize.query(sql);

    logger.info('DB Migration -- Done.');
  });
}

export default { up };
