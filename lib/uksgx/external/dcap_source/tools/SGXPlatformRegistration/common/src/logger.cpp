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
 * File: comomon.cpp
 *  
 * Description: Shared utility functions for logging data.
 */
#include <stdio.h>
#include <stdarg.h>
#include <ctime>
#include <string>
using namespace std;

#include "logger.h"

LogLevel glog_level = MP_REG_LOG_LEVEL_ERROR;

#ifndef _WIN32

extern "C" void log_message_aux(
    LogLevel level,
	const char *format,
	va_list argptr)
{
	FILE *f = fopen(LOG_FILE, "a");
	if (f == NULL)
	{
		printf("Could not open log file. \n");
		return;
	}
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, 80, "[%d-%m-%Y %I:%M:%S] ", timeinfo);
	string str(buffer);
	fprintf(f, "%s", str.c_str());

    switch (level) {
        case MP_REG_LOG_LEVEL_ERROR:
            fprintf(f, "%s", "ERROR: ");
            break;
        case MP_REG_LOG_LEVEL_INFO:
        default:
            fprintf(f, "%s", "INFO: ");
            break;
    }

	vfprintf(f, format, argptr);
	fclose(f);
}

extern "C" void log_message(LogLevel level, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	log_message_aux(level, format, ap);
	va_end(ap);
}

#else

#include <Windows.h>
#include "messages.h"

#define SVCNAME				TEXT("IntelMPAService")
#define TIME_BUFFER_SIZE	80
#define MESSAGE_BUFFER_SIZE 512
#define LOG_FILE_NAME		"mpa.log"

#ifndef TEST_MODE
void SvcReportEvent(LogLevel level, char* buffer)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
    WORD eventType = 0;

	hEventSource = RegisterEventSource(NULL, SVCNAME);

	if (NULL != hEventSource)
	{
		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = buffer;
        
        switch (level) {
            case MP_REG_LOG_LEVEL_ERROR:
                eventType = EVENTLOG_ERROR_TYPE;
                break;
            case MP_REG_LOG_LEVEL_INFO:
            default:
                eventType = EVENTLOG_INFORMATION_TYPE;
                break;
        }

		ReportEvent(hEventSource,  // event log handle
            eventType, // event type
			0,					 // event category
			SVC_INFO,		     // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}

#else
static void writeToStdout(LogLevel level, const char* buffer)
{
    switch (level) {
        case MP_REG_LOG_LEVEL_ERROR:
            printf("%s", "ERROR: ");
            break;
        case MP_REG_LOG_LEVEL_INFO:
            printf("%s", "INFO: ");
            break;
        default:
            break;
    }

    printf("%s", buffer);
}
#endif

static void common_log_message(LogLevel level, const char *format, va_list ap)
{
	char buffer[MESSAGE_BUFFER_SIZE] = {};
	vsnprintf(buffer, MESSAGE_BUFFER_SIZE, format, ap);

#ifdef TEST_MODE
    writeToStdout(level, buffer);
#else
	SvcReportEvent(level, buffer);
#endif
}

#define CALL_COMMON_LOG {\
	va_list ap;\
    if (glog_level < level) return; \
	va_start(ap, format);\
	common_log_message(level, format, ap);\
	va_end(ap);}


extern "C" void agent_log_message(LogLevel level, const char *format, ...) CALL_COMMON_LOG
extern "C" void management_log_message(LogLevel level, const char *format, ...) CALL_COMMON_LOG
//Dlls
extern "C" void network_log_message_aux(LogLevel glog_level, LogLevel level, const char *format, ...) CALL_COMMON_LOG
extern "C" void uefi_log_message_aux(LogLevel glog_level, LogLevel level, const char *format, ...) CALL_COMMON_LOG

#endif
