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

/**
* File:
*     manage_metadata.cpp
* Description:
*     Parse the xml file to get the metadata and generate the output DLL
* with metadata.
*/

#include "metadata.h"
#include "tinyxml2.h"
#include "manage_metadata.h"
#include "se_trace.h"
#include "util_st.h"
#include "section.h"
#include "se_page_attr.h"
#include "elf_util.h"
#include "crypto_wrapper.h"
#include "global_data.h"
#include "se_version.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <iostream>
#include <iomanip>
#include <fstream>

using tinyxml2::XML_ERROR_FILE_COULD_NOT_BE_OPENED;
using tinyxml2::XML_ERROR_FILE_NOT_FOUND;
using tinyxml2::XML_SUCCESS;
using tinyxml2::XMLElement;
using tinyxml2::XMLError;

#define ALIGN_SIZE 0x1000

static bool traverser_parameter(const char *temp_name, const char *temp_text, xml_parameter_t *parameter, int parameter_count)
{
    assert(temp_name != NULL && parameter != NULL);
    uint64_t temp_value=0;
    if(temp_text == NULL)
    {
        se_trace(SE_TRACE_ERROR, LACK_VALUE_FOR_ELEMENT_ERROR, temp_name);
        return false;
    }
    else
    {
        if(strchr(temp_text, '-'))
        {
            se_trace(SE_TRACE_ERROR, INVALID_VALUE_FOR_ELEMENT_ERROR, temp_name);
            return false;
        }

        errno = 0;
        char* endptr = NULL;
        temp_value = (uint64_t)strtoull(temp_text, &endptr, 0);
        if(*endptr!='\0'||errno!=0)   //Invalid value or valid value but out of the representable range
        {
            se_trace(SE_TRACE_ERROR, INVALID_VALUE_FOR_ELEMENT_ERROR, temp_name);
            return false;
        }
    }

    //Look for the matched one
    int i=0;
    for(; i<parameter_count&&STRCMP(temp_name,parameter[i].name); i++);
    if(i>=parameter_count) //no matched, return false
    {
        se_trace(SE_TRACE_ERROR, UNREC_ELEMENT_ERROR, temp_name);
        return false;
    }
    //found one matched
    if(parameter[i].flag==1)  //repeated definition of XML element, return false
    {
        se_trace(SE_TRACE_ERROR, REPEATED_DEFINE_ERROR, temp_name);
        return false;
    }
    parameter[i].flag = 1;
    if((temp_value<parameter[i].min_value)||
            (temp_value>parameter[i].max_value)) // the value is invalid, return false
    {
        se_trace(SE_TRACE_ERROR, VALUE_OUT_OF_RANGE_ERROR, temp_name);
        return false;
    }
    parameter[i].value = temp_value;
    SE_TRACE_DEBUG("%s = %s\n", temp_name, temp_text);
    return true;
}

bool parse_metadata_file(const char *xmlpath, xml_parameter_t *parameter, int parameter_count)
{
    const char* temp_name=NULL;

    assert(parameter != NULL);
    if(xmlpath == NULL) // user didn't define the metadata xml file.
    {
        se_trace(SE_TRACE_NOTICE, "Use default metadata...\n");
        return true;
    }
    //use the metadata file that user gives us. parse xml file
    tinyxml2::XMLDocument doc;
    XMLError loadOkay = doc.LoadFile(xmlpath);
    if(loadOkay != XML_SUCCESS)
    {
        if(doc.ErrorID() == XML_ERROR_FILE_COULD_NOT_BE_OPENED)
        {
            se_trace(SE_TRACE_ERROR, OPEN_FILE_ERROR, xmlpath);
        }
        else if(doc.ErrorID() == XML_ERROR_FILE_NOT_FOUND)
        {
            se_trace(SE_TRACE_ERROR, XML_NOT_FOUND_ERROR, xmlpath);
        }
        else
        {
            se_trace(SE_TRACE_ERROR, XML_FORMAT_ERROR);
        }
        return false;
    }
    doc.Print();//Write the document to standard out using formatted printing ("pretty print").

    XMLElement *pmetadata_element = doc.FirstChildElement("EnclaveConfiguration");
    if(!pmetadata_element || pmetadata_element->GetText() != NULL)
    {
        se_trace(SE_TRACE_ERROR, XML_FORMAT_ERROR);
        return false;
    }
    XMLElement *sub_element = NULL;
    sub_element = pmetadata_element->FirstChildElement();
    const char *temp_text = NULL;

    while(sub_element)//parse xml node
    {
        if(sub_element->FirstAttribute() != NULL)
        {
            se_trace(SE_TRACE_ERROR, XML_FORMAT_ERROR);
            return false;
        }

        temp_name = sub_element->Value();
        temp_text = sub_element->GetText();

        //traverse every node. Compare with the default value.
        if(traverser_parameter(temp_name, temp_text, parameter, parameter_count) == false)
        {
            se_trace(SE_TRACE_ERROR, XML_FORMAT_ERROR);
            return false;
        }

        sub_element= sub_element->NextSiblingElement();
    }

    return true;
}

CMetadata::CMetadata(metadata_t *metadata, BinParser *parser)
    : m_metadata(metadata)
    , m_parser(parser), m_rva(0), m_gd_size(0), m_gd_template(NULL)
{
    memset(m_metadata, 0, sizeof(metadata_t));
    memset(&m_create_param, 0, sizeof(m_create_param));
    memset(&m_elrange_config_entry, 0, sizeof(m_elrange_config_entry));
}
CMetadata::~CMetadata()
{
}
bool CMetadata::build_metadata(const xml_parameter_t *parameter)
{
    if(!modify_metadata(parameter))
    {
        return false;
    }
    //elrange config entry
    if(!build_elrange_config_entry())
    {
        return false;
    }
    // layout table
    if(!build_layout_table())
    {
        return false;
    }
    // patch table
    if(!build_patch_table())
    {
        return false;
    }
    if(!build_layout_entries())
    {
        return false;
    }
    //vaildate the elrange config here
    //at this time, enclave size should be calculated out
    if(vaildate_elrange_config() == false)
    {
        return false;
    }
    if(!build_gd_template(m_gd_template, &m_gd_size))
    {
        return false;
    }

    return true;
}

#include <sstream>
#include <time.h>
bool CMetadata::get_time(uint32_t *date)
{
    assert(date != NULL);

    time_t rawtime = 0;
    if(time( &rawtime) == -1)
        return false;
    struct tm *timeinfo = gmtime(&rawtime);
    if(timeinfo  == NULL)
        return false;
    uint32_t tmp_date = (timeinfo->tm_year+1900)*10000 + (timeinfo->tm_mon+1)*100 + timeinfo->tm_mday;
    std::stringstream ss;
    ss<<"0x"<<tmp_date;
    ss>>std::hex>>tmp_date;
    *date = tmp_date;
    return true;
}

