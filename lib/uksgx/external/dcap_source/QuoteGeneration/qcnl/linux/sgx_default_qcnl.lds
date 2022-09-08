{
global:
    sgx_qcnl_get_pck_cert_chain;
    sgx_qcnl_free_pck_cert_chain;
    sgx_qcnl_get_pck_crl_chain;
    sgx_qcnl_free_pck_crl_chain;
    sgx_qcnl_get_tcbinfo;
    sgx_qcnl_free_tcbinfo;
    tdx_qcnl_get_tcbinfo;
    tdx_qcnl_free_tcbinfo;
    sgx_qcnl_get_qe_identity;
    sgx_qcnl_free_qe_identity;
    sgx_qcnl_get_qve_identity;
    sgx_qcnl_free_qve_identity;
    sgx_qcnl_get_root_ca_crl;
    sgx_qcnl_free_root_ca_crl;
    sgx_qcnl_get_api_version;
    sgx_qcnl_set_logging_callback;

local:
    *;
};

