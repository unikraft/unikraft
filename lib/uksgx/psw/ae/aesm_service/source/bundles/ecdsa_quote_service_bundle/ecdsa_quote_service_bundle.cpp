#include <quote_provider_service.h>
#include <pce_service.h>

#include <cppmicroservices/BundleActivator.h>
#include "cppmicroservices/BundleContext.h"
#include <cppmicroservices/GetBundleContext.h>

#include <iostream>
#include <dlfcn.h>
#include "aesm_logic.h"
#include "sgx_quote_3.h"
#include "sgx_ql_quote.h"
#include "sgx_ql_core_wrapper.h"

#include "cppmicroservices_util.h"
#include "sgx_pce.h"

using namespace cppmicroservices;
std::shared_ptr<IPceService> g_pce_service;
static AESMLogicMutex ecdsa_quote_mutex;
extern const sgx_ql_att_key_id_t g_default_ecdsa_p256_att_key_id;
extern "C" void* get_qpl_handle();

typedef quote3_error_t(*sgx_ql_set_logging_callback_t)(sgx_ql_logging_callback_t logger);

void sgx_ql_logging_callback(sgx_ql_log_level_t level, const char* message)
{
    if (level == SGX_QL_LOG_ERROR) {
        sgx_proc_log_report(1, message);
    }
    else if (level == SGX_QL_LOG_INFO) {
        sgx_proc_log_report(3, message);
    }
}

static sgx_pce_error_t ae_error_to_pce_error(uint32_t input)
{
    sgx_pce_error_t ret = SGX_PCE_SUCCESS;
    switch(input)
    {
    case AE_SUCCESS:
        ret = SGX_PCE_SUCCESS;
        break;
    case AE_INVALID_PARAMETER:
    case AE_INSUFFICIENT_DATA_IN_BUFFER:
        ret = SGX_PCE_INVALID_PARAMETER;
        break;
    case PCE_INVALID_REPORT:
        ret = SGX_PCE_INVALID_REPORT;
        break;
    case PCE_CRYPTO_ERROR:
        ret = SGX_PCE_CRYPTO_ERROR;
        break;
    case PCE_INVALID_PRIVILEGE:
        ret = SGX_PCE_INVALID_PRIVILEGE;
        break;
    case AE_OUT_OF_MEMORY_ERROR:
        ret = SGX_PCE_OUT_OF_EPC;
        break;
    default:
        ret = SGX_PCE_UNEXPECTED;
		break;
    }
    return ret;
}

static aesm_error_t quote3_error_to_aesm_error(quote3_error_t input)
{
    aesm_error_t ret = AESM_SUCCESS;
    switch(input)
    {
    case SGX_QL_SUCCESS:
        ret = AESM_SUCCESS;
        break;

    case SGX_QL_ERROR_UNEXPECTED:
        ret = AESM_UNEXPECTED_ERROR;
        break;

    case SGX_QL_ERROR_INVALID_PARAMETER:
        ret = AESM_PARAMETER_ERROR;
        break;

    case SGX_QL_ERROR_OUT_OF_MEMORY:
        ret = AESM_OUT_OF_MEMORY_ERROR;
        break;

    case SGX_QL_ERROR_ECDSA_ID_MISMATCH:
        ret = AESM_ECDSA_ID_MISMATCH;
        break;

    case SGX_QL_PATHNAME_BUFFER_OVERFLOW_ERROR:
        ret = AESM_PATHNAME_BUFFER_OVERFLOW_ERROR;
        break;

    case SGX_QL_FILE_ACCESS_ERROR:
        ret = AESM_FILE_ACCESS_ERROR;
        break;

    case SGX_QL_ERROR_STORED_KEY:
        ret = AESM_ERROR_STORED_KEY;
        break;

    case SGX_QL_ERROR_PUB_KEY_ID_MISMATCH:
        ret = AESM_PUB_KEY_ID_MISMATCH;
        break;

    case SGX_QL_ERROR_INVALID_PCE_SIG_SCHEME:
        ret = AESM_INVALID_PCE_SIG_SCHEME;
        break;

    case SGX_QL_ATT_KEY_BLOB_ERROR:
        ret = AESM_ATT_KEY_BLOB_ERROR;
        break;

    case SGX_QL_UNSUPPORTED_ATT_KEY_ID:
        ret = AESM_UNSUPPORTED_ATT_KEY_ID;
        break;

    case SGX_QL_UNSUPPORTED_LOADING_POLICY:
        ret = AESM_UNSUPPORTED_LOADING_POLICY;
        break;

    case SGX_QL_INTERFACE_UNAVAILABLE:
        ret = AESM_INTERFACE_UNAVAILABLE;
        break;

    case SGX_QL_PLATFORM_LIB_UNAVAILABLE:
        ret = AESM_PLATFORM_LIB_UNAVAILABLE;
        break;

    case SGX_QL_ATT_KEY_NOT_INITIALIZED:
        ret = AESM_ATT_KEY_NOT_INITIALIZED;
        break;

    case SGX_QL_ATT_KEY_CERT_DATA_INVALID:
        ret = AESM_ATT_KEY_CERT_DATA_INVALID;
        break;

    case SGX_QL_NO_PLATFORM_CERT_DATA:
        ret = AESM_NO_PLATFORM_CERT_DATA;
        break;

    case SGX_QL_OUT_OF_EPC:
        ret = AESM_OUT_OF_EPC;
        break;

    case SGX_QL_ERROR_REPORT:
        ret = AESM_ERROR_REPORT;
        break;

    case SGX_QL_ENCLAVE_LOST:
        ret = AESM_ENCLAVE_LOST;
        break;

    case SGX_QL_INVALID_REPORT:
        ret = AESM_INVALID_REPORT;
        break;

    case SGX_QL_ENCLAVE_LOAD_ERROR:
        ret = AESM_ENCLAVE_LOAD_ERROR;
        break;

    case SGX_QL_UNABLE_TO_GENERATE_QE_REPORT:
        ret = AESM_UNABLE_TO_GENERATE_QE_REPORT;
        break;

    case SGX_QL_KEY_CERTIFCATION_ERROR:
        ret = AESM_KEY_CERTIFICATION_ERROR;
        break;

    default:
        ret = AESM_UNEXPECTED_ERROR;
		break;
    }
    return ret;
}

