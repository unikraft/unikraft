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


#include "aesm_long_lived_thread.h"
#include "oal/internal_log.h"
#include "se_time.h"
#include "se_wrapper.h"
#include <time.h>
#include <assert.h>

ae_error_t BaseThreadIOCache::start(BaseThreadIOCache *&out_ioc, uint32_t timeout_value)
{
    return get_thread().set_thread_start(this, out_ioc, timeout_value);
}

void BaseThreadIOCache::deref(void)
{
    get_thread().deref(this);
}

void BaseThreadIOCache::set_status_finish(void)
{
    get_thread().set_status_finish(this);
}

//This is thread entry wrapper for all threads
static ae_error_t aesm_long_lived_thread_entry(aesm_thread_arg_type_t arg)
{
    BaseThreadIOCache *cache=(BaseThreadIOCache *)arg;
    ae_error_t ae_err = cache->entry();
    cache->set_status_finish();
    return ae_err;
}

bool ThreadStatus::find_or_insert_iocache(BaseThreadIOCache* ioc, BaseThreadIOCache *&out_ioc)
{
    AESMLogicLock locker(thread_mutex);
    std::list<BaseThreadIOCache *>::reverse_iterator it;
    out_ioc=NULL;
    if(thread_state == ths_stop){
        AESM_DBG_TRACE("thread %p has been stopped and ioc %p not inserted", this,ioc);
        delete ioc;
        return false;//never visit any item after thread is stopped
    }
    time_t cur=time(NULL);
    AESM_DBG_TRACE("cache size %d",(int)output_cache.size());
    BaseThreadIOCache *remove_candidate = NULL;
    for(it=output_cache.rbegin();it!=output_cache.rend();++it){//visit the cache in reverse order so that the newest item will be visited firstly
        BaseThreadIOCache *pioc=*it;
        if((pioc->status==ioc_idle)&&(pioc->timeout<cur)){
            if(pioc->ref_count==0&&remove_candidate==NULL){
                remove_candidate = pioc;
            }
            continue;//value timeout
        }
        if(*pioc==*ioc){//matched value find
            pioc->ref_count++;//reference it
            AESM_DBG_TRACE("IOC %p matching input IOC %p (ref_count:%d,status:%d,timeout:%d) in thread %p",pioc, ioc,(int)pioc->ref_count,(int)pioc->status, (int)pioc->timeout, this);
            out_ioc= pioc;
            delete ioc;
            return false;
        }
    }
    if(thread_state == ths_busy){//It is not permitted to insert in busy status
        AESM_DBG_TRACE("thread busy when trying insert input ioc %p",ioc);
        delete ioc;
        return false;
    }
    if(remove_candidate!=NULL){
        output_cache.remove(remove_candidate);
        delete remove_candidate;
    }
    if(output_cache.size()>=MAX_OUTPUT_CACHE){
        std::list<BaseThreadIOCache *>::iterator fit;
        bool erased=false;
        for(fit = output_cache.begin(); fit!=output_cache.end();++fit){
            BaseThreadIOCache *pioc=*fit;
            if(pioc->ref_count==0){//find a not timeout item to remove
                assert(pioc->status==ioc_idle);
                AESM_DBG_TRACE("erase idle ioc %p", pioc);
                output_cache.erase(fit);
                erased = true;
                AESM_DBG_TRACE("thread %p cache size %d",this, output_cache.size());
                delete pioc;
                break;
            }
        }
        if(!erased){//no item could be removed
            AESM_DBG_TRACE("no free ioc found and cannot insert ioc %p",ioc);
            delete ioc;
            return false;//similar as busy status
        }
    }
    output_cache.push_back(ioc);
    out_ioc = cur_iocache = ioc;
    cur_iocache->ref_count=2;//initialize to be refenced by parent thread and the thread itself
    thread_state = ths_busy;//mark thread to be busy that the thread to be started
    status_clock = se_get_tick_count();
    AESM_DBG_TRACE("successfully add ioc %p (status=%d,timeout=%d) into thread %p",out_ioc, (int)out_ioc->status, (int)out_ioc->timeout, this);
    return true;
}

ThreadStatus::ThreadStatus():output_cache()
{
    thread_state = ths_idle;
    status_clock = 0;
    cur_iocache = NULL;
}



