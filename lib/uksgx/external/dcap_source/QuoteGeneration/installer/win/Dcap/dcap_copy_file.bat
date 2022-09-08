echo off

set BUILDNUM="0"
set TOOLSFOLDER="..\Tools"
set RELEASEFILEFOLDER="..\..\..\..\x64\Release"
set PREBUILTFILEFOLDER=..\..\..\psw\ae\data\prebuilt\win
set LICENSEFOLDER="..\..\..\..\"
set THIRDPARTYLICENSEFOLDER="..\..\..\"

:START

if exist "%~dp0Output" (
  del /Q "%~dp0Output\*"
) else (
  mkdir "%~dp0Output"
)

if NOT exist "%~dp0%PREBUILTFILEFOLDER%" (
    echo ERROR: Please decompress prebuilt enclave package before compiling.
    goto :End
)

pushd "%~dp0"

echo **************************************************
echo * Copy needed files *
echo **************************************************
copy /y "%PREBUILTFILEFOLDER%\pce.signed.dll" "%~dp0output\pce.signed.dll"
copy /y "%PREBUILTFILEFOLDER%\qe3.signed.dll" "%~dp0output\qe3.signed.dll"
copy /y "%PREBUILTFILEFOLDER%\id_enclave.signed.dll" "%~dp0output\id_enclave.signed.dll"
copy /y "%RELEASEFILEFOLDER%\sgx_dcap_ql.dll" "%~dp0output\sgx_dcap_ql.dll"

copy /y "%PREBUILTFILEFOLDER%\qve.signed.dll" "%~dp0output\qve.signed.dll"
copy /y "%RELEASEFILEFOLDER%\sgx_dcap_quoteverify.dll" "%~dp0output\sgx_dcap_quoteverify.dll"
copy /y "%LICENSEFOLDER%\License.txt" "%~dp0output\License.txt"
copy /y "%THIRDPARTYLICENSEFOLDER%\ThirdPartyLicenses.txt" "%~dp0output\ThirdPartyLicenses.txt"

echo **************************************************
echo * Signing components files *
echo **************************************************
call "%TOOLSFOLDER%\Sign.bat"  "%~dp0output\sgx_dcap_ql.dll"
call "%TOOLSFOLDER%\Sign.bat"  "%~dp0output\sgx_dcap_quoteverify.dll"

echo **************************************************
echo * Done *
echo **************************************************

popd

:End
