#include <select_att_key_id.h>
#include <quote_provider_service.h>
#include <quote_proxy_service.h>
#include <aesm_quoting_type.h>

#include <cppmicroservices/Bundle.h>
#include <cppmicroservices/BundleActivator.h>
#include "cppmicroservices/BundleContext.h"
#include <cppmicroservices/GetBundleContext.h>
#include <cppmicroservices/Constants.h>
#include <cppmicroservices/ServiceEvent.h>

#include <iostream>
#include "aesm_logic.h"
#include "sgx_ql_quote.h"
#include "aesm_config.h"
#define SGX_MAX_ATT_KEY_IDS 10
#define BUNLE_ATT_KEY_NUM_MAX   2

using namespace cppmicroservices;

using quote_provider_t = std::shared_ptr<IQuoteProviderService>;
typedef struct _available_key_id_t
{
    sgx_att_key_id_ext_t key_id;
    quote_provider_t service;
} available_key_id_t;


class QuoteExServiceImp : public IQuoteProxyService
{
private:
    bool initialized;
    uint32_t default_quoting_type;
    std::vector<available_key_id_t> available_key_ids;
    std::vector<quote_provider_t> available_providers;
    ListenerToken listenerToken;
    AESMLogicMutex quote_ex_mutex;

public:
    QuoteExServiceImp():initialized(false), default_quoting_type(AESM_QUOTING_DEFAULT_VALUE) {}

    ae_error_t start()
    {
        AESMLogicLock lock(quote_ex_mutex);
        aesm_config_infos_t info = {0};
        if (initialized == true)
        {
            AESM_DBG_INFO("quote_ex bundle has been started");
            return AE_SUCCESS;
        }

        AESM_DBG_INFO("Starting quote_ex bundle");

        auto context = cppmicroservices::GetBundleContext();
        if (!context)
            return AE_FAILURE;
        auto refs = context.GetServiceReferences<IQuoteProviderService>();
        for (auto sr : refs)
        {
            auto bundle = sr.GetBundle();
            if (!bundle)
                continue;
            if (IQuoteProviderService::VERSION != bundle.GetVersion().GetMajor())
                continue;

            auto service = context.GetService(sr);
            if (service
                && (AE_SUCCESS == service->start()))
            {
                uint32_t num = 0;
                sgx_att_key_id_ext_t att_key_id_ext_list[BUNLE_ATT_KEY_NUM_MAX] ={0};

                available_providers.push_back(service);
                if (AESM_SUCCESS != service->get_att_key_id_num(&num))
                    continue;
                if (num > BUNLE_ATT_KEY_NUM_MAX)
                    continue;
                if (AESM_SUCCESS != service->get_att_key_id((uint8_t *)att_key_id_ext_list, sizeof(att_key_id_ext_list)))
                    continue;
                for (int i = 0; i <num; i++)
                {
                    available_key_id_t temp = {0};
                    memcpy_s(&temp.key_id, sizeof(temp.key_id), &att_key_id_ext_list[i], sizeof(att_key_id_ext_list[i]));
                    temp.service = service;
                    available_key_ids.push_back(temp);
                    AESM_DBG_INFO("quote type %d available", temp.key_id.base.algorithm_id);
                }
            }
        }

        if(true == read_aesm_config(info))
        {
            default_quoting_type = info.quoting_type;
        }
        else
            default_quoting_type = AESM_QUOTING_DEFAULT_VALUE;

        AESM_DBG_INFO("default quoting type is %d", default_quoting_type);

        // Listen for events pertaining to IQuoteProviderService.
        listenerToken = context.AddServiceListener(std::bind(&QuoteExServiceImp::ServiceChanged, this, std::placeholders::_1),
            std::string("(") + Constants::OBJECTCLASS + "=" + us_service_interface_iid<IQuoteProviderService>()+ ")" );

        initialized = true;
        AESM_DBG_INFO("quote_ex bundle started");
        return AE_SUCCESS;
    }

