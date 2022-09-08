#include <launch_service.h>

#include "uae_service_internal.h"
#include "aesm_logic.h"
#include "service_enclave_mrsigner.hh"
#include "LEClass.h"
#include "aesm_long_lived_thread.h"

#include <cppmicroservices/BundleActivator.h>
#include "cppmicroservices/BundleContext.h"
#include <cppmicroservices/GetBundleContext.h>
#include "cppmicroservices_util.h"
using namespace cppmicroservices;

AESMLogicMutex _le_mutex;
std::shared_ptr<INetworkService> g_network_service;
std::shared_ptr<ILaunchService>  g_launch_service;


extern "C" bool is_launch_token_required();
extern ae_error_t start_white_list_thread(unsigned long timeout=THREAD_TIMEOUT);
extern ThreadStatus white_list_thread;

extern "C" void init_get_launch_token(const func_get_launch_token_t func);

extern "C" sgx_status_t get_launch_token(const enclave_css_t *signature,
                                         const sgx_attributes_t *attribute,
                                         sgx_launch_token_t *launch_token)
{
    return g_launch_service->get_launch_token(signature, attribute, launch_token);
}


class LeLaunchServiceImp : public ILaunchService
{
private:
    bool _is_qe_psvn_set;
    bool _is_pse_psvn_set;
    bool _is_pce_psvn_set;
    psvn_t _qe_psvn;
    psvn_t _pce_psvn;
    psvn_t _pse_psvn;
    bool initialized;

    ae_error_t save_unverified_white_list(const uint8_t *white_list_cert, uint32_t white_list_cert_size)
    {
        wl_cert_chain_t old_cert;
        const wl_cert_chain_t *p_new_cert = reinterpret_cast<const wl_cert_chain_t *>(white_list_cert);
        uint32_t old_cert_size = sizeof(old_cert);
        memset(&old_cert, 0, sizeof(old_cert));
        if((aesm_read_data(FT_PERSISTENT_STORAGE, AESM_WHITE_LIST_CERT_TO_BE_VERIFY_FID, reinterpret_cast<uint8_t *>(&old_cert), &old_cert_size) == AE_SUCCESS)
            && (old_cert_size == sizeof(old_cert)) && (white_list_cert_size >= sizeof(wl_cert_chain_t)))
        {
            if(_ntohl(p_new_cert->wl_cert.wl_version) <= _ntohl(old_cert.wl_cert.wl_version))
            {
                AESM_DBG_WARN("White list version downgraded! current version is %d, new version is %d",
                            _ntohl(old_cert.wl_cert.wl_version), _ntohl(p_new_cert->wl_cert.wl_version));
                return OAL_PARAMETER_ERROR;  // OAL_PARAMETER_ERROR used here is to indicate the white list is incorrect
            }
        }
        return aesm_write_data(FT_PERSISTENT_STORAGE, AESM_WHITE_LIST_CERT_TO_BE_VERIFY_FID, white_list_cert, white_list_cert_size);
    }
    ae_error_t set_psvn(uint16_t prod_id, uint16_t isv_svn, sgx_cpu_svn_t cpu_svn, uint32_t mrsigner_index)
    {
        if(prod_id == QE_PROD_ID){
            if(mrsigner_index == AE_MR_SIGNER){
                if(_is_qe_psvn_set){
                    if(0!=memcmp(&_qe_psvn.isv_svn, &isv_svn, sizeof(isv_svn))||
                        0!=memcmp(&_qe_psvn.cpu_svn, &cpu_svn, sizeof(sgx_cpu_svn_t))){
                            AESM_DBG_ERROR("PSVN unmatched for QE/PVE");
                            return AE_PSVN_UNMATCHED_ERROR;
                    }
                }else{
                    if(0!=memcpy_s(&_qe_psvn.isv_svn, sizeof(_qe_psvn.isv_svn), &isv_svn, sizeof(isv_svn))||
                        0!=memcpy_s(&_qe_psvn.cpu_svn, sizeof(_qe_psvn.cpu_svn), &cpu_svn, sizeof(sgx_cpu_svn_t))){
                            AESM_DBG_ERROR("memcpy failed");
                            return AE_FAILURE;
                    }
                    AESM_DBG_TRACE("get QE or PvE isv_svn=%d",(int)isv_svn);
                    _is_qe_psvn_set = true;
                    return AE_SUCCESS;
                }
            }else if(mrsigner_index==PCE_MR_SIGNER){
                if(_is_pce_psvn_set){
                    if(0!=memcmp(&_pce_psvn.isv_svn, &isv_svn, sizeof(isv_svn))||
                        0!=memcmp(&_pce_psvn.cpu_svn, &cpu_svn, sizeof(sgx_cpu_svn_t))){
                            AESM_DBG_ERROR("PSVN unmatched for PCE");
                            return AE_PSVN_UNMATCHED_ERROR;
                    }
                }else{
                    if(0!=memcpy_s(&_pce_psvn.isv_svn, sizeof(_pce_psvn.isv_svn), &isv_svn, sizeof(isv_svn))||
                        0!=memcpy_s(&_pce_psvn.cpu_svn, sizeof(_pce_psvn.cpu_svn), &cpu_svn, sizeof(sgx_cpu_svn_t))){
                            AESM_DBG_ERROR("memcpy failed");
                            return AE_FAILURE;
                    }
                    AESM_DBG_TRACE("get PCE isv_svn=%d", (int)isv_svn);
                    _is_pce_psvn_set = true;
                    return AE_SUCCESS;
                }
            }
        }else if(prod_id == PSE_PROD_ID){
            if(mrsigner_index == AE_MR_SIGNER){
                if(_is_pse_psvn_set){
                    if(0!=memcmp(&_pse_psvn.isv_svn, &isv_svn, sizeof(isv_svn))||
                        0!=memcmp(&_pse_psvn.cpu_svn, &cpu_svn, sizeof(sgx_cpu_svn_t))){
                            AESM_DBG_ERROR("PSVN unmatched for PSE");
                            return AE_PSVN_UNMATCHED_ERROR;
                    }
                }else{
                    if(0!=memcpy_s(&_pse_psvn.isv_svn, sizeof(_pse_psvn.isv_svn), &isv_svn, sizeof(isv_svn))||
                        0!=memcpy_s(&_pse_psvn.cpu_svn, sizeof(_pse_psvn.cpu_svn), &cpu_svn, sizeof(sgx_cpu_svn_t))){
                            AESM_DBG_ERROR("memcpy failed");
                            return AE_FAILURE;
                    }
                    AESM_DBG_TRACE("get PSE isv_svn=%d", (int)isv_svn);
                    _is_pse_psvn_set = true;
                    return AE_SUCCESS;
                }
            }
        }
        return AE_SUCCESS;
    }

