#include <epid_quote_service.h>
#include <quote_provider_service.h>
#include <network_service.h>
#include <pce_service.h>

#include <cppmicroservices/BundleActivator.h>
#include "cppmicroservices/BundleContext.h"
#include <cppmicroservices/GetBundleContext.h>

#include <iostream>
// Need this macro definition before inttypes.h to use printing format
// specifiers(SCNu32 and PRIu32 used below) in C++
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "aesm_xegd_blob.h"
#include "aesm_logic.h"
#include "pve_logic.h"
#include "qe_logic.h"
#include "aesm_epid_blob.h"
#include "aesm_long_lived_thread.h"
#include "QEClass.h"
#include "PVEClass.h"
#include "util.h"
#include "pce_service.h"
#include "es_info.h"
#include "endpoint_select_info.h"
#include "cppmicroservices_util.h"
#include "service_enclave_mrsigner.hh"
#include "sgx_ql_quote.h"
#include "se_sig_rl.h"
#include "platform_info_logic.h"

using namespace cppmicroservices;
std::shared_ptr<INetworkService> g_network_service;
std::shared_ptr<IPceService> g_pce_service;


extern ThreadStatus epid_thread;
static uint32_t active_extended_epid_group_id = 0;
static AESMLogicMutex _qe_pve_mutex;


#define CHECK_EPID_PROVISIONG_STATUS \
    if(!query_pve_thread_status()){\
        return AESM_BUSY;\
    }

bool query_pve_thread_status(void)
{
    return epid_thread.query_status_and_reset_clock();
}


uint32_t get_active_extended_epid_group_id_internal()
{
    return active_extended_epid_group_id;
}

static ae_error_t thread_to_load_qe(aesm_thread_arg_type_t arg)
{
    epid_blob_with_cur_psvn_t epid_data;
    ae_error_t ae_ret = AE_FAILURE;
    UNUSED(arg);
    AESM_DBG_TRACE("start to load qe");
    memset(&epid_data, 0, sizeof(epid_data));
    AESMLogicLock lock(_qe_pve_mutex);
    if((ae_ret = EPIDBlob::instance().read(epid_data)) == AE_SUCCESS)
    {
        AESM_DBG_TRACE("EPID blob is read successfully, loading QE ...");
        ae_ret = CQEClass::instance().load_enclave();
        if(AE_SUCCESS != ae_ret)
        {
            AESM_DBG_WARN("fail to load QE: %d", ae_ret);
        }else{
            AESM_DBG_TRACE("QE loaded successfully");
            bool resealed = false;
            sgx_cpu_svn_t cpusvn;
            memset(&cpusvn, 0, sizeof(cpusvn));
            se_static_assert(SGX_TRUSTED_EPID_BLOB_SIZE_SDK>=SGX_TRUSTED_EPID_BLOB_SIZE_SIK);
            // Just take this chance to reseal EPID blob in case TCB is
            // upgraded, return value is ignored and no provisioning is
            // triggered.
            ae_ret = static_cast<ae_error_t>(CQEClass::instance().verify_blob(
                epid_data.trusted_epid_blob,
                SGX_TRUSTED_EPID_BLOB_SIZE_SDK,
                &resealed, &cpusvn));
            if(AE_SUCCESS != ae_ret)
            {
                AESM_DBG_WARN("Failed to verify EPID blob: %d", ae_ret);
                EPIDBlob::instance().remove();
            }else{
                uint32_t epid_xeid;
                // Check whether EPID blob XEGDID is aligned with active extended group id if it exists.
                if ((EPIDBlob::instance().get_extended_epid_group_id(&epid_xeid) == AE_SUCCESS) && (epid_xeid == get_active_extended_epid_group_id_internal())) {
                    AESM_DBG_TRACE("EPID blob Verified");
                    // XEGDID is aligned
                    if (true == resealed)
                    {
                        AESM_DBG_TRACE("EPID blob is resealed");
                        if ((ae_ret = EPIDBlob::instance().write(epid_data))
                            != AE_SUCCESS)
                        {
                            AESM_DBG_WARN("Failed to update epid blob: %d", ae_ret);
                        }
                    }
                }
                else { // XEGDID is NOT aligned
                    AESM_DBG_TRACE("XEGDID mismatch in EPIDBlob, remove it...");
                    EPIDBlob::instance().remove();
                }
            }
        }
    }else{
        AESM_DBG_TRACE("Fail to read EPID Blob");
    }
    AESM_DBG_TRACE("QE Thread finished succ");
    return AE_SUCCESS;
}

