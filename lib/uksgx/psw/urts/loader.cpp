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


#include "se_wrapper.h"
#include "se_error_internal.h"
#include "arch.h"
#include "util.h"
#include "loader.h"
#include "se_page_attr.h"
#include "enclave.h"
#include "enclave_creator.h"
#include "routine.h"
#include "sgx_attributes.h"
#include "se_vendor.h"
#include "se_detect.h"
#include "binparser.h"
#include "metadata.h"
#include "edmm_utility.h"
#include <assert.h>
#include <vector>
#include <tuple>
#include <algorithm>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sys/mman.h>
#include "sgx_enclave_common.h"

const char * layout_id_str[] = {
    "Undefined",
    "HEAP_MIN",
    "HEAP_INIT",
    "HEAP_MAX",
    "TCS",
    "TD",
    "SSA",
    "STACK_MAX",
    "STACK_MIN",
    "THREAD_GROUP",
    "GUARD",
    "HEAP_DYN_MIN",
    "HEAP_DYN_INIT",
    "HEAP_DYN_MAX",
    "TCS_DYN",
    "TD_DYN",
    "SSA_DYN",
    "STACK_DYN_MAX",
    "STACK_DYN_MIN",
    "THREAD_GROUP_DYN",
    "RSRV_MIN",
    "RSRV_INIT",
    "RSRV_MAX"
};

// enclave creator instance
extern EnclaveCreator* g_enclave_creator;

EnclaveCreator* get_enclave_creator(void)
{
    return g_enclave_creator;
}

CLoader::CLoader(uint8_t *mapped_file_base, BinParser &parser)
    : m_mapped_file_base(mapped_file_base)
    , m_enclave_id(0)
    , m_start_addr(NULL)
    , m_elrange_start_address(0)
    , m_elrange_size(0)
    , m_metadata(NULL)
    , m_parser(parser)
{
    memset(&m_secs, 0, sizeof(m_secs));
}

CLoader::~CLoader()
{

}

sgx_enclave_id_t CLoader::get_enclave_id() const
{
    return m_enclave_id;
}

const void* CLoader::get_start_addr() const
{
    return m_start_addr;
}

uint64_t CLoader::get_elrange_start_addr() const
{
    return m_elrange_start_address;
}

uint64_t CLoader::get_elrange_size() const
{
    return m_elrange_size;
}

const std::vector<std::pair<tcs_t *, bool>>& CLoader::get_tcs_list() const
{
    return m_tcs_list;
}

const secs_t& CLoader::get_secs() const
{
    return m_secs;
}

void* CLoader::get_symbol_address(const char * const symbol)
{
    uint64_t rva = m_parser.get_symbol_rva(symbol);
    if(0 == rva)
        return NULL;
    return GET_PTR(void, m_start_addr, rva);
}

// is_relocation_page returns true if the specified RVA is a writable relocation page based on the bitmap.
bool CLoader::is_relocation_page(const uint64_t rva, std::vector<uint8_t> *bitmap)
{
    uint64_t page_frame = rva >> SE_PAGE_SHIFT;
    //NOTE:
    //  Current enclave size is not beyond 128G, so the type-casting from (uint64>>15) to (size_t) is OK.
    //  In the future, if the max enclave size is extended to beyond (1<<49), this type-casting will not work.
    //  It only impacts the enclave signing process. (32bit signing tool to sign 64 bit enclaves)
    size_t index = (size_t)(page_frame / 8);
    if(bitmap && (index < bitmap->size()))
    {
        return ((*bitmap)[index] & (1 << (page_frame % 8)));
    }
    return false;
}

int CLoader::build_mem_region(const section_info_t &sec_info)
{
    int ret = SGX_SUCCESS;
    uint64_t offset = 0;
    sec_info_t sinfo;
    memset(&sinfo, 0, sizeof(sinfo));

    // Build pages of the section that are contain initialized data.  Each page
    // needs to be added individually as the page may hold relocation data, in
    // which case the page needs to be marked writable.
    while(offset < sec_info.raw_data_size)
    {
        uint64_t rva = sec_info.rva + offset;
        uint64_t size = MIN((SE_PAGE_SIZE - PAGE_OFFSET(rva)), (sec_info.raw_data_size - offset));
        sinfo.flags = sec_info.flag;

        if(is_relocation_page(rva, sec_info.bitmap) && !(sec_info.flag & SI_FLAG_W))
        {
            sinfo.flags = sec_info.flag | SI_FLAG_W;
            assert(g_enclave_creator != NULL);
            if(g_enclave_creator->use_se_hw() == true)
            {
                ret = mprotect((void*)(TRIM_TO_PAGE(rva) + (uint64_t)m_start_addr), SE_PAGE_SIZE, 
                               (int)(sinfo.flags & SI_MASK_MEM_ATTRIBUTE));
                if(ret != 0)
                {
                    SE_TRACE(SE_TRACE_WARNING, "mprotect(rva=0x%llx, len=%d, flags=%d) failed\n",
                             rva, SE_PAGE_SIZE, int(sinfo.flags & SI_MASK_MEM_ATTRIBUTE));
                    return SGX_ERROR_UNEXPECTED;
                }
            }
        }

        if (size == SE_PAGE_SIZE)
            ret = build_pages(rva, size, sec_info.raw_data + offset, sinfo, ADD_EXTEND_PAGE);
        else
            ret = build_partial_page(rva, size, sec_info.raw_data + offset, sinfo, ADD_EXTEND_PAGE);
        if(SGX_SUCCESS != ret)
            return ret;

        // only the first time that rva may be not page aligned
        offset += SE_PAGE_SIZE - PAGE_OFFSET(rva);
    }
    
    // Add any remaining uninitialized data.  We can call build_pages directly
    // even if there are partial pages since the source is null, i.e. everything
    // is filled with '0'.  Uninitialied data cannot be a relocation table, ergo
    // there is no need to check the relocation bitmap.
    if(sec_info.virtual_size > offset)
    {
        uint64_t rva = sec_info.rva + offset;
        size_t size = (size_t)(ROUND_TO_PAGE(sec_info.virtual_size - offset + PAGE_OFFSET(rva)));

        rva = TRIM_TO_PAGE(rva);

        sinfo.flags = sec_info.flag;
        if(SGX_SUCCESS != (ret = build_pages(rva, size, 0, sinfo, ADD_EXTEND_PAGE)))
            return ret;
    }

    return SGX_SUCCESS;
}

