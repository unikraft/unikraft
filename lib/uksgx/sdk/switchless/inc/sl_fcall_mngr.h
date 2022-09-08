/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _SL_FCALL_MNGR_H_
#define _SL_FCALL_MNGR_H_

/*
 * sl_call_mngr - switchless Call manager
 *
 * sl_call_mngr is a cross-enclave data structure. switchless OCall requests can be
 * made via a trusted object of sl_call_mngr, which is "cloned" from an
 * untrusted object of sl_call_mngr. The latter is created by the untrusted
 * code and are used by untrusted workers to process switchless OCall requests.
 * The same is true for switchless ECalls.
 *
 * What is cross-enclave data structures?
 *
 * A cross-enclave data structure is a data structure that are designed to be
 * shared by both non-enclave and enclave code, and to be used securely by the
 * enclave code.
 *
 * The life cycle of a cross-enclave data structure is as follows. First, the
 * non-enclave code allocates and initializes an object of the cross-enclave
 * data structure and then passes the pointer of the object to the enclave
 * code. Then, the enclave code creates an trusted object out of the given,
 * untrusted object; this is called "clone". This clone operation will do all
 * proper security checks. Finally, the enclave code can access and manipuate
 * its cloned object securely. 
 *
 */

#include <sgx_error.h>
#include <sl_siglines.h>


typedef enum {
    SL_TYPE_OCALL,
    SL_TYPE_ECALL
} sl_call_type_t;

typedef sgx_status_t(*sl_call_func_t)(const void* /* ms */);

typedef struct {
    uint32_t        size;
    sl_call_func_t  funcs[];
} sl_call_table_t; /* compatible with sgx_ocall_table_t */


typedef enum {
    SL_INIT,
    SL_SUBMITTED,
    SL_ACCEPTED,
    SL_DONE
} sl_call_status_t;

#pragma pack(push, 1)

struct sl_call_task {
    volatile sl_call_status_t  status;        // status of the current task
    uint32_t                   func_id;       // function id to be called (index to the call table)
    void*                      func_data;     // data to be passed to the function 
    sgx_status_t               ret_code;      // return code of the function 
};

#define SL_INVALID_FUNC_ID ((uint32_t)-1)

struct sl_call_mngr {
    sl_call_type_t          type;           // type of the call manager (ECALL / OCALL)
    struct sl_siglines      siglns;         // signal lines to pass task request from/to trusted/untrusted side
    struct sl_call_task*    tasks;          // array of tasks  
    const sl_call_table_t*  call_table;     // functions call table 
};

#pragma pack(pop)


/********************************************************************************************************
                                  ******    Call Manager   ******
                     
siglns:
                            1==pending
                      +———————————————————————————————————————————————————————+
        event_lines - | 0 | 0 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
                      +———————————————————————————————————————————————————————+
                                ^
                                |     bit index (i.e. line) as array index        
                                |---------------------------------------------------
                                |                                                  |
                      +———————————————————————————————————————————————————————+    |
        free_lines -  | 1 | 1 | 0 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |    |
                      +———————————————————————————————————————————————————————+    |
                             0==line in use              1==line is free           |
                                                                                   |
                     ---------------------------------------------------------------
                     |
tasks:               |        array of tasks
                     |     +——————————————————————+
                     |     |                      |       
                     |     +——————————————————————+   /+————————————————+
                     |     |                      |  / |  status        |  
                     |     +——————————————————————+ /  |  func_id == 0--|-----
                     ----->|                      |<   |  func_data     |    |
                           +——————————————————————+ \  |  ret_code      |    |
                           |                      |  \ |                |    |
                           +——————————————————————+   \+————————————————+    |
                           |                      |                          |  
                           +——————————————————————+                          | index to call_table
                                                                             |
                                                                             |    
call_table.funcs[]                                                           |
                          array of function pointers                         |
                           +——————————————————————+                          |
                           |       func1          |<--------------------------
                           +——————————————————————+
                           |        NULL          |
                           +——————————————————————+
                           |       func2          |
                           +——————————————————————+
                           |       func4          |
                           +——————————————————————+
                           |        NULL          |
                           +——————————————————————+


For ECALL manager all of the data is allocated outside the enclave, except call_table
For OCALL manager all of the data is allocated outside the enclave, except siglines.free_lines

Flow of the swtichless call request (sending threads): 
      1) scan the free_lines bitmap for a bit with value==1, atomically change bit to 0, save the bit index => line
      2) fill the data in tasks[line]
      3) set status of tasks[line] to SL_SUBMITTED
      4) set atomically bit[line] to 1 in event_lines bitmap. i.e. signal to other side that the task is ready 
      5) start polling status of tasks[line] for specified number of tries
         5.1) if tasks[line] status has not been changed, revoke signal by atomically changing bit[line] to 0 in event_lines bitmap
         5.2) return to caller
      6) wait till the status in tasks[line] changes to SL_DONE (polling)
      7) get the return code from tasks[line]
      8) set atomically bit[line] to 1 in free_lines bitmap

Flow of processing the switchless call request (in the loop, worker threads):
      1) scan the event_lines bitmap for a bit with value==1, atomically change bit to 0, save the bit index => line
      2) set status of tasks[line] to SL_ACCEPTED
      3) execute function using func_id as index to call_table.funcs[], save return code in tasks[line]
      4) set status of tasks[line] to SL_DONE 


**************************************************************************************************************/


__BEGIN_DECLS

#ifndef SL_INSIDE_ENCLAVE /* Untrusted */

uint32_t sl_call_mngr_init(struct sl_call_mngr* mngr, 
                           sl_call_type_t type, 
                           uint32_t max_pending_ocalls);

void sl_call_mngr_destroy(struct sl_call_mngr* mngr);

#else /* Trusted */

uint32_t sl_call_mngr_clone(struct sl_call_mngr* mngr,
                            const struct sl_call_mngr* untrusted);

#endif /* SL_INSIDE_ENCLAVE */

static inline sl_call_type_t sl_call_mngr_get_type(struct sl_call_mngr* mngr) {
    return mngr->type;
}


__END_DECLS

#endif /* _SL_FCALL_MNGR_H_ */