bool CMetadata::fill_enclave_css(const xml_parameter_t *para)
{
    assert(para != NULL);
    uint32_t date = 0;
    if(false == get_time(&date))
        return false;

    //*****fill the header*******************//
    uint8_t header[12] = {6, 0, 0, 0, 0xE1, 0, 0, 0, 0, 0, 1, 0};
    uint8_t header2[16] = {1, 1, 0, 0, 0x60, 0, 0, 0, 0x60, 0, 0, 0, 1, 0, 0, 0};
    memcpy_s(&m_metadata->enclave_css.header.header, sizeof(m_metadata->enclave_css.header.header), &header, sizeof(header));
    memcpy_s(&m_metadata->enclave_css.header.header2, sizeof(m_metadata->enclave_css.header.header2), &header2, sizeof(header2));

    // For 'type', signing tool clears the bit 31 for product enclaves 
    // and set the bit 31 for debug enclaves
    m_metadata->enclave_css.header.type = (para[RELEASETYPE].value & 0x01) ? (1<<31) : 0;
    m_metadata->enclave_css.header.module_vendor = (para[INTELSIGNED].value&0x01) ? 0x8086 : 0;
    m_metadata->enclave_css.header.date = date;

    //hardware version
    m_metadata->enclave_css.header.hw_version = (uint32_t)para[HW].value;


    // Misc_select/Misc_mask
    m_metadata->enclave_css.body.misc_select = (uint32_t)para[MISCSELECT].value;
    m_metadata->enclave_css.body.misc_mask = (uint32_t)para[MISCMASK].value;
    //low 64 bit
    m_metadata->enclave_css.body.attributes.flags = 0;
    m_metadata->enclave_css.body.attribute_mask.flags = ~SGX_FLAGS_DEBUG;
    if(para[DISABLEDEBUG].value == 1)
    {
        m_metadata->enclave_css.body.attributes.flags &=  ~SGX_FLAGS_DEBUG;
        m_metadata->enclave_css.body.attribute_mask.flags |= SGX_FLAGS_DEBUG;
    }
    if(para[PROVISIONKEY].value == 1)
    {
        m_metadata->enclave_css.body.attributes.flags |= SGX_FLAGS_PROVISION_KEY;
        m_metadata->enclave_css.body.attribute_mask.flags |= SGX_FLAGS_PROVISION_KEY;
    }
    if(para[LAUNCHKEY].value == 1)
    {
        m_metadata->enclave_css.body.attributes.flags |= SGX_FLAGS_EINITTOKEN_KEY;
        m_metadata->enclave_css.body.attribute_mask.flags |= SGX_FLAGS_EINITTOKEN_KEY;
    }
    if (para[ENABLEKSS].value == 1)
    {
        m_metadata->enclave_css.body.attributes.flags |= SGX_FLAGS_KSS;
        m_metadata->enclave_css.body.attribute_mask.flags |= SGX_FLAGS_KSS;
    }
    if (memcpy_s(&(m_metadata->enclave_css.body.isvext_prod_id), SGX_ISVEXT_PROD_ID_SIZE, 
         &(para[ISVEXTPRODID_L].value), sizeof(para[ISVEXTPRODID_L].value)))
    {
        return false;
    }
    if (memcpy_s((uint8_t *)&(m_metadata->enclave_css.body.isvext_prod_id) + sizeof(para[ISVEXTPRODID_L].value), 
        SGX_ISVEXT_PROD_ID_SIZE - sizeof(para[ISVEXTPRODID_L].value), &(para[ISVEXTPRODID_H].value), sizeof(para[ISVEXTPRODID_H].value)))
    {
        return false;
    }
    if (memcpy_s(&(m_metadata->enclave_css.body.isv_family_id), SGX_ISV_FAMILY_ID_SIZE, 
        &(para[ISVFAMILYID_L].value), sizeof(para[ISVFAMILYID_L].value)))
    {
        return false;
    }
    if (memcpy_s((uint8_t *)&(m_metadata->enclave_css.body.isv_family_id) + sizeof(para[ISVFAMILYID_L].value), 
        SGX_ISV_FAMILY_ID_SIZE - sizeof(para[ISVFAMILYID_L].value), &(para[ISVFAMILYID_H].value), sizeof(para[ISVFAMILYID_H].value)))
    {
        return false;
    }

    bin_fmt_t bf = m_parser->get_bin_format();
    if(bf == BF_PE64 || bf == BF_ELF64)
    {
        m_metadata->enclave_css.body.attributes.flags |= SGX_FLAGS_MODE64BIT;
        m_metadata->enclave_css.body.attribute_mask.flags |= SGX_FLAGS_MODE64BIT;
    }
    // high 64 bit
    //default setting
    m_metadata->enclave_css.body.attributes.xfrm = SGX_XFRM_LEGACY;
    m_metadata->enclave_css.body.attribute_mask.xfrm = SGX_XFRM_LEGACY | SGX_XFRM_RESERVED; // LEGACY and reservied bits would be checked.
    switch(para[PKRU].value)
    {
        case FEATURE_MUST_BE_DISABLED:
                // PKRU must be disabled
                m_metadata->enclave_css.body.attributes.xfrm &= ~SGX_XFRM_PKRU;
                m_metadata->enclave_css.body.attribute_mask.xfrm |= SGX_XFRM_PKRU;
                break;
        case FEATURE_MUST_BE_ENABLED:
                // PKRU must be enabled
                m_metadata->enclave_css.body.attributes.xfrm |= SGX_XFRM_PKRU;
                m_metadata->enclave_css.body.attribute_mask.xfrm |= SGX_XFRM_PKRU;
                break;
        case FEATURE_LOADER_SELECTS:
        default:
                m_metadata->enclave_css.body.attributes.xfrm &= ~SGX_XFRM_PKRU;
                m_metadata->enclave_css.body.attribute_mask.xfrm &= ~SGX_XFRM_PKRU;
                break;
    }

    m_metadata->enclave_css.body.isv_prod_id = (uint16_t)para[PRODID].value;
    m_metadata->enclave_css.body.isv_svn = (uint16_t)para[ISVSVN].value;
    return true;
}

bool CMetadata::modify_metadata(const xml_parameter_t *parameter)
{
    assert(parameter != NULL);
    if(!check_xml_parameter(parameter))
        return false;
    if(!fill_enclave_css(parameter))
        return false;

    //if elrange is set, will update the major version to SGX_2_ELRANGE_MAJOR_VERSION
    if(parameter[ELRANGESIZE].value == 0)
    {
        m_metadata->version = META_DATA_MAKE_VERSION(MAJOR_VERSION,MINOR_VERSION );
    }
    else
    {
        m_metadata->version = META_DATA_MAKE_VERSION(SGX_2_ELRANGE_MAJOR_VERSION,MINOR_VERSION );
    }

    
    m_metadata->size = offsetof(metadata_t, data);
    m_metadata->tcs_policy = (uint32_t)parameter[TCSPOLICY].value;
    m_metadata->ssa_frame_size = SSA_FRAME_SIZE;
    m_metadata->max_save_buffer_size = MAX_SAVE_BUF_SIZE;
    m_metadata->magic_num = METADATA_MAGIC;
    m_metadata->desired_misc_select = 0;
    m_metadata->tcs_min_pool = (uint32_t)parameter[TCSMINPOOL].value;
    m_metadata->enclave_css.body.misc_select = (uint32_t)parameter[MISCSELECT].value;
    m_metadata->enclave_css.body.misc_mask =  (uint32_t)parameter[MISCMASK].value;

    //store the elrange config for further check
    m_elrange_config_entry.enclave_image_address = parameter[ENCLAVEIMAGEADDRESS].value;
    m_elrange_config_entry.elrange_start_address = parameter[ELRANGESTARTADDRESS].value;
    m_elrange_config_entry.elrange_size = parameter[ELRANGESIZE].value;
    
    //set metadata.attributes
    //low 64 bit: it's the same as enclave_css
    memset(&m_metadata->attributes, 0, sizeof(sgx_attributes_t));
    m_metadata->attributes.flags = m_metadata->enclave_css.body.attributes.flags;
    //high 64 bit
    //set bits that will not be checked
    m_metadata->attributes.xfrm = ~m_metadata->enclave_css.body.attribute_mask.xfrm;
    //set bits that have been set '1' and need to be checked
    m_metadata->attributes.xfrm |= (m_metadata->enclave_css.body.attributes.xfrm & m_metadata->enclave_css.body.attribute_mask.xfrm);

    return true;
}

