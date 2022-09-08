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
 * File: FSUefi.cpp
 *   
 * Description: Implementation for the high-level UEFI functionality to 
 * communicate with BIOS using UEFI variables.
 */
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "FSUefi.h"
#include "uefi_logger.h"

static uint8_t attributes[4] = {0x07,0x00,0x00,0x00};

long FSUefi::fdGetVarFileSize(int fd)
{
    struct stat stat_buf;
    int rc = fstat(fd, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}


const string FSUefi::createFullPath(const string &path, const char* uefiVarName) {
    string sUefiVarName;
    sUefiVarName = string(uefiVarName);
    string sFullPath = path + sUefiVarName;
    return sFullPath;
}

uint8_t* FSUefi::readUEFIVar(const char* varName, size_t &dataSize)
{
    uint8_t *entire_var = NULL;
    uint8_t *var_data = NULL;
    int fd = -1;

    // get abs uefi path
    string fullPath = createFullPath(m_uefiAbsPath, varName);
    const char *var_name_path = fullPath.c_str();
    
    do {
        errno = 0;
        fd = open(var_name_path ,O_RDONLY);
        if(fd == -1)
        {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "readUEFIVar: failed to open uefi variable %s ,error: %s\n", var_name_path, strerror(errno));
            break;
        }

        // get uefi file size
        size_t fileSize = fdGetVarFileSize(fd);
        // actual data size without uefi attribute
        dataSize = fileSize - sizeof(attributes);

        entire_var = new uint8_t[fileSize];
        if (!entire_var) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "readUEFIVar: allocation failure\n");
            break;
        }
        
        errno = 0;
        ssize_t bytesRead = read(fd, entire_var, fileSize);
        if(bytesRead <= 0)
        {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "readUEFIVar: failed to read uefi variable %s ,error: %s\n", var_name_path, strerror(errno));
            break;
        }
        
        var_data = new uint8_t[dataSize];
        if (!var_data) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "readUEFIVar: allocation failure\n");
            break;
        }

        memcpy(var_data, entire_var+sizeof(attributes), dataSize);
    } while(0);

    if (entire_var) {
        delete entire_var;
    }
    if (fd != -1) {
        close(fd);
    }
    return var_data;
}

int FSUefi::writeUEFIVar(const char* varName, const uint8_t* data, size_t dataSize, bool create)
{
    int fd = -1;
    int rc = -1;
    int flags, oflags = 0;
    mode_t mode = 0;
    ssize_t bytesWrote = 0;
    char* buffer = NULL;
    int lastOpenErrno = 0;
    
    do {
        if(dataSize + sizeof(attributes) < dataSize) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "writeUEFIVar: the data size is too big\n");
            break;
        }
        buffer = new char[dataSize+sizeof(attributes)];
        if (!buffer) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "writeUEFIVar: allocation failure\n");
            break;
        }

        // zero the buffer and copy data
        memset(buffer, 0, sizeof(attributes) + dataSize);
        memcpy(buffer, attributes, sizeof(attributes));
        memcpy(buffer + sizeof(attributes), data, dataSize);

        // get abs uefi path
        string fullPath = createFullPath(m_uefiAbsPath, varName);
        const char *UEFIvarNamePath = fullPath.c_str();
        
        // set read only flag
        flags = O_RDONLY;

        errno=0;
        fd = open(UEFIvarNamePath, flags);
        if(fd == -1) {
            if (ENOENT == errno) {
                lastOpenErrno = errno;
                if (create) {
                    // set flags: create, user r/w permission, group read permission, others read permission.
                    flags |= O_CREAT;
                    mode |= S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
                    fd = open(UEFIvarNamePath, flags, mode);
                }
            }
            if (fd == -1) {
                if ((create) && (ENOENT == lastOpenErrno)) {
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "writeUEFIVar: failed to create uefi variable %s, error: %s\n", UEFIvarNamePath, strerror(errno));
                } else {
                    uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "writeUEFIVar: failed to open uefi variable %s, error: %s\n", UEFIvarNamePath, strerror(errno));
                }
                break;
            }
        }
        
        // remove immutable flag
        rc = ioctl(fd, FS_IOC_GETFLAGS, &oflags);
        if (rc < 0) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "writeUEFIVar: failed to get UEFI var %d flags, error: %s\n", UEFIvarNamePath, strerror(errno));
            break;
		}
        
        // remove immutable flag if needed
		if (oflags & FS_IMMUTABLE_FL) {
			oflags &= ~FS_IMMUTABLE_FL;
			rc = ioctl(fd, FS_IOC_SETFLAGS, &oflags);
			if (rc < 0) {
                uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "writeUEFIVar: failed to set UEFI var %d flags, error: %s\n", UEFIvarNamePath, strerror(errno));
                break;
			}
		}
        
        // close fd and re-open with write only
        close(fd);
        fd = open(UEFIvarNamePath, O_WRONLY);
        if (fd == -1) {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "writeUEFIVar: failed to re-open uefi variable %s, error: %s\n", UEFIvarNamePath, strerror(errno));
             break;
        }

        bytesWrote = write(fd, buffer, dataSize+sizeof(attributes));
        if(bytesWrote != (ssize_t)(dataSize+sizeof(attributes)))
        {
            uefi_log_message(MP_REG_LOG_LEVEL_ERROR, "writeUEFIVar: failed to write uefi variable %s, wrote %d bytes, error: %s\n", UEFIvarNamePath, bytesWrote, strerror(errno));
            break;
        }
        
        bytesWrote -= sizeof(attributes);
    } while (0);

    if (buffer) {
        delete[] buffer;
    }
    if (fd != -1) {
        close(fd);
    }
    return (int)bytesWrote;
}
