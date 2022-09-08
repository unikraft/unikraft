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

use clap::{Arg, App};
use std::mem::size_of;
use std::convert::TryInto;
use std::time::SystemTime;

use sgx_dcap_quoteverify_rs as qvl;
use sgx_dcap_quoteverify_sys as qvl_sys;


#[cfg(debug_assertions)]
const SGX_DEBUG_FLAG: i32 = 1;
#[cfg(not(debug_assertions))]
const SGX_DEBUG_FLAG: i32 = 0;

const SAMPLE_ISV_ENCLAVE: &str = "../QuoteVerificationSample/enclave.signed.so\0";
const DEFAULT_QUOTE: &str = "../QuoteGenerationSample/quote.dat";


// C library bindings

#[link(name = "sgx_urts")]
extern {
    fn sgx_create_enclave(file_name: *const u8,
                            debug: i32,
                            launch_token: *mut [u8; 1024usize],
                            launch_token_updated: *mut i32,
                            enclave_id: *mut u64,
                            misc_attr: *mut qvl_sys::sgx_misc_attribute_t) -> u32;
    fn sgx_destroy_enclave(enclave_id: u64) -> u32;
}

#[link(name = "enclave_untrusted")]
extern {
    fn ecall_get_target_info(eid: u64, retval: *mut u32, target_info: *mut qvl_sys::sgx_target_info_t) -> u32;
    fn sgx_tvl_verify_qve_report_and_identity(eid: u64, retval: *mut qvl::quote3_error_t,
                                                p_quote: *const u8, quote_size: u32,
                                                p_qve_report_info: *const qvl::sgx_ql_qe_report_info_t,
                                                expiration_check_date: i64,
                                                collateral_expiration_status: u32,
                                                quote_verification_result: qvl::sgx_ql_qv_result_t,
                                                p_supplemental_data: *const u8,
                                                supplemental_data_size: u32,
                                                qve_isvsvn_threshold: qvl_sys::sgx_isv_svn_t) -> u32;
}


