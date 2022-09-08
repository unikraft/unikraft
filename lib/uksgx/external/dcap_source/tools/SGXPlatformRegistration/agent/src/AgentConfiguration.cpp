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
 * File: AgentConfiguration.cpp
 *  
 * Description: Linux implementation for retrieving the MP Agent
 *              configurations.  
 */
#include "AgentConfiguration.h"
#include "agent_logger.h"
#include "common.h"
#include <sys/types.h>
#include <regex.h>
#include <string.h>

#define MAX_LINE 1024
#define URL_PATTERN "[[:blank:]]*(([http[s]?://)?[^[:blank:]]*)[[:blank:]]*"  //pattern used to match a URL which should be started with http:// or https://

#ifdef _WIN32
#define OS_PATH "[[:blank:]]*(\\(.*[\\])?)"
#define strncasecmp _strnicmp
#else
#define OS_PATH "[[:blank:]]*(/(.*[/])?)[[:blank:]]*"
#define fopen_s(pf,filename,mode) (((*(pf))=fopen((filename),(mode)))==NULL?1:0)
#endif
#define URL_PATH_PATTERN "([[:blank:]])*([/]{1}.*/?)([[:blank:]])*"  
#define SUBSCRIPTION_KEY_PATTERN "[[:blank:]]*([0-9a-fA-F]{32})([[:blank:]])*"
#define WORD_PATTERN "[[:blank:]]*([a-z]*)+([[:blank:]])*"
#define OPTION_COMMENT "(#.*)?"

typedef struct _config_entry_t{
    bool initialized;
    regex_t reg;
} config_entry_t;

enum _config_value_t{
    config_comment,
    config_space,
    config_proxy_url,
    config_proxy_type,
    config_uefi_path,
    config_server_subscription_key,
    config_log_level,
    config_value_nums
};

struct _config_patterns_t{
    enum _config_value_t id;
    const char *pattern;
} config_patterns[]={
    {config_comment, "^[[:blank:]]*#"},   //matching a line with comments only (It is started by #)
    {config_space, "^[[:blank:]]*$"},   //matching empty line
    {config_proxy_url,"^[[:blank:]]*proxy[[:blank:]]*url[[:blank:]]*=" URL_PATTERN OPTION_COMMENT "$"}, //matching line in format: proxy url= ...
    {config_proxy_type, "^[[:blank:]]*proxy[[:blank:]]*type[[:blank:]]*=[[:blank:]]*([^[:blank:]]+)[[:blank:]]*" OPTION_COMMENT "$"},//matching line in format: proxy type = [direct|default|manual]
    {config_uefi_path, "^[[:blank:]]*uefi[[:blank:]]*path[[:blank:]]*=" OS_PATH OPTION_COMMENT "$"}, //matching line in format: uefi path= ...
    {config_server_subscription_key, "^[[:blank:]]*subscription key[[:blank:]]*=" SUBSCRIPTION_KEY_PATTERN OPTION_COMMENT "$"}, //matching line in format: dd package subscription key = hexval
    {config_log_level, "^[[:blank:]]*log level[[:blank:]]*=" WORD_PATTERN OPTION_COMMENT "$"} //matching line in format: log level = info
};

static const char *log_level_name[]={
    "none",
    "func",
    "error",
    "info"
};

static const char *proxy_type_name[]={
    "default",
    "direct",
    "manual"
};

#define NUM_CONFIG_PATTERNS (sizeof(config_patterns)/sizeof(config_patterns[0]))
#define NUM_PROXY_TYPE (sizeof(proxy_type_name)/sizeof(proxy_type_name[0]))
#define NUM_LOG_LEVEL (sizeof(log_level_name)/sizeof(log_level_name[0]))
#define MAX_MATCHED_REG_EXP 5

//initialize all regular expression pattern
void init_config_patterns(config_entry_t entries[])
{
    uint32_t i;
    for(i=0;i<NUM_CONFIG_PATTERNS;++i){
       uint32_t entry_id = config_patterns[i].id;
       if(entry_id>=config_value_nums){
          agent_log_message(MP_REG_LOG_LEVEL_ERROR, "config id %d is too large\n", entry_id);
          continue;
       }
       if(entries[entry_id].initialized){
          agent_log_message(MP_REG_LOG_LEVEL_ERROR, "duplicated item for config id %d\n",entry_id);
          continue;
       }
       if(regcomp(&entries[entry_id].reg,config_patterns[i].pattern, REG_EXTENDED|REG_ICASE)!=0){
          agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Invalid config pattern %s\n", config_patterns[i].pattern);
          continue;
       }
       entries[entry_id].initialized=true;
    }
}

void release_config_patterns(config_entry_t entries[])
{
    uint32_t i;
    for(i=0;i<config_value_nums;++i){
        if(entries[i].initialized){
             entries[i].initialized=false;
             regfree(&entries[i].reg);
        }
    }
}

//function to decode log level type from string to integer value
LogLevel read_log_level(const char *str, uint32_t len)
{
     uint32_t i;
     for(i=0;i<NUM_LOG_LEVEL;++i){
        if(strncasecmp(log_level_name[i],str,len)==0){
            return (LogLevel)i;
        }
     }
     agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Invalid log level %.*s\n",len,str);
     return MP_REG_LOG_LEVEL_ERROR;
}

