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
import logger from './Logger.js';
import Config from 'config';
import Constants from '../constants/index.js';
import { sequelize, PcsVersion } from '../dao/models/index.js';
import Umzug from 'umzug';
import * as fs from 'fs';

export function get_api_version_from_url(url) {
  if (!url) return 0;

  let verstr = url.match(/\/v([1-9][0-9]*)\//);
  if (!verstr || verstr[0].length < 4) {
    throw new Error('Unsupported API version');
  }
  let ver = parseInt(verstr[0].substr(2).slice(0, -1));
  if (ver != 3 && ver != 4) {
    throw new Error('Unsupported API version');
  }
  return ver;
}

export function getTcbInfoIssuerChainName(version) {
  if (version == 3) {
    return Constants.SGX_TCB_INFO_ISSUER_CHAIN;
  } else {
    return Constants.TCB_INFO_ISSUER_CHAIN;
  }
}

// Check the version of PCS service currently configured
export function startup_check() {
  if (global.PCS_VERSION != 3 && global.PCS_VERSION != 4) {
    logger.error(
      'The PCS API version ' +
        global.PCS_VERSION +
        ' configured is not supported. Should be v3 or v4.'
    );
    return false;
  }
  return true;
}

async function test_connection() {
  try {
    return await sequelize.authenticate();
  } catch (err) {
    throw new Error('Failed to connect to the database.');
  }
}

async function test_db_status() {
  const sql = 'select * from pck_crl';
  try {
    await sequelize.query(sql, {
      type: sequelize.QueryTypes.SELECT,
    });
    return true;
  } catch (err) {
    return false;
  }
}

async function db_migration() {
  const umzug = new Umzug({
    storage: 'sequelize',
    storageOptions: {
      sequelize: sequelize,
      tableName: 'umzug',
    },
    migrations: {
      pattern: /\.js|\.up\.sql$/,
      customResolver: (path) => {
        return {
          up: async (sequelize) => {
            if (path.endsWith('.up.sql')) {
              const sqls = fs.readFileSync(path, 'utf-8').split(';');
              let queries = [];
              for (const sql of sqls) {
                queries.push(sequelize.query(sql));
              }
              return Promise.all(queries);
            } else {
              const migration = await import(path);
              return migration.default.up(sequelize);
            }
          },
          down: async (sequelize) => {
            if (path.endsWith('.up.sql')) {
              const downPath = path.replace('.up.sql', '.down.sql');
              if (fs.existsSync(downPath)) {
                const sqls = fs.readFileSync(downPath, 'utf-8').split(';');
                let queries = [];
                for (const sql of sqls) {
                  queries.push(sequelize.query(sql));
                }
                return Promise.all(queries);
              }
            } else {
              const migration = await import(path);
              return migration.default.down(sequelize);
            }
          },
        };
      },
      params: [sequelize],
    },
    logger: console,
  });
  // Auto migration from previous databases
  await umzug.up();
}

export async function database_check() {
  try {
    await test_connection();

    if (Config.has('init_db') && Config.get('init_db') == false) {
      return true;
    }

    let url = new URL(Config.get('uri'));

    let db_initialized = await test_db_status();
    if (!db_initialized) {
      // auto-migration
      await db_migration();
      // update pcs_version
      await PcsVersion.upsert({
        id: 1,
        api_version: Constants.API_VERSION,
        server_addr: url.hostname,
        db_version: Constants.DB_VERSION,
      });
      return true;
    } else {
      // For an existing database, we need to check its API version and server address
      const sql = 'select * from pcs_version';
      let result = await sequelize.query(sql, {
        type: sequelize.QueryTypes.SELECT,
      });
      if (result.length != 1) {
        // This database is created by PCCS v1.8 or earlier
        logger.error(
          `Can't find the version information of the caching database. ` +
            `Please delete the caching db and try again.`
        );
        return false;
      }
      if (result[0].server_addr != url.hostname) {
        // If PCS server address changes, the database won't be valid any more
        logger.error(
          'The server address used by the caching db is different ' +
            'from the one in the configuration file.'
        );
        return false;
      }
      // auto-migration
      await db_migration();

      return true;
    }
  } catch (err) {
    logger.error(err);
    return false;
  }
}
