#ifndef EPID_QUOTE_SERVICE_EXPORT_H
#define EPID_QUOTE_SERVICE_EXPORT_H
#include "quote_service.h"
#include "aesm_error.h"
#include "aeerror.h"
#include "es_info.h"
#include "tlv_common.h"
#include "platform_info_blob.h"


struct IEpidQuoteService : public IQuoteService
{
    // The value should be the same as the major version in manifest.json
    enum {VERSION = 2};
    virtual ~IEpidQuoteService() = default;

    virtual aesm_error_t get_extended_epid_group_id(
        uint32_t* x_group_id) = 0;
    virtual aesm_error_t switch_extended_epid_group(
        uint32_t x_group_id) = 0;
    virtual uint32_t endpoint_selection(
        endpoint_selection_infos_t& es_info) = 0;
    virtual ae_error_t need_epid_provisioning(
        const platform_info_blob_wrapper_t* p_platform_info_blob) = 0;
    virtual aesm_error_t provision(
        bool performance_rekey_used,
        uint32_t timeout_usec) = 0;
    virtual const char *get_server_url(
        aesm_network_server_enum_type_t type) = 0;
    virtual const char *get_pse_provisioning_url(
        const endpoint_selection_infos_t& es_info) = 0;
    virtual aesm_error_t report_attestation_status(
        uint8_t* platform_info, uint32_t platform_info_size,
        uint32_t attestation_status,
        uint8_t* update_info, uint32_t update_info_size) = 0;
    
    virtual aesm_error_t check_update_status(
        uint8_t* platform_info, uint32_t platform_info_size,
        uint8_t* update_info, uint32_t update_info_size,
        uint32_t attestation_status, uint32_t* status) = 0;        
};

#endif /* EPID_QUOTE_SERVICE_EXPORT_H */
