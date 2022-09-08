#ifndef PCE_SERVICE_EXPORT_H
#define PCE_SERVICE_EXPORT_H
#include "service.h"
#include "sgx_report.h"
#include "aeerror.h"
#include "aesm_error.h"
#include "epid_pve_type.h"
#include "se_sig_rl.h"


struct IPceService : virtual public IService
{
    // The value should be the same as the major version in manifest.json
    enum {VERSION = 2};
    virtual ~IPceService() = default;

    //TODO: shall we remove thse 2 interfaces?
    // These interfaces provide other bundles the abilities
    // to manage the EPC usage and reduce latency. But it will
    // increase the coupling between bundles.
    virtual ae_error_t load_enclave() = 0;
    virtual uint32_t pce_get_target(
        sgx_target_info_t *p_target,
        sgx_isv_svn_t *p_isvsvn) = 0;

    virtual uint32_t get_pce_info(
        const sgx_report_t *p_report,
        const uint8_t *p_pek,
        uint32_t pek_size,
        uint8_t crypto_suite,
        uint8_t *p_encrypted_ppid,
        uint32_t encrypted_ppid_size,
        uint32_t *p_encrypted_ppid_out_size,
        sgx_isv_svn_t* p_pce_isvsvn,
        uint16_t* p_pce_id,
        uint8_t *p_signature_scheme) = 0;

    virtual uint32_t pce_sign_report(
        const sgx_isv_svn_t *p_isv_svn,
        const sgx_cpu_svn_t *p_cpu_svn,
        const sgx_report_t *p_report,
        uint8_t *p_sig,
        uint32_t sig_size,
        uint32_t *p_sig_out_size) = 0;
};

#endif /* PCE_SERVICE_EXPORT_H */
