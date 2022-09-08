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
//! Intel(R) Software Guard Extensions Data Center Attestation Primitives (Intel(R) SGX DCAP)
//! Rust wrapper for Quote Verification Library
//! ================================================
//! 
//! This is a safe wrapper for **sgx-dcap-quoteverify-sys**.

use std::ffi::CString;
use sgx_dcap_quoteverify_sys as qvl_sys;

pub use qvl_sys::quote3_error_t;
pub use qvl_sys::sgx_ql_request_policy_t;
pub use qvl_sys::sgx_ql_qv_supplemental_t;
pub use qvl_sys::sgx_ql_qve_collateral_t;
pub use qvl_sys::tdx_ql_qve_collateral_t;
pub use qvl_sys::sgx_ql_qv_result_t;
pub use qvl_sys::sgx_ql_qe_report_info_t;
pub use qvl_sys::sgx_qv_path_type_t;


/// When the Quoting Verification Library is linked to a process, it needs to know the proper enclave loading policy.
/// The library may be linked with a long lived process, such as a service, where it can load the enclaves and leave
/// them loaded (persistent). This better ensures that the enclaves will be available upon quote requests and not subject
/// to EPC limitations if loaded on demand. However, if the Quoting library is linked with an application process, there
/// may be many applications with the Quoting library and a better utilization of EPC is to load and unloaded the quoting
/// enclaves on demand (ephemeral).  The library will be shipped with a default policy of loading enclaves and leaving
/// them loaded until the library is unloaded (PERSISTENT). If the policy is set to EPHEMERAL, then the QE and PCE will
/// be loaded and unloaded on-demand.  If either enclave is already loaded when the policy is change to EPHEMERAL, the
/// enclaves will be unloaded before returning.
///
/// # Param
/// - **policy**\
/// Sets the requested enclave loading policy to either *SGX_QL_PERSISTENT*, *SGX_QL_EPHEMERAL*
/// or *SGX_QL_DEFAULT*.
///
/// # Return
/// - ***SGX_QL_SUCCESS***\
/// Successfully set the enclave loading policy for the quoting library's enclaves.\
/// - ***SGX_QL_UNSUPPORTED_LOADING_POLICY***\
/// The selected policy is not support by the quoting library.\
/// - ***SGX_QL_ERROR_UNEXPECTED***\
/// Unexpected internal error.
///
/// # Examples
/// ```
/// use sgx_dcap_quoteverify_rs::*;
/// 
/// let policy = sgx_ql_request_policy_t::SGX_QL_DEFAULT;
/// let ret = sgx_qv_set_enclave_load_policy(policy);
/// 
/// assert_eq!(ret, quote3_error_t::SGX_QL_SUCCESS);
/// ```
pub fn sgx_qv_set_enclave_load_policy(policy: sgx_ql_request_policy_t) -> quote3_error_t {
    unsafe {qvl_sys::sgx_qv_set_enclave_load_policy(policy)}
}

/// Get SGX supplemental data required size.
///
/// # Param
/// - **p_data_size\[OUT\]**\
/// Pointer to hold the size of the buffer in bytes required to contain all of the supplemental data.
///
/// # Return
/// Status code of the operation, one of:
/// - *SGX_QL_SUCCESS*
/// - *SGX_QL_ERROR_INVALID_PARAMETER*
/// - *SGX_QL_ERROR_QVL_QVE_MISMATCH*
/// - *SGX_QL_ENCLAVE_LOAD_ERROR*
///
/// # Examples
/// ```
/// use sgx_dcap_quoteverify_rs::*;
/// 
/// let mut data_size: u32 = 0;
/// let ret = sgx_qv_get_quote_supplemental_data_size(&mut data_size);
/// 
/// assert_eq!(ret, quote3_error_t::SGX_QL_SUCCESS);
/// assert_eq!(data_size, std::mem::size_of::<sgx_ql_qv_supplemental_t>() as u32);
/// ```
pub fn sgx_qv_get_quote_supplemental_data_size(p_data_size: &mut u32) -> quote3_error_t {
    unsafe {qvl_sys::sgx_qv_get_quote_supplemental_data_size(p_data_size as *mut u32)}
}

