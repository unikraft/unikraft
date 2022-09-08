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

namespace {
    layout_entry_t *get_entry_by_id(const metadata_t *const metadata, uint16_t id, bool do_assert = true)
    {
        layout_entry_t *layout_start = GET_PTR(layout_entry_t, metadata, metadata->dirs[DIR_LAYOUT].offset);
        layout_entry_t *layout_end = GET_PTR(layout_entry_t, metadata, metadata->dirs[DIR_LAYOUT].offset + metadata->dirs[DIR_LAYOUT].size);
        for (layout_entry_t *layout = layout_start; layout < layout_end; layout++)
        {
            if(layout->id == id)
                return layout;
        }
        if (do_assert)
            assert(false);
        return NULL;
    }

    elrange_config_entry_t *get_elrange_config_entry(const metadata_t *const metadata)
    {   
        if(MAJOR_VERSION_OF_METADATA(metadata->version) <= SGX_MAJOR_VERSION_GAP)
        {
            return NULL;
        }
        //elrange config entry is placed at the beginning of data
        data_directory_t* dir = GET_PTR(data_directory_t, metadata, offsetof(metadata_t, data));
        if(dir == NULL)
        {
            return NULL;
        }
        elrange_config_entry_t* elrange_config_entry = GET_PTR(elrange_config_entry_t, metadata, dir->offset);
        return elrange_config_entry;
    }
    