void ThreadStatus::stop_thread(uint64_t stop_tick_count)
{
    //change state to stop
    thread_mutex.lock();
    thread_state = ths_stop;

    do{
        std::list<BaseThreadIOCache *>::iterator it;
        for(it=output_cache.begin(); it!=output_cache.end();++it){
           BaseThreadIOCache *p=*it;
           if(p->status != ioc_stop){//It has not been processed
               p->status = ioc_stop;
               break;
           }
        }
        if(it!=output_cache.end()){//found item to stop
           BaseThreadIOCache *p=*it;
           p->ref_count++;
           thread_mutex.unlock();
           wait_iocache_timeout(p, stop_tick_count);
           thread_mutex.lock();
        }else{
            break;
        }
    }while(1);

    thread_mutex.unlock();
    //This function should only be called at AESM exit
    //Leave memory leak here is OK and all pointer to BaseThreadIOCache will not be released
}

ae_error_t ThreadStatus::wait_for_cur_thread(uint64_t millisecond)
{
    BaseThreadIOCache *ioc=NULL;
    uint64_t stop_tick_count;
    if(millisecond == AESM_THREAD_INFINITE){
        stop_tick_count = THREAD_INFINITE_TICK_COUNT;
    }else{
        stop_tick_count = se_get_tick_count() + (millisecond*se_get_tick_count_freq()+500)/1000;
    }
    thread_mutex.lock();
    if(cur_iocache!=NULL){
        ioc = cur_iocache;
        ioc->ref_count++;
    }
    thread_mutex.unlock();
    if(ioc!=NULL){
        return wait_iocache_timeout(ioc, stop_tick_count);
    }
    return AE_SUCCESS;
}

ae_error_t ThreadStatus::wait_iocache_timeout(BaseThreadIOCache* ioc, uint64_t stop_tick_count)
{
    ae_error_t ae_ret=AE_SUCCESS;
    uint64_t cur_tick_count = se_get_tick_count();
    uint64_t freq = se_get_tick_count_freq();
    bool need_wait=false;
    aesm_thread_t handle=NULL;
    thread_mutex.lock();
    if(ioc->thread_handle!=NULL&&(cur_tick_count<stop_tick_count||stop_tick_count==THREAD_INFINITE_TICK_COUNT)){
        AESM_DBG_TRACE("wait for busy ioc %p(refcount=%d)",ioc,ioc->ref_count);
        need_wait = true;
        handle = ioc->thread_handle;
    }
    thread_mutex.unlock();
    if(need_wait){
        unsigned long diff_time;
        if(stop_tick_count == THREAD_INFINITE_TICK_COUNT){
            diff_time = AESM_THREAD_INFINITE;
        }else{
            double wtime=(double)(stop_tick_count-cur_tick_count)*1000.0/(double)freq;
            diff_time = (unsigned long)(wtime+0.5);
        }
        ae_ret= aesm_wait_thread(handle, &ae_ret, diff_time);
    }
    deref(ioc);
    return ae_ret;
}

void ThreadStatus::deref(BaseThreadIOCache *ioc)
{
    aesm_thread_t handle = NULL;
    time_t cur=time(NULL);
    {
        AESMLogicLock locker(thread_mutex);
        AESM_DBG_TRACE("deref ioc %p (ref_count=%d,status=%d,timeout=%d) of thread %p",ioc,(int)ioc->ref_count,(int)ioc->status,(int)ioc->timeout, this);
        --ioc->ref_count;
        if(ioc->ref_count == 0){//try free the thread handle now
            handle = ioc->thread_handle;
            ioc->thread_handle = NULL;
            if(ioc->status == ioc_busy){
                ioc->status = ioc_idle;
            }
            AESM_DBG_TRACE("free thread handle for ioc %p",ioc);
        }
        if(ioc->ref_count==0 &&(ioc->status==ioc_stop||ioc->timeout<cur)){
            AESM_DBG_TRACE("free ioc %p",ioc);
            output_cache.remove(ioc);
            AESM_DBG_TRACE("thread %p cache's size is %d",this, (int)output_cache.size());
            delete ioc;
        }
    }
    if(handle!=NULL){
        aesm_free_thread(handle);
    }
}

