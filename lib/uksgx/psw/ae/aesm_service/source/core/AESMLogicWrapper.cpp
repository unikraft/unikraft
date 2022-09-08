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
#include "AESMLogicWrapper.h"
#include <iostream>
#include <unistd.h>

#include <cppmicroservices/Bundle.h>
#include <cppmicroservices/BundleContext.h>
#include <cppmicroservices/BundleImport.h>
#include <cppmicroservices/Framework.h>
#include <cppmicroservices/FrameworkFactory.h>
#include <cppmicroservices/FrameworkEvent.h>

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <dirent.h>
#include <iostream>

#include "sgx_quote.h"
#include "launch_service.h"
#include "epid_quote_service.h"
#include "quote_proxy_service.h"
#include "pce_service.h"
#include "network_service.h"
#include "cppmicroservices_util.h"

#include "arch.h"
#include "sgx_urts.h"
#include "oal/oal.h"

static cppmicroservices::BundleContext g_fw_ctx;
using namespace cppmicroservices;
static Framework g_fw = FrameworkFactory().NewFramework();


#ifdef US_PLATFORM_POSIX
#define PATH_SEPARATOR "/"
#else
#define PATH_SEPARATOR "\\"
#endif

#define BUNDLE_SUBFOLDER "bundles"

static std::vector<std::string> get_bundles()
{
    static std::string bundle_dir;
    std::vector<std::string> files;
    if (bundle_dir.empty())
    {
        char buf[PATH_MAX] = {0};
        Dl_info dl_info;
        if (0 == dladdr(__builtin_return_address(0), &dl_info) ||
            NULL == dl_info.dli_fname)
        return files;
        if (strnlen(dl_info.dli_fname, sizeof(buf)) >= sizeof(buf))
        return files;
        (void)strncpy(buf, dl_info.dli_fname, sizeof(buf));
        std::string aesm_path(buf);

        size_t i = aesm_path.rfind(PATH_SEPARATOR, aesm_path.length());
        if (i != std::string::npos)
        {
            bundle_dir = aesm_path.substr(0, i);
            size_t j = bundle_dir.rfind(PATH_SEPARATOR, bundle_dir.length());
            if (j != std::string::npos && bundle_dir.substr(j, i) == BUNDLE_SUBFOLDER)
                bundle_dir  += PATH_SEPARATOR;
            else
                bundle_dir = bundle_dir + PATH_SEPARATOR + BUNDLE_SUBFOLDER;
        }
        else
            bundle_dir = aesm_path;
    }
    std::shared_ptr<DIR> directory_ptr(opendir(bundle_dir.c_str()), [](DIR *dir) { dir &&closedir(dir); });
    struct dirent *dirent_ptr;
    if (!directory_ptr)
    {
        std::cout << "Error opening : " << std::strerror(errno) << bundle_dir << std::endl;
        return files;
    }

    while ((dirent_ptr = readdir(directory_ptr.get())) != nullptr)
    {
        std::string file_name = std::string(dirent_ptr->d_name);
        size_t length = file_name.length();
        if (file_name.length() <= strlen(US_LIB_EXT))
            continue;
        else if (file_name.substr(length - strlen(US_LIB_EXT), length) != US_LIB_EXT)
            continue;
        files.push_back(bundle_dir + PATH_SEPARATOR + file_name);
    }
    return files;
}

static void intall_bundles()
{
    for (auto name : get_bundles())
    {
        if (g_fw_ctx.GetBundles(name).empty())
        {
            auto bundles = g_fw_ctx.InstallBundles(name);
            for (auto &bundle : bundles)
            {
                bundle.Start();
            }
        }
    }
}