static ae_error_t read_global_extended_epid_group_id(uint32_t *xeg_id)
{
    char path_name[MAX_PATH];
    ae_error_t ae_ret = aesm_get_pathname(FT_PERSISTENT_STORAGE, EXTENDED_EPID_GROUP_ID_FID, path_name, MAX_PATH);
    if(AE_SUCCESS != ae_ret){
        return ae_ret;
    }
    FILE * f = fopen(path_name, "r");
    if( f == NULL){
        return OAL_CONFIG_FILE_ERROR;
    }
    ae_ret = OAL_CONFIG_FILE_ERROR;
    // Need to define __STDC_FORMAT_MACROS to use SCNu32 in C++
    if(fscanf(f, "%" SCNu32, xeg_id)==1){
        ae_ret = AE_SUCCESS;
    }
    fclose(f);
    return ae_ret;
}
static ae_error_t set_global_extended_epid_group_id(uint32_t xeg_id)
{
    char path_name[MAX_PATH];
    ae_error_t ae_ret = aesm_get_pathname(FT_PERSISTENT_STORAGE, EXTENDED_EPID_GROUP_ID_FID, path_name, MAX_PATH);
    if(AE_SUCCESS != ae_ret){
        return ae_ret;
    }
    FILE *f = fopen(path_name, "w");
    if(f == NULL){
        return OAL_CONFIG_FILE_ERROR;
    }
    ae_ret = OAL_CONFIG_FILE_ERROR;
    // Need to define __STDC_FORMAT_MACROS to use PRIu32 in C++
    if(fprintf(f, "%" PRIu32, xeg_id)>0){
        ae_ret = AE_SUCCESS;
    }
    fclose(f);
    return ae_ret;
}


class EpidQuoteServiceImp : public IEpidQuoteService, public IQuoteProviderService
{
private:
    bool initialized;
    aesm_thread_t qe_thread;
public:
    EpidQuoteServiceImp():initialized(false), qe_thread(NULL){}


