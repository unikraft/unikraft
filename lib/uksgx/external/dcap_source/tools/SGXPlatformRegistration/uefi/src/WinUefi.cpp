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
 * File: WinUefi.cpp
 *   
 * Description: Windows specific implementation for the MPUefi class to 
 * communicate with the BIOS UEFI variables.
 */
#include <string.h>
#include "WinUefi.h"
#include "uefi_logger.h"

#define BASE_BUF_SIZE (32*1024)
#define MAX_BUF_SIZE (4096*1024)


#ifdef TEST_MODE
#define GetFirmwareEnvironmentVariableA ReadFromUefiFile
#define SetFirmwareEnvironmentVariableA WriteToUefiFile

#define UEFI_DIR_NAME "uefi_mock"

int writeBufferToFile(const char *filename, uint8_t *buffer, size_t buffSize) {
    int ret = 0;
    size_t writtenSize = 0;
    FILE *file = NULL;

    if (fopen_s(&file, filename, "wb") != 0 || file == NULL) {
        ret = MP_INVALID_PARAMETER;
        goto out;
    }

    writtenSize = fwrite(buffer, sizeof(uint8_t), buffSize, file);
    if (writtenSize != buffSize) {
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

    if (fopen_s(&file, filename, "rb") != 0 || file == NULL) {
        ret = MP_INVALID_PARAMETER;
        goto out;
    }

    ret = fseek(file, 0, SEEK_END); // seek to end of file
    if (0 != ret) {
        ret = MP_UNEXPECTED_ERROR;
        goto out;
    }
    fileSize = ftell(file); // get current file pointer
    ret = fseek(file, 0, SEEK_SET); // seek back to beginning of file
    if (0 != ret) {
        ret = MP_UNEXPECTED_ERROR;
        goto out;
    }

    if ((unsigned long)fileSize > buffSize) {
        ret = MP_INVALID_PARAMETER;
        goto out;
    }

    writtenSize = fread(buffer, sizeof(uint8_t), fileSize, file);
    if (writtenSize != (unsigned long)fileSize) {
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

DWORD
ReadFromUefiFile(
	_In_ LPCSTR lpName,
	_In_ LPCSTR lpGuid,
	_Out_writes_bytes_to_opt_(nSize, return) PVOID pBuffer,
	_In_ DWORD    nSize
)
{
	FILE* f = NULL;
	size_t size = nSize;
	int ret = 0;
	char filename[256] = {};
	sprintf_s(filename, 256, "%s\\%s.%s", UEFI_DIR_NAME, lpName, lpGuid);

	if ((ret = readFileToBuffer(filename, (uint8_t*)pBuffer, size)) != 0)
	{
		if (ret == 2) // set error so underlying code will get the OS error
			SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return 0;
	}
	return (DWORD)size;
}

BOOL
WriteToUefiFile(
	_In_ LPCSTR lpName,
	_In_ LPCSTR lpGuid,
	_In_reads_bytes_opt_(nSize) PVOID pValue,
	_In_ DWORD    nSize
)
{
	FILE* f = NULL;
	size_t nWrite = 0;
	char filename[256] = {};
	sprintf_s(filename, 256, "%s\\%s.%s", UEFI_DIR_NAME, lpName, lpGuid);

	if (writeBufferToFile(filename, (uint8_t*)pValue, nSize) != 0)
		return FALSE;

	return TRUE;
}
#endif

BOOL WinUefi::SetPrivilege(
	HANDLE hToken,          // access token handle
	LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	BOOL bEnablePrivilege   // to enable or disable privilege
)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(
		NULL,            // lookup privilege on local system
		lpszPrivilege,   // privilege to lookup 
		&luid))        // receives LUID of privilege
	{
		uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "LookupPrivilegeValue error: %u\n", GetLastError());
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.

	if (!AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		(PTOKEN_PRIVILEGES)NULL,
		(PDWORD)NULL))
	{
		uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "AdjustTokenPrivileges error: %u\n", GetLastError());
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "The token does not have the specified privilege\n");
		return FALSE;
	}

	return TRUE;
}