template <class S>
static bool intall_and_get_service(std::shared_ptr<S>& service)
{
    try
    {
        if (!g_fw_ctx)
            return false;
        auto ref = g_fw_ctx.GetServiceReference<S>();
        if (!ref) {
            intall_bundles();
            ref = g_fw_ctx.GetServiceReference<S>();
            if (!ref)
                return false;
        }
        auto bundle = ref.GetBundle();
        if (!bundle)
            return false;
        if (S::VERSION != bundle.GetVersion().GetMajor())
            return false;
        service = g_fw_ctx.GetService(ref);
        if (AE_SUCCESS == service->start())
        {
            return true;
        }
        intall_bundles();
        return AE_SUCCESS == service->start();

    }
    catch(...)
    {
        return false;
    }
}

AESMLogicWrapper::AESMLogicWrapper()
{
}

aesm_error_t AESMLogicWrapper::select_att_key_id(uint32_t att_key_id_list_size,
                               const uint8_t *att_key_id_list,
                               uint32_t *select_att_key_id_size,
                               uint8_t **select_key_id)
{
    uint8_t *output_att_key_id = new uint8_t[sizeof(sgx_att_key_id_t)]();
    uint32_t output_att_key_id_size = sizeof(sgx_att_key_id_t);

    aesm_error_t result = AESM_SERVICE_UNAVAILABLE;

    std::shared_ptr<IQuoteProxyService> service;
    for (int retry = 1 ;retry >= 0; retry--)
    {
        if (!intall_and_get_service(service))
        {
            delete[] output_att_key_id;
            return AESM_SERVICE_UNAVAILABLE;
        }

        result = service->select_att_key_id(att_key_id_list, att_key_id_list_size,
            output_att_key_id, output_att_key_id_size);
        if (result == AESM_SUCCESS)
        {
            *select_key_id = output_att_key_id;
            *select_att_key_id_size = output_att_key_id_size;
            break;
        }
        else
        {
            //Try to install quoting bundle when unsupported attestation key id found
            if (result == AESM_UNSUPPORTED_ATT_KEY_ID && retry > 0)
            {
                try
                {
                    intall_bundles();
                    continue;
                }
                catch(...)
                {
                    continue;
                }
            }
            delete[] output_att_key_id;
            break;
        }
    }
    return result;
}


aesm_error_t AESMLogicWrapper::get_supported_att_key_id_num(
    uint32_t *att_key_id_num)
{
    aesm_error_t result = AESM_SERVICE_UNAVAILABLE;
    std::shared_ptr<IQuoteProxyService> service;
    if (!get_service_wrapper(service, g_fw_ctx))
    {
        return AESM_SERVICE_UNAVAILABLE;
    }

    result = service->get_att_key_id_num(
        att_key_id_num);
    if (0 == *att_key_id_num)
    {
        return AESM_SERVICE_UNAVAILABLE;
    }
    return result;
}

aesm_error_t AESMLogicWrapper::get_supported_att_key_ids(
    uint8_t **att_key_ids, uint32_t att_key_ids_size)
{
    uint32_t local_att_key_id_num = 0;
    uint8_t *output_att_key_ids = NULL;

    aesm_error_t result = AESM_SERVICE_UNAVAILABLE;
    std::shared_ptr<IQuoteProxyService> service;
    if (!get_service_wrapper(service, g_fw_ctx))
    {
        return AESM_SERVICE_UNAVAILABLE;
    }

    result = service->get_att_key_id_num(&local_att_key_id_num);
    if (result != AESM_SUCCESS)
    {
        return result;
    }
    if (att_key_ids_size % sizeof(sgx_att_key_id_ext_t) != 0 ||
        att_key_ids_size / sizeof(sgx_att_key_id_ext_t) != local_att_key_id_num)
    {
        return AESM_PARAMETER_ERROR;
    }

    output_att_key_ids = new uint8_t[att_key_ids_size]();
    result = service->get_att_key_id(
        output_att_key_ids, att_key_ids_size);
    if (result == AESM_SUCCESS)
    {
        *att_key_ids = output_att_key_ids;
    }
    else
    {
        delete[] output_att_key_ids;
    }
    return result;
}

