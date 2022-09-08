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
 * File: RegistrationLogic.cpp
 *  
 * Description: Implemenation of the high level class that the agent uses to perform the 
 * registration flows.  It will perform agent intialization and determine 
 * if there the platform supports registration flows. 
 */
#include "RegistrationLogic.h"
#include "RegistrationService.h"
#include "AgentConfiguration.h"
#include "agent_logger.h"
#include "se_version.h"

#ifdef _WIN32
extern bool SvcUninstall();
#endif

void RegistrationLogic::registerPlatform() {
	MPConfigurations conf;
	AgentConfiguration agentConfigurations;
    
    // Read agent configuration file
	agentConfigurations.read(conf);

    // Create RegistrationService instance with the required configurations
    RegistrationService registrationService(conf);
    agent_log_message(MP_REG_LOG_LEVEL_FUNC, "SGX Registration Agent version: %s\n", STRPRODUCTVER);

#ifdef _WIN32
    if (!registrationService.isMultiPackageCapable()) {
        agent_log_message(MP_REG_LOG_LEVEL_FUNC, "Platform doesn't support registration. removing service..\n");
        if (!SvcUninstall()) {
            agent_log_message(MP_REG_LOG_LEVEL_FUNC, "Failed to remove service.\n");
        } else {
            agent_log_message(MP_REG_LOG_LEVEL_FUNC, "Successfully removed windows SGX registration service.\n");
        }
        return;
    }
    else {
        agent_log_message(MP_REG_LOG_LEVEL_INFO, "Multi-Package capable.\n");
    }

#endif

    agent_log_message(MP_REG_LOG_LEVEL_FUNC, "Starts Registration Agent Flow.\n");
    // Preform registration flow if needed
    registrationService.registerPlatformIfNeeded();
    agent_log_message(MP_REG_LOG_LEVEL_FUNC, "Finished Registration Agent Flow.\n");
}