    ae_error_t start()
    {
        ae_error_t ae_ret = AE_SUCCESS;
        AESMLogicLock lock(_qe_pve_mutex);
        if (initialized == true)
        {
            AESM_DBG_INFO("epid bundle has been started");
            return AE_SUCCESS;
        }

        AESM_DBG_INFO("Starting epid bundle");
        auto context = cppmicroservices::GetBundleContext();
        get_service_wrapper(g_network_service, context);
        if (g_network_service == nullptr || g_network_service->start())
        {
            AESM_DBG_ERROR("Starting epid bundle failed because network service is not available");
            return AE_FAILURE;
        }
        get_service_wrapper(g_pce_service, context);
        if (g_pce_service == nullptr || g_pce_service->start())
        {
            AESM_DBG_ERROR("Starting epid bundle failed because pce service is not available");
            return AE_FAILURE;
        }

        ae_ret = read_global_extended_epid_group_id(&active_extended_epid_group_id);
        if (AE_SUCCESS != ae_ret){
            AESM_DBG_INFO("Fail to read extended epid group id, default extended epid group used");
            active_extended_epid_group_id = DEFAULT_EGID; // Use default extended epid group id 0 if it is not available from data file
        }
        else{
            AESM_DBG_INFO("active extended group id %d used", active_extended_epid_group_id);
        }
        if (AE_SUCCESS != (XEGDBlob::verify_xegd_by_xgid(active_extended_epid_group_id)) ||
            AE_SUCCESS != (EndpointSelectionInfo::verify_file_by_xgid(active_extended_epid_group_id))){// Try to load XEGD and URL file to make sure it is valid
            AESM_LOG_WARN_ADMIN("%s", g_admin_event_string_table[SGX_ADMIN_EVENT_PCD_NOT_AVAILABLE]);
            AESM_LOG_WARN("%s: original extended epid group id = %d", g_event_string_table[SGX_EVENT_PCD_NOT_AVAILABLE], active_extended_epid_group_id);
            active_extended_epid_group_id = DEFAULT_EGID;// If the active extended epid group id read from data file is not valid, switch to default extended epid group id
        }

        ae_error_t aesm_ret = aesm_create_thread(thread_to_load_qe, 0,&qe_thread);
        if(AE_SUCCESS != aesm_ret){
            AESM_DBG_WARN("Fail to create thread to preload QE:(ae %d)",aesm_ret);
            return AE_FAILURE;
        }
        initialized = true;
        AESM_DBG_INFO("epid bundle started");
        return AE_SUCCESS;
    }
    void stop()
    {
        ae_error_t ae_ret, thread_ret;

        ae_ret = aesm_wait_thread(qe_thread, &thread_ret, AESM_STOP_TIMEOUT);
        if (ae_ret != AE_SUCCESS || thread_ret != AE_SUCCESS)
        {
            AESM_DBG_INFO("aesm_wait_thread failed(qe_thread):(ae %d) (%d)", ae_ret, thread_ret);
        }
        (void)aesm_free_thread(qe_thread);//release thread handle to free memory

        uint64_t stop_tick_count = se_get_tick_count()+500/1000;
        epid_thread.stop_thread(stop_tick_count);

        CPVEClass::instance().unload_enclave();
        CQEClass::instance().unload_enclave();
        initialized = false;
        AESM_DBG_INFO("epid bundle stopped");
    }

    aesm_error_t init_quote(
        uint8_t *target_info,
        uint32_t target_info_size,
        uint8_t *gid,
        uint32_t gid_size)
    {
        ae_error_t ret = AE_SUCCESS;
        sgx_target_info_t pce_target_info;
        uint16_t pce_isv_svn = 0xFFFF;
        memset(&pce_target_info, 0, sizeof(pce_target_info));
        AESM_DBG_INFO("init_quote");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        if(sizeof(sgx_target_info_t) != target_info_size ||
           sizeof(sgx_epid_group_id_t) != gid_size)
        {
            return AESM_PARAMETER_ERROR;
        }
        AESMLogicLock lock(_qe_pve_mutex);
        CHECK_EPID_PROVISIONG_STATUS;
        if(!g_pce_service)
            return AESM_SERVICE_UNAVAILABLE;
        ret = g_pce_service->load_enclave();
        if(AE_SUCCESS == ret)
            ret = static_cast<ae_error_t>(g_pce_service->pce_get_target(&pce_target_info, &pce_isv_svn));
        if(AE_SUCCESS != ret)
        {
            if(AESM_AE_OUT_OF_EPC == ret)
                return AESM_OUT_OF_EPC;
            else if(AESM_AE_NO_DEVICE == ret)
                return AESM_NO_DEVICE_ERROR;
            else if(AE_SERVER_NOT_AVAILABLE == ret)
                return AESM_SERVICE_UNAVAILABLE;
            return AESM_UNEXPECTED_ERROR;
        }
        return QEAESMLogic::init_quote(
                   reinterpret_cast<sgx_target_info_t *>(target_info),
                   gid, gid_size, pce_isv_svn);
    }