    void ServiceChanged(const ServiceEvent& event)
    {
        AESMLogicLock lock(quote_ex_mutex);
        if (false == initialized)
            return;
        auto context = cppmicroservices::GetBundleContext();
        if (!context)
            return;
        std::string objectClass = ref_any_cast<std::vector<std::string>>(event.GetServiceReference().GetProperty(Constants::OBJECTCLASS)).front();

        if (event.GetType() == ServiceEvent::SERVICE_REGISTERED)
        {
            AESM_DBG_INFO(" Service of type  %s is  registered.", objectClass.c_str());
            ServiceReference<IQuoteProviderService> sr = event.GetServiceReference();
            auto bundle = sr.GetBundle();
            if (!bundle)
                return;
            if (IQuoteProviderService::VERSION != bundle.GetVersion().GetMajor())
                return;
            auto service = context.GetService(sr);
            if (service && (AE_SUCCESS == service->start()))
            {
                uint32_t num = 0;
                sgx_att_key_id_ext_t att_key_id_ext_list[BUNLE_ATT_KEY_NUM_MAX] = { 0 };

                available_providers.push_back(service);
                if (AESM_SUCCESS != service->get_att_key_id_num(&num))
                    return;
                if (num > BUNLE_ATT_KEY_NUM_MAX)
                    return;
                if (AESM_SUCCESS != service->get_att_key_id((uint8_t *)att_key_id_ext_list, sizeof(att_key_id_ext_list)))
                    return;
                for (int i = 0; i < num; i++)
                {
                    available_key_id_t temp = { 0 };
                    memcpy_s(&temp.key_id, sizeof(temp.key_id), &att_key_id_ext_list[i], sizeof(att_key_id_ext_list[i]));
                    temp.service = service;
                    available_key_ids.push_back(temp);
                    AESM_DBG_INFO("quote type %d available", temp.key_id.base.algorithm_id);
                }
            }
        }
        else if (event.GetType() == ServiceEvent::SERVICE_UNREGISTERING) {
            AESM_DBG_INFO(" Service of type  %s is  unregistered.", objectClass.c_str());
            //aesm_service is stopping. Nothing to do.
        }
    }

    void stop()
    {
        for (auto it : available_providers)
        {
            it->stop();
        }
        AESM_DBG_INFO("quote_ex bundle stopped");
        initialized = false;
    }

    aesm_error_t select_att_key_id(
        const uint8_t *p_att_key_id_list,
        uint32_t att_key_id_list_size,
        uint8_t *p_selected_key_id,
        uint32_t selected_key_id_size)
    {
        //Make sure it has the same size of the user visible structure sgx_att_key_id_t
        se_static_assert(sizeof(sgx_att_key_id_ext_t) == sizeof(sgx_att_key_id_t));

        AESM_DBG_INFO("select_att_key_id");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        if ((NULL == p_att_key_id_list && att_key_id_list_size)
            || (NULL != p_att_key_id_list && att_key_id_list_size < (sizeof(sgx_att_key_id_ext_t) + sizeof(sgx_ql_att_key_id_list_header_t)))
            || (NULL == p_selected_key_id)
            || (selected_key_id_size < sizeof(sgx_att_key_id_ext_t)))
            return AESM_PARAMETER_ERROR;

        sgx_ql_att_key_id_list_t *p_list = (sgx_ql_att_key_id_list_t *)p_att_key_id_list;

        memset(p_selected_key_id, 0, selected_key_id_size);
        std::vector<sgx_att_key_id_ext_t> matched_ids;

        // If the att_key_id_list is NULL, it means the user-specified quote types(att_key_id_list) including all 3 types.
        if (!p_att_key_id_list)
        {
            for (auto it : available_key_ids)
                matched_ids.push_back(it.key_id);
        }
        else
        {
            if (0 != p_list->header.id || 0 != p_list->header.version)
            {
                return AESM_PARAMETER_ERROR;
            }
            if (p_list->header.num_att_ids > SGX_MAX_ATT_KEY_IDS)
            {
                return AESM_PARAMETER_ERROR;
            }
            // sanity check for the list size and num_att_ids
            if (p_list->header.num_att_ids * sizeof(sgx_att_key_id_ext_t) + sizeof(sgx_ql_att_key_id_list_header_t) != att_key_id_list_size)
            {
                return AESM_PARAMETER_ERROR;
            }

            for (int i = 0; i < p_list->header.num_att_ids; i++)
            {
                AESM_DBG_INFO("trying to find quote type %d", (p_list->ext_id_list + i)->base.algorithm_id);
                for (auto it : available_key_ids)
                {
                    if (!memcmp(p_list->ext_id_list + i, &it.key_id.base, sizeof(it.key_id.base)))
                    {
                        matched_ids.push_back(*(p_list->ext_id_list + i));
                        AESM_DBG_INFO("requested quote type %d is available", it.key_id.base.algorithm_id);
                    }
                }
            }
        }
        if (matched_ids.empty())
            return AESM_UNSUPPORTED_ATT_KEY_ID;
        // If only one ID matched, return it.
        if (matched_ids.size() == 1)
        {
            memcpy_s(p_selected_key_id, selected_key_id_size, &matched_ids[0], sizeof(matched_ids[0]));
            return AESM_SUCCESS;
        }
        // If multiple ID matched, return the "tie-breaker" if it is available.
        for (auto it : matched_ids)
        {
            if (default_quoting_type == it.base.algorithm_id)
            {
                memcpy_s(p_selected_key_id, selected_key_id_size, &it, sizeof(it));
                return AESM_SUCCESS;
            }
        }
        // If the "tie-breaker" is not available, return the first available item.
        memcpy_s(p_selected_key_id, selected_key_id_size, &matched_ids[0], sizeof(matched_ids[0]));
        return AESM_SUCCESS;
    }

