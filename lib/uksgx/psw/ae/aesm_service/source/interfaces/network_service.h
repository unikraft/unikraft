#ifndef NETWORK_SERVICE_EXPORT_H
#define NETWORK_SERVICE_EXPORT_H
#include "service.h"
#include "stdint.h"
#include "aeerror.h"
#include "aesm_error.h"


typedef enum _network_protocol_type_t
{
	HTTP = 0,
	HTTPS,
} network_protocol_type_t;

typedef enum _http_methods_t
{
	GET = 0,
	POST,
} http_methods_t;

struct INetworkService : virtual public IService
{
    // The value should be the same as the major version in manifest.json
    enum {VERSION = 2};
    virtual ~INetworkService() = default;

    virtual ae_error_t aesm_send_recv_msg(
        const char *url,
        const uint8_t *msg,
        uint32_t msg_size,
        uint8_t* &resp_msg,
        uint32_t& resp_size,
        http_methods_t type,
        bool is_ocsp) = 0;

    virtual void aesm_free_response_msg(
        uint8_t *resp) = 0;
        
    virtual ae_error_t aesm_send_recv_msg_encoding(
        const char *url,
        const uint8_t *msg,
        uint32_t msg_size,
        uint8_t* &resp,
        uint32_t& resp_size) = 0;
};

#endif /* NETWORK_SERVICE_EXPORT_H */
