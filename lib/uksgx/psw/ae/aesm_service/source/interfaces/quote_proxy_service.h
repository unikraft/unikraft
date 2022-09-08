#ifndef QUOTE_PROXY_SERVICE_EXPORT_H
#define QUOTE_PROXY_SERVICE_EXPORT_H
#include "quote_ex_service.h"
#include "select_att_key_id.h"
#include "get_att_key_id.h"

struct IQuoteProxyService : public IQuoteExService, public ISelectAttKeyID, public IGetAttKeyID
{
    // The value should be the same as the major version in manifest.json
    enum {VERSION = 2};
    virtual ~IQuoteProxyService() = default;
};

#endif /* QUOTE_PROXY_SERVICE_EXPORT_H */

