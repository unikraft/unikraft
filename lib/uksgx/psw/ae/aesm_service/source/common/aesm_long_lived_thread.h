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

#ifndef _AESM_LONG_LIVED_THREAD_H_
#define _AESM_LONG_LIVED_THREAD_H_
#include "aesm_logic.h"
#include <list>

#define AESM_STOP_TIMEOUT (60*1000) /*waiting for 1 minute at most*/

enum _thread_state
{
    ths_idle,
    ths_busy,
    ths_stop//The thread is to be stopped and no new job will be accepted
};

enum _io_cache_state
{
    ioc_idle, //thread has been finished
    ioc_busy, //thread not finished yet
    ioc_stop  //thread stop required
};

#define MAX_OUTPUT_CACHE 50
#define THREAD_INFINITE_TICK_COUNT 0xFFFFFFFFFFFFFFFFLL
class ThreadStatus;
class BaseThreadIOCache;
typedef ae_error_t (*long_lived_thread_func_t)(BaseThreadIOCache *cache);

//Base class for cached data of each thread to fork
class BaseThreadIOCache:private Uncopyable{
    time_t timeout; //The data will timeout after the time if the state is not busy
    int ref_count; //ref_count is used to track how many threads are currently referencing the data
    _io_cache_state status;
    //handle of the thread, some thread will be waited by other threads so that we could not 
    //   free the handle until all other threads have got notification that the thread is terminated
    aesm_thread_t thread_handle;
    friend class ThreadStatus;
protected:
    ae_error_t ae_ret;
    BaseThreadIOCache():ref_count(0),status(ioc_busy){
        timeout=0;
        thread_handle=NULL;
        ae_ret = AE_FAILURE;
    }
    virtual ThreadStatus& get_thread()=0;
public:
    virtual ae_error_t entry(void)=0;
    virtual bool operator==(const BaseThreadIOCache& oc)const=0;
    ae_error_t start(BaseThreadIOCache *&out_ioc, uint32_t timeout=THREAD_TIMEOUT);
    void deref(void);
    void set_status_finish();
public:
    virtual ~BaseThreadIOCache(){}
};

class ThreadStatus: private Uncopyable
{
private:
    AESMLogicMutex thread_mutex;
    _thread_state thread_state;
    uint64_t    status_clock;
    BaseThreadIOCache *cur_iocache;
    std::list<BaseThreadIOCache *>output_cache;
protected:
    friend class BaseThreadIOCache;
    //function to look up cached output, there will be no real thread associated with the input ioc
    //If a match is found the input parameter will be free automatically and the matched value is returned
    //return true if a thread will be forked for the out_ioc
    bool find_or_insert_iocache(BaseThreadIOCache* ioc, BaseThreadIOCache *&out_ioc);

public:
    ThreadStatus();
    void set_status_finish(BaseThreadIOCache* ioc);//only called at the end of aesm_long_lived_thread_entry
    void deref(BaseThreadIOCache* iocache);
    ae_error_t wait_iocache_timeout(BaseThreadIOCache* ioc, uint64_t stop_tick_count);

    //create thread and wait at most 'timeout' for the thread to be finished
    // It will first look up whether there is a previous run with same input before starting the thread
    // we should not delete ioc after calling to this function
    ae_error_t set_thread_start(BaseThreadIOCache* ioc,  BaseThreadIOCache *&out_ioc, uint32_t timeout=THREAD_TIMEOUT);

    void stop_thread(uint64_t stop_milli_second);//We need wait for thread to be terminated and all thread_handle in list to be closed

    ~ThreadStatus(){stop_thread(THREAD_INFINITE_TICK_COUNT);}//ThreadStatus instance should be global object. Otherwise, it is possible that the object is destroyed before a thread waiting for and IOCache got notified and causing exception

    ae_error_t wait_for_cur_thread(uint64_t millisecond);

    //function to query whether current thread is idle,
    //if it is idle, return true and reset clock to current clock value
    bool query_status_and_reset_clock(void);
};

#define INIT_THREAD(cache_type, timeout, init_list) \
    BaseThreadIOCache *ioc = new cache_type init_list; \
    BaseThreadIOCache *out_ioc = NULL; \
    ae_error_t ae_ret = AE_FAILURE; \
    ae_ret = ioc->start(out_ioc, (uint32_t)(timeout)); \
    if(ae_ret != AE_SUCCESS){ \
        if(out_ioc!=NULL){out_ioc->deref();}\
        return ae_ret; \
    }\
    assert(out_ioc!=NULL);\
    cache_type *pioc = dynamic_cast<cache_type *>(out_ioc);\
    assert(pioc!=NULL);
    //now the thread has finished it's execution and we could read result without lock
#define COPY_OUTPUT(x)  x=pioc->x
#define FINI_THREAD() \
    ae_ret = pioc->ae_ret;\
    pioc->deref();/*derefence the cache object after usage of it*/ \
    return ae_ret;

#endif