int CLoader::build_sections(std::vector<uint8_t> *bitmap)
{
    int ret = SGX_SUCCESS;
    std::vector<Section*> sections = m_parser.get_sections();
    uint64_t max_rva =0;
    Section* last_section = NULL;

    for(unsigned int i = 0; i < sections.size() ; i++)
    {
        
        
        
        if((META_DATA_MAKE_VERSION(SGX_1_5_MAJOR_VERSION,SGX_1_5_MINOR_VERSION ) == m_metadata->version) &&
            (last_section != NULL) &&
           (ROUND_TO_PAGE(last_section->virtual_size() + last_section->get_rva()) < ROUND_TO_PAGE(ROUND_TO_PAGE(last_section->virtual_size()) + last_section->get_rva())) &&
           (ROUND_TO_PAGE(last_section->get_rva() + last_section->virtual_size()) < (sections[i]->get_rva() & (~(SE_PAGE_SIZE - 1)))))
        {
            size_t size = SE_PAGE_SIZE;
            sec_info_t sinfo;
            memset(&sinfo, 0, sizeof(sinfo));
            sinfo.flags = last_section->get_si_flags();
            uint64_t rva = ROUND_TO_PAGE(last_section->get_rva() + last_section->virtual_size());
            if(SGX_SUCCESS != (ret = build_pages(rva, size, 0, sinfo, ADD_EXTEND_PAGE)))
                return ret;
        }

        if(sections[i]->get_rva() > max_rva) 
        {
            max_rva = sections[i]->get_rva();
            last_section = sections[i];
        }

        section_info_t sec_info = { sections[i]->raw_data(), sections[i]->raw_data_size(), sections[i]->get_rva(), sections[i]->virtual_size(), sections[i]->get_si_flags(), bitmap };

        if(SGX_SUCCESS != (ret = build_mem_region(sec_info)))
            return ret;
    }
    
    
    
    if((META_DATA_MAKE_VERSION(SGX_1_5_MAJOR_VERSION,SGX_1_5_MINOR_VERSION ) == m_metadata->version) &&
        (last_section != NULL) &&
       (ROUND_TO_PAGE(last_section->virtual_size() + last_section->get_rva()) < ROUND_TO_PAGE(ROUND_TO_PAGE(last_section->virtual_size()) + last_section->get_rva())))
    {
        size_t size = SE_PAGE_SIZE;
        sec_info_t sinfo;
        memset(&sinfo, 0, sizeof(sinfo));
        sinfo.flags = last_section->get_si_flags();
        uint64_t rva = ROUND_TO_PAGE(last_section->get_rva() + last_section->virtual_size());
        if(SGX_SUCCESS != (ret = build_pages(rva, size, 0, sinfo, ADD_EXTEND_PAGE)))
            return ret;
    }

    return SGX_SUCCESS;
}

int CLoader::build_partial_page(const uint64_t rva, const uint64_t size, const void *source, const sec_info_t &sinfo, const uint32_t attr)
{
    // RVA may or may not be aligned.
    uint64_t offset = PAGE_OFFSET(rva);

    // Initialize the page with '0', this serves as both the padding at the start
    // of the page (if it's not aligned) as well as the fill for any unitilized
    // bytes at the end of the page, e.g. .bss data.
    uint8_t page_data[SE_PAGE_SIZE]  __attribute__ ((aligned(4096)));
    memset(page_data, 0, SE_PAGE_SIZE);

    // The amount of raw data may be less than the number of bytes on the page,
    // but that portion of page_data has already been filled (see above).
    memcpy_s(&page_data[offset], (size_t)(SE_PAGE_SIZE - offset), source, (size_t)size);

    // Add the page, trimming the start address to make it page aligned.
    return build_pages(TRIM_TO_PAGE(rva), SE_PAGE_SIZE, page_data, sinfo, attr);
}