bool CMetadata::check_xml_parameter(const xml_parameter_t *parameter)
{

    //stack/heap must be page-align
    if( (parameter[STACKMAXSIZE].value % ALIGN_SIZE)
     || (parameter[STACKMINSIZE].value % ALIGN_SIZE) )
    {
        se_trace(SE_TRACE_ERROR, SET_STACK_SIZE_ERROR);
        return false;
    }
    if(parameter[STACKMINSIZE].value > parameter[STACKMAXSIZE].value)
    {
        se_trace(SE_TRACE_ERROR, SET_STACK_SIZE_ERROR);
        return false;
    }

    if( (parameter[HEAPMAXSIZE].value % ALIGN_SIZE)
     || (parameter[HEAPMINSIZE].value % ALIGN_SIZE)
     || (parameter[HEAPINITSIZE].value % ALIGN_SIZE) )
    {
        se_trace(SE_TRACE_ERROR, SET_HEAP_SIZE_ALIGN_ERROR);
        return false;
    }

    if (parameter[HEAPINITSIZE].flag != 0)
    {
        if (parameter[HEAPINITSIZE].value > parameter[HEAPMAXSIZE].value)
        {
            se_trace(SE_TRACE_ERROR, SET_HEAP_SIZE_INIT_MAX_ERROR);
            return false;
        }
        if (parameter[HEAPMINSIZE].value > parameter[HEAPINITSIZE].value)
        {
            se_trace(SE_TRACE_ERROR, SET_HEAP_SIZE_INIT_MIN_ERROR);
            return false;
        }
    }
    else
    {
        if (parameter[HEAPMINSIZE].value > parameter[HEAPMAXSIZE].value)
        {
            se_trace(SE_TRACE_ERROR, SET_HEAP_SIZE_MAX_MIN_ERROR);
            return false;
        }
    }

    if ((parameter[RSRVMAXSIZE].value % ALIGN_SIZE)
        || (parameter[RSRVMINSIZE].value % ALIGN_SIZE)
        || (parameter[RSRVINITSIZE].value % ALIGN_SIZE))
    {
        se_trace(SE_TRACE_ERROR, SET_RSRV_SIZE_ALIGN_ERROR);
        return false;
    }

    if (parameter[RSRVINITSIZE].flag != 0)
    {
        if (parameter[RSRVINITSIZE].value > parameter[RSRVMAXSIZE].value)
        {
            se_trace(SE_TRACE_ERROR, SET_RSRV_SIZE_INIT_MAX_ERROR);
            return false;
        }
        if (parameter[RSRVMINSIZE].value > parameter[RSRVINITSIZE].value)
        {
            se_trace(SE_TRACE_ERROR, SET_RSRV_SIZE_INIT_MIN_ERROR);
            return false;
        }
    }
    else
    {
        if (parameter[RSRVMINSIZE].value > parameter[RSRVMAXSIZE].value)
        {
            se_trace(SE_TRACE_ERROR, SET_RSRV_SIZE_MAX_MIN_ERROR);
            return false;
        }
    }

   if(parameter[RSRVEXECUTABLE].flag != 0)
   {
       if(parameter[RSRVEXECUTABLE].value != 0 && parameter[RSRVEXECUTABLE].value != 1)
       {
           se_trace(SE_TRACE_ERROR, SET_RSRV_EXECUTABLE_ERROR);
           return false;
       }
   }

    // LE setting:  HW != 0, Licensekey = 1
    // Other enclave setting: HW = 0, Licensekey = 0
    if((parameter[HW].value == 0 && parameter[LAUNCHKEY].value != 0) ||
        (parameter[HW].value != 0 && parameter[LAUNCHKEY].value == 0))
    {
        se_trace(SE_TRACE_ERROR, SET_HW_LE_ERROR);
        return false;
    }

    if (parameter[TCSMAXNUM].flag != 0)
    {
        if (parameter[TCSMAXNUM].value < parameter[TCSNUM].value)
        {
            se_trace(SE_TRACE_ERROR, SET_TCS_MAX_NUM_ERROR);
            return false;
        }

        if ((parameter[TCSMINPOOL].flag != 0)
                && (parameter[TCSMINPOOL].value > parameter[TCSMAXNUM].value))
        {
            se_trace(SE_TRACE_ERROR, SET_TCS_MIN_POOL_ERROR);
            return false;
        }
    }
    else if ((parameter[TCSMINPOOL].flag != 0)
                && (parameter[TCSMINPOOL].value > parameter[TCSNUM].value))
    {
        se_trace(SE_TRACE_ERROR, SET_TCS_MIN_POOL_ERROR);
        return false;
    }

    if ((parameter[ISVEXTPRODID_H].value || parameter[ISVEXTPRODID_L].value ||
        parameter[ISVFAMILYID_H].value || parameter[ISVFAMILYID_L].value) &&
        parameter[ENABLEKSS].value == 0)
    {
        se_trace(SE_TRACE_ERROR, SET_ENABLE_KSS_ERROR);
        return false;
    }

    if(parameter[ENCLAVEIMAGEADDRESS].flag != 0 && parameter[ELRANGESIZE].flag ==0)
    {
        se_trace(SE_TRACE_ERROR, SET_ENCLAVEIMAGEADDRESS_SET_ERROR);
        return false;
    }

    if(parameter[ELRANGESTARTADDRESS].flag != 0 && parameter[ELRANGESIZE].flag ==0)
    {
        se_trace(SE_TRACE_ERROR, SET_ELRANGESTARTADDRESS_SET_ERROR);
        return false;
    }

    m_create_param.heap_init_size = parameter[HEAPINITSIZE].flag ? parameter[HEAPINITSIZE].value : parameter[HEAPMAXSIZE].value;
    m_create_param.heap_min_size = parameter[HEAPMINSIZE].value;
    m_create_param.heap_max_size = parameter[HEAPMAXSIZE].value;
    m_create_param.rsrv_init_size = parameter[RSRVINITSIZE].flag ? parameter[RSRVINITSIZE].value : parameter[RSRVMAXSIZE].value;
    m_create_param.rsrv_min_size = parameter[RSRVMINSIZE].value;
    m_create_param.rsrv_max_size = parameter[RSRVMAXSIZE].value;
    m_create_param.rsrv_executable = parameter[RSRVEXECUTABLE].flag ? parameter[RSRVEXECUTABLE].value : 0;
    m_create_param.stack_max_size = parameter[STACKMAXSIZE].value;
    m_create_param.stack_min_size = parameter[STACKMINSIZE].value;
    m_create_param.tcs_num = (uint32_t)parameter[TCSNUM].value;
    m_create_param.tcs_max_num = (uint32_t)(parameter[TCSMAXNUM].flag ? parameter[TCSMAXNUM].value : parameter[TCSNUM].value);
    m_create_param.tcs_min_pool = (uint32_t)parameter[TCSMINPOOL].value;
    m_create_param.tcs_policy = (uint32_t)parameter[TCSPOLICY].value;

    se_trace(SE_TRACE_ERROR, "tcs_num %d, tcs_max_num %d, tcs_min_pool %d\n", m_create_param.tcs_num, m_create_param.tcs_max_num, m_create_param.tcs_min_pool);
    SE_TRACE_DEBUG("RSRV_MIN_SIZE  = 0x%016llX\n", m_create_param.rsrv_min_size);
    SE_TRACE_DEBUG("RSRV_INIT_SIZE = 0x%016llX\n", m_create_param.rsrv_init_size);
    SE_TRACE_DEBUG("RSRV_MAX_SIZE  = 0x%016llX\n", m_create_param.rsrv_max_size);

    return true;
}

void *CMetadata::alloc_buffer_from_metadata(uint32_t size)
{
    void *addr = GET_PTR(void, m_metadata, m_metadata->size);
    m_metadata->size += size;
    if((m_metadata->size < size) || (m_metadata->size > METADATA_SIZE))
    {
        return NULL;
    }
    return addr;
}