/*NOTE: This function is called by sgx_ql_set_enclave_load_policy(SGX_QL_PERSISTENT)
  below. In AESM, only valid policy is SGX_QL_PERSISTENT which is the default value.
  So we can ignore it. */
extern "C" __attribute__((visibility("default"))) sgx_pce_error_t sgx_set_pce_enclave_load_policy(
    sgx_ql_request_policy_t policy)
{
    return SGX_PCE_SUCCESS;
}

extern "C" __attribute__((visibility("default"))) sgx_pce_error_t sgx_pce_get_target(sgx_target_info_t *p_target,
    sgx_isv_svn_t *p_isvsvn)
{
    return ae_error_to_pce_error(g_pce_service->pce_get_target(p_target, p_isvsvn));
}

extern "C" __attribute__((visibility("default"))) sgx_pce_error_t sgx_get_pce_info(const sgx_report_t *p_report,
    const uint8_t *p_pek,
    uint32_t pek_size,
    uint8_t crypto_suite,
    uint8_t *p_encrypted_ppid,
    uint32_t encrypted_ppid_size,
    uint32_t *p_encrypted_ppid_out_size,
    sgx_isv_svn_t* p_pce_isvsvn,
    uint16_t* p_pce_id,
    uint8_t *p_signature_scheme)
{
    return ae_error_to_pce_error(g_pce_service->get_pce_info(p_report,
                p_pek, pek_size, crypto_suite, p_encrypted_ppid,
                encrypted_ppid_size, p_encrypted_ppid_out_size,
                p_pce_isvsvn, p_pce_id, p_signature_scheme));
}

extern "C" __attribute__((visibility("default"))) sgx_pce_error_t sgx_pce_sign_report(const sgx_isv_svn_t *p_isv_svn,
    const sgx_cpu_svn_t *p_cpu_svn,
    const sgx_report_t *p_report,
    uint8_t *p_sig,
    uint32_t sig_size,
    uint32_t *p_sig_out_size)
{
    return ae_error_to_pce_error(g_pce_service->pce_sign_report(p_isv_svn,
                p_cpu_svn, p_report, p_sig, sig_size, p_sig_out_size));
}
extern "C" quote3_error_t load_qe(sgx_enclave_id_t *p_qe_eid,
                                  sgx_misc_attribute_t *p_qe_attributes,
                                  sgx_launch_token_t *p_launch_token);

class EcdsaQuoteServiceImp : public IQuoteProviderService
{
private:

    bool initialized;
public:
    EcdsaQuoteServiceImp():initialized(false) {}