    aesm_error_t init_quote_ex(
        const uint8_t *att_key_id, uint32_t att_key_id_size,
        uint8_t *target_info, uint32_t target_info_size,
        uint8_t *pub_key_id, size_t *pub_key_id_size)
    {
        AESM_DBG_INFO("init_quote_ex");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        if ((NULL == att_key_id)
            || (att_key_id_size < sizeof(sgx_att_key_id_ext_t)))
            return AESM_PARAMETER_ERROR;

        AESM_DBG_INFO("trying to find quote type %d", ((sgx_att_key_id_ext_t *)att_key_id)->base.algorithm_id);
        for (auto it : available_key_ids)
        {
            if (!memcmp(att_key_id, &it.key_id.base, sizeof(it.key_id.base)))
            {
                return it.service->init_quote_ex(att_key_id, att_key_id_size,
                            target_info, target_info_size,
                            pub_key_id, pub_key_id_size);
            }
        }
        return AESM_UNSUPPORTED_ATT_KEY_ID;
}

    aesm_error_t get_quote_size_ex(
        const uint8_t *att_key_id, uint32_t att_key_id_size,
        uint32_t *quote_size)
    {
        AESM_DBG_INFO("get_quote_size_ex");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        if ((NULL == att_key_id)
            || (att_key_id_size < sizeof(sgx_att_key_id_ext_t)))
            return AESM_PARAMETER_ERROR;

        AESM_DBG_INFO("trying to find quote type %d", ((sgx_att_key_id_ext_t *)att_key_id)->base.algorithm_id);
        for (auto it : available_key_ids)
        {
            if (!memcmp(att_key_id, &it.key_id.base, sizeof(it.key_id.base)))
            {
                return it.service->get_quote_size_ex(att_key_id, att_key_id_size, quote_size);
            }
        }
        return AESM_UNSUPPORTED_ATT_KEY_ID;
    }

    aesm_error_t get_quote_ex(
        const uint8_t *app_report, uint32_t app_report_size,
        const uint8_t *att_key_id, uint32_t att_key_id_size,
        uint8_t *qe_report_info, uint32_t qe_report_info_size,
        uint8_t *quote, uint32_t quote_size)
    {
        AESM_DBG_INFO("get_quote_ex");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        if ((NULL == att_key_id)
            || (att_key_id_size < sizeof(sgx_att_key_id_ext_t)))
            return AESM_PARAMETER_ERROR;

        AESM_DBG_INFO("trying to find quote type %d", ((sgx_att_key_id_ext_t *)att_key_id)->base.algorithm_id);
        for (auto it : available_key_ids)
        {
            if (!memcmp(att_key_id, &it.key_id.base, sizeof(it.key_id.base)))
            {
                return it.service->get_quote_ex(app_report, app_report_size,
                    att_key_id, att_key_id_size,
                    qe_report_info, qe_report_info_size,
                    quote, quote_size);
            }
        }
        return AESM_UNSUPPORTED_ATT_KEY_ID;
    }
    aesm_error_t get_att_key_id_num(
        uint32_t *att_key_id_num)
    {
        AESM_DBG_INFO("get_att_key_id_num");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        if (NULL == att_key_id_num)
            return AESM_PARAMETER_ERROR;
        *att_key_id_num = (uint32_t)available_key_ids.size();
        return AESM_SUCCESS;
    }
    aesm_error_t get_att_key_id(
        uint8_t *att_key_id,
        uint32_t att_key_id_size)
    {
        AESM_DBG_INFO("get_att_key_id");
        if (false == initialized)
            return AESM_SERVICE_UNAVAILABLE;
        if ((NULL == att_key_id)
            || (0 == att_key_id_size))
            return AESM_PARAMETER_ERROR;
        auto num = available_key_ids.size();
        if ((att_key_id_size / sizeof(sgx_att_key_id_ext_t)) < num)
            return AESM_PARAMETER_ERROR;
        sgx_att_key_id_ext_t *p = (sgx_att_key_id_ext_t *)att_key_id;
        for (auto it : available_key_ids)
        {
            memcpy(p, &it.key_id, sizeof(it.key_id));
            p++;
        }
        return AESM_SUCCESS;
    }
};

class Activator : public BundleActivator
{
  void Start(BundleContext ctx)
  {
    auto service = std::make_shared<QuoteExServiceImp>();
    ctx.RegisterService<IQuoteProxyService>(service);
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
CPPMICROSERVICES_INITIALIZE_BUNDLE(quote_ex_service_bundle_name)
#endif
