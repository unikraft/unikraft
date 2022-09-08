@REM  Copyright (C) 2011-2019 Intel Corporation. All rights reserved.
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

set svn_ver=%1%
set rel_dir_name=PCKIDRetrievalTool_v1.7.100.0
set TOOLSFOLDER=.\Tools\
set flag=0

echo "Please copy sgx_urts.dll, sgx_launch.dll and sgx_enclave_common.dll to current directory!"

del /Q %rel_dir_name%\*
rd %rel_dir_name% 

mkdir %rel_dir_name%
echo:
echo ========= copy the binary Files  ===============
CALL :COPY_FILE x64\release\pck_id_retrieval_tool_enclave.signed.dll %rel_dir_name%
CALL :COPY_FILE x64\release\dcap_quoteprov.dll %rel_dir_name%
CALL :COPY_FILE x64\release\PCKIDRetrievalTool.exe %rel_dir_name%
CALL :COPY_FILE sgx_urts.dll %rel_dir_name%
CALL :COPY_FILE sgx_launch.dll %rel_dir_name%
CALL :COPY_FILE sgx_enclave_common.dll %rel_dir_name%
CALL :COPY_FILE ..\SGXPlatformRegistration\x64\release\mp_uefi.dll %rel_dir_name%
CALL :COPY_FILE ..\..\x64\release\sgx_dcap_ql.dll %rel_dir_name%
CALL :COPY_FILE ..\..\QuoteGeneration\psw\ae\data\prebuilt\win\pce.signed.dll %rel_dir_name%
CALL :COPY_FILE ..\..\QuoteGeneration\psw\ae\data\prebuilt\win\qe3.signed.dll %rel_dir_name%
CALL :COPY_FILE network_setting.conf %rel_dir_name%
CALL :COPY_FILE README_standalone.txt %rel_dir_name%\README.txt
CALL :COPY_FILE License.txt %rel_dir_name%



echo:
echo ========= Signing the binary Files  ===============
call "%TOOLSFOLDER%\Sign.bat" %rel_dir_name%\pck_id_retrieval_tool_enclave.signed.dll
call "%TOOLSFOLDER%\Sign.bat" %rel_dir_name%\dcap_quoteprov.dll
call "%TOOLSFOLDER%\Sign.bat" %rel_dir_name%\PCKIDRetrievalTool.exe
call "%TOOLSFOLDER%\Sign.bat" %rel_dir_name%\sgx_urts.dll
call "%TOOLSFOLDER%\Sign.bat" %rel_dir_name%\sgx_launch.dll
call "%TOOLSFOLDER%\Sign.bat" %rel_dir_name%\sgx_enclave_common.dll
call "%TOOLSFOLDER%\Sign.bat" %rel_dir_name%\mp_uefi.dll
call "%TOOLSFOLDER%\Sign.bat" %rel_dir_name%\sgx_dcap_ql.dll
call "%TOOLSFOLDER%\Sign.bat" %rel_dir_name%\pce.signed.dll
call "%TOOLSFOLDER%\Sign.bat" %rel_dir_name%\qe3.signed.dll



IF /I "%flag%" NEQ "0" (
echo "%flag%"
       goto exit
) else (
        powershell Compress-Archive -Path '%rel_dir_name%\*' -DestinationPath '%rel_dir_name%.zip' -Force 
        echo *** SGX PCK Cert ID Retrieal zip package Build Succesful. Bye bye.***  
        goto finish
)



:COPY_FILE
echo f | xcopy %1 %2 /Y /F
IF /I "%ERRORLEVEL%" NEQ "0" (
        set /A flag = %ERRORLEVEL%
        goto copy_failure
)
EXIT /B 

:copy_failure
echo ------------------------------------------
echo -        Failed to copy files            -
echo ------------------------------------------
EXIT /B

:exit
echo ------------------------------------------
echo -  Some error happens, please check it.  -
echo ------------------------------------------

:finish