    ae_error_t start()
    {
        AESMLogicLock lock(ecdsa_quote_mutex);
        if (initialized == true)
        {
            AESM_DBG_INFO("ecdsa bundle has been started");
            return AE_SUCCESS;
        }

        AESM_DBG_INFO("Starting ecdsa bundle");
        auto context = cppmicroservices::GetBundleContext();
        get_service_wrapper(g_pce_service, context);

        if (!g_pce_service)
        {
            AESM_DBG_ERROR("Starting ecdsa bundle failed because pce service is not available");
            return AE_FAILURE;
        }

        if (g_pce_service->start() != AE_SUCCESS)
        {
            AESM_DBG_ERROR("Starting ecdsa bundle failed because pce service failed to start");
            return AE_FAILURE;
        }

        if (SGX_QL_SUCCESS != sgx_ql_set_enclave_load_policy(SGX_QL_PERSISTENT))
        {
            AESM_DBG_ERROR("Starting ecdsa bundle failed because pce service failed to start");
            return AE_FAILURE;
        }

        quote3_error_t ret = SGX_QL_SUCCESS;
        sgx_enclave_id_t qe_eid = 0;
        sgx_misc_attribute_t qe_attributes ={ 0 };
        sgx_launch_token_t launch_token = { 0 };

        ret = load_qe(&qe_eid, &qe_attributes, &launch_token);
        if (SGX_QL_SUCCESS != ret)
        {
            AESM_LOG_ERROR("Failed to load QE3: 0x%x", ret);
            AESM_DBG_ERROR("Starting ecdsa bundle failed because QE3 failed to load");
            return AE_FAILURE;
        }
                    
        // Set logging callback for default quote provider library
        void* handle = get_qpl_handle();
        if (handle != NULL) {
            char *error;
            sgx_ql_set_logging_callback_t ql_set_logging_callback = (sgx_ql_set_logging_callback_t)dlsym(handle, "sgx_ql_set_logging_callback");
            if ((error = dlerror()) == NULL && ql_set_logging_callback != NULL)  {
                // Set logging function detected
                ql_set_logging_callback(sgx_ql_logging_callback);
            }
            else {
                AESM_LOG_ERROR("Failed to set logging callback for the quote provider library.");
            }
        }

        initialized = true;
        AESM_DBG_INFO("ecdsa bundle started");
        return AE_SUCCESS;
    }
    void stop()
    {
        sgx_ql_set_enclave_load_policy(SGX_QL_EPHEMERAL);
        initialized = false;
        AESM_DBG_INFO("ecdsa bundle stopped");
    }

    aesm_error_t init_quote_ex(
        const uint8_t *att_key_id_ext, uint32_t att_key_id_ext_size,
        uint8_t *target_info, uint32_t target_info_size,
        uint8_t *pub_key_id, size_t *pub_key_id_size)
    {
        AESM_DBG_INFO("init_quote_ex");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        // att_key_id_ext has been checked by caller
        if((NULL != target_info && sizeof(sgx_target_info_t) != target_info_size)
           || (NULL == pub_key_id && NULL == pub_key_id_size))
        {
            return AESM_PARAMETER_ERROR;
        }
        AESMLogicLock lock(ecdsa_quote_mutex);
        return quote3_error_to_aesm_error(sgx_ql_init_quote(
                //TODO: need to update sgx_ql_get_quote_size to add "const" for att_key_id
                &((sgx_att_key_id_ext_t *)att_key_id_ext)->base,
                reinterpret_cast<sgx_target_info_t *>(target_info),
                false,
                pub_key_id_size,
                pub_key_id));
    }

    aesm_error_t get_quote_size_ex(
        const uint8_t *att_key_id_ext, uint32_t att_key_id_ext_size,
        uint32_t *quote_size)
    {
        AESM_DBG_INFO("get_quote_size_ex");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        // att_key_id_ext has been checked by caller
        AESMLogicLock lock(ecdsa_quote_mutex);
        return quote3_error_to_aesm_error(sgx_ql_get_quote_size(
                //TODO: need to update sgx_ql_get_quote_size to add "const" for att_key_id
                &((sgx_att_key_id_ext_t *)att_key_id_ext)->base,
                quote_size));
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
        // att_key_id_ext has been checked by caller
        if((NULL != app_report && sizeof(sgx_report_t) != app_report_size)
           || (NULL != qe_report_info && sizeof(sgx_ql_qe_report_info_t) != qe_report_info_size)
           || (NULL == qe_report_info && qe_report_info_size))
        {
            return AESM_PARAMETER_ERROR;
        }
        AESMLogicLock lock(ecdsa_quote_mutex);
        return quote3_error_to_aesm_error(sgx_ql_get_quote(
                    reinterpret_cast<const sgx_report_t *>(app_report),
                    //TODO: need to update sgx_ql_get_quote_size to add "const" for att_key_id
                    &((sgx_att_key_id_ext_t *)att_key_id_ext)->base,
                    reinterpret_cast<sgx_ql_qe_report_info_t *>(qe_report_info),
                    quote, quote_size));
    }

    aesm_error_t get_att_key_id(uint8_t *att_key_id, uint32_t att_key_id_size)
    {
        AESM_DBG_INFO("get_att_key_id");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        if(NULL == att_key_id || sizeof(sgx_att_key_id_ext_t) > att_key_id_size)
        {
            return AESM_PARAMETER_ERROR;
        }
        sgx_ql_get_keyid((sgx_att_key_id_ext_t *)att_key_id);
        // sgx_ql_get_keyid should always return success
        return AESM_SUCCESS;
    }

    aesm_error_t get_att_key_id_num(uint32_t *att_key_id_num)
    {
        if(NULL == att_key_id_num)
        {
            return AESM_PARAMETER_ERROR;
        }
        *att_key_id_num = 1;
        return AESM_SUCCESS;
    }
};

class Activator : public BundleActivator
{
  void Start(BundleContext ctx)
  {
    auto service = std::make_shared<EcdsaQuoteServiceImp>();
    ctx.RegisterService<IQuoteProviderService>(service);
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
CPPMICROSERVICES_INITIALIZE_BUNDLE(ecdsa_quote_service_bundle_name)
#endif