aesm_error_t AESMLogicWrapper::init_quote_ex(
    uint32_t att_key_id_size, const uint8_t *att_key_id,
    uint8_t **target_info, uint32_t *target_info_size,
    bool b_pub_key_id, size_t *pub_key_id_size, uint8_t **pub_key_id)
{
    uint8_t *output_target_info = new uint8_t[sizeof(sgx_target_info_t)]();
    uint32_t output_target_info_size = sizeof(sgx_target_info_t);
    uint8_t *output_pub_key_id = NULL;
    size_t output_pub_key_id_size = 0;
    if (b_pub_key_id)
    {
        output_pub_key_id = new uint8_t[sizeof(sgx_sha256_hash_t)]();
        output_pub_key_id_size = *pub_key_id_size;
    }

    aesm_error_t result = AESM_SERVICE_UNAVAILABLE;

    std::shared_ptr<IQuoteProxyService> service;
    if (!intall_and_get_service(service))
    {
        delete[] output_target_info;
        if (b_pub_key_id)
            delete[] output_pub_key_id;
        return AESM_SERVICE_UNAVAILABLE;
    }

    result = service->init_quote_ex(att_key_id, att_key_id_size,
                                    output_target_info, output_target_info_size,
                                    output_pub_key_id, &output_pub_key_id_size);
    if (result == AESM_SUCCESS)
    {
        *target_info = output_target_info;
        *target_info_size = output_target_info_size;
        *pub_key_id_size = output_pub_key_id_size;
        *pub_key_id = output_pub_key_id;
    }
    else
    {
        delete[] output_target_info;
        delete[] output_pub_key_id;
    }
    return result;
}

aesm_error_t AESMLogicWrapper::get_quote_size_ex(
    uint32_t att_key_id_size, const uint8_t *att_key_id,
    uint32_t *quote_size)
{
    std::shared_ptr<IQuoteProxyService> service;
    if (!intall_and_get_service(service))
    {
        return AESM_SERVICE_UNAVAILABLE;
    }

    return service->get_quote_size_ex(att_key_id, att_key_id_size, quote_size);
}

aesm_error_t AESMLogicWrapper::get_quote_ex(
    uint32_t report_size, const uint8_t *report,
    uint32_t att_key_id_size, const uint8_t *att_key_id,
    uint32_t qe_report_info_size, uint8_t *qe_report_info,
    uint32_t quote_size, uint8_t **quote)
{
    uint8_t *output_quote = new uint8_t[quote_size]();

    aesm_error_t result = AESM_SERVICE_UNAVAILABLE;

    std::shared_ptr<IQuoteProxyService> service;
    if (!intall_and_get_service(service))
    {
        delete[] output_quote;
        return AESM_SERVICE_UNAVAILABLE;
    }

    result = service->get_quote_ex(report, report_size, att_key_id, att_key_id_size, qe_report_info,
                                   qe_report_info_size, output_quote, quote_size);
    if (result == AESM_SUCCESS)
    {
        *quote = output_quote;
    }
    else
    {
        delete[] output_quote;
    }
    return result;
}

aesm_error_t AESMLogicWrapper::initQuote(uint8_t **target_info,
                                         uint32_t *target_info_length,
                                         uint8_t **gid,
                                         uint32_t *gid_length)
{
    uint8_t *output_target_info = new uint8_t[sizeof(sgx_target_info_t)]();
    uint8_t *output_gid = new uint8_t[sizeof(sgx_epid_group_id_t)]();
    uint32_t output_target_info_length = sizeof(sgx_target_info_t);
    uint32_t output_gid_length = sizeof(sgx_epid_group_id_t);
    aesm_error_t result = AESM_SERVICE_UNAVAILABLE;

    std::shared_ptr<IEpidQuoteService> service;
    if (!intall_and_get_service(service))
    {
        delete[] output_target_info;
        delete[] output_gid;
        return AESM_SERVICE_UNAVAILABLE;
    }

    result = service->init_quote(output_target_info, output_target_info_length, output_gid, output_gid_length);
    if (result == AESM_SUCCESS)
    {
        *target_info = output_target_info;
        *target_info_length = output_target_info_length;

        *gid = output_gid;
        *gid_length = output_gid_length;
    }
    else
    {
        delete[] output_target_info;
        delete[] output_gid;
    }
    return result;
}

