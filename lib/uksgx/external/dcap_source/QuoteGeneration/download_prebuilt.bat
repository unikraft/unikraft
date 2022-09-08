@REM  Copyright (C) 2011-2022 Intel Corporation. All rights reserved.
@REM 
@REM  Redistribution and use in source and binary forms, with or without
@REM  modification, are permitted provided that the following conditions
@REM  are met:
@REM 
@REM    * Redistributions of source code must retain the above copyright
@REM      notice, this list of conditions and the following disclaimer.
@REM    * Redistributions in binary form must reproduce the above copyright
@REM      notice, this list of conditions and the following disclaimer in
@REM      the documentation and/or other materials provided with the
@REM      distribution.
@REM    * Neither the name of Intel Corporation nor the names of its
@REM      contributors may be used to endorse or promote products derived
@REM      from this software without specific prior written permission.
@REM 
@REM  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
@REM  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
@REM  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
@REM  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
@REM  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
@REM  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
@REM  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
@REM  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
@REM  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
@REM  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
@REM  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
@REM 

@echo off

set ae_file_name=prebuilt_windows_dcap_1.14.zip
set checksum_file=SHA256SUM_prebuilt_windows_dcap_1.14.cfg
set server_url_path=https://download.01.org/intel-sgx/sgx-dcap/1.14/windows/
set server_ae_url=%server_url_path%/%ae_file_name%
set server_checksum_url=%server_url_path%/%checksum_file%

pushd "%~dp0"

@del /Q %ae_file_name% 2>nul
@powershell "($client = new-object System.Net.WebClient) -and ($client.DownloadFile('%server_ae_url%', '%ae_file_name%')) -and (exit)" >nul
if NOT exist %ae_file_name% (
    echo Fail to download file %server_ae_url%
    goto :End
)

@del /Q %checksum_file% 2>nul
@powershell "($client = new-object System.Net.WebClient) -and ($client.DownloadFile('%server_checksum_url%', '%checksum_file%')) -and (exit)" >nul
if NOT exist %checksum_file% (
    echo Fail to download file %server_checksum_url%
    goto :End
)

for /f "tokens=1,2,*" %%a in ('type %checksum_file% ') do (
    for /f "delims=" %%i in (' powershell "((certutil -hashfile %%b SHA256)[1] -replace \" \", \"\")" ') do (
        if NOT %%i == %%a (
            echo Checksum verification failure
            goto :End
        )
        goto :Unzip
    )
)

:Unzip
@powershell "( Expand-Archive -Path '%~dp0%ae_file_name%'  -DestinationPath '%~dp0' -Force )"
del /Q %ae_file_name% %checksum_file%

:End
popd