    bool do_update_global_data(const metadata_t *const metadata,
                                const create_param_t* const create_param,
                               global_data_t* global_data)
    {
        layout_entry_t *layout_heap = get_entry_by_id(metadata, LAYOUT_ID_HEAP_MIN);
        layout_entry_t *layout_rsrv = get_entry_by_id(metadata, LAYOUT_ID_RSRV_MIN, false);
        if(layout_heap == NULL)
        {
            return false;
        }

        if (NULL == layout_rsrv)
        {
            global_data->rsrv_offset = (sys_word_t)0;
            global_data->rsrv_size = (sys_word_t)0;
            global_data->rsrv_executable = (sys_word_t)0;
        }
        else
        {
            global_data->rsrv_offset = (sys_word_t)(layout_rsrv->rva);
            global_data->rsrv_size = (sys_word_t)(create_param->rsrv_init_size);
            global_data->rsrv_executable = (sys_word_t)(create_param->rsrv_executable);
        }

        global_data->enclave_size = (sys_word_t)metadata->enclave_size;
        global_data->heap_offset = (sys_word_t)layout_heap->rva;
        global_data->heap_size = (sys_word_t)(create_param->heap_init_size);
        global_data->thread_policy = (sys_word_t)metadata->tcs_policy;
        global_data->tcs_max_num = (sys_word_t)create_param->tcs_max_num;
        global_data->tcs_num = (sys_word_t)create_param->tcs_num;
        thread_data_t *thread_data = &global_data->td_template;

        thread_data->stack_limit_addr = (sys_word_t)create_param->stack_limit_addr;
        thread_data->stack_base_addr = (sys_word_t)create_param->stack_base_addr;
        thread_data->last_sp = thread_data->stack_base_addr;
        thread_data->xsave_size = create_param->xsave_size;
        thread_data->first_ssa_gpr = (sys_word_t)create_param->ssa_base_addr + metadata->ssa_frame_size * SE_PAGE_SIZE - (uint32_t)sizeof(ssa_gpr_t);
        thread_data->flags = 0;
        // TD address relative to TCS
        thread_data->tls_addr = (sys_word_t)create_param->tls_addr;
        thread_data->self_addr = (sys_word_t)create_param->td_addr;
        thread_data->tls_array = thread_data->self_addr + (sys_word_t)offsetof(thread_data_t, tls_addr);

        // TCS template
        if(0 != memcpy_s(&global_data->tcs_template, sizeof(global_data->tcs_template), 
                          GET_PTR(void, metadata, get_entry_by_id(metadata, LAYOUT_ID_TCS)->content_offset), 
                          get_entry_by_id(metadata, LAYOUT_ID_TCS)->content_size))
        {
            return false;
        }

        // layout table: dynamic heap + dynamic thread group 
        layout_entry_t *layout_start = GET_PTR(layout_entry_t, metadata, metadata->dirs[DIR_LAYOUT].offset);
        layout_entry_t *layout_end = GET_PTR(layout_entry_t, metadata, metadata->dirs[DIR_LAYOUT].offset + metadata->dirs[DIR_LAYOUT].size);
        global_data->layout_entry_num = 0;

        SE_TRACE_DEBUG("\n");
        se_trace(SE_TRACE_DEBUG, "Global Data:\n");
        se_trace(SE_TRACE_DEBUG, "\tEnclave size     = 0x%016llX\n", global_data->enclave_size);
        se_trace(SE_TRACE_DEBUG, "\tHeap Offset      = 0x%016llX\n", global_data->heap_offset);
        se_trace(SE_TRACE_DEBUG, "\tHeap Size        = 0x%016llX\n", global_data->heap_size);
        se_trace(SE_TRACE_DEBUG, "\tReserved Mem Offset      = 0x%016llX\n", global_data->rsrv_offset);
        se_trace(SE_TRACE_DEBUG, "\tReserved Mem Size        = 0x%016llX\n", global_data->rsrv_size);
        se_trace(SE_TRACE_DEBUG, "\tThread Policy    = 0x%016llX\n", global_data->thread_policy);
        se_trace(SE_TRACE_DEBUG, "\tLayout Table:\n");

        unsigned entry_cnt = 1;

        for (layout_entry_t *layout = layout_start; layout < layout_end; layout++)
        {
            uint16_t entry_id = layout->id;

            if (!IS_GROUP_ID(entry_id)) {
                se_trace(SE_TRACE_DEBUG, "\tEntry Id(%2u) = %4u, %-16s,  ", entry_cnt, entry_id, layout_id_str[entry_id]);
                se_trace(SE_TRACE_DEBUG, "Page Count = %5u,  ", layout->page_count);
                se_trace(SE_TRACE_DEBUG, "Attributes = 0x%02X,  ", layout->attributes);
                se_trace(SE_TRACE_DEBUG, "Flags = 0x%016llX,  ", layout->si_flags);
                se_trace(SE_TRACE_DEBUG, "RVA = 0x%016llX --- 0x%016llX\n", layout->rva,
                    layout->rva + 4096 * layout->page_count);
            }
            else {
#ifndef DISABLE_TRACE
                layout_group_t *layout_grp = reinterpret_cast<layout_group_t*>(layout);
#endif
                se_trace(SE_TRACE_DEBUG, "\tEntry Id(%2u) = %4u, %-16s,  ", entry_cnt, entry_id, layout_id_str[entry_id & ~(GROUP_FLAG)]);
                se_trace(SE_TRACE_DEBUG, "Entry Count = %4u,  ", layout_grp->entry_count);
                se_trace(SE_TRACE_DEBUG, "Load Times = %u,     ", layout_grp->load_times);
                se_trace(SE_TRACE_DEBUG, "LStep = 0x%016llX\n", layout_grp->load_step);
            }

            if(0 != memcpy_s(&global_data->layout_table[global_data->layout_entry_num], 
                     sizeof(global_data->layout_table) - global_data->layout_entry_num * sizeof(layout_t), 
                     layout, 
                     sizeof(layout_t)))
            {
                return false;
            }
            global_data->layout_entry_num++;
            entry_cnt++;
        }
        elrange_config_entry_t* elrange_config_entry = get_elrange_config_entry(metadata);
        if(elrange_config_entry != NULL)
        {
            global_data->enclave_image_address = elrange_config_entry->enclave_image_address;
            global_data->elrange_start_address= elrange_config_entry->elrange_start_address;
            global_data->elrange_size = elrange_config_entry->elrange_size;
        }
        else
        {
            global_data->enclave_image_address = 0;
            global_data->elrange_start_address= 0;
            global_data->elrange_size = global_data->enclave_size;
        }
        return true;
    }
}