//function to decode proxy type from string to integer value
ProxyType read_proxy_type(const char *str, uint32_t len)
{
     uint32_t i;
     for(i=0;i<NUM_PROXY_TYPE;++i){
        if(strncasecmp(proxy_type_name[i],str,len)==0){
            return (ProxyType)i;
        }
     }
     agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Invalid proxy type %.*s\n",len,str);
     return MP_REG_PROXY_TYPE_DIRECT_ACCESS;
}

//Function to processing one line in config file
//  If any pattern is matched, get the correspondent data and set it into the output parameter 'infos'
bool config_process_one_line(const char *line, config_entry_t entries[], MPConfigurations& conf)
{
    uint32_t i;
    regmatch_t matches[MAX_MATCHED_REG_EXP];
    for(i=0;i<config_value_nums;++i){
        if(!entries[i].initialized){
            continue;
        }
        if(regexec(&entries[i].reg, line, MAX_MATCHED_REG_EXP, matches, 0)==0){
            switch(i){
            case config_comment:
            case config_space:
                //ignore comment and space only line
                break;
            case config_proxy_url:
                if(matches[1].rm_eo-matches[1].rm_so>= MAX_PATH_SIZE){
                    agent_log_message(MP_REG_LOG_LEVEL_ERROR, "too long proxy url in config file\n");
                }else{
                    memcpy(conf.proxy.proxy_url, line+matches[1].rm_so,matches[1].rm_eo-matches[1].rm_so);
                    conf.proxy.proxy_url[matches[1].rm_eo-matches[1].rm_so]='\0';
                }
                break;
            case config_proxy_type: //It is a proxy type, we need change the string to integer by calling function read_proxy_type
                conf.proxy.proxy_type = read_proxy_type(line+matches[1].rm_so, matches[1].rm_eo-matches[1].rm_so);
                break;
            case config_log_level: //It is a log level, we need change the string to integer by calling function read_log_level
                conf.log_level = read_log_level(line+matches[1].rm_so, matches[1].rm_eo-matches[1].rm_so);
                break;
            case config_uefi_path:
                if(matches[1].rm_eo-matches[1].rm_so>= MAX_PATH_SIZE){
                    agent_log_message(MP_REG_LOG_LEVEL_ERROR, "too long uefi path in config file\n");
                }else{
                    memcpy(conf.uefi_path, line+matches[1].rm_so, matches[1].rm_eo-matches[1].rm_so);
                    conf.uefi_path[matches[1].rm_eo-matches[1].rm_so]='\0';
                }
                break;
            case config_server_subscription_key:
                if(matches[1].rm_eo-matches[1].rm_so != SUBSCRIPTION_KEY_SIZE){
                    agent_log_message(MP_REG_LOG_LEVEL_ERROR, "too long subscription key in config file %d\n", SUBSCRIPTION_KEY_SIZE);
                } else {
                    memcpy(conf.server_add_package_subscription_key, line+matches[1].rm_so, matches[1].rm_eo-matches[1].rm_so);
                }
                break;
            default:
                agent_log_message(MP_REG_LOG_LEVEL_ERROR, "reg exp type %d not processed\n", i);
                break;
            }
            break;
        }
    }
    if(i>=config_value_nums){//the line matching nothing
        agent_log_message(MP_REG_LOG_LEVEL_ERROR, "mp_reg config file error: invalid line[%s]\n",line);
        return false;
    }
    return true;
}

bool AgentConfiguration::read(MPConfigurations& conf)
{
    char line[MAX_LINE];
    int line_no=0;
    bool ret = true;
    config_entry_t entries[config_value_nums];
    memset(&entries,0,sizeof(entries));
    memset(&conf, 0, sizeof(MPConfigurations));
    conf.log_level = MP_REG_LOG_LEVEL_ERROR;//default log level
    
    memcpy(&(conf.uefi_path), EFIVARS_FILE_SYSTEM, sizeof(EFIVARS_FILE_SYSTEM));
    
	string conf_file_path = getConfDirectory();
	if (conf_file_path.length() <= 0) {
		agent_log_message(MP_REG_LOG_LEVEL_ERROR, "failed to get configuration directory.\n");
		return false;
	}
	conf_file_path += "mpa_registration.conf";

	FILE *f = NULL;
	if (fopen_s(&f, conf_file_path.c_str(), "r") != 0 || f == NULL) {
         agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Cannot read configuration file %s\n", conf_file_path.c_str());
         return false;
    }

    init_config_patterns(entries);
    while(fgets(line, MAX_LINE, f)!=NULL){
        size_t len=strnlen(line, MAX_LINE);
        if(len>0&&line[len-1]=='\n')line[len-1]='\0';//remove the line ending
        line_no++;
        if(!config_process_one_line(line, entries, conf)){
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "format error in file %s:%d [%s]\n", conf_file_path.c_str(), line_no, line);
            ret = false;//continue process the file but save the error status
        }
    }
    release_config_patterns(entries);
    fclose(f);
    if(conf.proxy.proxy_type>=NUM_PROXY_TYPE||
          (conf.proxy.proxy_type==MP_REG_PROXY_TYPE_MANUAL_PROXY&&conf.proxy.proxy_url[0]=='\0')){
            agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Invalid proxy type %d\n",conf.proxy.proxy_type);
            conf.proxy.proxy_type = MP_REG_PROXY_TYPE_DEFAULT_PROXY;
            ret = false;
    }
    return ret;
}