    ae_error_t get_white_list_size_without_lock(uint32_t *white_list_cert_size)
    {
        uint32_t white_cert_size = 0;
        ae_error_t ae_ret = aesm_query_data_size(FT_PERSISTENT_STORAGE, AESM_WHITE_LIST_CERT_FID, &white_cert_size);
        if (AE_SUCCESS == ae_ret)
        {
            if (white_cert_size != 0)
            { //file existing and not 0 size
                *white_list_cert_size = white_cert_size;
                return AE_SUCCESS;
            }
            else
                return AE_FAILURE;
        }
        else
        {
            return ae_ret;
        }
    }

  public:
    LeLaunchServiceImp()
        : _is_qe_psvn_set(false),
        _is_pse_psvn_set(false),
        _is_pce_psvn_set(false),
        _qe_psvn({0}),
        _pce_psvn({0}),
        _pse_psvn({0}),
        initialized(false)
    {
    }

    ae_error_t start()
    {
        AESMLogicLock lock(_le_mutex);
        ae_error_t ae_ret = AE_SUCCESS;
        if (initialized == true)
        {
            AESM_DBG_INFO("le bundle has been started");
            return AE_SUCCESS;
        }

        AESM_DBG_INFO("Starting le bundle");
        ae_ret = CLEClass::instance().load_enclave();
        if(AE_SUCCESS != ae_ret)
        {
            AESM_DBG_INFO("fail to load LE: %d", ae_ret);
            AESM_LOG_FATAL("%s", g_event_string_table[SGX_EVENT_SERVICE_UNAVAILABLE]);
            return ae_ret;
        }
        auto context = cppmicroservices::GetBundleContext();
        get_service_wrapper(g_network_service, context);
        get_service_wrapper(g_launch_service, context);
        init_get_launch_token(::get_launch_token);
        start_white_list_thread(0);
        initialized = true;
        AESM_DBG_INFO("le bundle started");
        return AE_SUCCESS;
    }
    void stop()
    {
        uint64_t stop_tick_count = se_get_tick_count()+500/1000;
        white_list_thread.stop_thread(stop_tick_count);
        CLEClass::instance().unload_enclave();
        initialized = false;
        AESM_DBG_INFO("le bundle stopped");
    }