int CLoader::build_pages(const uint64_t start_rva, const uint64_t size, const void *source, const sec_info_t &sinfo, const uint32_t attr)
{
    int ret = SGX_SUCCESS;
    uint64_t offset = 0;
    uint64_t rva = start_rva;

    assert(IS_PAGE_ALIGNED(start_rva) && IS_PAGE_ALIGNED(size));

    while(offset < size)
    {
        //call driver to add page;
        if(SGX_SUCCESS != (ret = get_enclave_creator()->add_enclave_page(ENCLAVE_ID_IOCTL, GET_PTR(void, source, 0), rva, sinfo, attr)))
        {
            //if add page failed , we should remove enclave somewhere;
            return ret;
        }
        offset += SE_PAGE_SIZE;
        rva += SE_PAGE_SIZE;
    }

    return SGX_SUCCESS;
}

int CLoader::post_init_action(layout_t *layout_start, layout_t *layout_end, uint64_t delta)
{
    int ret = SGX_SUCCESS;

    for(layout_t *layout = layout_start; layout < layout_end; layout++)
    {
        if (!IS_GROUP_ID(layout->group.id) && (layout->entry.attributes & PAGE_ATTR_POST_REMOVE))
        {
            uint64_t start_addr = layout->entry.rva + delta + (uint64_t)get_start_addr();
            uint64_t page_count = (uint64_t)layout->entry.page_count;
            if (SGX_SUCCESS != (ret = get_enclave_creator()->trim_range(start_addr, start_addr + (page_count << SE_PAGE_SHIFT))))
                return ret;

        }
        else if (IS_GROUP_ID(layout->group.id))
        {
            uint64_t step = 0;
            for(uint32_t j = 0; j < layout->group.load_times; j++)
            {
                step += layout->group.load_step;
                if(SGX_SUCCESS != (ret = post_init_action(&layout[-layout->group.entry_count], layout, step)))
                    return ret;
            }
        }
    }
    return SGX_SUCCESS;
}
  
int CLoader::post_init_action_commit(layout_t *layout_start, layout_t *layout_end, uint64_t delta)
{
    int ret = SGX_SUCCESS;

    for(layout_t *layout = layout_start; layout < layout_end; layout++)
    {
        if (!IS_GROUP_ID(layout->group.id) && (layout->entry.attributes & PAGE_ATTR_POST_REMOVE))
        {
            uint64_t start_addr = layout->entry.rva + delta + (uint64_t)get_start_addr();
            uint64_t page_count = (uint64_t)layout->entry.page_count;

            for (uint64_t i = 0; i < page_count; i++)
            {
                if (SGX_SUCCESS != (ret = get_enclave_creator()->trim_accept(start_addr + (i << SE_PAGE_SHIFT))))
                    return ret;
            }
        }
        else if (IS_GROUP_ID(layout->group.id))
        {
            uint64_t step = 0;
            for(uint32_t j = 0; j < layout->group.load_times; j++)
            {
                step += layout->group.load_step;
                if(SGX_SUCCESS != (ret = post_init_action_commit(&layout[-layout->group.entry_count], layout, step)))
                    return ret;
            }
        }
    }
    return SGX_SUCCESS;
}
      