ae_error_t ThreadStatus::set_thread_start(BaseThreadIOCache* ioc, BaseThreadIOCache *&out_ioc, uint32_t timeout)
{
    ae_error_t ae_ret = AE_SUCCESS;
    ae_error_t ret = AE_FAILURE;
    out_ioc=NULL;
    bool fork_required = find_or_insert_iocache(ioc, out_ioc);
    if(fork_required){
        ae_ret = aesm_create_thread(aesm_long_lived_thread_entry, (aesm_thread_arg_type_t)out_ioc, &out_ioc->thread_handle);
        if (ae_ret != AE_SUCCESS)
        {
            AESM_DBG_TRACE("fail to create thread for ioc %p",out_ioc);
            AESMLogicLock locker(thread_mutex);
            thread_state = ths_idle;
            out_ioc->status = ioc_idle;//set to finished status
            cur_iocache = NULL;
            deref(out_ioc);
            return ae_ret;
        }else{
            AESM_DBG_TRACE("succ create thread %p for ioc %p",this, out_ioc);
        }
    }

    if(out_ioc == NULL){
        AESM_DBG_TRACE("no ioc created for input ioc %p in thread %p",ioc, this);
        return OAL_THREAD_TIMEOUT_ERROR;
    }

    {//check whether thread has been finished
        AESMLogicLock locker(thread_mutex);
        if(out_ioc->status!=ioc_busy){//job is done
            AESM_DBG_TRACE("job done for ioc %p (status=%d,timeout=%d,ref_count=%d) in thread %p",out_ioc, (int)out_ioc->status,(int)out_ioc->timeout,(int)out_ioc->ref_count,this);
            return AE_SUCCESS;
        }
    }

    if(timeout >= AESM_THREAD_INFINITE ){
        ae_ret = aesm_join_thread(out_ioc->thread_handle, &ret);
    }else{
        uint64_t now = se_get_tick_count();
        double timediff = static_cast<double>(timeout) - (static_cast<double>(now - status_clock))/static_cast<double>(se_get_tick_count_freq()) *1000;
        if (timediff <= 0.0) {
            AESM_DBG_ERROR("long flow thread timeout");
            return OAL_THREAD_TIMEOUT_ERROR;
        }
        else{
            AESM_DBG_TRACE("timeout:%u,timediff: %f", timeout,timediff);
            ae_ret = aesm_wait_thread(out_ioc->thread_handle, &ret, (unsigned long)timediff);
        }
    }
    AESM_DBG_TRACE("wait for ioc %p (status=%d,timeout=%d,ref_count=%d) result:%d",out_ioc,(int)out_ioc->status,(int)out_ioc->timeout,(int)out_ioc->ref_count, ae_ret);
    return ae_ret;
};

#define TIMEOUT_SHORT_TIME    60
#define TIMEOUT_FOR_A_WHILE   (5*60)
#define TIMEOUT_LONG_TIME     (3600*24) //at most once every day
static time_t get_timeout_via_ae_error(ae_error_t ae)
{
    time_t cur=time(NULL);
    switch(ae){
    case AE_SUCCESS:
    case OAL_PROXY_SETTING_ASSIST:
    case OAL_NETWORK_RESEND_REQUIRED:
        return cur-1;//always timeout, the error code will never be reused
    case PVE_INTEGRITY_CHECK_ERROR:
    case OAL_NETWORK_UNAVAILABLE_ERROR:
    case OAL_NETWORK_BUSY:
    case PVE_SERVER_BUSY_ERROR:
        return cur+TIMEOUT_SHORT_TIME; //retry after short time
    case QE_REVOKED_ERROR:
    case PVE_REVOKED_ERROR:
    case PVE_MSG_ERROR:
    case PVE_PERFORMANCE_REKEY_NOT_SUPPORTED:
    case PSW_UPDATE_REQUIRED:
        return cur+TIMEOUT_LONG_TIME;
    default:
        return cur+TIMEOUT_SHORT_TIME;//retry quicky for unknown error
    }
}

void ThreadStatus::set_status_finish(BaseThreadIOCache* ioc)
{
    aesm_thread_t handle = NULL;
    {
        AESMLogicLock locker(thread_mutex);
        assert(thread_state==ths_busy||thread_state==ths_stop);
        assert(ioc->status == ioc_busy);
        AESM_DBG_TRACE("set finish status for ioc %p(status=%d,timeout=%d,ref_count=%d) of thread %p",ioc, (int)ioc->status,(int)ioc->timeout,(int)ioc->ref_count,this);
        if(thread_state==ths_busy){
            AESM_DBG_TRACE("set thread %p to idle", this);
            thread_state=ths_idle;
            cur_iocache = NULL;
        }
        ioc->status=ioc_idle;
        ioc->ref_count--;
        ioc->timeout = get_timeout_via_ae_error(ioc->ae_ret);
        if(ioc->ref_count==0){//try free thread handle
            handle = ioc->thread_handle;
            ioc->thread_handle = NULL;
            AESM_DBG_TRACE("thread handle release for ioc %p and status to idle of thread %p",ioc, this);
        }
    }
    if(handle!=NULL){
        aesm_free_thread(handle);
    }
}

bool ThreadStatus::query_status_and_reset_clock(void)
{
    AESMLogicLock locker(thread_mutex);
    if(thread_state == ths_busy || thread_state == ths_stop)
        return false;
    status_clock = se_get_tick_count();
    return true;
}