/// Quote verification with QvE/QVL
///
/// # Param
/// - **quote**\
/// ECDSA quote buffer.
/// - **use_qve**\
/// Set quote verification mode.\
///     - If true, quote verification will be performed by Intel QvE.
///     - If false, quote verification will be performed by untrusted QVL.
///
fn ecdsa_quote_verification(quote: &[u8], use_qve: bool) {

    let mut supplemental_data_size = 0u32;      // mem::zeroed() is safe as long as the struct doesn't have zero-invalid types, like pointers
    let mut supplemental_data: qvl::sgx_ql_qv_supplemental_t = unsafe { std::mem::zeroed() };
    let mut quote_verification_result = qvl::sgx_ql_qv_result_t::SGX_QL_QV_RESULT_UNSPECIFIED;
    let mut qve_report_info: qvl::sgx_ql_qe_report_info_t = unsafe { std::mem::zeroed() };
    let rand_nonce = "59jslk201fgjmm;\0";
    let mut collateral_expiration_status = 1u32;

    let mut updated = 0i32;
    let mut verify_qveid_ret = qvl::quote3_error_t::SGX_QL_ERROR_UNEXPECTED;
    let mut eid: u64 = 0;
    let mut token = [0u8; 1024usize];

    if use_qve {

        // Trusted quote verification

        // set nonce
        //
        qve_report_info.nonce.rand.copy_from_slice(rand_nonce.as_bytes());

        // get target info of SampleISVEnclave. QvE will target the generated report to this enclave.
        //
        let sgx_ret = unsafe { sgx_create_enclave(SAMPLE_ISV_ENCLAVE.as_ptr(), SGX_DEBUG_FLAG,
                                        &mut token as *mut [u8; 1024usize],
                                        &mut updated as *mut i32,
                                        &mut eid as *mut u64,
                                        std::ptr::null_mut()) };
        if sgx_ret != 0 {
            println!("\tError: Can't load SampleISVEnclave: {:#04x}", sgx_ret);
            return;
        }
        let mut get_target_info_ret = 1u32;
        let sgx_ret = unsafe { ecall_get_target_info(eid, &mut get_target_info_ret as *mut u32,
                                                    &mut qve_report_info.app_enclave_target_info as *mut qvl_sys::sgx_target_info_t) };
        if sgx_ret != 0 || get_target_info_ret != 0 {
            println!("\tError in sgx_get_target_info. {:#04x}", get_target_info_ret);
        } else {
            println!("\tInfo: get target info successfully returned.");
        }

        // call DCAP quote verify library to set QvE loading policy
        //
        let dcap_ret = qvl::sgx_qv_set_enclave_load_policy(qvl::sgx_ql_request_policy_t::SGX_QL_DEFAULT);
        if qvl::quote3_error_t::SGX_QL_SUCCESS == dcap_ret {
            println!("\tInfo: sgx_qv_set_enclave_load_policy successfully returned.");
        } else {
            println!("\tError: sgx_qv_set_enclave_load_policy failed: {:#04x}", dcap_ret as u32);
        }

        // call DCAP quote verify library to get supplemental data size
        //
        let dcap_ret = qvl::sgx_qv_get_quote_supplemental_data_size(&mut supplemental_data_size);
        if qvl::quote3_error_t::SGX_QL_SUCCESS == dcap_ret && std::mem::size_of::<qvl::sgx_ql_qv_supplemental_t>() as u32 == supplemental_data_size {
            println!("\tInfo: sgx_qv_get_quote_supplemental_data_size successfully returned.");
        } else {
            if dcap_ret != qvl::quote3_error_t::SGX_QL_SUCCESS {
                println!("\tError: sgx_qv_get_quote_supplemental_data_size failed: {:#04x}", dcap_ret as u32);
            }
            if supplemental_data_size != size_of::<qvl::sgx_ql_qv_supplemental_t>().try_into().unwrap() {
                println!("\tWarning: Quote supplemental data size is different between DCAP QVL and QvE, please make sure you installed DCAP QVL and QvE from same release.");
            }
            supplemental_data_size = 0u32;
        }

        // set current time. This is only for sample purposes, in production mode a trusted time should be used.
        //
        let current_time: i64 = SystemTime::now().duration_since(SystemTime::UNIX_EPOCH).unwrap().as_secs().try_into().unwrap();

        let p_supplemental_data = match supplemental_data_size {
            0 => None,
            _ => Some(&mut supplemental_data),
        };


        // call DCAP quote verify library for quote verification
        // here you can choose 'trusted' or 'untrusted' quote verification by specifying parameter '&qve_report_info'
        // if '&qve_report_info' is NOT NULL, this API will call Intel QvE to verify quote
        // if '&qve_report_info' is NULL, this API will call 'untrusted quote verify lib' to verify quote, this mode doesn't rely on SGX capable system, but the results can not be cryptographically authenticated
        let dcap_ret = qvl::sgx_qv_verify_quote(
            quote,
            None,
            current_time,
            &mut collateral_expiration_status,
            &mut quote_verification_result,
            Some(&mut qve_report_info),
            supplemental_data_size,
            p_supplemental_data);
        if qvl::quote3_error_t::SGX_QL_SUCCESS == dcap_ret {
            println!("\tInfo: App: sgx_qv_verify_quote successfully returned.");
        } else {
            println!("\tError: App: sgx_qv_verify_quote failed: {:#04x}", dcap_ret as u32);
        }


        // Threshold of QvE ISV SVN. The ISV SVN of QvE used to verify quote must be greater or equal to this threshold
        // e.g. You can get latest QvE ISVSVN in QvE Identity JSON file from
        // https://api.trustedservices.intel.com/sgx/certification/v2/qve/identity
        // Make sure you are using trusted & latest QvE ISV SVN as threshold
        //
        let qve_isvsvn_threshold: qvl_sys::sgx_isv_svn_t = 5;

        let p_supplemental_data = match supplemental_data_size {
            0 => std::ptr::null(),
            _ => &supplemental_data as *const qvl::sgx_ql_qv_supplemental_t as *const u8,
        };

        // call sgx_dcap_tvl API in SampleISVEnclave to verify QvE's report and identity
        //
        let sgx_ret = unsafe { sgx_tvl_verify_qve_report_and_identity(eid, &mut verify_qveid_ret as *mut qvl::quote3_error_t,
            quote.as_ptr(), quote.len() as u32,
            &qve_report_info as *const qvl::sgx_ql_qe_report_info_t,
            current_time,
            collateral_expiration_status,
            quote_verification_result,
            p_supplemental_data,
            supplemental_data_size,
            qve_isvsvn_threshold) };
        if sgx_ret != 0 || verify_qveid_ret != qvl::quote3_error_t::SGX_QL_SUCCESS {
            println!("\tError: Ecall: Verify QvE report and identity failed. {:#04x}", verify_qveid_ret as u32);
        } else {
            println!("\tInfo: Ecall: Verify QvE report and identity successfully returned.")
        }

        unsafe { sgx_destroy_enclave(eid) };

    } else {

        // Untrusted quote verification

        // call DCAP quote verify library to get supplemental data size
        //
        let dcap_ret = qvl::sgx_qv_get_quote_supplemental_data_size(&mut supplemental_data_size);
        if qvl::quote3_error_t::SGX_QL_SUCCESS == dcap_ret && std::mem::size_of::<qvl::sgx_ql_qv_supplemental_t>() as u32 == supplemental_data_size {
            println!("\tInfo: sgx_qv_get_quote_supplemental_data_size successfully returned.");
        } else {
            if dcap_ret != qvl::quote3_error_t::SGX_QL_SUCCESS {
                println!("\tError: sgx_qv_get_quote_supplemental_data_size failed: {:#04x}", dcap_ret as u32);
            }
            if supplemental_data_size != size_of::<qvl::sgx_ql_qv_supplemental_t>().try_into().unwrap() {
                println!("\tWarning: Quote supplemental data size is different between DCAP QVL and QvE, please make sure you installed DCAP QVL and QvE from same release.");
            }
            supplemental_data_size = 0u32;
        }

        // set current time. This is only for sample purposes, in production mode a trusted time should be used.
        //
        let current_time: i64 = SystemTime::now().duration_since(SystemTime::UNIX_EPOCH).unwrap().as_secs().try_into().unwrap();

        let p_supplemental_data = match supplemental_data_size {
            0 => None,
            _ => Some(&mut supplemental_data),
        };


        // call DCAP quote verify library for quote verification
        // here you can choose 'trusted' or 'untrusted' quote verification by specifying parameter '&qve_report_info'
        // if '&qve_report_info' is NOT NULL, this API will call Intel QvE to verify quote
        // if '&qve_report_info' is NULL, this API will call 'untrusted quote verify lib' to verify quote, this mode doesn't rely on SGX capable system, but the results can not be cryptographically authenticated
        let dcap_ret = qvl::sgx_qv_verify_quote(
            quote,
            None,
            current_time,
            &mut collateral_expiration_status,
            &mut quote_verification_result,
            None,
            supplemental_data_size,
            p_supplemental_data);
        if qvl::quote3_error_t::SGX_QL_SUCCESS == dcap_ret {
            println!("\tInfo: App: sgx_qv_verify_quote successfully returned.");
        } else {
            println!("\tError: App: sgx_qv_verify_quote failed: {:#04x}", dcap_ret as u32);
        }

    }
    
    // check verification result
    //
    match quote_verification_result {
        qvl::sgx_ql_qv_result_t::SGX_QL_QV_RESULT_OK => {
            // check verification collateral expiration status
            // this value should be considered in your own attestation/verification policy
            //
            if 0u32 == collateral_expiration_status {
                println!("\tInfo: App: Verification completed successfully.");
            } else {
                println!("\tWarning: App: Verification completed, but collateral is out of date based on 'expiration_check_date' you provided.");
            }
        },
        qvl::sgx_ql_qv_result_t::SGX_QL_QV_RESULT_CONFIG_NEEDED |
        qvl::sgx_ql_qv_result_t::SGX_QL_QV_RESULT_OUT_OF_DATE |
        qvl::sgx_ql_qv_result_t::SGX_QL_QV_RESULT_OUT_OF_DATE_CONFIG_NEEDED |
        qvl::sgx_ql_qv_result_t::SGX_QL_QV_RESULT_SW_HARDENING_NEEDED |
        qvl::sgx_ql_qv_result_t::SGX_QL_QV_RESULT_CONFIG_AND_SW_HARDENING_NEEDED => {
            println!("\tWarning: App: Verification completed with Non-terminal result: {:x}", quote_verification_result as u32);
        },
        qvl::sgx_ql_qv_result_t::SGX_QL_QV_RESULT_INVALID_SIGNATURE |
        qvl::sgx_ql_qv_result_t::SGX_QL_QV_RESULT_REVOKED |
        qvl::sgx_ql_qv_result_t::SGX_QL_QV_RESULT_UNSPECIFIED | _ => {
            println!("\tError: App: Verification completed with Terminal result: {:x}", quote_verification_result as u32);
        },
    }

    // check supplemental data if necessary
    //
    if supplemental_data_size > 0 {

        // you can check supplemental data based on your own attestation/verification policy
        // here we only print supplemental data version for demo usage
        //
        println!("\tInfo: Supplemental data version: {}", supplemental_data.version);
    }
}