/*
* Called within build_layout_table(), used to assign the rva to entry layout  
* and load_step to group layout.
*/
bool CMetadata::update_layout_entries()
{
    m_rva = calculate_sections_size();
    if(m_rva == 0)
    {
        se_trace(SE_TRACE_ERROR, INVALID_ENCLAVE_ERROR);
        se_trace(SE_TRACE_ERROR, "Sections size is 0\n");
        return false;
    }

    SE_TRACE_DEBUG("\n");

    for(uint32_t i = 0; i < m_layouts.size(); i++)
    {
        if(!IS_GROUP_ID(m_layouts[i].entry.id))
        {
            m_layouts[i].entry.rva = m_rva;
            m_rva += (((uint64_t)m_layouts[i].entry.page_count) << SE_PAGE_SHIFT);

            se_trace(SE_TRACE_DEBUG, "\tEntry Id(%2u) = %4u, %-16s,  ", i, m_layouts[i].entry.id, layout_id_str[m_layouts[i].entry.id]);
            se_trace(SE_TRACE_DEBUG, "Page Count = %5u,  ", m_layouts[i].entry.page_count);
            se_trace(SE_TRACE_DEBUG, "Attributes = 0x%02X,  ", m_layouts[i].entry.attributes);
            se_trace(SE_TRACE_DEBUG, "Flags = 0x%016llX,  ", m_layouts[i].entry.si_flags);
            se_trace(SE_TRACE_DEBUG, "RVA = 0x%016llX --- 0x%016llX\n", m_layouts[i].entry.rva, m_rva-1);
        }
        else
        {
           for (uint32_t j = 0; j < m_layouts[i].group.entry_count; j++)
           {
                m_layouts[i].group.load_step += ((uint64_t)(m_layouts[i-j-1].entry.page_count)) << SE_PAGE_SHIFT;
           }
           uint64_t temp_rva = m_rva;
           m_rva += m_layouts[i].group.load_times * m_layouts[i].group.load_step;

           se_trace(SE_TRACE_DEBUG, "\tEntry Id(%2u) = %4u, %-16s,  ", i, m_layouts[i].entry.id, layout_id_str[m_layouts[i].entry.id & ~(GROUP_FLAG)]);
           se_trace(SE_TRACE_DEBUG, "Entry Count = %4u,  ", m_layouts[i].group.entry_count);
           se_trace(SE_TRACE_DEBUG, "Load Times = %u,     ", m_layouts[i].group.load_times);
           se_trace(SE_TRACE_DEBUG, "LStep = 0x%016llX,  ", m_layouts[i].group.load_step);
           se_trace(SE_TRACE_DEBUG, "RVA = 0x%016llX --- 0x%016llX\n", temp_rva, m_rva-1);
        }
    }
    return true;
}

static inline bool is_power_of_two(size_t n)
{
    return (n != 0) && (!(n & (n - 1)));
}

bool CMetadata::vaildate_elrange_config()
{
    if(m_elrange_config_entry.elrange_size == 0)
    {
        return true;
    }

    if(m_elrange_config_entry.enclave_image_address % SE_PAGE_SIZE != 0)
    {
        se_trace(SE_TRACE_ERROR, SET_ENCLAVEIMAGEADDRESS_ALIGN_ERROR);
        return false;
    }
    
    if(m_elrange_config_entry.elrange_start_address % SE_PAGE_SIZE != 0)
    {
        se_trace(SE_TRACE_ERROR, SET_ELRANGESTARTADDRESS_PAGE_ALIGN_ERROR);
        return false;
    }

    if(m_elrange_config_entry.elrange_size%SE_PAGE_SIZE != 0)
    {
        se_trace(SE_TRACE_ERROR, SET_ELRANGE_PAGE_ALIGN_ERROR);
        return false;
    }
    
    if((m_elrange_config_entry.elrange_start_address & (m_elrange_config_entry.elrange_size -1)) != 0)
    {
        se_trace(SE_TRACE_ERROR, SET_ELRANGESTARTADDRESS_ALIGN_ERROR);
        return false;
    }
    
    if(m_elrange_config_entry.elrange_start_address > m_elrange_config_entry.enclave_image_address)
    {
        se_trace(SE_TRACE_ERROR, SET_ELRANGESTARTADDRESS_RANGE_ERROR);
        return false;
    }

    uint64_t enclave_end = m_elrange_config_entry.enclave_image_address + m_rva;
    if(enclave_end < m_elrange_config_entry.enclave_image_address || enclave_end < m_rva)
    {
        se_trace(SE_TRACE_ERROR, SET_ENCLAVEIMAGEADDRESS_ERROR);
        return false;
    }

    uint64_t elrange_end = m_elrange_config_entry.elrange_start_address+ m_elrange_config_entry.elrange_size;
    if(elrange_end < m_elrange_config_entry.elrange_start_address || elrange_end < m_elrange_config_entry.elrange_size)
    {
        se_trace(SE_TRACE_ERROR, SET_ELRANGE_ERROR);
        return false;
    }
    
    if(enclave_end > elrange_end)
    {
        se_trace(SE_TRACE_ERROR, SET_ELRANGE_RANGE_ERROR);
        return false;
    }
    
    if(!is_power_of_two(m_elrange_config_entry.elrange_size))
    {
        se_trace(SE_TRACE_ERROR, SET_ELRANGE_ALIGN_ERROR);
        return false;
    }
    return true;

}

bool CMetadata::build_elrange_config_entry()
{
    if(m_elrange_config_entry.elrange_size == 0)
    {
        return true;
    }
    //place the elrange config entry at the beginning of data
    uint32_t size = (uint32_t)(sizeof(data_directory_t));
    data_directory_t* dir = (data_directory_t*)alloc_buffer_from_metadata(size);
    if(dir == NULL)
    {
        se_trace(SE_TRACE_ERROR, INVALID_ENCLAVE_ERROR);
        se_trace(SE_TRACE_ERROR, "Elrange config table could not be allocated\n");
        return false;
    }

    size = (uint32_t)(sizeof(elrange_config_entry_t));
    elrange_config_entry_t *config_table = (elrange_config_entry_t *) alloc_buffer_from_metadata(size);
    if(config_table == NULL)
    {
        se_trace(SE_TRACE_ERROR, INVALID_ENCLAVE_ERROR);
        se_trace(SE_TRACE_ERROR, "Elrange config table could not be allocated\n");
        return false;
    }
    
    config_table->elrange_size = m_elrange_config_entry.elrange_size;
    config_table->elrange_start_address = m_elrange_config_entry.elrange_start_address;
    config_table->enclave_image_address = m_elrange_config_entry.enclave_image_address;
    
    dir->offset = (uint32_t)PTR_DIFF(config_table, m_metadata);
    dir->size = size;
    
    return true;
}

bool CMetadata::build_layout_entries()
{
    SE_TRACE_DEBUG("\n");

    // enclave virtual size
    m_metadata->enclave_size = calculate_enclave_size(m_rva);
    if (m_metadata->enclave_size == (uint64_t)-1)
    {
        se_trace(SE_TRACE_ERROR, OUT_OF_EPC_ERROR);
        return false;
    }

    // Add extra EPC pages to round the enclave size to power of 2
    // We add a layout of guard pages
    if (m_metadata->enclave_size - m_rva > 0)
    {
        uint32_t extra_pages = (uint32_t)((m_metadata->enclave_size - m_rva) >> SE_PAGE_SHIFT);
        layout_t layout;
        memset(&layout, 0, sizeof(layout));
        layout.entry.id = LAYOUT_ID_GUARD;
        layout.entry.rva = m_rva;
        layout.entry.page_count = extra_pages;
        m_layouts.push_back(layout);

        se_trace(SE_TRACE_DEBUG, "\tEntry Id(%2u) = %4u, %-16s,  ", 0, layout.entry.id, layout_id_str[layout.entry.id]);
        se_trace(SE_TRACE_DEBUG, "Page Count = %5u,  ", layout.entry.page_count);
        se_trace(SE_TRACE_DEBUG, "Attributes = 0x%02X,  ", layout.entry.attributes);
        se_trace(SE_TRACE_DEBUG, "Flags = 0x%016llX,  ", layout.entry.si_flags);
        se_trace(SE_TRACE_DEBUG, "RVA = 0x%016llX --- 0x%016llX\n", layout.entry.rva, m_metadata->enclave_size - 1);
    }

    uint32_t size = (uint32_t)(m_layouts.size() * sizeof(layout_t));

    layout_t *layout_table = (layout_t *) alloc_buffer_from_metadata(size);
    if(layout_table == NULL)
    {
        se_trace(SE_TRACE_ERROR, INVALID_ENCLAVE_ERROR);
        se_trace(SE_TRACE_ERROR, "Layout table could not be allocated\n");
        return false;
    }
    m_metadata->dirs[DIR_LAYOUT].offset = (uint32_t)PTR_DIFF(layout_table, m_metadata);
    m_metadata->dirs[DIR_LAYOUT].size = size;


    for(uint32_t i = 0; i < m_layouts.size(); i++, layout_table++)
    {
        memcpy_s(layout_table, sizeof(layout_t), &m_layouts[i], sizeof(layout_t));
    }

    return true;
}