int CLoader::build_context(const uint64_t start_rva, layout_entry_t *layout)
{
    int ret = SGX_ERROR_UNEXPECTED;
    uint8_t added_page[SE_PAGE_SIZE] __attribute__ ((aligned(4096)));
    sec_info_t sinfo;
    memset(added_page, 0, SE_PAGE_SIZE);
    memset(&sinfo, 0, sizeof(sinfo));
    uint64_t rva = start_rva + layout->rva;

    assert(IS_PAGE_ALIGNED(rva));
    se_trace(SE_TRACE_DEBUG, "\t%s\n", __FUNCTION__);
    se_trace(SE_TRACE_DEBUG, "\tEntry Id     = %4u, %-16s, ", layout->id, layout_id_str[layout->id & ~(GROUP_FLAG)]);
    se_trace(SE_TRACE_DEBUG, "Page Count = %5u, ", layout->page_count);
    se_trace(SE_TRACE_DEBUG, "Attributes = 0x%02X, ", layout->attributes);
    se_trace(SE_TRACE_DEBUG, "Flags = 0x%016llX, ", layout->si_flags);
    se_trace(SE_TRACE_DEBUG, "RVA = 0x%016llX -> ", layout->rva);
    se_trace(SE_TRACE_DEBUG, "RVA = 0x%016llX\n", rva);

    if (layout->attributes & PAGE_ATTR_EADD)
    {
        uint16_t attributes = layout->attributes;
#ifdef SE_SIM
        attributes = attributes & (uint16_t)(~PAGE_ATTR_EREMOVE);
#endif

        if (layout->content_offset)
        {
            if(layout->si_flags == SI_FLAGS_TCS)
            {
                memset(added_page, 0, SE_PAGE_SIZE);
                memcpy_s(added_page, SE_PAGE_SIZE, GET_PTR(uint8_t, m_metadata, layout->content_offset), layout->content_size);

                tcs_t *ptcs = reinterpret_cast<tcs_t*>(added_page);
                ptcs->ossa += rva;
                ptcs->ofs_base += rva;
                ptcs->ogs_base += rva;
                if(!(attributes & PAGE_ATTR_EREMOVE))
                {
                    m_tcs_list.push_back(std::make_pair(GET_PTR(tcs_t, m_start_addr, rva), false));
                }
                sinfo.flags = layout->si_flags;
                if(SGX_SUCCESS != (ret = build_pages(rva, ((uint64_t)layout->page_count) << SE_PAGE_SHIFT, added_page, sinfo, attributes)))
                {
                    return ret;
                }
            }
            else // guard page should not have content_offset != 0 
            {   
                           
                section_info_t sec_info = {GET_PTR(uint8_t, m_metadata, layout->content_offset), layout->content_size, rva, ((uint64_t)layout->page_count) << SE_PAGE_SHIFT, layout->si_flags, NULL};
                if(SGX_SUCCESS != (ret = build_mem_region(sec_info)))
                {
                    return ret;
                }
            }
        }
        else if (layout->si_flags != SI_FLAG_NONE)
        {
            sinfo.flags = layout->si_flags;
           
            void *source = NULL;
            if(layout->content_size)
            {
                for(uint32_t *p = (uint32_t *)added_page; p < GET_PTR(uint32_t, added_page, SE_PAGE_SIZE); p++)
                {
                    *p = layout->content_size;
                }
                source = added_page;
            }
            
            if(SGX_SUCCESS != (ret = build_pages(rva, ((uint64_t)layout->page_count) << SE_PAGE_SHIFT, source, sinfo, layout->attributes)))
            {
                return ret;
            }

        }
    }

    if(layout->attributes & PAGE_ATTR_POST_ADD)
    {
#ifndef SE_SIM
        if(layout->id == LAYOUT_ID_TCS_DYN)
        {
            m_tcs_list.push_back(std::make_pair(GET_PTR(tcs_t, m_start_addr, rva), true));
        }
#endif
    }
    return SGX_SUCCESS;
}
int CLoader::build_contexts(layout_t *layout_start, layout_t *layout_end, uint64_t delta)
{
    int ret = SGX_ERROR_UNEXPECTED;
    for(layout_t *layout = layout_start; layout < layout_end; layout++)
    {
        se_trace(SE_TRACE_DEBUG, "%s, step = 0x%016llX\n", __FUNCTION__, delta);

        if (!IS_GROUP_ID(layout->group.id))
        {
            if(SGX_SUCCESS != (ret = build_context(delta, &layout->entry)))
            {
                return ret;
            }
        }
        else
        {
            se_trace(SE_TRACE_DEBUG, "\tEntry Id(%2u) = %4u, %-16s, ", 0, layout->entry.id, layout_id_str[layout->entry.id & ~(GROUP_FLAG)]);
            se_trace(SE_TRACE_DEBUG, "Entry Count = %4u, ", layout->group.entry_count);
            se_trace(SE_TRACE_DEBUG, "Load Times = %u,    ", layout->group.load_times);
            se_trace(SE_TRACE_DEBUG, "LStep = 0x%016llX\n", layout->group.load_step);

            uint64_t step = 0;
            for(uint32_t j = 0; j < layout->group.load_times; j++)
            {
                step += layout->group.load_step;
                if(SGX_SUCCESS != (ret = build_contexts(&layout[-layout->group.entry_count], layout, step)))
                {
                    return ret;
                }
            }
        }
    }
    return SGX_SUCCESS;
}

static inline bool is_power_of_two(size_t n)
{
    return (n != 0) && (!(n & (n - 1)));
}


int CLoader::set_elrange_config()
{   
    if(MAJOR_VERSION_OF_METADATA(m_metadata->version) <= SGX_MAJOR_VERSION_GAP)
    {
        return SGX_SUCCESS;
    }
    //elrange is placed at the beginning of data
    data_directory_t* dir = GET_PTR(data_directory_t, m_metadata, offsetof(metadata_t, data));
    if(dir == NULL)
    {
        return SGX_ERROR_UNEXPECTED;
    }
    
    elrange_config_entry_t* elrange_config_entry = GET_PTR(elrange_config_entry_t, m_metadata, dir->offset);
    if(elrange_config_entry == NULL)
    {
        return SGX_ERROR_INVALID_METADATA;
    }
    
    if(elrange_config_entry->elrange_size ==0)
    {
        return SGX_ERROR_INVALID_METADATA;
    }
    
    //validate the el_range params
    if(elrange_config_entry->elrange_start_address > elrange_config_entry->enclave_image_address)
    {
        return SGX_ERROR_INVALID_METADATA;
    }

    if((elrange_config_entry->elrange_size % SE_PAGE_SIZE != 0) ||
        (elrange_config_entry->elrange_start_address% SE_PAGE_SIZE != 0) ||
        (elrange_config_entry->enclave_image_address% SE_PAGE_SIZE != 0))
    {
        return SGX_ERROR_INVALID_METADATA;
    }

    if(!is_power_of_two(elrange_config_entry->elrange_size))
    {
        return SGX_ERROR_INVALID_METADATA;
    }  

    if((elrange_config_entry->elrange_start_address & (elrange_config_entry->elrange_size -1 )) !=0)
    {
        return SGX_ERROR_INVALID_METADATA;
    }

    uint64_t elrange_end = elrange_config_entry->elrange_start_address + elrange_config_entry->elrange_size;
    if(elrange_end < elrange_config_entry->elrange_start_address || elrange_end < elrange_config_entry->elrange_size)
    {
        return SGX_ERROR_INVALID_METADATA;
    }
    
    if(elrange_config_entry->enclave_image_address >= elrange_end ||
       (elrange_config_entry->enclave_image_address + m_metadata->enclave_size) > elrange_end)
    {
        return SGX_ERROR_INVALID_METADATA;
    }
    
    m_elrange_start_address = elrange_config_entry->elrange_start_address;
    m_elrange_size = elrange_config_entry->elrange_size;
    m_start_addr = reinterpret_cast<void*>(elrange_config_entry->enclave_image_address); 
    
    return SGX_SUCCESS;
}



