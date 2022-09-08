#ifndef GET_ATT_KEY_ID_H
#define GET_ATT_KEY_ID_H
#include "aesm_error.h"

struct IGetAttKeyID
{
    // The value should be the same as the major version in manifest.json
    enum {VERSION = 2};
    virtual ~IGetAttKeyID() = default;

    virtual aesm_error_t get_att_key_id(
        uint8_t *att_key_id,
        uint32_t att_key_id_size) = 0;
    virtual aesm_error_t get_att_key_id_num(
        uint32_t *att_key_id_num) = 0;
};

#endif /* GET_ATT_KEY_ID_H */