    aesm_error_t get_quote(
        const uint8_t *report, uint32_t report_size,
        uint32_t quote_type,
        const uint8_t *spid, uint32_t spid_size,
        const uint8_t *nonce, uint32_t nonce_size,
        const uint8_t *sigrl, uint32_t sigrl_size,
        uint8_t *qe_report, uint32_t qe_report_size,
        uint8_t *quote, uint32_t buf_size)
    {
        ae_error_t ret = AE_SUCCESS;
        sgx_target_info_t pce_target_info;
        uint16_t pce_isv_svn = 0xFFFF;
        memset(&pce_target_info, 0, sizeof(pce_target_info));
        AESM_DBG_INFO("get_quote");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        if(sizeof(sgx_report_t) != report_size ||
           sizeof(sgx_spid_t) != spid_size)
        {
            return AESM_PARAMETER_ERROR;
        }
        if((nonce && sizeof(sgx_quote_nonce_t) != nonce_size)
            || (qe_report && sizeof(sgx_report_t) != qe_report_size))

        {
            return AESM_PARAMETER_ERROR;
        }
        AESMLogicLock lock(_qe_pve_mutex);
        CHECK_EPID_PROVISIONG_STATUS;
        if(!g_pce_service)
            return AESM_SERVICE_UNAVAILABLE;

        ret = g_pce_service->load_enclave();
        if(AE_SUCCESS == ret)
            ret = static_cast<ae_error_t>(g_pce_service->pce_get_target(&pce_target_info, &pce_isv_svn));
        if(AE_SUCCESS != ret)
        {
            if(AESM_AE_OUT_OF_EPC == ret)
                return AESM_OUT_OF_EPC;
            else if(AESM_AE_NO_DEVICE == ret)
                return AESM_NO_DEVICE_ERROR;
            else if(AE_SERVER_NOT_AVAILABLE == ret)
                return AESM_SERVICE_UNAVAILABLE;
            return AESM_UNEXPECTED_ERROR;
        }
        return QEAESMLogic::get_quote(report, quote_type, spid, nonce, sigrl,
                                      sigrl_size, qe_report, quote, buf_size, pce_isv_svn);
    }

    aesm_error_t get_extended_epid_group_id(
        uint32_t* extended_epid_group_id)
    {
        AESM_DBG_INFO("get_extended_epid_group");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        if (NULL == extended_epid_group_id)
            return AESM_PARAMETER_ERROR;
        *extended_epid_group_id = get_active_extended_epid_group_id_internal();
        return AESM_SUCCESS;
    }

    aesm_error_t switch_extended_epid_group(
        uint32_t extended_epid_group_id)
    {
        AESM_DBG_INFO("switch_extended_epid_group");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        ae_error_t ae_ret;
        if ((ae_ret = XEGDBlob::verify_xegd_by_xgid(extended_epid_group_id)) != AE_SUCCESS ||
            (ae_ret = EndpointSelectionInfo::verify_file_by_xgid(extended_epid_group_id)) != AE_SUCCESS){
            AESM_DBG_INFO("Fail to switch to extended epid group to %d due to XEGD blob for URL blob not available", extended_epid_group_id);
            return AESM_PARAMETER_ERROR;
        }
        ae_ret = set_global_extended_epid_group_id(extended_epid_group_id);
        if (ae_ret != AE_SUCCESS){
            AESM_DBG_INFO("Fail to switch to extended epid group %d", extended_epid_group_id);
            return AESM_UNEXPECTED_ERROR;
        }

        AESM_DBG_INFO("Succ to switch to extended epid group %d in data file, restart aesm required to use it", extended_epid_group_id);
        return AESM_SUCCESS;
    }

    uint32_t endpoint_selection(
        endpoint_selection_infos_t& es_info)
    {
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        AESMLogicLock lock(_qe_pve_mutex);
        SGX_DBGPRINT_ONE_STRING_TWO_INTS_ENDPOINT_SELECTION(__FUNCTION__" (line, 0)", __LINE__, 0);
        return EndpointSelectionInfo::instance().start_protocol(es_info);
    }

    ae_error_t need_epid_provisioning(
        const platform_info_blob_wrapper_t* p_platform_info_blob)
    {
        return PlatformInfoLogic::need_epid_provisioning(p_platform_info_blob);
    }

    aesm_error_t provision(
        bool performance_rekey_used,
        uint32_t timeout_usec)
    {
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        AESMLogicLock lock(_qe_pve_mutex);
        CHECK_EPID_PROVISIONG_STATUS;
        return PvEAESMLogic::provision(performance_rekey_used, timeout_usec);
    }
    const char *get_server_url(
        aesm_network_server_enum_type_t type)
    {
        return EndpointSelectionInfo::instance().get_server_url(type);
    }
    const char *get_pse_provisioning_url(
        const endpoint_selection_infos_t& es_info)
    {
        return NULL;

    }

