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
import PccsError from '../utils/PccsError.js';
import PccsStatus from '../constants/pccs_status_code.js';
import Constants from '../constants/index.js';
import Ajv from 'ajv';
import X509 from '../x509/x509.js';
import * as platformsDao from '../dao/platformsDao.js';
import * as pckcertDao from '../dao/pckcertDao.js';
import * as platformTcbsDao from '../dao/platformTcbsDao.js';
import * as fmspcTcbDao from '../dao/fmspcTcbDao.js';
import * as pckcrlDao from '../dao/pckcrlDao.js';
import * as enclaveIdentityDao from '../dao/enclaveIdentityDao.js';
import * as pckCertchainDao from '../dao/pckCertchainDao.js';
import * as pcsCertificatesDao from '../dao/pcsCertificatesDao.js';
import * as pckLibWrapper from '../lib_wrapper/pcklib_wrapper.js';
import * as appUtil from '../utils/apputil.js';
import { PLATFORM_COLLATERAL_SCHEMA_V4 } from './pccs_schemas.js';
import { sequelize } from '../dao/models/index.js';

const ajv = new Ajv();

function toUpper(str) {
  if (str) return str.toUpperCase();
  else return str;
}

function verify_cert(root1, root2) {
  if (Boolean(root1) && Boolean(root2) && root1 != root2) return false;
  return true;
}