int CLoader::build_secs(sgx_attributes_t * const secs_attr, sgx_config_id_t *config_id, sgx_config_svn_t config_svn, sgx_misc_attribute_t * const misc_attr)
{
    uint32_t ex_features = 0;
    const void* ex_features_p[32] = { 0 };
    
    memset(&m_secs, 0, sizeof(secs_t)); //should set resvered field of secs as 0.
    //create secs structure.
    m_secs.base = 0;
    m_secs.size = m_metadata->enclave_size;
    m_secs.misc_select = misc_attr->misc_select;

    if(memcpy_s(&m_secs.attributes,  sizeof(m_secs.attributes), secs_attr, sizeof(m_secs.attributes)))
        return SGX_ERROR_UNEXPECTED;
    m_secs.ssa_frame_size = m_metadata->ssa_frame_size;
    if (config_id)
    {
        if (memcpy_s(m_secs.config_id, SGX_CONFIGID_SIZE, config_id, SGX_CONFIGID_SIZE))
            return SGX_ERROR_UNEXPECTED;
    }
    m_secs.config_svn = config_svn;
    
    int ret = set_elrange_config();
    if(ret != SGX_SUCCESS)
    {
        return ret;
    }
    
    EnclaveCreator *enclave_creator = get_enclave_creator();
    if(NULL == enclave_creator)
        return SGX_ERROR_UNEXPECTED;

    enclave_elrange_t enclave_elrange;
    memset(&enclave_elrange, 0, sizeof(enclave_elrange));
    if(m_elrange_size != 0)
    {
        enclave_elrange.elrange_size = m_elrange_size;
        enclave_elrange.enclave_image_address = reinterpret_cast<uint64_t>(m_start_addr);
        enclave_elrange.elrange_start_address = m_elrange_start_address;
        
        ex_features = ENCLAVE_CREATE_EX_EL_RANGE;
        ex_features_p[ENCLAVE_CREATE_EX_EL_RANGE_BIT_IDX] = &enclave_elrange; 
    }
    
    ret = enclave_creator->create_enclave(&m_secs, &m_enclave_id, &m_start_addr, ex_features, ex_features_p);
    if(SGX_SUCCESS == ret)
    {
        SE_TRACE(SE_TRACE_NOTICE, "Enclave start addr. = %p, Size = 0x%llx, %llu KB\n", 
                 m_start_addr, m_metadata->enclave_size, m_metadata->enclave_size/1024);
    }
    // m_secs.mr_enclave value is not set previously
    if(memcpy_s(&m_secs.mr_enclave, sizeof(sgx_measurement_t), &m_metadata->enclave_css.body.enclave_hash, sizeof(sgx_measurement_t)))
        return SGX_ERROR_UNEXPECTED;

    return ret;
}
int CLoader::build_image(SGXLaunchToken * const lc, sgx_attributes_t * const secs_attr, sgx_config_id_t *config_id, sgx_config_svn_t config_svn, le_prd_css_file_t *prd_css_file, sgx_misc_attribute_t * const misc_attr)
{
    int ret = SGX_SUCCESS;


    if(SGX_SUCCESS != (ret = build_secs(secs_attr, config_id, config_svn, misc_attr)))
    {
        SE_TRACE(SE_TRACE_WARNING, "build secs failed\n");
        return ret;
    };

    // read reloc bitmap before patch the enclave file
    // If load_enclave_ex try to load the enclave for the 2nd time,
    // the enclave image is already patched, and parser cannot read the information.
    // For linux, there's no map conflict. We assume load_enclave_ex will not do the retry.
    std::vector<uint8_t> bitmap;
    if(!m_parser.get_reloc_bitmap(bitmap))
        return SGX_ERROR_INVALID_ENCLAVE;

    // patch enclave file
    patch_entry_t *patch_start = GET_PTR(patch_entry_t, m_metadata, m_metadata->dirs[DIR_PATCH].offset);
    patch_entry_t *patch_end = GET_PTR(patch_entry_t, m_metadata, m_metadata->dirs[DIR_PATCH].offset + m_metadata->dirs[DIR_PATCH].size);
    for(patch_entry_t *patch = patch_start; patch < patch_end; patch++)
    {
        memcpy_s(GET_PTR(void, m_parser.get_start_addr(), patch->dst), patch->size, GET_PTR(void, m_metadata, patch->src), patch->size);
    }

    //build sections, copy export function table as well;
    if(SGX_SUCCESS != (ret = build_sections(&bitmap)))
    {
        SE_TRACE(SE_TRACE_WARNING, "build sections failed\n");
        goto fail;
    }

    // build heap/thread context
    SE_TRACE_DEBUG("\n");
    se_trace(SE_TRACE_DEBUG, "\tMetadata Version = 0x%016llX\n", m_metadata->version);
    if (SGX_SUCCESS != (ret = build_contexts(GET_PTR(layout_t, m_metadata, m_metadata->dirs[DIR_LAYOUT].offset), 
                                      GET_PTR(layout_t, m_metadata, m_metadata->dirs[DIR_LAYOUT].offset + m_metadata->dirs[DIR_LAYOUT].size),
                                      0)))
    {
        SE_TRACE(SE_TRACE_WARNING, "build heap/thread context failed\n");
        goto fail;
    }

    //initialize Enclave
    ret = get_enclave_creator()->init_enclave(ENCLAVE_ID_IOCTL, const_cast<enclave_css_t *>(&m_metadata->enclave_css), lc, prd_css_file);
    if(SGX_SUCCESS != ret)
    {
        SE_TRACE(SE_TRACE_WARNING, "init_enclave failed\n");
        goto fail;
    }

    return SGX_SUCCESS;

fail:
    get_enclave_creator()->destroy_enclave(ENCLAVE_ID_IOCTL, m_secs.size);

    return ret;
}
bool CLoader::is_metadata_buffer(uint32_t offset, uint32_t size)
{
    if((offsetof(metadata_t, data) > offset) || (offset >= m_metadata->size))
    {
        return false;
    }
    uint32_t end = offset + size;
    if ((end < offset) || (end < size) || (end > m_metadata->size))
    {
        return false;
    }
    return true;
}
bool CLoader::is_enclave_buffer(uint64_t offset, uint64_t size)
{
    if(offset >= m_metadata->enclave_size)
    {
        return false;
    }
    uint64_t end = offset + size;
    if ((end < offset) || (end < size) || (end > m_metadata->enclave_size))
    {
        return false;
    }
    return true;
}
int CLoader::validate_layout_table()
{
    layout_t *layout_start = GET_PTR(layout_t, m_metadata, m_metadata->dirs[DIR_LAYOUT].offset);
    layout_t *layout_end = GET_PTR(layout_t, m_metadata, m_metadata->dirs[DIR_LAYOUT].offset + m_metadata->dirs[DIR_LAYOUT].size);
    std::vector<std::pair<uint64_t, uint64_t>> rva_vector;
    for (layout_t *layout = layout_start; layout < layout_end; layout++)
    {
        if(!IS_GROUP_ID(layout->entry.id))  // layout entry
        {
            rva_vector.push_back(std::make_pair(layout->entry.rva, ((uint64_t)layout->entry.page_count) << SE_PAGE_SHIFT));
            if(layout->entry.content_offset)
            {
                if(false == is_metadata_buffer(layout->entry.content_offset, layout->entry.content_size))
                {
                    return SGX_ERROR_INVALID_METADATA;
                }
            }
        }
        else // layout group
        {
            if (layout->group.entry_count > (uint32_t)(PTR_DIFF(layout, layout_start)/sizeof(layout_t)))
            {
                return SGX_ERROR_INVALID_METADATA;
            }
            uint64_t load_step = 0;
            for(uint32_t i = 0; i < layout->group.load_times; i++)
            {
                load_step += layout->group.load_step;
                if(load_step > m_metadata->enclave_size)
                {
                    return SGX_ERROR_INVALID_METADATA;
                }
                for(layout_entry_t *entry = &layout[-layout->group.entry_count].entry; entry < &layout->entry; entry++)
                {
                    if(IS_GROUP_ID(entry->id))
                    {
                        return SGX_ERROR_INVALID_METADATA;
                    }
                    rva_vector.push_back(std::make_pair(entry->rva + load_step, ((uint64_t)entry->page_count) << SE_PAGE_SHIFT));
                    // no need to check integer overflow for entry->rva + load_step, because
                    // entry->rva and load_step are less than enclave_size, whose size is no more than 37 bit
                }
            }
        }
    }
    sort(rva_vector.begin(), rva_vector.end());
    for (std::vector<std::pair<uint64_t, uint64_t>>::iterator it = rva_vector.begin(); it != rva_vector.end(); it++)
    {
        if(!IS_PAGE_ALIGNED(it->first))
        {
            return SGX_ERROR_INVALID_METADATA;
        }
        if(false == is_enclave_buffer(it->first, it->second))
        {
            return SGX_ERROR_INVALID_METADATA;
        }
        if((it+1) != rva_vector.end())
        {
            if((it->first+it->second) > (it+1)->first)
            {
                return SGX_ERROR_INVALID_METADATA;
            }
        }
    }
    return SGX_SUCCESS;
}