    aesm_error_t init_quote_ex(
        const uint8_t *att_key_id_ext, uint32_t att_key_id_ext_size,
        uint8_t *target_info, uint32_t target_info_size,
        uint8_t *pub_key_id, size_t *pub_key_id_size)
    {
        AESM_DBG_INFO("init_quote_ex");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        AESMLogicLock lock(_qe_pve_mutex);
        // att_key_id_ext has been checked by caller
        if((NULL != target_info && sizeof(sgx_target_info_t) != target_info_size)
           || (NULL == pub_key_id && NULL == pub_key_id_size)
           || (NULL != pub_key_id && sizeof(sgx_epid_group_id_t) != *pub_key_id_size))
        {
            return AESM_PARAMETER_ERROR;
        }

        if(NULL == pub_key_id)
        {
            *pub_key_id_size = sizeof(sgx_epid_group_id_t);
            return AESM_SUCCESS;
        }

        return init_quote(target_info, target_info_size, pub_key_id, *pub_key_id_size);
    }

    aesm_error_t get_quote_size_ex(
        const uint8_t *att_key_id_ext, uint32_t att_key_id_ext_size,
        uint32_t *p_quote_size)
    {
        AESM_DBG_INFO("get_quote_size_ex");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        // att_key_id_ext has been checked by caller

        sgx_status_t ret = SGX_SUCCESS;
        uint32_t quote_size = 0;
        ret = sgx_calc_quote_size(NULL, 0, &quote_size);
        if (SGX_ERROR_INVALID_PARAMETER == ret)
            return AESM_PARAMETER_ERROR;
        else if (SGX_SUCCESS != ret)
            return AESM_UNEXPECTED_ERROR;
        else
            *p_quote_size = quote_size;

        return AESM_SUCCESS;
    }

    aesm_error_t get_quote_ex(
        const uint8_t *app_report, uint32_t app_report_size,
        const uint8_t *att_key_id_ext, uint32_t att_key_id_ext_size,
        uint8_t *qe_report_info, uint32_t qe_report_info_size,
        uint8_t *quote, uint32_t quote_size)
    {
        AESM_DBG_INFO("get_quote_ex");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        AESMLogicLock lock(_qe_pve_mutex);
        // att_key_id_ext has been checked by caller
        if((NULL == app_report && sizeof(sgx_report_t) != app_report_size)
           || (NULL != qe_report_info && sizeof(sgx_ql_qe_report_info_t) != qe_report_info_size)
           || (NULL == qe_report_info && 0 != qe_report_info_size))
        {
            return AESM_PARAMETER_ERROR;
        }
        sgx_att_key_id_ext_t *p_att_key_id_ext = (sgx_att_key_id_ext_t *)att_key_id_ext;
        sgx_qe_report_info_t *p_qe_report_info = (sgx_qe_report_info_t *)qe_report_info;
        return get_quote(app_report, app_report_size, p_att_key_id_ext->att_key_type,
                    p_att_key_id_ext->spid, sizeof(p_att_key_id_ext->spid),
                    p_qe_report_info ? (uint8_t *)&p_qe_report_info->nonce:NULL, p_qe_report_info ? sizeof(p_qe_report_info->nonce):0,
                    NULL, 0,
                    p_qe_report_info ? (uint8_t *)&p_qe_report_info->qe_report:NULL, p_qe_report_info ? sizeof(p_qe_report_info->qe_report):0,
                    quote, quote_size);
    }