export async function addPlatformCollateral(collateralJson, version) {
  return await sequelize.transaction(async (t) => {
    //check parameters
    let valid;
    if (version < 4)
      valid = ajv.validate(PLATFORM_COLLATERAL_SCHEMA_V3, collateralJson);
    else valid = ajv.validate(PLATFORM_COLLATERAL_SCHEMA_V4, collateralJson);
    if (!valid) {
      throw new PccsError(PccsStatus.PCCS_STATUS_INVALID_REQ);
    }

    // process the collaterals
    let platforms = collateralJson.platforms;
    let collaterals = collateralJson.collaterals;
    let tcbinfos = collaterals.tcbinfos;

    // For every platform we have a set of PCK certs
    for (const platform_certs of collaterals.pck_certs) {
      // Flush and add certs for this platform
      await pckcertDao.deleteCerts(platform_certs.qe_id, platform_certs.pce_id);
      for (const cert of platform_certs.certs) {
        await pckcertDao.upsertPckCert(
          toUpper(platform_certs.qe_id),
          toUpper(platform_certs.pce_id),
          toUpper(cert.tcbm),
          decodeURIComponent(cert.cert)
        );
      }

      // We will update platforms both in cache and in the request list
      // make a full list based on the cache data and the input data
      let cached_platform_tcbs = await platformTcbsDao.getPlatformTcbsById(
        platform_certs.qe_id,
        platform_certs.pce_id
      );
      let new_platforms = platforms.filter(
        (o) =>
          o.pce_id == platform_certs.pce_id && o.qe_id == platform_certs.qe_id
      );
      let new_raw_tcbs = new_platforms.filter(
        (o) => Boolean(o.cpu_svn) && Boolean(o.pce_svn)
      );
      let platforms_all = [];
      for (const cached_platform of cached_platform_tcbs) {
        platforms_all.push({
          qe_id: cached_platform.qe_id,
          pce_id: cached_platform.pce_id,
          cpu_svn: cached_platform.cpu_svn,
          pce_svn: cached_platform.pce_svn,
        });
      }
      for (const raw_tcb of new_raw_tcbs) {
        platforms_all.push({
          qe_id: raw_tcb.qe_id,
          pce_id: raw_tcb.pce_id,
          cpu_svn: raw_tcb.cpu_svn,
          pce_svn: raw_tcb.pce_svn,
        });
      }
      // Remove duplicates
      let platforms_cleaned = platforms_all.filter(
        (element, index, self) =>
          index ===
          self.findIndex(
            (t) =>
              t.qe_id === element.qe_id &&
              t.pce_id === element.pce_id &&
              t.cpu_svn === element.cpu_svn &&
              t.pce_svn === element.pce_svn
          )
      );

      let mycerts = platform_certs.certs;
      if (mycerts == null || mycerts.length == 0) {
        throw new PccsError(PccsStatus.PCCS_STATUS_INVALID_REQ);
      }
      // parse arbitary cert to get fmspc value
      const x509 = new X509();
      if (!x509.parseCert(decodeURIComponent(mycerts[0].cert))) {
        logger.error('Invalid certificate format.');
        throw new PccsError(PccsStatus.PCCS_STATUS_INVALID_REQ);
      }

      let fmspc = x509.fmspc;
      let ca = x509.ca;
      if (fmspc == null || ca == null) {
        logger.error('Invalid certificate format.');
        throw new PccsError(PccsStatus.PCCS_STATUS_INVALID_REQ);
      }
      // get tcbinfo for the fmspc
      let tcbinfo = tcbinfos.find((o) => o.fmspc === fmspc);
      if (tcbinfo == null) {
        logger.error("Can't find TCB info.");
        throw new PccsError(PccsStatus.PCCS_STATUS_INVALID_REQ);
      }

      let tcbinfo_str;
      if (version < 4) tcbinfo_str = JSON.stringify(tcbinfo.tcbinfo);
      else tcbinfo_str = JSON.stringify(tcbinfo.sgx_tcbinfo);

      let pem_certs = mycerts.map((o) => decodeURIComponent(o.cert));
      for (let platform of platforms_cleaned) {
        // get the best cert with PCKCertSelectionTool
        let cert_index = pckLibWrapper.pck_cert_select(
          platform.cpu_svn,
          platform.pce_svn,
          platform.pce_id,
          tcbinfo_str,
          pem_certs,
          pem_certs.length
        );
        if (cert_index == -1) {
          logger.error('Failed to select the best certificate for ' + platform);
          throw new PccsError(PccsStatus.PCCS_STATUS_INVALID_REQ);
        }

        // update platform_tcbs table
        await platformTcbsDao.upsertPlatformTcbs(
          toUpper(platform.qe_id),
          toUpper(platform.pce_id),
          toUpper(platform.cpu_svn),
          toUpper(platform.pce_svn),
          mycerts[cert_index].tcbm
        );
      }

      // update platforms table for new platforms only
      for (const platform of new_platforms) {
        // update platforms/pck_cert table
        await platformsDao.upsertPlatform(
          toUpper(platform.qe_id),
          toUpper(platform.pce_id),
          toUpper(platform.platform_manifest),
          toUpper(platform.enc_ppid),
          toUpper(fmspc),
          toUpper(ca)
        );
      }
    }

    // loop through tcbinfos
    for (const tcbinfo of tcbinfos) {
      tcbinfo.fmspc = toUpper(tcbinfo.fmspc);
      tcbinfo.version = version;
      if (version < 4 && tcbinfo.tcbinfo) {
        tcbinfo.type = Constants.PROD_TYPE_SGX;
        tcbinfo.tcbinfo = Buffer.from(JSON.stringify(tcbinfo.tcbinfo));
        await fmspcTcbDao.upsertFmspcTcb(tcbinfo);
      }
      if (version >= 4) {
        if (tcbinfo.sgx_tcbinfo) {
          tcbinfo.type = Constants.PROD_TYPE_SGX;
          tcbinfo.tcbinfo = Buffer.from(JSON.stringify(tcbinfo.sgx_tcbinfo));
          await fmspcTcbDao.upsertFmspcTcb(tcbinfo);
        }
        if (tcbinfo.tdx_tcbinfo) {
          tcbinfo.type = Constants.PROD_TYPE_TDX;
          tcbinfo.tcbinfo = Buffer.from(JSON.stringify(tcbinfo.tdx_tcbinfo));
          await fmspcTcbDao.upsertFmspcTcb(tcbinfo);
        }
      }
    }

    // Update or insert PCK CRL
    if (collaterals.pckcacrl) {
      if (collaterals.pckcacrl.processorCrl)
        await pckcrlDao.upsertPckCrl(
          Constants.CA_PROCESSOR,
          Buffer.from(collaterals.pckcacrl.processorCrl, 'hex')
        );
      if (collaterals.pckcacrl.platformCrl)
        await pckcrlDao.upsertPckCrl(
          Constants.CA_PLATFORM,
          Buffer.from(collaterals.pckcacrl.platformCrl, 'hex')
        );
    }

    // Update or insert QE Identity
    if (collaterals.qeidentity) {
      await enclaveIdentityDao.upsertEnclaveIdentity(
        Constants.QE_IDENTITY_ID,
        collaterals.qeidentity,
        version
      );
    }
    // Update or insert TDQE Identity
    if (collaterals.tdqeidentity) {
      await enclaveIdentityDao.upsertEnclaveIdentity(
        Constants.TDQE_IDENTITY_ID,
        collaterals.tdqeidentity,
        version
      );
    }
    // Update or insert QvE Identity
    if (collaterals.qveidentity) {
      await enclaveIdentityDao.upsertEnclaveIdentity(
        Constants.QVE_IDENTITY_ID,
        collaterals.qveidentity,
        version
      );
    }

    // Update or insert PCK Certchain
    await pckCertchainDao.upsertPckCertchain(Constants.CA_PROCESSOR);
    await pckCertchainDao.upsertPckCertchain(Constants.CA_PLATFORM);

    // Update or insert PCS certificates
    let rootCert = new Array();
    if (
      Boolean(
        collaterals.certificates[Constants.SGX_PCK_CERTIFICATE_ISSUER_CHAIN]
      )
    ) {
      if (
        Boolean(
          collaterals.certificates[Constants.SGX_PCK_CERTIFICATE_ISSUER_CHAIN][
            Constants.CA_PROCESSOR
          ]
        )
      ) {
        rootCert[0] = await pcsCertificatesDao.upsertPckCertificateIssuerChain(
          Constants.CA_PROCESSOR,
          collaterals.certificates[Constants.SGX_PCK_CERTIFICATE_ISSUER_CHAIN][
            Constants.CA_PROCESSOR
          ]
        );
      }
      if (
        Boolean(
          collaterals.certificates[Constants.SGX_PCK_CERTIFICATE_ISSUER_CHAIN][
            Constants.CA_PLATFORM
          ]
        )
      ) {
        rootCert[1] = await pcsCertificatesDao.upsertPckCertificateIssuerChain(
          Constants.CA_PLATFORM,
          collaterals.certificates[Constants.SGX_PCK_CERTIFICATE_ISSUER_CHAIN][
            Constants.CA_PLATFORM
          ]
        );
      }
    }
    if (
      Boolean(
        collaterals.certificates[appUtil.getTcbInfoIssuerChainName(version)]
      )
    ) {
      rootCert[2] = await pcsCertificatesDao.upsertTcbInfoIssuerChain(
        collaterals.certificates[appUtil.getTcbInfoIssuerChainName(version)]
      );
    }
    if (
      Boolean(
        collaterals.certificates[Constants.SGX_ENCLAVE_IDENTITY_ISSUER_CHAIN]
      )
    ) {
      rootCert[3] = await pcsCertificatesDao.upsertEnclaveIdentityIssuerChain(
        collaterals.certificates[Constants.SGX_ENCLAVE_IDENTITY_ISSUER_CHAIN]
      );
    }
    if (
      !verify_cert(rootCert[0], rootCert[1]) ||
      !verify_cert(rootCert[1], rootCert[2]) ||
      !verify_cert(rootCert[2], rootCert[3])
    ) {
      throw new PccsError(PccsStatus.PCCS_STATUS_INTEGRITY_ERROR);
    }

    // Update or insert rootcacrl in DER format
    if (collaterals.rootcacrl) {
      await pcsCertificatesDao.upsertRootCACrl(
        Buffer.from(collaterals.rootcacrl, 'hex')
      );
    }
  });
}