aesm_error_t AESMLogicWrapper::getQuote(uint32_t reportLength, const uint8_t *report,
                                        uint32_t quoteType,
                                        uint32_t spidLength, const uint8_t *spid,
                                        uint32_t nonceLength, const uint8_t *nonce,
                                        uint32_t sig_rlLength, const uint8_t *sig_rl,
                                        uint32_t bufferSize, uint8_t **quote,
                                        bool b_qe_report, uint32_t *qe_reportSize, uint8_t **qe_report)
{
    uint8_t *output_quote = new uint8_t[bufferSize]();
    uint8_t *output_qe_report = NULL;
    uint32_t output_qe_reportSize = 0;
    if (b_qe_report)
    {
        output_qe_report = new uint8_t[sizeof(sgx_report_t)]();
        output_qe_reportSize = sizeof(sgx_report_t);
    }
    aesm_error_t result = AESM_SERVICE_UNAVAILABLE;
    std::shared_ptr<IEpidQuoteService> service;
    if (!intall_and_get_service(service))
    {
        delete[] output_quote;
        if (output_qe_report)
            delete[] output_qe_report;
        return AESM_SERVICE_UNAVAILABLE;
    }

    result = service->get_quote(report, reportLength,
                                quoteType,
                                spid, spidLength,
                                nonce, nonceLength,
                                sig_rl, sig_rlLength,
                                output_qe_report, output_qe_reportSize,
                                output_quote, bufferSize);
    if (result == AESM_SUCCESS)
    {
        *quote = output_quote;

        *qe_report = output_qe_report;
        *qe_reportSize = output_qe_reportSize;
    }
    else
    {
        delete[] output_quote;
        if (output_qe_report)
            delete[] output_qe_report;
    }
    return result;
}

aesm_error_t AESMLogicWrapper::getLaunchToken(const uint8_t *measurement,
                                              uint32_t measurement_size,
                                              const uint8_t *mrsigner,
                                              uint32_t mrsigner_size,
                                              const uint8_t *se_attributes,
                                              uint32_t se_attributes_size,
                                              uint8_t **launch_token,
                                              uint32_t *launch_token_size)
{
    uint32_t output_launch_token_size = sizeof(token_t);
    uint8_t *output_launch_token = new uint8_t[sizeof(token_t)]();
    aesm_error_t result = AESM_SERVICE_UNAVAILABLE;
    std::shared_ptr<ILaunchService> service;
    if (!intall_and_get_service(service))
    {
        delete[] output_launch_token;
        return AESM_SERVICE_UNAVAILABLE;
    }

    result = service->get_launch_token(measurement,
                                       measurement_size,
                                       mrsigner,
                                       mrsigner_size,
                                       se_attributes,
                                       se_attributes_size,
                                       output_launch_token,
                                       output_launch_token_size);
    if (result == AESM_SUCCESS)
    {
        *launch_token = output_launch_token;
        *launch_token_size = output_launch_token_size;
    }
    else
    {
        delete[] output_launch_token;
    }
    return result;
}

aesm_error_t AESMLogicWrapper::reportAttestationStatus(uint8_t *platform_info, uint32_t platform_info_size,
                                                       uint32_t attestation_error_code,
                                                       uint8_t **update_info, uint32_t update_info_size)