int CLoader::validate_patch_table()
{
    patch_entry_t *patch_start = GET_PTR(patch_entry_t, m_metadata, m_metadata->dirs[DIR_PATCH].offset);
    patch_entry_t *patch_end = GET_PTR(patch_entry_t, m_metadata, m_metadata->dirs[DIR_PATCH].offset + m_metadata->dirs[DIR_PATCH].size);
    for(patch_entry_t *patch = patch_start; patch < patch_end; patch++)
    {
        if(false == is_metadata_buffer(patch->src, patch->size))
        {
            return SGX_ERROR_INVALID_METADATA;
        }
        if(false == is_enclave_buffer(patch->dst, patch->size))
        {
            return SGX_ERROR_INVALID_METADATA;
        }
    }
    return SGX_SUCCESS;
}
int CLoader::validate_metadata()
{
    if(!m_metadata)
        return SGX_ERROR_INVALID_METADATA;
    uint64_t urts_version = META_DATA_MAKE_VERSION(MAJOR_VERSION,MINOR_VERSION);

    //if the version of metadata does NOT match the version of metadata in urts, we should NOT launch enclave.
    if(MAJOR_VERSION_OF_METADATA(urts_version)%SGX_MAJOR_VERSION_GAP < MAJOR_VERSION_OF_METADATA(m_metadata->version)%SGX_MAJOR_VERSION_GAP)
    {
        SE_TRACE(SE_TRACE_ERROR, "Mismatch between the metadata urts required and the metadata in use.\n");
        return SGX_ERROR_INVALID_VERSION;
    }

    if(m_metadata->tcs_policy > TCS_POLICY_UNBIND)
        return SGX_ERROR_INVALID_METADATA;
    if(m_metadata->ssa_frame_size < SSA_FRAME_SIZE_MIN || m_metadata->ssa_frame_size > SSA_FRAME_SIZE_MAX)
        return SGX_ERROR_INVALID_METADATA;
    uint64_t size = m_metadata->enclave_size;
    if(size > m_parser.get_enclave_max_size())
    {
        SE_TRACE(SE_TRACE_ERROR, "The enclave size setting in metadata is too large.\n");
        return SGX_ERROR_INVALID_METADATA;
    }
    while ((size != 0) && ((size & 1) != 1))
    {
        size = size >> 1;
    }
    if(size != 1)
    {
        return SGX_ERROR_INVALID_METADATA;
    }

    // check dirs
    for(uint32_t i = 0; i < DIR_NUM; i++)
    {
        if(false == is_metadata_buffer(m_metadata->dirs[i].offset, m_metadata->dirs[i].size))
        {
            return SGX_ERROR_INVALID_METADATA;
        }
    }
    // check layout table
    int status = validate_layout_table();
    if(SGX_SUCCESS != status)
    {
        return status;
    }
    // check patch table
    status = validate_patch_table();
    if(SGX_SUCCESS != status)
    {
        return status;
    }

    return SGX_SUCCESS;
}

