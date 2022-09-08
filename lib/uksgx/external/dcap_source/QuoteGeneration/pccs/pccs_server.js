#!/usr/bin/env node
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
import morgan from 'morgan';
import express from 'express';
import logger from './utils/Logger.js';
import node_schedule from 'node-schedule';
import body_parser from 'body-parser';
import { sgxRouter, tdxRouter } from './routes/index.js';
import * as fs from 'fs';
import * as https from 'https';
import * as auth from './middleware/auth.js';
import * as error from './middleware/error.js';
import addRequestId from './middleware/addRequestId.js';
import * as refreshService from './services/refreshService.js';
import * as appUtil from './utils/apputil.js';
import { cachingModeManager } from './services/caching_modes/cachingModeManager';
import {
  LazyCachingMode,
  ReqCachingMode,
  OfflineCachingMode,
} from './services/caching_modes/cachingMode.js';

process.on('uncaughtException', function (exception) {
  logger.error(exception);
});

// Create ./logs if it doesn't exist
fs.mkdir('./logs', (err) => {
  /* do nothing*/
});

const app = express();
const { urlencoded, json } = body_parser;
const { scheduleJob } = node_schedule;

  // Get PCS API version from the config file
  global.PCS_VERSION = appUtil.get_api_version_from_url(Config.get('uri'));

// startup check
if (!appUtil.startup_check()) {
  logger.endAndExitProcess();
}

appUtil.database_check().then((db_init_ok) => {
  if (!db_init_ok) {
    logger.endAndExitProcess();
  }

  // Change storage file permission if DB is sqlite
  if (Config.get('DB_CONFIG') == 'sqlite') {
    fs.chmod(Config.get('sqlite').options.storage, 0o640, () => {});
  }

  // logger
  app.use(morgan('combined', { stream: logger.stream }));

  // Add Request-ID
  app.use(addRequestId);

  // body parser middleware, this will let us get the data from a POST
  app.use(urlencoded({ extended: true }));
  app.use(json({ limit: '200000kb' }));

  // authentication middleware for v3
  app.get('/sgx/certification/v3/platforms', auth.validateAdmin);
  app.post('/sgx/certification/v3/platforms', auth.validateUser);
  app.use('/sgx/certification/v3/platformcollateral', auth.validateAdmin);
  app.use('/sgx/certification/v3/refresh', auth.validateAdmin);
  if (global.PCS_VERSION == 4) {
    // authentication middleware for v4
    app.get('/sgx/certification/v4/platforms', auth.validateAdmin);
    app.post('/sgx/certification/v4/platforms', auth.validateUser);
    app.use('/sgx/certification/v4/platformcollateral', auth.validateAdmin);
    app.use('/sgx/certification/v4/refresh', auth.validateAdmin);
  }

  // router
  app.use('/sgx/certification/v3', sgxRouter);
  if (global.PCS_VERSION == 4) {
    app.use('/sgx/certification/v4', sgxRouter);
    app.use('/tdx/certification/v4', tdxRouter);
  }

  // error handling middleware
  app.use(error.errorHandling);

  // set caching mode
  let cacheMode = Config.get('CachingFillMode');
  if (cacheMode == 'LAZY') {
    cachingModeManager.cachingMode = new LazyCachingMode();
  } else if (cacheMode == 'REQ') {
    cachingModeManager.cachingMode = new ReqCachingMode();
  } else if (cacheMode == 'OFFLINE') {
    cachingModeManager.cachingMode = new OfflineCachingMode();
  } else {
    logger.error('Unknown caching mode. Please check your configuration file.');
    logger.endAndExitProcess();
  }

  // Start HTTPS server
  let privateKey;
  let certificate;
  try {
    privateKey = fs.readFileSync('./ssl_key/private.pem', 'utf8');
    certificate = fs.readFileSync('./ssl_key/file.crt', 'utf8');
  } catch (err) {
    logger.error('The private key or certificate for HTTPS server is missing.');
    logger.endAndExitProcess();
  }
  const credentials = { key: privateKey, cert: certificate };
  const httpsServer = https.createServer(credentials, app);
  httpsServer.listen(
    Config.get('HTTPS_PORT'),
    Config.get('hosts'),
    function () {
      logger.info(
        'HTTPS Server is running on: https://localhost:' +
          Config.get('HTTPS_PORT')
      );
      app.emit('app_started'); // test app need to wait on this event
    }
  );

  // Schedule the refresh job in cron-style
  // # ┌───────────── minute (0 - 59)
  // # │ ┌───────────── hour (0 - 23)
  // # │ │ ┌───────────── day of the month (1 - 31)
  // # │ │ │ ┌───────────── month (1 - 12)
  // # │ │ │ │ ┌───────────── day of the week (0 - 6) (Sunday to Saturday;
  // # │ │ │ │ │                                   7 is also Sunday on some systems)
  // # │ │ │ │ │
  // # │ │ │ │ │
  // # * * * * * command to execute
  //
  scheduleJob(Config.get('RefreshSchedule'), refreshService.scheduledRefresh);
});

export default app;
