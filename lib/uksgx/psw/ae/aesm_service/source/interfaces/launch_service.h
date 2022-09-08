#ifndef LAUNCH_SERVICE_EXPORT_H
#define LAUNCH_SERVICE_EXPORT_H
#include "service.h"
#include "stdint.h"
#include "aesm_error.h"
#include "sgx_error.h"
#include "sgx_urts.h"
#include "arch.h"

struct ILaunchService : virtual public IService
{
    // The value should be the same as the major version in manifest.json
    enum {VERSION = 2};
    virtual ~ILaunchService() = default;

    virtual aesm_error_t get_launch_token(
        const uint8_t *mrenclave, uint32_t mrenclave_size,
        const uint8_t *public_key, uint32_t public_key_size,
        const uint8_t *se_attributes, uint32_t se_attributes_size,
        uint8_t *lictoken, uint32_t lictoken_size) = 0;
    virtual sgx_status_t get_launch_token(
        const enclave_css_t *signature,
        const sgx_attributes_t *attribute,
        sgx_launch_token_t *launch_token) = 0;
    virtual aesm_error_t white_list_register(
        const uint8_t *white_list_cert,
        uint32_t white_list_cert_size) = 0;
    virtual aesm_error_t get_white_list(
        uint8_t *white_list_cert, uint32_t buf_size) = 0;
    virtual aesm_error_t get_white_list_size(
        uint32_t *white_list_cert_size) = 0;
};

#endif /* LAUNCH_SERVICE_EXPORT_H */
