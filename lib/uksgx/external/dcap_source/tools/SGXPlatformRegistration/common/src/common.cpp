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
 * Description: Shared utility functions for file I/O
 */
#ifdef _WIN32
#include "atlstr.h"
#endif

#include "MPConfigurations.h"
#include "common.h"

#ifndef _WIN32
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),(mode)))==NULL
#endif

string getExeDirectory() {
#ifdef _WIN32
	TCHAR szPath[MAX_PATH];
	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		return "";
	}
	string path = szPath;
	string::size_type pos = path.find_last_of("\\");
	return path.substr(0, pos + 1);
#else
	return string(LINUX_INSTALL_PATH);
#endif
}

string getConfDirectory() {
#ifdef _WIN32
	return getExeDirectory();
#else
	return string(MP_REG_CONFIG_FILE);
#endif
}

int writeBufferToFile(const char *filename, uint8_t *buffer, size_t buffSize) {
    int ret = 0;
    size_t writtenSize = 0;
    FILE *file = NULL;

    if (fopen_s(&file, filename, "wb") || file == NULL) {
        //agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Failed to open file: %s\n", filename);
        ret = MP_INVALID_PARAMETER;
        goto out;
    }

    writtenSize = fwrite(buffer, sizeof(uint8_t), buffSize, file);
    if (writtenSize != buffSize) {
        //agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Failed to write data to file: %s\n", filename);
        ret = MP_UNEXPECTED_ERROR;
        goto out;
    }

out:
    if (file) {
        fclose(file);
    }
    return ret;
}

int readFileToBuffer(const char *filename, uint8_t *buffer, size_t &buffSize) {
    int ret = 0;
    size_t writtenSize = 0;
    FILE *file = NULL;
    long fileSize;

    if (fopen_s(&file, filename, "rb") || file == NULL) {
        //agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Failed to open file: %s\n", filename);
        ret = MP_INVALID_PARAMETER;
        goto out;
    }

    ret = fseek(file, 0, SEEK_END); // seek to end of file
    if (0 != ret) {
        //agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Failed to seek in file: %s\n", filename);
        ret = MP_UNEXPECTED_ERROR;
        goto out;
    }
    fileSize = ftell(file); // get current file pointer
    ret = fseek(file, 0, SEEK_SET); // seek back to beginning of file
    if (0 != ret) {
        //agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Failed to seek in file: %s\n", filename);
        ret = MP_UNEXPECTED_ERROR;
        goto out;
    }

    if ((unsigned long)fileSize > buffSize) {
        //agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Received file: %s is too big for expected file size: %d.\n", filename, buffSize);
        ret = MP_INVALID_PARAMETER;
        goto out;
    }

    writtenSize = fread(buffer, sizeof(uint8_t), fileSize, file);
    if (writtenSize != (unsigned long)fileSize) {
        //agent_log_message(MP_REG_LOG_LEVEL_ERROR, "Failed to read data from file: %s\n", filename);
        ret = MP_UNEXPECTED_ERROR;
        goto out;
    }

    buffSize = fileSize;
out:
    if (file) {
        fclose(file);
    }
    return ret;
}