/// Perform SGX ECDSA quote verification.
///
/// # Param
/// - **quote\[IN\]**\
/// SGX Quote, presented as u8 vector.
/// - **p_quote_collateral\[IN\]**\
/// This is a pointer to the Quote Certification Collateral provided by the caller.
/// - **expiration_check_date\[IN\]**\
/// This is the date that the QvE will use to determine if any of the inputted collateral have expired.
/// - **p_collateral_expiration_status\[OUT\]**\
/// Address of the outputted expiration status.  This input must not be NULL.
/// - **p_quote_verification_result\[OUT\]**\
/// Address of the outputted quote verification result.
/// - **p_qve_report_info\[IN/OUT\]**\
/// This parameter can be used in 2 ways.\
///     - If p_qve_report_info is NOT None, the API will use Intel QvE to perform quote verification, and QvE will generate a report using the target_info in sgx_ql_qe_report_info_t structure.\
///     - if p_qve_report_info is None, the API will use QVL library to perform quote verification, not that the results can not be cryptographically authenticated in this mode.
/// - **supplemental_data_size\[IN\]**\
/// Size of the buffer pointed to by p_quote (in bytes).
/// - **p_supplemental_data\[OUT\]**\
/// The parameter is optional.  If it is None, supplemental_data_size must be 0.
///
/// # Return
/// Status code of the operation, one of:
/// - *SGX_QL_SUCCESS*
/// - *SGX_QL_ERROR_INVALID_PARAMETER*
/// - *SGX_QL_QUOTE_FORMAT_UNSUPPORTED*
/// - *SGX_QL_QUOTE_CERTIFICATION_DATA_UNSUPPORTED*
/// - *SGX_QL_UNABLE_TO_GENERATE_REPORT*
/// - *SGX_QL_CRL_UNSUPPORTED_FORMAT*
/// - *SGX_QL_ERROR_UNEXPECTED*
///
pub fn sgx_qv_verify_quote(
    quote: &[u8],
    p_quote_collateral: Option<&sgx_ql_qve_collateral_t>,
    expiration_check_date: i64,
    p_collateral_expiration_status: &mut u32,
    p_quote_verification_result: &mut sgx_ql_qv_result_t,
    p_qve_report_info: Option<&mut sgx_ql_qe_report_info_t>,
    supplemental_data_size: u32,
    p_supplemental_data: Option<&mut sgx_ql_qv_supplemental_t>
) -> quote3_error_t {

    // Match Option types to raw pointers
    //
    let p_quote_collateral = match p_quote_collateral {
        Some(p) => p as *const sgx_ql_qve_collateral_t,
        None => std::ptr::null(),
    };
    let p_qve_report_info = match p_qve_report_info {
        Some(p) => p as *mut sgx_ql_qe_report_info_t,
        None => std::ptr::null_mut(),
    };
    let p_supplemental_data = match p_supplemental_data {
        Some(p) => p as *mut sgx_ql_qv_supplemental_t as *mut u8,
        None => std::ptr::null_mut(),
    };

    unsafe {
        qvl_sys::sgx_qv_verify_quote(
            quote.as_ptr(),
            quote.len() as u32,
            p_quote_collateral,
            expiration_check_date,
            p_collateral_expiration_status as *mut u32,
            p_quote_verification_result as *mut sgx_ql_qv_result_t,
            p_qve_report_info,
            supplemental_data_size,
            p_supplemental_data)
    }
}

/// Get TDX supplemental data required size.
///
/// # Param
/// - **p_data_size\[OUT\]**\
/// Pointer to hold the size of the buffer in bytes required to contain all of the supplemental data.
///
/// # Return
/// Status code of the operation, one of:
/// - *SGX_QL_SUCCESS*
/// - *SGX_QL_ERROR_INVALID_PARAMETER*
/// - *SGX_QL_ERROR_QVL_QVE_MISMATCH*
/// - *SGX_QL_ENCLAVE_LOAD_ERROR*
///
/// # Examples
/// ```
/// use sgx_dcap_quoteverify_rs::*;
/// 
/// let mut data_size: u32 = 0;
/// let ret = tdx_qv_get_quote_supplemental_data_size(&mut data_size);
/// 
/// assert_eq!(ret, quote3_error_t::SGX_QL_SUCCESS);
/// assert_eq!(data_size, std::mem::size_of::<sgx_ql_qv_supplemental_t>() as u32);
/// ```
pub fn tdx_qv_get_quote_supplemental_data_size(p_data_size: &mut u32) -> quote3_error_t {
    unsafe {qvl_sys::tdx_qv_get_quote_supplemental_data_size(p_data_size as *mut u32)}
}

