#ifndef QUOTE_EX_SERVICE_EXPORT_H
#define QUOTE_EX_SERVICE_EXPORT_H
#include "service.h"
#include <stdint.h>
#include <stddef.h>
#include "aesm_error.h"


struct IQuoteExService : virtual public IService
{
    // The value should be the same as the major version in manifest.json
    enum {VERSION = 2};
    virtual ~IQuoteExService() = default;

    virtual aesm_error_t init_quote_ex(
        const uint8_t *att_key_id, uint32_t att_key_id_size,
        uint8_t *target_info, uint32_t target_info_size,
        uint8_t *pub_key_id, size_t *pub_key_id_size) = 0;
    virtual aesm_error_t get_quote_size_ex(
        const uint8_t *att_key_id, uint32_t att_key_id_size,
        uint32_t *quote_size) = 0;
    virtual aesm_error_t get_quote_ex(
        const uint8_t *app_report, uint32_t app_report_size,
        const uint8_t *att_key_id, uint32_t att_key_id_size,
        uint8_t *qe_report_info, uint32_t qe_report_info_size,
        uint8_t *quote, uint32_t quote_size) = 0;
};

#endif /* QUOTE_EX_SERVICE_EXPORT_H */