WinUefi::WinUefi(const LogLevel logLevel)
{
	TokenHandle = INVALID_HANDLE_VALUE;
    m_logLevel = logLevel;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &TokenHandle) == 0)
	{
		uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "OpenProcessToken failed, error %u\n", GetLastError());
		return;
	}

    if (SetPrivilege(TokenHandle, SE_SYSTEM_ENVIRONMENT_NAME, TRUE) != TRUE)
    {
        uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "SetPrivilege failed\n");
        CloseHandle(TokenHandle);
        TokenHandle = INVALID_HANDLE_VALUE;
        return;
    }

#ifdef TEST_MODE
	system("mkdir uefi_mock >nul 2>nul");
#endif
}


WinUefi::~WinUefi()
{
	if (TokenHandle != INVALID_HANDLE_VALUE)
	{
		SetPrivilege(TokenHandle, SE_SYSTEM_ENVIRONMENT_NAME, FALSE);
		CloseHandle(TokenHandle);
		TokenHandle = INVALID_HANDLE_VALUE;
	}
}



uint8_t* WinUefi::readUEFIVar(const char* varName, size_t &dataSize)
{
	uint8_t *var_data = nullptr;
	DWORD var_data_size = BASE_BUF_SIZE;
	DWORD out_size = 0;
	DWORD err = 0;
	bool res = false;

	if (varName == NULL)
	{
		uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "wrong input parameters");
		return nullptr;
	}

	if (TokenHandle == INVALID_HANDLE_VALUE)
	{
		uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "privileges not set, can't access UEFI\n");
		return nullptr;
	}

	const char* seperator = strchr(varName, '-');
	if (seperator == NULL)
	{
		uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "input variable does not include '-'\n");
		return nullptr;
	}
	int64_t index = seperator - varName;

	string var(varName, index);
	string guid(&varName[index + 1]);

	guid = "{" + guid + "}";

	var_data = new uint8_t[var_data_size];

	do {
		out_size = GetFirmwareEnvironmentVariableA(var.c_str() , guid.c_str(), var_data, var_data_size);
		if (out_size != 0)
		{
			dataSize = out_size;
			res = true;
			break;
		}

		err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER)
		{
			uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "GetFirmwareEnvironmentVariableA failed: %d\n", err);
			break;
		}

		// not enough size
		delete var_data;
		var_data = nullptr;
		
		var_data_size *= 4;
		if (var_data_size > MAX_BUF_SIZE)
		{
			uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "GetFirmwareEnvironmentVariableA required buffer size is bigger than %d bytes\n", MAX_BUF_SIZE);
			break;
		}

		var_data = new uint8_t[var_data_size];

	} while (true);
	
	if (res == false)
	{
		if (var_data != nullptr)
			delete var_data;
		var_data = nullptr;
	}

	return var_data;
}

int WinUefi::writeUEFIVar(const char* varName, const uint8_t* data, size_t dataSize, bool create)
{
    (void)create;
	if (varName == NULL || data == NULL || dataSize == 0)
	{
		uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "wrong input parameters");
		return 0;
	}

	if (TokenHandle == INVALID_HANDLE_VALUE)
	{
		uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "privileges not set, can't access UEFI\n");
		return 0;
	}

	const char* seperator = strchr(varName, '-');
	if (seperator == NULL)
	{
		uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "input variable does not include '-'\n");
		return 0;
	}
	int64_t index = seperator - varName;

	string var(varName, index);
	string guid(&varName[index + 1]);

	guid = "{" + guid + "}";

	if (SetFirmwareEnvironmentVariableA(var.c_str(), guid.c_str(), (PVOID)data, (DWORD)dataSize) == FALSE)
	{
		uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "SetFirmwareEnvironmentVariableA failed, error %u\n", GetLastError());
		return -1;
	}

	return (int)dataSize;
}