bool CMetadata::build_layout_table()
{
    layout_t layout;
    memset(&layout, 0, sizeof(layout));

    layout_t guard_page;
    memset(&guard_page, 0, sizeof(guard_page));
    guard_page.entry.id = LAYOUT_ID_GUARD;
    guard_page.entry.page_count = SE_GUARD_PAGE_SIZE >> SE_PAGE_SHIFT;

    std::vector<layout_t> thread_layouts;
    // heap
    layout.entry.id = LAYOUT_ID_HEAP_MIN;
    layout.entry.page_count = (uint32_t)(m_create_param.heap_min_size >> SE_PAGE_SHIFT);
    layout.entry.attributes = PAGE_ATTR_EADD;
    layout.entry.si_flags = SI_FLAGS_RW;
    m_layouts.push_back(layout);

    if(m_create_param.heap_init_size > m_create_param.heap_min_size)
    {
        layout.entry.id = LAYOUT_ID_HEAP_INIT;
        layout.entry.page_count = (uint32_t)((m_create_param.heap_init_size - m_create_param.heap_min_size) >> SE_PAGE_SHIFT);
        layout.entry.attributes = PAGE_ATTR_EADD | PAGE_ATTR_POST_REMOVE | PAGE_ATTR_POST_ADD;
        layout.entry.si_flags = SI_FLAGS_RW;
        m_layouts.push_back(layout);
    }

    if(m_create_param.heap_max_size > m_create_param.heap_init_size)
    {
        layout.entry.id = LAYOUT_ID_HEAP_MAX;
        layout.entry.page_count = (uint32_t)((m_create_param.heap_max_size - m_create_param.heap_init_size) >> SE_PAGE_SHIFT);
        layout.entry.attributes = PAGE_ATTR_POST_ADD;
        layout.entry.si_flags = SI_FLAGS_RW;
        m_layouts.push_back(layout);
    }

    // thread context memory layout
    // guard page | stack | guard page | TCS | SSA | guard page | TLS

    // vector 'thread_layouts' serves as a template for thread context
    // guard page
    thread_layouts.push_back(guard_page);

    // stack
    if(m_create_param.stack_max_size > m_create_param.stack_min_size)
    {
        layout.entry.id = LAYOUT_ID_STACK_MAX;
        layout.entry.page_count = (uint32_t)((m_create_param.stack_max_size - m_create_param.stack_min_size) >> SE_PAGE_SHIFT);
        layout.entry.attributes = PAGE_ATTR_EADD | PAGE_ATTR_EEXTEND | PAGE_DIR_GROW_DOWN;
        layout.entry.si_flags = SI_FLAGS_RW;
        layout.entry.content_size = 0xCCCCCCCC;
        thread_layouts.push_back(layout);
    }
    layout.entry.id = LAYOUT_ID_STACK_MIN;
    layout.entry.page_count = (uint32_t)(m_create_param.stack_min_size >> SE_PAGE_SHIFT);
    layout.entry.attributes = PAGE_ATTR_EADD | PAGE_ATTR_EEXTEND;
    layout.entry.si_flags = SI_FLAGS_RW;
    layout.entry.content_size = 0xCCCCCCCC;
    thread_layouts.push_back(layout);

    // guard page
    thread_layouts.push_back(guard_page);

    // tcs
    layout.entry.id = LAYOUT_ID_TCS;
    layout.entry.page_count = TCS_SIZE >> SE_PAGE_SHIFT;
    layout.entry.attributes = PAGE_ATTR_EADD | PAGE_ATTR_EEXTEND;
    layout.entry.si_flags = SI_FLAGS_TCS;
    tcs_t *tcs_template = (tcs_t *) alloc_buffer_from_metadata(TCS_TEMPLATE_SIZE);
    if(tcs_template == NULL)
    {
        se_trace(SE_TRACE_ERROR, INVALID_ENCLAVE_ERROR);
        return false;
    }
    layout.entry.content_offset = (uint32_t)PTR_DIFF(tcs_template, m_metadata),
    layout.entry.content_size = TCS_TEMPLATE_SIZE;
    thread_layouts.push_back(layout);
    memset(&layout, 0, sizeof(layout));

    // ssa
    layout.entry.id = LAYOUT_ID_SSA;
    layout.entry.page_count = SSA_FRAME_SIZE * SSA_NUM;
    layout.entry.attributes = PAGE_ATTR_EADD | PAGE_ATTR_EEXTEND;
    layout.entry.si_flags = SI_FLAGS_RW;
    thread_layouts.push_back(layout);

    // guard page
    thread_layouts.push_back(guard_page);

    // td
    layout.entry.id = LAYOUT_ID_TD;
    layout.entry.page_count = 1;
    const Section *section = m_parser->get_tls_section();
    if(section)
    {
        layout.entry.page_count += (uint32_t)(ROUND_TO_PAGE(section->virtual_size()) >> SE_PAGE_SHIFT);
    }
    layout.entry.attributes = PAGE_ATTR_EADD | PAGE_ATTR_EEXTEND;
    layout.entry.si_flags = SI_FLAGS_RW;
    thread_layouts.push_back(layout);

    // adding utility thread context, part of its stack can be added and removed dynamically
    for (auto l : thread_layouts)
    {
        if (l.entry.id == LAYOUT_ID_STACK_MAX)
            l.entry.attributes |= PAGE_ATTR_POST_ADD | PAGE_ATTR_POST_REMOVE;
        m_layouts.push_back(l);
    }

    uint32_t tcs_min_pool = 0; /* Number of static threads (EADD) */
    uint32_t tcs_eremove = 0;
    if(m_create_param.tcs_min_pool > m_create_param.tcs_num - 1)
    {
        tcs_min_pool = m_create_param.tcs_num - 1;
        tcs_eremove = 0;
    }
    else
    {
        tcs_min_pool = m_create_param.tcs_min_pool;
        tcs_eremove = m_create_param.tcs_num -1 - m_create_param.tcs_min_pool;
    }

    // adding thread contexts corresponding to tcs_min_pool
    /* There won't be any static threads if
     * m_create_param.tcs_num is 1 or
     * m_create_param_tcs_min_pool is 0 */
    if (tcs_min_pool > 0)
    {
        auto end = m_layouts.end();
        m_layouts.insert(end, thread_layouts.begin(), thread_layouts.end());

        if (tcs_min_pool > 1)
        {
            // group for tcs min pool
            memset(&layout, 0, sizeof(layout));
            layout.group.id = LAYOUT_ID_THREAD_GROUP;
            layout.group.entry_count = (uint16_t)(thread_layouts.size());
            layout.group.load_times = tcs_min_pool - 1;
            m_layouts.push_back(layout);
        }
    }
    // adding thread contexts corresponding to tcs_eremove
    if (tcs_eremove > 0)
    {
        for(auto l : thread_layouts)
        {
            if(l.entry.id != LAYOUT_ID_GUARD)
            {
                l.entry.attributes |= PAGE_ATTR_EREMOVE;
            }
            m_layouts.push_back(l);
        }

        if (tcs_eremove > 1)
        {
            memset(&layout, 0, sizeof(layout));
            layout.group.id = LAYOUT_ID_THREAD_GROUP;
            layout.group.entry_count = (uint16_t)(thread_layouts.size());
            layout.group.load_times = tcs_eremove - 1;
            m_layouts.push_back(layout);
        }
    }
    // dynamic thread contexts
    if (m_create_param.tcs_max_num > tcs_min_pool + 1)
    {
        for(auto l : thread_layouts)
        {
            if(l.entry.id == LAYOUT_ID_STACK_MAX)
            {
                l.entry.id = (uint16_t)(LAYOUT_ID_HEAP_DYN_MIN - LAYOUT_ID_HEAP_MIN + l.entry.id);
                l.entry.attributes = PAGE_ATTR_POST_ADD | PAGE_DIR_GROW_DOWN;
            }
            else if(l.entry.id != LAYOUT_ID_GUARD)
            {
                l.entry.id = (uint16_t)(LAYOUT_ID_HEAP_DYN_MIN - LAYOUT_ID_HEAP_MIN + l.entry.id);
                l.entry.attributes = PAGE_ATTR_POST_ADD | PAGE_ATTR_DYN_THREAD;
            }
            m_layouts.push_back(l);
        }

        // dynamic thread group
        memset(&layout, 0, sizeof(layout));
        layout.group.id = LAYOUT_ID_THREAD_GROUP_DYN;
        layout.group.entry_count = (uint16_t)(thread_layouts.size());
        layout.group.load_times = m_create_param.tcs_max_num - tcs_min_pool - 1;
        m_layouts.push_back(layout);
    }

    // RSRV region
    if (m_create_param.rsrv_min_size > 0 ||
        m_create_param.rsrv_init_size > 0 ||
        m_create_param.rsrv_max_size > 0)
    {
        memset(&layout, 0, sizeof(layout));
        layout.entry.id = LAYOUT_ID_RSRV_MIN;
        layout.entry.page_count = (uint32_t)(m_create_param.rsrv_min_size >> SE_PAGE_SHIFT);
        layout.entry.attributes = PAGE_ATTR_EADD;
        layout.entry.si_flags = m_create_param.rsrv_executable ? SI_FLAGS_RWX : SI_FLAGS_RW;
        m_layouts.push_back(layout);

        if (m_create_param.rsrv_init_size > m_create_param.rsrv_min_size)
        {
            layout.entry.id = LAYOUT_ID_RSRV_INIT;
            layout.entry.page_count = (uint32_t)((m_create_param.rsrv_init_size - m_create_param.rsrv_min_size) >> SE_PAGE_SHIFT);
            layout.entry.attributes = PAGE_ATTR_EADD | PAGE_ATTR_POST_REMOVE | PAGE_ATTR_POST_ADD;
            layout.entry.si_flags = m_create_param.rsrv_executable ? SI_FLAGS_RWX : SI_FLAGS_RW;
            m_layouts.push_back(layout);
        }

        if (m_create_param.rsrv_max_size > m_create_param.rsrv_init_size)
        {
            layout.entry.id = LAYOUT_ID_RSRV_MAX;
            layout.entry.page_count = (uint32_t)((m_create_param.rsrv_max_size - m_create_param.rsrv_init_size) >> SE_PAGE_SHIFT);
            layout.entry.attributes = PAGE_ATTR_POST_ADD;
            layout.entry.si_flags = SI_FLAGS_RW;
            m_layouts.push_back(layout);
        }
    }

    // update layout entries
    if(false == update_layout_entries())
    {
        return false;
    }

    // tcs template
    if(false == build_tcs_template(tcs_template))
    {
        se_trace(SE_TRACE_ERROR, INVALID_ENCLAVE_ERROR);
        se_trace(SE_TRACE_ERROR, "Could not build TCS template\n");
        return false;
    }
    return true;
}