/// Perform TDX ECDSA quote verification.
///
/// # Param
/// - **quote\[IN\]**\
/// TDX Quote, presented as u8 vector.
/// - **p_quote_collateral\[IN\]**\
/// This is a pointer to the Quote Certification Collateral provided by the caller.
/// - **expiration_check_date\[IN\]**\
/// This is the date that the QvE will use to determine if any of the inputted collateral have expired.
/// - **p_collateral_expiration_status\[OUT\]**\
/// Address of the outputted expiration status.  This input must not be NULL.
/// - **p_quote_verification_result\[OUT\]**\
/// Address of the outputted quote verification result.
/// - **p_qve_report_info\[IN/OUT\]**\
/// This parameter can be used in 2 ways.\
///     - If p_qve_report_info is NOT None, the API will use Intel QvE to perform quote verification, and QvE will generate a report using the target_info in sgx_ql_qe_report_info_t structure.\
///     - if p_qve_report_info is None, the API will use QVL library to perform quote verification, not that the results can not be cryptographically authenticated in this mode.
/// - **supplemental_data_size\[IN\]**\
/// Size of the buffer pointed to by p_quote (in bytes).
/// - **p_supplemental_data\[OUT\]**\
/// The parameter is optional.  If it is None, supplemental_data_size must be 0.
///
/// # Return
/// Status code of the operation, one of:
/// - *SGX_QL_SUCCESS*
/// - *SGX_QL_ERROR_INVALID_PARAMETER*
/// - *SGX_QL_QUOTE_FORMAT_UNSUPPORTED*
/// - *SGX_QL_QUOTE_CERTIFICATION_DATA_UNSUPPORTED*
/// - *SGX_QL_UNABLE_TO_GENERATE_REPORT*
/// - *SGX_QL_CRL_UNSUPPORTED_FORMAT*
/// - *SGX_QL_ERROR_UNEXPECTED*
///
pub fn tdx_qv_verify_quote(
    quote: &[u8],
    p_quote_collateral: Option<&tdx_ql_qve_collateral_t>,
    expiration_check_date: i64,
    p_collateral_expiration_status: &mut u32,
    p_quote_verification_result: &mut sgx_ql_qv_result_t,
    p_qve_report_info: Option<&mut sgx_ql_qe_report_info_t>,
    supplemental_data_size: u32,
    p_supplemental_data: Option<&mut sgx_ql_qv_supplemental_t>
) -> quote3_error_t {

    // Match Option types to raw pointers
    //
    let p_quote_collateral = match p_quote_collateral {
        Some(p) => p as *const tdx_ql_qve_collateral_t,
        None => std::ptr::null(),
    };
    let p_qve_report_info = match p_qve_report_info {
        Some(p) => p as *mut sgx_ql_qe_report_info_t,
        None => std::ptr::null_mut(),
    };
    let p_supplemental_data = match p_supplemental_data {
        Some(p) => p as *mut sgx_ql_qv_supplemental_t as *mut u8,
        None => std::ptr::null_mut(),
    };

    unsafe {
        qvl_sys::tdx_qv_verify_quote(
            quote.as_ptr(),
            quote.len() as u32,
            p_quote_collateral,
            expiration_check_date,
            p_collateral_expiration_status as *mut u32,
            p_quote_verification_result as *mut sgx_ql_qv_result_t,
            p_qve_report_info,
            supplemental_data_size,
            p_supplemental_data)
    }
}

/// Set the full path of QVE and QPL library.
/// The function takes the enum and the corresponding full path.
///
/// # Param
/// - **path_type**\
/// The type of binary being passed in.
/// - **p_path**\
/// It should be a valid full path.
///
/// # Return
/// - ***SGX_QL_SUCCESS***\
/// Successfully set the full path.
/// - ***SGX_QL_ERROR_INVALID_PARAMETER***\
/// p_path is not a valid full path or the path is too long.
///
#[cfg(target_os = "linux")]
pub fn sgx_qv_set_path(path_type: sgx_qv_path_type_t,  p_path: CString) -> quote3_error_t {
    unsafe {qvl_sys::sgx_qv_set_path(path_type, p_path.as_ptr())}
}
