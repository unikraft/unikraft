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
/** File: local_cache.h 
 *  
 * Description: Implementation of local cache for PCK certificate chain
 *
 */
#ifndef LOCALCACHE_H_
#define LOCALCACHE_H_
#pragma once

#include "qcnl_config.h"
#include "se_memcpy.h"
#include "qcnl_util.h"
#include <list>
#include <time.h>
#include <unordered_map>
#include <vector>

using namespace std;

template <typename Key, typename Value>
class MemoryCache {
private:
    list<Key> keys_;
    unordered_map<Key, pair<Value, typename list<Key>::iterator>> map_;
    size_t size_;

public:
    // Set default cache size to 5. In fact 1 is enough for PCK certchain. 
    // We set 5 here tentatively for future expansion
    MemoryCache() : size_(5) {} 

    void set(const Key key, const Value value) {
        auto pos = map_.find(key);
        if (pos == map_.end()) {
            keys_.push_front(key);
            map_[key] = {value, keys_.begin()};
            if (map_.size() > size_) {
                map_.erase(keys_.back());
                keys_.pop_back();
            }
        } else {
            keys_.erase(pos->second.second);
            keys_.push_front(key);
            map_[key] = {value, keys_.begin()};
        }
    }

    bool get(const Key key, Value &value) {
        auto pos = map_.find(key);
        if (pos == map_.end())
            return false;
        keys_.erase(pos->second.second);
        keys_.push_front(key);
        map_[key] = {pos->second.first, keys_.begin()};
        value = pos->second.first;
        return true;
    }
};

struct CacheItemHeader {
    time_t expiry;
};

// (key, value) pair, where
//    Cache Key = QE_ID || CPU_SVN || PCE_SVN || PCE_ID
//    Cache value = CacheItemHeader || sgx_ql_config_t
class LocalMemCache {
private:
    //
    MemoryCache<string, vector<uint8_t>> m_cache;

    //
    string pck_cert_id_to_key(const sgx_ql_pck_cert_id_t *p_pck_cert_id) {
        string key = "";

        if (!p_pck_cert_id) {
            return "";
        }

        if (!concat_string_with_hex_buf(key, p_pck_cert_id->p_qe3_id, p_pck_cert_id->qe3_id_size))
            return "";

        if (!concat_string_with_hex_buf(key, reinterpret_cast<const uint8_t *>(p_pck_cert_id->p_platform_cpu_svn), sizeof(sgx_cpu_svn_t)))
            return "";

        if (!concat_string_with_hex_buf(key, reinterpret_cast<const uint8_t *>(p_pck_cert_id->p_platform_pce_isv_svn), sizeof(sgx_isv_svn_t)))
            return "";

        if (!concat_string_with_hex_buf(key, reinterpret_cast<const uint8_t *>(&p_pck_cert_id->pce_id), sizeof(p_pck_cert_id->pce_id)))
            return "";

        return key;
    }

    bool is_cache_item_expired(time_t expiry) {
        time_t current_time = time(NULL);

        if (current_time == ((time_t)-1) || current_time >= expiry)
            return true;

        return false;
    }

public:
    static LocalMemCache &Instance() {
        static LocalMemCache myInstance;
        return myInstance;
    }

    LocalMemCache(LocalMemCache const &) = delete;
    LocalMemCache(LocalMemCache &&) = delete;
    LocalMemCache &operator=(LocalMemCache const &) = delete;
    LocalMemCache &operator=(LocalMemCache &&) = delete;

    bool get_pck_cert_chain(const sgx_ql_pck_cert_id_t *p_pck_cert_id,
                            sgx_ql_config_t **pp_quote_config) {
        if (!p_pck_cert_id)
            return false;

        string cache_key = pck_cert_id_to_key(p_pck_cert_id);
        if (cache_key.empty())
            return false;

        vector<uint8_t> value;
        bool ret = false;
        const uint32_t ql_config_header_size = sizeof(sgx_ql_config_t) - sizeof(uint8_t *);

        do {
            if (!m_cache.get(cache_key, value))
                break;

            if (value.size() < sizeof(CacheItemHeader) + ql_config_header_size)
                break;

            // Check expiry
            CacheItemHeader cache_header = {0};
            if (memcpy_s(&cache_header, sizeof(CacheItemHeader), value.data(), sizeof(CacheItemHeader)) != 0)
                break;
            if (is_cache_item_expired(cache_header.expiry))
                break;

            const uint8_t *p_ql_config_cache = value.data() + sizeof(CacheItemHeader);

            *pp_quote_config = (sgx_ql_config_t *)malloc(sizeof(sgx_ql_config_t));
            if (*pp_quote_config == NULL)
                break;

            if (memcpy_s(*pp_quote_config, ql_config_header_size, p_ql_config_cache, ql_config_header_size) != 0)
                break;

            if ((*pp_quote_config)->cert_data_size != value.size() - sizeof(CacheItemHeader) - ql_config_header_size)
                break;

            (*pp_quote_config)->p_cert_data = (uint8_t *)malloc((*pp_quote_config)->cert_data_size);
            if (!(*pp_quote_config)->p_cert_data)
                break;

            if (memcpy_s((*pp_quote_config)->p_cert_data, (*pp_quote_config)->cert_data_size,
                         p_ql_config_cache + ql_config_header_size, (*pp_quote_config)->cert_data_size) != 0)
                break;

            ret = true;
        } while (0);

        if (!ret) {
            sgx_qcnl_free_pck_cert_chain(*pp_quote_config);
        }

        return ret;
    }

    void set_pck_cert_chain(const sgx_ql_pck_cert_id_t *p_pck_cert_id,
                            sgx_ql_config_t **pp_quote_config) {
        if (!p_pck_cert_id) 
            return;
            
        string cache_key = pck_cert_id_to_key(p_pck_cert_id);
        if (cache_key.empty())
            return;

        time_t current_time = time(NULL);
        if (current_time == ((time_t)-1))
            return;
        CacheItemHeader cache_header = {0};
        cache_header.expiry = current_time + QcnlConfig::Instance()->getCacheExpireHour() * 3600;

        vector<uint8_t> value;
        // Append cache header
        uint8_t *p_data = reinterpret_cast<uint8_t *>(&cache_header);
        value.insert(value.end(), p_data, p_data + sizeof(cache_header));

        // Append sgx_ql_config_t strcture header
        p_data = reinterpret_cast<uint8_t *>(*pp_quote_config);
        const uint32_t ql_config_header_size = sizeof(sgx_ql_config_t) - sizeof(uint8_t *);
        value.insert(value.end(), p_data, p_data + ql_config_header_size);

        // Append certificate data
        value.insert(value.end(), (*pp_quote_config)->p_cert_data, (*pp_quote_config)->p_cert_data + (*pp_quote_config)->cert_data_size);

        m_cache.set(cache_key, value);
    }

protected:
    LocalMemCache() {}
    ~LocalMemCache() {}
};

#endif // LOCALCACHE_H_