bool CMetadata::build_patch_entries(std::vector<patch_entry_t> &patches)
{
    uint32_t size = (uint32_t)(patches.size() * sizeof(patch_entry_t));
    patch_entry_t *patch_table = (patch_entry_t *) alloc_buffer_from_metadata(size);
    if(patch_table == NULL)
    {
        se_trace(SE_TRACE_ERROR, INVALID_ENCLAVE_ERROR);
        se_trace(SE_TRACE_ERROR, "Could not allocate patch table");
        return false;
    }
    m_metadata->dirs[DIR_PATCH].offset = (uint32_t)PTR_DIFF(patch_table, m_metadata);
    m_metadata->dirs[DIR_PATCH].size = size;

    for(uint32_t i = 0; i < patches.size(); i++)
    {
        memcpy_s(patch_table, sizeof(patch_entry_t), &patches[i], sizeof(patch_entry_t));
        patch_table++;
    }
    return true;
}
bool CMetadata::build_patch_table()
{
    const uint8_t *base_addr = (const uint8_t *)m_parser->get_start_addr();
    std::vector<patch_entry_t> patches;
    patch_entry_t patch;
    memset(&patch, 0, sizeof(patch));

    // td template
    m_gd_size = m_parser->get_global_data_size();
    m_gd_template = (uint8_t *)alloc_buffer_from_metadata(m_gd_size);
    if(m_gd_template == NULL)
    {
        return false;
    }

    uint64_t rva = m_parser->get_symbol_rva("g_global_data");
    if(0 == rva)
    {
        se_trace(SE_TRACE_ERROR, INVALID_ENCLAVE_ERROR);
        se_trace(SE_TRACE_ERROR, "Could not find g_global_data\n");
         return false;
    }
    
    // Check whether the same SDK is used for enclave building and signing
    global_data_t *gdata = (global_data_t *)get_rawdata_by_rva(rva);
    if (gdata -> sdk_version != VERSION_UINT)
    {
        se_trace(SE_TRACE_ERROR, SDK_VERSION_ERROR);
        return false;
    }

    patch.dst = (uint64_t)PTR_DIFF(gdata, base_addr);
    patch.src = (uint32_t)PTR_DIFF(m_gd_template, m_metadata);
    patch.size = m_gd_size;
    patches.push_back(patch);

    // patch the image header
    uint32_t size = 0;
    uint8_t *zero = (uint8_t *)alloc_buffer_from_metadata(0);  // get addr only, size will be determined later
    if(zero == NULL)
    {
        se_trace(SE_TRACE_ERROR, INVALID_ENCLAVE_ERROR);
        se_trace(SE_TRACE_ERROR, "Could not allocate 0 bytes\n");
        return false;
    }
    bin_fmt_t bf = m_parser->get_bin_format();
    if(bf == BF_ELF32)
    {
        Elf32_Ehdr *elf_hdr = (Elf32_Ehdr *)base_addr;
        patch.dst = (uint64_t)PTR_DIFF(&elf_hdr->e_shnum, base_addr);
        patch.src = (uint32_t)PTR_DIFF(zero, m_metadata);
        patch.size = (uint32_t)sizeof(elf_hdr->e_shnum);
        patches.push_back(patch);
        size = patch.size;

        patch.dst = (uint64_t)PTR_DIFF(&elf_hdr->e_shoff, base_addr);
        patch.src = (uint32_t)PTR_DIFF(zero, m_metadata);
        patch.size = (uint32_t)sizeof(elf_hdr->e_shoff);
        patches.push_back(patch);
        if (patch.size > size) size = patch.size;

        patch.dst = (uint64_t)PTR_DIFF(&elf_hdr->e_shstrndx, base_addr);
        patch.src = (uint32_t)PTR_DIFF(zero, m_metadata);
        patch.size = (uint32_t)sizeof(elf_hdr->e_shstrndx);
        patches.push_back(patch);
        if (patch.size > size) size = patch.size;
 
        // Modify GNU_RELRO info to eliminate the impact of enclave measurement.
        Elf32_Phdr *prg_hdr = GET_PTR(Elf32_Phdr, base_addr, elf_hdr->e_phoff);
        for (unsigned idx = 0; idx < elf_hdr->e_phnum; ++idx, ++prg_hdr)
        {
            if(prg_hdr->p_type == PT_GNU_RELRO)
            {
                patch.dst = (uint64_t)PTR_DIFF(prg_hdr, base_addr);
                patch.src = (uint32_t)PTR_DIFF(zero, m_metadata);
                patch.size = (uint32_t)sizeof(Elf32_Phdr);
                patches.push_back(patch);
                if (patch.size > size) size = patch.size;
                break;
            }
        }
    }
    else if(bf == BF_ELF64)
    {
        Elf64_Ehdr *elf_hdr = (Elf64_Ehdr *)base_addr;

        patch.dst = (uint64_t)PTR_DIFF(&elf_hdr->e_shnum, base_addr);
        patch.src = (uint32_t)PTR_DIFF(zero, m_metadata);
        patch.size = (uint32_t)sizeof(elf_hdr->e_shnum);
        patches.push_back(patch);
        size = patch.size;

        patch.dst = (uint64_t)PTR_DIFF(&elf_hdr->e_shoff, base_addr);
        patch.src = (uint32_t)PTR_DIFF(zero, m_metadata);
        patch.size = (uint32_t)sizeof(elf_hdr->e_shoff);
        patches.push_back(patch);
        if (patch.size > size) size = patch.size;

        patch.dst = (uint64_t)PTR_DIFF(&elf_hdr->e_shstrndx, base_addr);
        patch.src = (uint32_t)PTR_DIFF(zero, m_metadata);
        patch.size = (uint32_t)sizeof(elf_hdr->e_shstrndx);
        patches.push_back(patch);
        if (patch.size > size) size = patch.size;
    }
    zero = (uint8_t *)alloc_buffer_from_metadata(size); // alloc buffer again with the accurate size
    if(zero == NULL)
    {
        se_trace(SE_TRACE_ERROR, INVALID_ENCLAVE_ERROR);
        return false;
    }
    memset(zero, 0, size);

    if(false == build_patch_entries(patches))
    {
        se_trace(SE_TRACE_ERROR, NO_MEMORY_ERROR); 
        return false;
    }
    return true;
}