    aesm_error_t get_att_key_id(uint8_t *att_key_id, uint32_t att_key_id_size)
    {
        AESM_DBG_INFO("get_att_key_id");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        // check parameters, and epid bundle support both linkable and unlinkable quote
        if(NULL == att_key_id || sizeof(sgx_att_key_id_ext_t) * 2 > att_key_id_size)
        {
            return AESM_PARAMETER_ERROR;
        }
        // Fill the unlinkable att_key_id;
        sgx_att_key_id_ext_t *p_att_key_id_ext = (sgx_att_key_id_ext_t *)att_key_id;
        sgx_ql_att_key_id_t *p_ql_att_key_id = (sgx_ql_att_key_id_t *)&p_att_key_id_ext->base;
        //Clear the output strcuture, and also make the following fields to be 0s
        //id, version, extended_prod_id, config_id, family_id, spid, att_key_type(SGX_UNLINKABLE_SIGNATURE)
        memset(att_key_id, 0, att_key_id_size);
        p_ql_att_key_id->mrsigner_length = (uint16_t)sizeof(G_SERVICE_ENCLAVE_MRSIGNER[AE_MR_SIGNER]);
        memcpy_s(p_ql_att_key_id->mrsigner, sizeof(p_ql_att_key_id->mrsigner),
                        &G_SERVICE_ENCLAVE_MRSIGNER[AE_MR_SIGNER],
                        sizeof(G_SERVICE_ENCLAVE_MRSIGNER[AE_MR_SIGNER]));
        p_ql_att_key_id->prod_id = 1;
        p_ql_att_key_id->algorithm_id = SGX_QL_ALG_EPID;
        p_att_key_id_ext->att_key_type = SGX_UNLINKABLE_SIGNATURE;

        // Fill the linkable att_key_id;
        p_att_key_id_ext++;
        p_ql_att_key_id = (sgx_ql_att_key_id_t *)&p_att_key_id_ext->base;
        p_ql_att_key_id->mrsigner_length = (uint16_t)sizeof(G_SERVICE_ENCLAVE_MRSIGNER[AE_MR_SIGNER]);
        memcpy_s(p_ql_att_key_id->mrsigner, sizeof(p_ql_att_key_id->mrsigner),
                        &G_SERVICE_ENCLAVE_MRSIGNER[AE_MR_SIGNER],
                        sizeof(G_SERVICE_ENCLAVE_MRSIGNER[AE_MR_SIGNER]));
        p_ql_att_key_id->prod_id = 1;
        p_ql_att_key_id->algorithm_id = SGX_QL_ALG_RESERVED_1;
        p_att_key_id_ext->att_key_type = SGX_LINKABLE_SIGNATURE;

        return AESM_SUCCESS;
    }

    aesm_error_t get_att_key_id_num(uint32_t *att_key_id_num)
    {
        if(NULL == att_key_id_num)
        {
            return AESM_PARAMETER_ERROR;
        }
        *att_key_id_num = 2;
        return AESM_SUCCESS;
    }

    aesm_error_t report_attestation_status(
        uint8_t* platform_info, uint32_t platform_info_size,
        uint32_t attestation_status,
        uint8_t* update_info, uint32_t update_info_size)
    {
        AESM_DBG_INFO("report_attestation_status");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        AESMLogicLock lock(_qe_pve_mutex);
        return  PlatformInfoLogic::report_attestation_status(platform_info,platform_info_size,
            attestation_status,
            update_info, update_info_size);
    }

    aesm_error_t check_update_status(
        uint8_t* platform_info, uint32_t platform_info_size,
        uint8_t* update_info, uint32_t update_info_size,
        uint32_t config, uint32_t* status)
    {
        AESM_DBG_INFO("check_update_status");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        AESMLogicLock lock(_qe_pve_mutex);
        return  PlatformInfoLogic::check_update_status(platform_info,platform_info_size,
            update_info, update_info_size,
            config, status);
    }
};

class Activator : public BundleActivator
{
  void Start(BundleContext ctx)
  {
    auto service = std::make_shared<EpidQuoteServiceImp>();
    ctx.RegisterService<IEpidQuoteService, IQuoteProviderService>(service);
  }

  void Stop(BundleContext)
  {
    // Nothing to do
  }
};

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(Activator)

// [no-cmake]
// The code below is required if the CMake
// helper functions are not used.
#ifdef NO_CMAKE
CPPMICROSERVICES_INITIALIZE_BUNDLE(epid_quote_service_bundle_name)
#endif
