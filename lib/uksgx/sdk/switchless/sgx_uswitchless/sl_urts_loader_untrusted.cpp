#include "sl_urts_loader.h"
#include "uswitchless.h"
#include "sgx_switchless_itf.h"

/*
The purpose of this class is to enable automatic set of switchless function pointers in SGX runtime.
We are decalring globals variable of this class, so compiler generates call to its constructor upon program initialization.
The executable that links to this static library (sgx_uswitchless.lib) MUST add /WHOLEARCHIVE:sgx_uswtichless.lib option when linking.
*/

sgx_switchless_funcs_t g_switchless_itf = 
{
    sl_init_uswitchless,
    sl_uswitchless_do_switchless_ecall,
    sl_destroy_uswitchless,
    sl_uswitchless_check_switchless_ocall_fallback,
    sl_uswitchless_on_first_ecall
};


urts_loader_t::urts_loader_t()
{
    sgx_set_switchless_itf(&g_switchless_itf);
}

urts_loader_t urts_loader;

