#ifndef SELECT_ATT_KEY_ID_H
#define SELECT_ATT_KEY_ID_H
#include <stdint.h>
#include "aesm_error.h"

struct ISelectAttKeyID
{
    // The value should be the same as the major version in manifest.json
    enum {VERSION = 2};
    virtual ~ISelectAttKeyID() = default;
    
    virtual aesm_error_t select_att_key_id(
        const uint8_t *p_att_key_id_list,
        uint32_t att_key_id_list_size,
        uint8_t *p_selected_key_id,
        uint32_t selected_key_id_size) = 0;
};

#endif /* SELECT_ATT_KEY_ID_H */