    aesm_error_t get_launch_token(
        const uint8_t *mrenclave, uint32_t mrenclave_size,
        const uint8_t *public_key, uint32_t public_key_size,
        const uint8_t *se_attributes, uint32_t se_attributes_size,
        uint8_t *lictoken, uint32_t lictoken_size)
    {
        AESM_DBG_INFO("enter function");
        if (initialized == false)
            return AESM_SERVICE_UNAVAILABLE;
        AESMLogicLock lock(_le_mutex);

        ae_error_t ret_le = AE_SUCCESS;
        if (NULL == mrenclave ||
            NULL == public_key ||
            NULL == se_attributes ||
            NULL == lictoken)
        {
            // Sizes are checked in get_launch_token_internal()
            AESM_DBG_TRACE("Invalid parameter");
            return AESM_PARAMETER_ERROR;
        }
        if (!is_launch_token_required())
        {
            //Should not be called
            AESM_LOG_ERROR("InKernel LE loaded");
            return AESM_SERVICE_UNAVAILABLE;
        }
        ae_error_t ae_ret = CLEClass::instance().load_enclave();
        if(ae_ret == AE_SERVER_NOT_AVAILABLE)
        {
            AESM_LOG_ERROR("%s", g_event_string_table[SGX_EVENT_SERVICE_UNAVAILABLE]);
            AESM_DBG_FATAL("LE not loaded due to AE_SERVER_NOT_AVAILABLE, possible SGX Env Not Ready");
            return AESM_NO_DEVICE_ERROR;
        }
        else if(ae_ret == AESM_AE_OUT_OF_EPC)
        {
            AESM_DBG_ERROR("LE not loaded due to out of EPC");
            return AESM_OUT_OF_EPC;
        }
        else if(AE_FAILED(ae_ret))
        {
            AESM_DBG_ERROR("LE not loaded:%d", ae_ret);
            return AESM_SERVICE_UNAVAILABLE;
        }
        ret_le = static_cast<ae_error_t>(CLEClass::instance().get_launch_token_internal(
            const_cast<uint8_t *>(mrenclave), mrenclave_size,
            const_cast<uint8_t *>(public_key), public_key_size,
            const_cast<uint8_t *>(se_attributes), se_attributes_size,
            lictoken, lictoken_size));

        switch (ret_le)
        {
        case AE_SUCCESS:
            return AESM_SUCCESS;
        case LE_INVALID_PARAMETER:
            AESM_DBG_TRACE("Invalid parameter");
            return AESM_PARAMETER_ERROR;
        case LE_INVALID_ATTRIBUTE:
        case LE_INVALID_PRIVILEGE_ERROR:
            AESM_DBG_TRACE("Launch token error");
            return AESM_GET_LICENSETOKEN_ERROR;
        case LE_WHITELIST_UNINITIALIZED_ERROR:
            AESM_DBG_TRACE("LE whitelist uninitialized error");
            return AESM_UNEXPECTED_ERROR;
        default:
            AESM_DBG_WARN("unexpeted error (ae %d)", ret_le);
            return AESM_UNEXPECTED_ERROR;
        }
    }