{
    uint8_t *output_update_info = new uint8_t[update_info_size]();
    aesm_error_t result = AESM_SERVICE_UNAVAILABLE;
    std::shared_ptr<IEpidQuoteService> service;
    if (!intall_and_get_service(service))
    {
        delete[] output_update_info;
        return AESM_SERVICE_UNAVAILABLE;
    }
    result = service->report_attestation_status(platform_info, platform_info_size,
                                                attestation_error_code,
                                                output_update_info, update_info_size);

    //update_info is valid when result is AESM_UPDATE_AVAILABLE
    if (NULL != update_info && (result == AESM_SUCCESS || result == AESM_UPDATE_AVAILABLE))
    {
        *update_info = output_update_info;
    }
    else
    {
        delete[] output_update_info;
    }
    return result;
}

aesm_error_t AESMLogicWrapper::checkUpdateStatus(uint8_t* platform_info, uint32_t platform_info_size,
	uint8_t** update_info, uint32_t update_info_size,
	uint32_t config, uint32_t* status)

{
	aesm_error_t result = AESM_SERVICE_UNAVAILABLE;
	std::shared_ptr<IEpidQuoteService> service;
	if (!intall_and_get_service(service))
	{
		return AESM_SERVICE_UNAVAILABLE;
	}
	uint8_t* output_update_info = NULL;
	if (update_info != NULL && update_info_size != 0)
		output_update_info = new uint8_t[update_info_size]();

	result = service->check_update_status(platform_info, platform_info_size,
		output_update_info, update_info_size,
		config, status);

	//update_info is valid when result is AESM_UPDATE_AVAILABLE
	if (NULL != update_info && (result == AESM_SUCCESS || result == AESM_UPDATE_AVAILABLE))
	{
		*update_info = output_update_info;
	}
	else
	{
		if (NULL != output_update_info)
			delete[] output_update_info;
	}
	return result;
}

aesm_error_t AESMLogicWrapper::getWhiteListSize(uint32_t* white_list_size)
{
    std::shared_ptr<ILaunchService> service;
    if (!intall_and_get_service(service))
    {
        return AESM_SERVICE_UNAVAILABLE;
    }

    return service->get_white_list_size(white_list_size);
}

aesm_error_t AESMLogicWrapper::getWhiteList(uint8_t **white_list,
                                            uint32_t white_list_size)
{
    uint32_t local_white_list_size = 0;
    uint8_t *output_white_list = NULL;
    aesm_error_t result = AESM_SERVICE_UNAVAILABLE;
    std::shared_ptr<ILaunchService> service;
    if (!intall_and_get_service(service))
    {
        return AESM_SERVICE_UNAVAILABLE;
    }
    result = service->get_white_list_size(&local_white_list_size);
    if (result != AESM_SUCCESS)
    {
        return result;
    }
    if (white_list_size != local_white_list_size)
    {
        return AESM_PARAMETER_ERROR;
    }

    output_white_list = new uint8_t[white_list_size]();
    result = service->get_white_list(output_white_list, white_list_size);
    if (result == AESM_SUCCESS)
    {
        *white_list = output_white_list;
    }
    else
    {
        delete[] output_white_list;
    }
    return result;
}

aesm_error_t AESMLogicWrapper::sgxGetExtendedEpidGroupId(uint32_t *x_group_id)
{
    std::shared_ptr<IEpidQuoteService> service;
    if (!intall_and_get_service(service))
    {
        return AESM_SERVICE_UNAVAILABLE;
    }

    return service->get_extended_epid_group_id(x_group_id);
}

aesm_error_t AESMLogicWrapper::sgxSwitchExtendedEpidGroup(uint32_t x_group_id)
{
    std::shared_ptr<IEpidQuoteService> service;
    if (!intall_and_get_service(service))
    {
        return AESM_SERVICE_UNAVAILABLE;
    }

    return service->switch_extended_epid_group(x_group_id);
}

