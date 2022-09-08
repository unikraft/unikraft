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
import Router from 'express';
import {
  platformsController,
  platformCollateralController,
  pckcertController,
  pckcrlController,
  tcbinfoController,
  identityController,
  rootcacrlController,
  refreshController,
  crlController,
} from '../controllers/index.js';

// express routes for our API
const sgxRouter = Router();
const tdxRouter = Router();

//---------------- Routes for SGX APIs-------------------------------
sgxRouter
  .route('/platforms')
  .post(platformsController.postPlatforms)
  .get(platformsController.getPlatforms);

sgxRouter
  .route('/platformcollateral')
  .put(platformCollateralController.putPlatformCollateral);

sgxRouter.route('/pckcert').get(pckcertController.getPckCert);

sgxRouter.route('/pckcrl').get(pckcrlController.getPckCrl);

sgxRouter.route('/tcb').get(tcbinfoController.getSgxTcbInfo);

sgxRouter.route('/qe/identity').get(identityController.getEcdsaQeIdentity);

sgxRouter.route('/qve/identity').get(identityController.getQveIdentity);

sgxRouter.route('/rootcacrl').get(rootcacrlController.getRootCaCrl);

sgxRouter.route('/crl').get(crlController.getCrl);

sgxRouter
  .route('/refresh')
  .post(refreshController.refreshCache)
  .get(refreshController.refreshCache);

//---------------- Routes for TDX APIs-------------------------------
tdxRouter.route('/tcb').get(tcbinfoController.getTdxTcbInfo);

tdxRouter.route('/qe/identity').get(identityController.getTdQeIdentity);

export { sgxRouter, tdxRouter };