bool CLoader::is_ae(const enclave_css_t *enclave_css)
{
    assert(NULL != enclave_css);

    if(INTEL_VENDOR_ID == enclave_css->header.module_vendor
            && AE_PRODUCT_ID == enclave_css->body.isv_prod_id)
        return true;

    return false;
}

int CLoader::load_enclave(SGXLaunchToken *lc, int debug, const metadata_t *metadata, sgx_config_id_t *config_id, sgx_config_svn_t config_svn, le_prd_css_file_t *prd_css_file, sgx_misc_attribute_t *misc_attr)
{
    int ret = SGX_SUCCESS;
    sgx_misc_attribute_t sgx_misc_attr;
    memset(&sgx_misc_attr, 0, sizeof(sgx_misc_attribute_t));

    m_metadata = metadata;
    ret = validate_metadata();
    if(SGX_SUCCESS != ret)
    {
        SE_TRACE(SE_TRACE_ERROR, "The metadata setting is not correct\n");
        return ret;
    }

    ret = get_enclave_creator()->get_misc_attr(&sgx_misc_attr, const_cast<metadata_t *>(m_metadata), lc, debug);
    if(SGX_SUCCESS != ret)
    {
        return ret;
    }

    ret = build_image(lc, &sgx_misc_attr.secs_attr, config_id, config_svn, prd_css_file, &sgx_misc_attr);
    // Update misc_attr with secs.attr upon success.
    if(SGX_SUCCESS == ret)
    {
        //When run here EINIT success, so SGX_FLAGS_INITTED should be set by ucode. uRTS align it with EINIT instruction.
        sgx_misc_attr.secs_attr.flags |= SGX_FLAGS_INITTED;
        m_secs.attributes = sgx_misc_attr.secs_attr;
        if(misc_attr)
        {
            memcpy_s(misc_attr, sizeof(sgx_misc_attribute_t), &sgx_misc_attr, sizeof(sgx_misc_attribute_t));
        }
    }

    return ret;
}

int CLoader::load_enclave_ex(SGXLaunchToken *lc, bool debug, const metadata_t *metadata, sgx_config_id_t *config_id, sgx_config_svn_t config_svn, le_prd_css_file_t *prd_css_file, sgx_misc_attribute_t *misc_attr)
{
    unsigned int ret = SGX_SUCCESS, map_retry_count = 3;
    bool retry = true;

    while (retry)
    {
        ret = this->load_enclave(lc, debug, metadata, config_id, config_svn, prd_css_file, misc_attr);
        switch(ret)
        {
            //If CreateEnclave failed due to power transition, we retry it.
        case SGX_ERROR_ENCLAVE_LOST:     //caused by loading enclave while power transition occurs
            break;

            //If memory map fail or conflict occurs, we only retry 3 times.
        case SGX_ERROR_MEMORY_MAP_FAILURE:
        case SGX_ERROR_MEMORY_MAP_CONFLICT:
            if(0 == map_retry_count)
                retry = false;
            else
                map_retry_count--;
            break;

            //We don't re-load enclave due to other error code.
        default:
            retry = false;
            break;
        }
    }

    return ret;
}

int CLoader::destroy_enclave()
{
    return get_enclave_creator()->destroy_enclave(ENCLAVE_ID_IOCTL, m_secs.size);
}