    sgx_status_t get_launch_token(const enclave_css_t* signature,
        const sgx_attributes_t* attribute,
        sgx_launch_token_t* launch_token)
    {
        AESM_DBG_INFO("enter function");
        if (initialized == false)
            return SGX_ERROR_SERVICE_UNAVAILABLE;
        AESMLogicLock lock(_le_mutex);

        ae_error_t ret_le = AE_SUCCESS;
        uint32_t mrsigner_index = UINT32_MAX;

        if (!is_launch_token_required())
        {
            //Should not be called
            AESM_LOG_ERROR("InKernel LE loaded");
            return SGX_ERROR_SERVICE_UNAVAILABLE;
        }
        // load LE to get launch token
        if((ret_le=CLEClass::instance().load_enclave()) != AE_SUCCESS)
        {
            if(ret_le == AESM_AE_NO_DEVICE)
            {
                AESM_LOG_FATAL("LE not loaded due to no SGX device available, possible SGX Env Not Ready");
                return SGX_ERROR_NO_DEVICE;
            }
            else if(ret_le == AESM_AE_OUT_OF_EPC)
            {
                AESM_LOG_FATAL("LE not loaded due to out of EPC");
                return SGX_ERROR_OUT_OF_EPC;
            }
            else
            {
                AESM_LOG_FATAL("fail to load LE:%d",ret_le);
                return SGX_ERROR_SERVICE_UNAVAILABLE;
            }
        } 
        ret_le = static_cast<ae_error_t>(CLEClass::instance().get_launch_token_internal(
            const_cast<uint8_t*>(reinterpret_cast<const uint8_t *>(&signature->body.enclave_hash)),
            sizeof(sgx_measurement_t),
            const_cast<uint8_t*>(reinterpret_cast<const uint8_t *>(&signature->key.modulus)),
            sizeof(signature->key.modulus),
            const_cast<uint8_t*>(reinterpret_cast<const uint8_t *>(attribute)),
            sizeof(sgx_attributes_t),
            reinterpret_cast<uint8_t*>(launch_token),
            sizeof(token_t),
            &mrsigner_index));
        switch (ret_le)
        {
        case AE_SUCCESS:
            break;
        case LE_INVALID_PARAMETER:
            AESM_DBG_TRACE("Invalid parameter");
            return SGX_ERROR_INVALID_PARAMETER;
        case LE_INVALID_ATTRIBUTE:
        case LE_INVALID_PRIVILEGE_ERROR:
            AESM_DBG_TRACE("Launch token error");
            return SGX_ERROR_SERVICE_INVALID_PRIVILEGE;
        case LE_WHITELIST_UNINITIALIZED_ERROR:
            AESM_DBG_TRACE("LE whitelist uninitialized error");
            return SGX_ERROR_UNEXPECTED;
        default:
            AESM_DBG_WARN("unexpected error (ae%d)", ret_le);
            return SGX_ERROR_UNEXPECTED;
        }

        token_t *lt = reinterpret_cast<token_t *>(launch_token);
        ret_le = set_psvn(signature->body.isv_prod_id, signature->body.isv_svn, lt->cpu_svn_le, mrsigner_index);
        if(AE_PSVN_UNMATCHED_ERROR == ret_le)
        {
            //QE or PSE has been changed, but AESM doesn't restart. Will not provide service.
            return SGX_ERROR_SERVICE_UNAVAILABLE;
        }else if(AE_SUCCESS != ret_le) {
            AESM_DBG_ERROR("fail to save psvn:(ae%d)", ret_le);
            return SGX_ERROR_UNEXPECTED;
        }

        return SGX_SUCCESS;
    }

    aesm_error_t get_white_list(
        uint8_t *white_list_cert, uint32_t buf_size)
    {
        uint32_t white_cert_size = 0;
        if (initialized == false)
            return AESM_SERVICE_UNAVAILABLE;
        if (NULL == white_list_cert)
            return AESM_PARAMETER_ERROR;
        AESMLogicLock lock(_le_mutex);
        if (!is_launch_token_required())
        {
            AESM_LOG_INFO("InKernel LE loaded");
            return AESM_SERVICE_UNAVAILABLE;
        }
        ae_error_t ae_ret = get_white_list_size_without_lock(&white_cert_size);
        if (AE_SUCCESS != ae_ret)
            return AESM_UNEXPECTED_ERROR;
        if (white_cert_size != buf_size)
        {
            return AESM_PARAMETER_ERROR;
        }

        ae_ret = aesm_read_data(FT_PERSISTENT_STORAGE, AESM_WHITE_LIST_CERT_FID, white_list_cert, &white_cert_size);
        if (AE_SUCCESS != ae_ret)
        {
            AESM_DBG_WARN("Fail to read white cert list file");
            return AESM_UNEXPECTED_ERROR;
        }
        return AESM_SUCCESS;
    }