layout_entry_t *CMetadata::get_entry_by_id(uint16_t id, bool do_assert = true)
{
    for (uint32_t i = 0; i < m_layouts.size(); i++)
    {
        if(m_layouts[i].entry.id == id)
            return (layout_entry_t *)&m_layouts[i];
    }
    if (do_assert)
        assert(false);
    return NULL;
}

bool CMetadata::get_xsave_size(uint64_t xfrm, uint32_t *xsave_size)
{
    assert (xsave_size != NULL);

    struct {
        uint64_t bits;
        uint32_t size;
    } xsave_size_table[] = { // Note that the xsave_size should be in ascending order
        {SGX_XFRM_LEGACY, 512 + 64},                    // 512 for legacy features, 64 for xsave header
        {SGX_XFRM_AVX,    512 + 64 + 256},              // 256 for YMM0_H - YMM15_H registers
        {SGX_XFRM_MPX,    512 + 64 + 256 + 256},        // 256 for MPX
        {SGX_XFRM_AVX512, 512 + 64 + 256 + 256 + 1600}, // 1600 for k0 - k7, ZMM0_H - ZMM15_H, ZMM16 - ZMM31
	// Ignore PT as PT is a supervisor state.
        {SGX_XFRM_PKRU,     512 + 64 + 256 + 256 + 1600 + 64}, // 8 for PKRU, 56 for alignment
    };
    bool ret = true;
    *xsave_size = 0;
    if(!xfrm || (xfrm & SGX_XFRM_RESERVED))
    {
        return false;
    }
    for(size_t i = 0; i < sizeof(xsave_size_table)/sizeof(xsave_size_table[0]); i++)
    {
        if((xfrm & xsave_size_table[i].bits) == xsave_size_table[i].bits)
        {
            *xsave_size = xsave_size_table[i].size;
        }
    }
    return ret;
}

bool CMetadata::build_gd_template(uint8_t *data, uint32_t *data_size)
{
    if(false == get_xsave_size(m_metadata->attributes.xfrm, &m_create_param.xsave_size))
    {
        return false;
    }

    m_create_param.stack_base_addr = (size_t)(get_entry_by_id(LAYOUT_ID_STACK_MIN)->rva + m_create_param.stack_min_size - get_entry_by_id(LAYOUT_ID_TCS)->rva);
    m_create_param.stack_limit_addr = (size_t)(m_create_param.stack_base_addr - m_create_param.stack_max_size);
    m_create_param.ssa_base_addr = (size_t)(get_entry_by_id(LAYOUT_ID_SSA)->rva - get_entry_by_id(LAYOUT_ID_TCS)->rva);
    m_create_param.enclave_size = m_metadata->enclave_size;
    m_create_param.heap_offset = (size_t)get_entry_by_id(LAYOUT_ID_HEAP_MIN)->rva;
    layout_entry_t * layout_rsrv = get_entry_by_id(LAYOUT_ID_RSRV_MIN, false);
    if (NULL == layout_rsrv)
    {
        m_create_param.rsrv_offset = (size_t)0;
    }
    else
    {
        m_create_param.rsrv_offset = (size_t)layout_rsrv->rva;
    }

    size_t tmp_tls_addr = (size_t)(get_entry_by_id(LAYOUT_ID_TD)->rva - get_entry_by_id(LAYOUT_ID_TCS)->rva);
    m_create_param.td_addr = tmp_tls_addr + (size_t)((get_entry_by_id(LAYOUT_ID_TD)->page_count - 1) << SE_PAGE_SHIFT);

    const Section *section = m_parser->get_tls_section();
    if(section)
    {
        /* adjust the tls_addr to be the pointer to the actual TLS data area */
        m_create_param.tls_addr = (size_t)(m_create_param.td_addr - section->virtual_size());
        assert(TRIM_TO_PAGE(m_create_param.tls_addr) == tmp_tls_addr);
    }
    else
        m_create_param.tls_addr = tmp_tls_addr;

    if(false == m_parser->update_global_data(m_metadata, &m_create_param, data, data_size))
    {
        se_trace(SE_TRACE_ERROR, NO_MEMORY_ERROR);  // metadata structure doesnot have enough memory for global_data template
        return false;
    }
    return true;
}

bool CMetadata::build_tcs_template(tcs_t *tcs)
{
    uint64_t offset = 0;
    if(m_elrange_config_entry.elrange_size != 0)
    {
        offset = m_elrange_config_entry.enclave_image_address - m_elrange_config_entry.elrange_start_address;
    }
    tcs->oentry = m_parser->get_symbol_rva("enclave_entry");
    if(tcs->oentry == 0)
    {
        return false;
    }
    tcs->oentry += offset;
    tcs->nssa = SSA_NUM;
    tcs->cssa = 0;
    tcs->ossa = get_entry_by_id(LAYOUT_ID_SSA)->rva - get_entry_by_id(LAYOUT_ID_TCS)->rva + offset;
    //fs/gs pointer at TLS/TD
    tcs->ofs_base = tcs->ogs_base = get_entry_by_id(LAYOUT_ID_TD)->rva - get_entry_by_id(LAYOUT_ID_TCS)->rva + 
        (((uint64_t)get_entry_by_id(LAYOUT_ID_TD)->page_count - 1) << SE_PAGE_SHIFT) + offset;
    tcs->ofs_limit = tcs->ogs_limit = (uint32_t)-1;
    return true;
}

void* CMetadata::get_rawdata_by_rva(uint64_t rva)
{
    std::vector<Section*> sections = m_parser->get_sections();

    for(unsigned int i = 0; i < sections.size() ; i++)
    {
        uint64_t start_rva = TRIM_TO_PAGE(sections[i]->get_rva());
        uint64_t end_rva = ROUND_TO_PAGE(sections[i]->get_rva() + sections[i]->virtual_size());
        if(start_rva <= rva && rva < end_rva)
        {
            uint64_t offset = rva - sections[i]->get_rva();
            if (offset > sections[i]->raw_data_size())
            {
                return 0;
            }
            return GET_PTR(void, sections[i]->raw_data(), offset);
        }
    }

    return 0;
}