int CLoader::set_memory_protection()
{
    int ret = 0;
    //set memory protection for segments
    if(m_parser.set_memory_protection((uint64_t)m_start_addr) != true)
    {
        return SGX_ERROR_UNEXPECTED;
    }

    if ((META_DATA_MAKE_VERSION(MAJOR_VERSION,MINOR_VERSION) <= m_metadata->version) &&
            get_enclave_creator()->is_EDMM_supported(get_enclave_id()))
    {
        std::vector<std::tuple<uint64_t, uint64_t, uint32_t>> pages_to_protect;
        m_parser.get_pages_to_protect((uint64_t)m_start_addr, pages_to_protect);
        for (auto page : pages_to_protect)
        {   uint64_t start = 0, len = 0;
            uint32_t perm = 0;
            std::tie(start, len, perm) = page;
            ret = get_enclave_creator()->emodpr(start, len, (uint64_t)perm);
            if (ret != SGX_SUCCESS)
                return SGX_ERROR_UNEXPECTED;
        }

        //On EDMM supported platform, force set <ReservedMemMinSize> rsrv memory region's attribute to RW although it's was set to RWX by <ReservedMemExecutable>1</ReservedMemExecutable>
        layout_t *layout_start = GET_PTR(layout_t, m_metadata, m_metadata->dirs[DIR_LAYOUT].offset);
        layout_t *layout_end = GET_PTR(layout_t, m_metadata, m_metadata->dirs[DIR_LAYOUT].offset + m_metadata->dirs[DIR_LAYOUT].size);
        for (layout_t *layout = layout_start; layout < layout_end; layout++)
        {
            if (layout->entry.id ==  LAYOUT_ID_RSRV_MIN && layout->entry.si_flags == SI_FLAGS_RWX && layout->entry.page_count > 0)
            {
                ret = get_enclave_creator()->emodpr((uint64_t)m_start_addr + layout->entry.rva, (uint64_t)layout->entry.page_count << SE_PAGE_SHIFT, (uint64_t)(SI_FLAG_R |SI_FLAG_W ));
                if (ret != SGX_SUCCESS)
                    return SGX_ERROR_UNEXPECTED;
                break;
            }
        }
    }

    //set memory protection for context
    ret = set_context_protection(GET_PTR(layout_t, m_metadata, m_metadata->dirs[DIR_LAYOUT].offset),
                                    GET_PTR(layout_t, m_metadata, m_metadata->dirs[DIR_LAYOUT].offset + m_metadata->dirs[DIR_LAYOUT].size),
                                    0);
    if (SGX_SUCCESS != ret)
    {
        return ret;
    }
    
    return SGX_SUCCESS;

}

int CLoader::set_context_protection(layout_t *layout_start, layout_t *layout_end, uint64_t delta)
{
    int ret = SGX_ERROR_UNEXPECTED;
    for(layout_t *layout = layout_start; layout < layout_end; layout++)
    {
        if (!IS_GROUP_ID(layout->group.id))
        {
            if(!get_enclave_creator()->is_EDMM_supported(get_enclave_id()) && (layout->entry.id == LAYOUT_ID_RSRV_MIN ||layout->entry.id ==  LAYOUT_ID_RSRV_INIT))
                 //Don't change the rsrv memory's attributes if platform isn't support EDMM
                 continue;
             //Here: URTS will change rsrv memory's attributes to RW forcely although it's signed by sign_tool as RWX (<ReservedMemExecutable>1</ReservedMemExecutable>).

            int prot = 0 ;
            if(layout->entry.si_flags == SI_FLAG_NONE)
            {
                prot = SI_FLAG_NONE & SI_MASK_MEM_ATTRIBUTE;
            }
            else
            {
                prot = SI_FLAGS_RW & SI_MASK_MEM_ATTRIBUTE;
#ifndef SE_SIM

                //when a page is eremoved when loading, we should set this page to none access. 
                //if this page is accessed, a sigbus exception will be raised.
                uint16_t attributes = layout->entry.attributes;
                if(attributes & PAGE_ATTR_EADD && attributes & PAGE_ATTR_EREMOVE)
                {
                    if(attributes & PAGE_ATTR_EREMOVE)
                    {
                        prot = SI_FLAG_NONE & SI_MASK_MEM_ATTRIBUTE;
                    }
                }
#endif                          
            }

            ret = mprotect(GET_PTR(void, m_start_addr, layout->entry.rva + delta), 
                               (size_t)layout->entry.page_count << SE_PAGE_SHIFT,
                               prot); 
            if(ret != 0)
            {
                SE_TRACE(SE_TRACE_WARNING, "mprotect(rva=%" PRIu64 ", len=%" PRIu64 ", flags=%d) failed\n",
                         (uint64_t)m_start_addr + layout->entry.rva + delta, 
                         (uint64_t)layout->entry.page_count << SE_PAGE_SHIFT, 
                          prot);
                return SGX_ERROR_UNEXPECTED;
            }
        }
        else
        {
            uint64_t step = 0;
            for(uint32_t j = 0; j < layout->group.load_times; j++)
            {
                step += layout->group.load_step;
                if(SGX_SUCCESS != (ret = set_context_protection(&layout[-layout->group.entry_count], layout, step)))
                {
                    return ret;
                }
            }
        }
    }
    return SGX_SUCCESS;
}
