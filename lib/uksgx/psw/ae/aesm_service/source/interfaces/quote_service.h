#ifndef QUOTE_SERVICE_EXPORT_H
#define QUOTE_SERVICE_EXPORT_H
#include "service.h"
#include <stdint.h>
#include "aesm_error.h"

struct IQuoteService : virtual public IService
{
    // The value should be the same as the major version in manifest.json
    enum {VERSION = 2};
    virtual ~IQuoteService() = default;

    virtual aesm_error_t init_quote(
        uint8_t *target_info,
        uint32_t target_info_size,
        uint8_t *gid,
        uint32_t gid_size) = 0;

    virtual aesm_error_t get_quote(
        const uint8_t *report, uint32_t report_size,
        uint32_t quote_type,
        const uint8_t *spid, uint32_t spid_size,
        const uint8_t *nonce, uint32_t nonce_size,
        const uint8_t *sigrl, uint32_t sigrl_size,
        uint8_t *qe_report, uint32_t qe_report_size,
        uint8_t *quote, uint32_t buf_size) = 0;

};

#endif /* QUOTE_SERVICE_EXPORT_H */