aesm_error_t AESMLogicWrapper::sgxRegister(uint8_t *buf, uint32_t buf_size, uint32_t data_type)
{
    if (data_type == SGX_REGISTER_WHITE_LIST_CERT)
    {
        std::shared_ptr<ILaunchService> service;
        if (!intall_and_get_service(service))
        {
            return AESM_SERVICE_UNAVAILABLE;
        }

        return service->white_list_register(buf, buf_size);
    }
    else
    {
        return AESM_PARAMETER_ERROR;
    }
}

#include "ssl_compat_wrapper.h"

ae_error_t AESMLogicWrapper::service_start()
{
    try
    {
        // Initialize the framework, such that we can call
        // GetBundleContext() later.
        g_fw.Init();
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return AE_FAILURE;
    }

    // The framework inherits from the Bundle class; it is
    // itself a bundle.
    g_fw_ctx = g_fw.GetBundleContext();
    if (!g_fw_ctx)
    {
        std::cerr << "Invalid framework context" << std::endl;
        return AE_FAILURE;
    }
    else
    {
        std::cout << "The path of system bundle: " << g_fw.GetLocation() << std::endl;
    }

    // Install all bundles contained in the shared libraries
    // given as command line arguments.
#if defined(US_BUILD_SHARED_LIBS)
    try
    {
        for (auto name : get_bundles())
        {
            g_fw_ctx.InstallBundles(name);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
#endif

    try
    {
        // Start the framework itself.
        g_fw.Start();
        auto bundles = g_fw_ctx.GetBundles();
        for (auto &bundle : bundles)
        {
            bundle.Start();
            std::cerr << bundle.GetSymbolicName() << ":" << bundle.GetVersion() << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return AE_FAILURE;
    }

    crypto_initialize();

    AESM_DBG_INFO("aesm service is starting");

    {
        std::shared_ptr<INetworkService> service;
        if (get_service_wrapper(service, g_fw_ctx))
            service->start();
    }
    {
        ae_error_t ret = AE_SUCCESS;
        std::shared_ptr<ILaunchService> service;
        if (get_service_wrapper(service, g_fw_ctx))
            ret = service->start();
        if (AE_SUCCESS != ret && AESM_AE_NO_DEVICE != ret)
        {
            AESM_DBG_INFO("Failed to load LE and it's not because of LCP");
            return AE_FAILURE;
        }
    }
    {
        std::shared_ptr<IQuoteProxyService> service;
        if (get_service_wrapper(service, g_fw_ctx))
            service->start();
    }
    AESM_DBG_INFO("aesm service started");

    return AE_SUCCESS;
}

void AESMLogicWrapper::service_stop()
{
    AESM_DBG_INFO("AESMLogicWrapper::service_stop");
    std::shared_ptr<IQuoteProxyService> quote_ex_service;
    if (get_service_wrapper(quote_ex_service, g_fw_ctx))
        quote_ex_service->stop();
    std::shared_ptr<IPceService> pce_service;
    if (get_service_wrapper(pce_service, g_fw_ctx))
        pce_service->stop();
    std::shared_ptr<ILaunchService> launch_service;
    if (get_service_wrapper(launch_service, g_fw_ctx))
        launch_service->stop();
    std::shared_ptr<INetworkService> network_service;
    if (get_service_wrapper(network_service, g_fw_ctx))
        network_service->stop();
    g_fw.Stop();
    g_fw.WaitForStop(std::chrono::seconds(5));
    crypto_cleanup();
    AESM_DBG_INFO("aesm service down");
}

#if !defined(US_BUILD_SHARED_LIBS)
CPPMICROSERVICES_IMPORT_BUNDLE(pce_service_bundle_name)
CPPMICROSERVICES_IMPORT_BUNDLE(epid_quote_service_bundle_name)
CPPMICROSERVICES_IMPORT_BUNDLE(ecdsa_quote_service_bundle_name)
CPPMICROSERVICES_IMPORT_BUNDLE(le_launch_service_bundle_name)
CPPMICROSERVICES_IMPORT_BUNDLE(linux_network_service_bundle_name)
#endif