fn main() {
    // Specify quote path from command line arguments
    //
    let matches = App::new("Rust Quote Verification Sample App:")
                    .version("1.0")
                    .arg(Arg::with_name("quote_path")
                        .long("quote")
                        .value_name("FILE")
                        .help(format!("Specify quote path, default is {}", DEFAULT_QUOTE).as_str()))
                    .get_matches();
    let quote_path = matches.value_of("quote_path").unwrap_or(DEFAULT_QUOTE);

    //read quote from file
    //
    let quote = std::fs::read(quote_path).expect("Unable to read quote file");

    println!("Info: ECDSA quote path: {}", quote_path);


    // We demonstrate two different types of quote verification
    //      a. Trusted quote verification - quote will be verified by Intel QvE
    //      b. Untrusted quote verification - quote will be verified by untrusted QVL (Quote Verification Library)
    //          this mode doesn't rely on SGX capable system, but the results can not be cryptographically authenticated
    //

    // Trusted quote verification, ignore error checking
    //
    println!("\nTrusted quote verification:");
    ecdsa_quote_verification(&quote, true);

    println!("\n===========================================");

    // Unrusted quote verification, ignore error checking
    //
    println!("\nUntrusted quote verification:");
    ecdsa_quote_verification(&quote, false);

    println!();
}