uint64_t CMetadata::calculate_sections_size()
{
    std::vector<Section*> sections = m_parser->get_sections();
    uint64_t max_rva = 0;
    Section *last_section = NULL;

    for(unsigned int i = 0; i < sections.size() ; i++)
    {
        if(sections[i]->get_rva() > max_rva) {
            max_rva = sections[i]->get_rva();
            last_section = sections[i];
        }
    }

    uint64_t size = (NULL == last_section) ? (0) : (last_section->get_rva() + last_section->virtual_size());
    size = ROUND_TO_PAGE(size); 

    return size;
}

uint64_t CMetadata::calculate_enclave_size(uint64_t size)
{
    uint64_t enclave_max_size = m_parser->get_enclave_max_size();

    if(size > enclave_max_size)
        return (uint64_t)-1;

    uint64_t round_size = 1;
    while (round_size < size)
    {
        round_size <<=1;
        if(!round_size)
            return (uint64_t)-1;
    }

    se_trace(SE_TRACE_DEBUG, "Enclave: Size = 0x%016llX, Rounded Size = 0x%016llX\n", size, round_size);
    if(round_size > enclave_max_size)
        return (uint64_t)-1;

    return round_size;
}

bool update_metadata(const char *path, const metadata_t *metadata, uint64_t meta_offset)
{
    assert(path != NULL && metadata != NULL);

    return write_data_to_file(path, std::ios::in | std::ios::binary| std::ios::out,
        reinterpret_cast<uint8_t *>(const_cast<metadata_t *>( metadata)), METADATA_SIZE, (long)meta_offset);
}


#define PRINT_ELEMENT(stream, structure, element) \
    do {                                          \
        (stream) << #structure << "->" << #element << ": " << std::hex << "0x" << structure->element << std::endl; \
    }while(0)

#define PRINT_ARRAY(stream, structure, array, size) \
    do{  \
        (stream) << #structure << "->" << #array << ":" << std::hex; \
        for(size_t i = 0; i < size; i++) \
        { \
            if (i % 16 == 0) (stream) << std::endl; \
            (stream) << "0x" << std::setfill('0') << std::setw(2) << (uint32_t)(structure)->array[i] << " "; \
        } \
        (stream) << std::endl; \
    }while(0)

#define CONCAT(name, num) name##num
#define A(num)     CONCAT(metadata, num)
static void print_metadata_internal(std::ofstream &meta_ofs, const metadata_t *metadata)
{
    assert(metadata != NULL);
    PRINT_ELEMENT(meta_ofs, metadata, magic_num);
    PRINT_ELEMENT(meta_ofs, metadata, version);
    PRINT_ELEMENT(meta_ofs, metadata, size);
    PRINT_ELEMENT(meta_ofs, metadata, tcs_policy);
    PRINT_ELEMENT(meta_ofs, metadata, ssa_frame_size);
    PRINT_ELEMENT(meta_ofs, metadata, max_save_buffer_size);
    PRINT_ELEMENT(meta_ofs, metadata, desired_misc_select);
    PRINT_ELEMENT(meta_ofs, metadata, enclave_size);
    PRINT_ELEMENT(meta_ofs, metadata, attributes.flags);
    PRINT_ELEMENT(meta_ofs, metadata, attributes.xfrm);

    // css.header
    PRINT_ARRAY(meta_ofs, metadata, enclave_css.header.header, 12);
    PRINT_ELEMENT(meta_ofs, metadata, enclave_css.header.type);
    PRINT_ELEMENT(meta_ofs, metadata, enclave_css.header.module_vendor);
    PRINT_ELEMENT(meta_ofs, metadata, enclave_css.header.date);
    PRINT_ARRAY(meta_ofs, metadata, enclave_css.header.header2, 16);
    PRINT_ELEMENT(meta_ofs, metadata, enclave_css.header.hw_version);

    // css.key
    PRINT_ARRAY(meta_ofs, metadata, enclave_css.key.modulus, SE_KEY_SIZE);
    PRINT_ARRAY(meta_ofs, metadata, enclave_css.key.exponent, SE_EXPONENT_SIZE);
    PRINT_ARRAY(meta_ofs, metadata, enclave_css.key.signature, SE_KEY_SIZE);

    // css.body
    PRINT_ELEMENT(meta_ofs, metadata, enclave_css.body.misc_select);
    PRINT_ELEMENT(meta_ofs, metadata, enclave_css.body.misc_mask);
    PRINT_ELEMENT(meta_ofs, metadata, enclave_css.body.attributes.flags);
    PRINT_ELEMENT(meta_ofs, metadata, enclave_css.body.attributes.xfrm);
    PRINT_ELEMENT(meta_ofs, metadata, enclave_css.body.attribute_mask.flags);
    PRINT_ELEMENT(meta_ofs, metadata, enclave_css.body.attribute_mask.xfrm);
    PRINT_ARRAY(meta_ofs, metadata, enclave_css.body.enclave_hash.m, SGX_HASH_SIZE);
    PRINT_ELEMENT(meta_ofs, metadata, enclave_css.body.isv_prod_id);
    PRINT_ELEMENT(meta_ofs, metadata, enclave_css.body.isv_svn);

    // css.buffer
    PRINT_ARRAY(meta_ofs, metadata, enclave_css.buffer.q1, SE_KEY_SIZE); 
    PRINT_ARRAY(meta_ofs, metadata, enclave_css.buffer.q2, SE_KEY_SIZE);
}

bool print_metadata(const char *path, const metadata_t *metadata)
{
    assert(path != NULL && metadata != NULL);

    std::ofstream meta_ofs(path, std::ofstream::out | std::ofstream::trunc);
    if (!meta_ofs.good())
    {
        se_trace(SE_TRACE_ERROR, OPEN_FILE_ERROR, path);
        return false;
     }

    meta_ofs << "=============================" << std::endl
        << "The metadata information:" << std::endl
        << "=============================" << std::endl;
    print_metadata_internal(meta_ofs, metadata);

    // Print the compatible metadata info
    size_t compat_meta_count = 0;
    do {
        metadata_t *compatible_metadata = GET_PTR(metadata_t, metadata, metadata->size);
        if(compatible_metadata == NULL || (compatible_metadata->magic_num == METADATA_MAGIC && compatible_metadata->size == 0))
        {
            meta_ofs.close();
            return false;
        }
        if(compatible_metadata->magic_num != METADATA_MAGIC)
            break;

        compat_meta_count++;

        if(compat_meta_count == 1)
        {
            meta_ofs << std::endl << std::endl << std::endl
                << "====================================" << std::endl
                << "The compatible metadata information: " << std::endl
                << "====================================" << std::endl;
        }
        meta_ofs << std::endl <<  "Compatible metadata number "
            << compat_meta_count << ":" << std::endl
            << "------------------------------" << std::endl;
        print_metadata_internal(meta_ofs, compatible_metadata);

        metadata = compatible_metadata;

    }while(1);

    typedef struct _mrsigner_t
    {
      uint8_t value[SGX_HASH_SIZE];
    } mrsigner_t;
    mrsigner_t ms;
    memset(&ms, 0, sizeof(mrsigner_t));
    mrsigner_t *mrsigner = &ms;
    unsigned int signer_len = SGX_HASH_SIZE;

    if(sgx_EVP_Digest(EVP_sha256(), metadata->enclave_css.key.modulus, SE_KEY_SIZE, ms.value, &signer_len) != SGX_SUCCESS)
    {
        se_trace(SE_TRACE_ERROR, "ERROR: failed to calculate the mrsigner.\n");
        meta_ofs.close();
        return false;
    }
    meta_ofs << std::endl << std::endl
        << "===================" << std::endl
        << "The mrsigner value:" << std::endl
        << "===================" << std::endl;
    PRINT_ARRAY(meta_ofs, mrsigner, value, SGX_HASH_SIZE);

    meta_ofs.close();
    return true;
}