    aesm_error_t get_white_list_size(
        uint32_t *white_list_cert_size)
    {
        if (initialized == false)
            return AESM_SERVICE_UNAVAILABLE;
        if (NULL == white_list_cert_size)
            return AESM_PARAMETER_ERROR;
        AESMLogicLock lock(_le_mutex);
                if (!is_launch_token_required())
        {
            AESM_LOG_INFO("InKernel LE loaded");
            return AESM_SERVICE_UNAVAILABLE;
        }
        ae_error_t ae_ret = get_white_list_size_without_lock(white_list_cert_size);
        if (AE_SUCCESS == ae_ret)
            return AESM_SUCCESS;
        else
            return AESM_UNEXPECTED_ERROR;
        }

        aesm_error_t white_list_register(
            const uint8_t *white_list_cert,
            uint32_t white_list_cert_size)
        {
            AESM_DBG_INFO("enter function");
            if (initialized == false)
                return AESM_SERVICE_UNAVAILABLE;
            AESM_LOG_INFO_ADMIN("%s", g_admin_event_string_table[SGX_ADMIN_EVENT_WL_UPDATE_START]);
            AESMLogicLock lock(_le_mutex);
            ae_error_t ret_le = AE_SUCCESS;
            aesm_error_t ret = AESM_UNEXPECTED_ERROR;
            if (NULL == white_list_cert || 0 == white_list_cert_size)
            {
                AESM_DBG_TRACE("Invalid parameter");
                AESM_LOG_ERROR_ADMIN("%s", g_admin_event_string_table[SGX_ADMIN_EVENT_WL_UPDATE_FAIL]);
                return AESM_PARAMETER_ERROR;
            }
            if (!is_launch_token_required())
            {
                AESM_DBG_INFO("InKernel LE loaded");
                return AESM_SERVICE_UNAVAILABLE;
            }

            ae_error_t ae_ret = CLEClass::instance().load_enclave();
            if (AE_FAILED(ae_ret))
            {
                switch (ae_ret)
                {
                case AE_SERVER_NOT_AVAILABLE:
                    AESM_DBG_WARN("LE not loaded due to AE_SERVER_NOT_AVAILABLE, possible SGX Env Not Ready");
                    ret_le = save_unverified_white_list(white_list_cert, white_list_cert_size);
                    break;
                case AESM_AE_OUT_OF_EPC:
                    AESM_DBG_WARN("LE not loaded due to out of EPC");
                    ret = AESM_OUT_OF_EPC;
                    goto exit;
                default:
                    AESM_DBG_ERROR("LE not loaded:(ae%d)", ae_ret);
                    ret = AESM_UNEXPECTED_ERROR;
                    goto exit;
                }
            }
            else
            {
                ret_le = static_cast<ae_error_t>(CLEClass::instance().white_list_register(
                    white_list_cert, white_list_cert_size, true));
            }

            switch (ret_le)
            {
            case AE_SUCCESS:
                ret = AESM_SUCCESS;
                break;
            case LE_INVALID_PARAMETER:
                AESM_DBG_TRACE("Invalid parameter");
                ret = AESM_PARAMETER_ERROR;
                break;
            default:
                AESM_DBG_WARN("unexpeted error (ae %d)", ret_le);
                ret = AESM_UNEXPECTED_ERROR;
                break;
            }

        exit:
            // Always log success or failure to the Admin log before returning
            if (AE_FAILED(ae_ret) || AE_FAILED(ret_le))
            {
                AESM_LOG_ERROR_ADMIN("%s", g_admin_event_string_table[SGX_ADMIN_EVENT_WL_UPDATE_FAIL]);
        } else {
                const wl_cert_chain_t* wl = reinterpret_cast<const wl_cert_chain_t*>(white_list_cert);
                AESM_LOG_INFO_ADMIN("%s for Version: %d", g_admin_event_string_table[SGX_ADMIN_EVENT_WL_UPDATE_SUCCESS],
                    _ntohl(wl->wl_cert.wl_version));
        }

        return ret;
    }
};

class Activator : public BundleActivator
{
  void Start(BundleContext ctx)
  {
    auto service = std::make_shared<LeLaunchServiceImp>();
    ctx.RegisterService<ILaunchService>(service);
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
CPPMICROSERVICES_INITIALIZE_BUNDLE(le_launch_service_bundle_name)
#endif
