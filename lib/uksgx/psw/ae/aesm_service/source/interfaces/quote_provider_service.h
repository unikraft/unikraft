#ifndef QUOTE_PROVIDER_SERVICE_EXPORT_H
#define QUOTE_PROVIDER_SERVICE_EXPORT_H
#include "quote_ex_service.h"
#include "get_att_key_id.h"

struct IQuoteProviderService : public IQuoteExService, public IGetAttKeyID
{
    // The value should be the same as the major version in manifest.json
    enum {VERSION = 2};
    virtual ~IQuoteProviderService() = default;
};

#endif /* QUOTE_PROVIDER_SERVICE_EXPORT_H */

