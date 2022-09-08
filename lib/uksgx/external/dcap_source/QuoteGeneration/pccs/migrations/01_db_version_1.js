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
    logger.info('DB Migration (Ver.0 -> 1) -- Start');

    // update pcs_version table
    logger.debug('DB Migration -- Update pcs_version table');
    let sql = 'ALTER TABLE pcs_version ADD COLUMN db_version INT DEFAULT 1';
    await sequelize.query(sql);

    // update pck_crl.pck_crl from HEX string to BINARY
    logger.debug('DB Migration -- update pck_crl.pck_crl from HEX string to BINARY');
    sql = 'SELECT ca, pck_crl FROM pck_crl';
    let resultSet = await sequelize.query(sql, {
      type: sequelize.QueryTypes.SELECT,
    });
    for (const record of resultSet) {
      if (record.pck_crl) {
        sql = 'UPDATE pck_crl SET pck_crl = $newValue WHERE ca=$key';
        await sequelize.query(sql, {
          bind: {
            key: record.ca,
            newValue: Buffer.from(record.pck_crl.toString('utf8'), 'hex'),
          },
        });
      }
    }

    // update pcs_certificates.crl from HEX string to BINARY
    logger.debug('DB Migration -- update pcs_certificates.crl from HEX string to BINARY');
    sql = 'SELECT * FROM pcs_certificates WHERE id=1';
    resultSet = await sequelize.query(sql, {
      type: sequelize.QueryTypes.SELECT,
    });
    for (const record of resultSet) {
      if (record.crl) {
        sql = 'UPDATE pcs_certificates SET crl = $newValue WHERE id=$id';
        await sequelize.query(sql, {
          bind: {
            id: record.id,
            newValue: Buffer.from(record.crl.toString('utf8'), 'hex'),
          },
        });
      }
    }

    // update platforms(platform_manifest,enc_ppid) from HEX string to BINARY
    logger.debug('DB Migration -- update platforms(platform_manifest,enc_ppid) from HEX string to BINARY');
    sql = 'SELECT * FROM platforms';
    resultSet = await sequelize.query(sql, {
      type: sequelize.QueryTypes.SELECT,
    });
    for (const record of resultSet) {
      let new_platform_manifest = '';
      let new_enc_ppid = '';
      if (record.platform_manifest) {
        new_platform_manifest = Buffer.from(
          record.platform_manifest.toString('utf8'),
          'hex'
        );
      }
      if (record.enc_ppid) {
        new_enc_ppid = Buffer.from(record.enc_ppid.toString('utf8'), 'hex');
      }
      if (new_platform_manifest || new_enc_ppid) {
        sql =
          'UPDATE platforms SET platform_manifest = $newManifest, enc_ppid = $newPpid ' +
          ' WHERE qe_id=$qe_id AND pce_id=$pce_id';
        await sequelize.query(sql, {
          bind: {
            qe_id: record.qe_id,
            pce_id: record.pce_id,
            newManifest: new_platform_manifest,
            newPpid: new_enc_ppid,
          },
        });
      }
    }

    // update platforms_registered(platform_manifest,enc_ppid) from HEX string to BINARY
    logger.debug('DB Migration -- update platforms_registered(platform_manifest,enc_ppid) from HEX string to BINARY');
    sql = 'SELECT * FROM platforms_registered';
    resultSet = await sequelize.query(sql, {
      type: sequelize.QueryTypes.SELECT,
    });
    for (const record of resultSet) {
      let new_platform_manifest = '';
      let new_enc_ppid = '';
      if (record.platform_manifest) {
        new_platform_manifest = Buffer.from(
          record.platform_manifest.toString('utf8'),
          'hex'
        );
      }
      if (record.enc_ppid) {
        new_enc_ppid = Buffer.from(record.enc_ppid.toString('utf8'), 'hex');
      }
      if (new_platform_manifest || new_enc_ppid) {
        sql =
          'UPDATE platforms_registered SET platform_manifest = $newManifest, enc_ppid = $newPpid ' +
          ' WHERE qe_id=$qe_id AND pce_id=$pce_id AND cpu_svn=$cpu_svn AND pce_svn=$pce_svn ';
        await sequelize.query(sql, {
          bind: {
            qe_id: record.qe_id,
            pce_id: record.pce_id,
            cpu_svn: record.cpu_svn,
            pce_svn: record.pce_svn,
            newManifest: new_platform_manifest,
            newPpid: new_enc_ppid,
          },
        });
      }
    }

    logger.info('DB Migration -- Done.');
  });
}

export default { up };
