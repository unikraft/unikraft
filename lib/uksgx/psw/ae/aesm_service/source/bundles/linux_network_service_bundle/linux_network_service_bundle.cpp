#include <network_service.h>

#include <cppmicroservices/BundleActivator.h>

#include <iostream>
#include <curl/curl.h>

#include "oal/oal.h"
#include "aesm_encode.h"
#include "aesm_network.h"

using namespace cppmicroservices;

class LinuxNetworkServiceImp : public INetworkService
{
private:
    ae_error_t aesm_send_recv_msg_encoding_internal(
        const char *url,
        const uint8_t * msg,
        uint32_t msg_size,
        uint8_t* &resp,
        uint32_t& resp_size)
    {
        resp = NULL;
        resp_size = 0;

        ae_error_t ae_ret = AE_SUCCESS;
        uint8_t *encode_msg = NULL;
        uint32_t encoding_size = get_request_encoding_length(msg);
        uint8_t *recv_msg = NULL;
        uint32_t decode_buffer_size = 0;
        uint32_t recv_size = 0;
        if(encoding_size == 0){
            AESM_DBG_WARN("invalid msg_size 0 to send to url:%s", url);
            ae_ret = AE_FAILURE;
            goto ret_point;
        }
        encode_msg = reinterpret_cast<uint8_t *>(malloc(encoding_size));
        if(encode_msg == NULL){
            AESM_DBG_ERROR("malloc failed");
            ae_ret = AE_OUT_OF_MEMORY_ERROR;
            goto ret_point;
        }
    #ifdef DBG_LOG
        char dbg_hex_string[256];
        aesm_dbg_format_hex(msg, msg_size, dbg_hex_string, 256);
        AESM_DBG_TRACE("send msg \"%s\" to server:%s",dbg_hex_string, url);
    #endif
        memset(encode_msg, 0, encoding_size);
        if(!encode_request(msg, msg_size, encode_msg, &encoding_size)){
            AESM_DBG_ERROR("message encoding error, msg size %d",msg_size);
        ae_ret = AE_FAILURE;
            goto ret_point;
        }
        AESM_DBG_TRACE("encoded msg %.*s",encoding_size, encode_msg);
        ae_ret = aesm_network_send_receive(url, encode_msg, encoding_size,
            &recv_msg, &recv_size, POST, false);
        if(ae_ret != AE_SUCCESS ){
            AESM_DBG_ERROR("fail to send encoded msg (size=%d) to url:%s",encoding_size, url);
            goto ret_point;
        }
        if(recv_msg == NULL){
            AESM_DBG_ERROR("recv NULL message from backend server");
        ae_ret = AE_FAILURE;
            goto ret_point;
        }
        AESM_DBG_TRACE("response msg %.*s", recv_size, recv_msg);
        decode_buffer_size= get_response_decoding_length(recv_size);

        if(decode_buffer_size == 0){
            AESM_DBG_ERROR("response 0 length message from backend server:%s",url);
        ae_ret = AE_FAILURE;
            goto ret_point;
        }
        AESM_DBG_TRACE("Succ recv msg:%.*s", recv_size, recv_msg);

        resp = reinterpret_cast<uint8_t *>(malloc(decode_buffer_size));
        if(resp == NULL){
            AESM_DBG_ERROR("malloc error");
            ae_ret = AE_OUT_OF_MEMORY_ERROR;
            goto ret_point;
        }
        memset(resp, 0, decode_buffer_size);
        if(!decode_response(recv_msg,recv_size, resp,  &decode_buffer_size)){
            AESM_DBG_WARN("fail to decode message from server");
        ae_ret = AE_FAILURE;
            goto ret_point;
        }
    #ifdef DBG_LOG
        aesm_dbg_format_hex(resp, decode_buffer_size, dbg_hex_string, 256);
        AESM_DBG_TRACE("succ decode msg \"%s\" ",dbg_hex_string);
    #endif
        resp_size = decode_buffer_size;
    ret_point:
        aesm_free_network_response_buffer(recv_msg);
        if(ae_ret != AE_SUCCESS){
            if(resp!=NULL)free(resp);
            resp = NULL;
        }
        if(encode_msg!=NULL)free(encode_msg);
        return ae_ret;
    }
    
public:
    ae_error_t start()
    {
        return AE_SUCCESS;
    }

    void stop()
    {
    }
    
    ae_error_t aesm_send_recv_msg(
        const char *url,
        const uint8_t * msg,
        uint32_t msg_size,
        uint8_t* &resp_msg,
        uint32_t& resp_size,
        http_methods_t type,
        bool is_ocsp)
    {
        return ::aesm_network_send_receive(url, msg, msg_size, &resp_msg, &resp_size, type, is_ocsp);
    }

    void aesm_free_response_msg(uint8_t *resp)
    {
        if(resp!=NULL){
            free(resp);
        }
    }

    ae_error_t aesm_send_recv_msg_encoding(
        const char *url,
        const uint8_t * msg,
        uint32_t msg_size,
        uint8_t* &resp,
        uint32_t& resp_size)
    {
        resp = NULL;
        resp_size = 0;
        AESM_DBG_TRACE("send msg  to url %s",url);
        //call global function to encoding message, send/receive message via HTTP POST and decode response message
        return aesm_send_recv_msg_encoding_internal(url, msg, msg_size, resp, resp_size);
    }
};

class Activator : public BundleActivator
{
  void Start(BundleContext ctx)
  {
    CURLcode curl_code = curl_global_init(CURL_GLOBAL_DEFAULT);
    auto service = std::make_shared<LinuxNetworkServiceImp>();
    ctx.RegisterService<INetworkService>(service);
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
CPPMICROSERVICES_INITIALIZE_BUNDLE(linux_network_service_bundle_name)
#endif
