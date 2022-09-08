echo

set TOPFOLDER="..\..\..\"
set QGFOLDER="..\..\"
set QVFOLDER="%TOPFOLDER%\QuoteVerification"
set DEBUGFILEFOLDER="..\..\..\x64\Debug\"
set RELEASEFILEFOLDER="..\..\..\x64\Release\"
set PACKAGETNAME=DCAP_Components.1.14.100.0
set pwd=%~dp0DCAP_Components

pushd "%~dp0"

if not exist "%pwd%\Header Files\" mkdir "%pwd%\Header Files"
if not exist "%pwd%\lib\native\Debug Support\" mkdir "%pwd%\lib\native\Debug Support"
if not exist "%pwd%\lib\native\Libraries\" mkdir "%pwd%\lib\native\Libraries"

copy /y "%QGFOLDER%\quote_wrapper\ql\inc\sgx_dcap_ql_wrapper.h" "%pwd%\Header Files\sgx_dcap_ql_wrapper.h"
copy /y "%QVFOLDER%\QvE\Include\sgx_qve_header.h" "%pwd%\Header Files\sgx_qve_header.h"
copy /y "%QVFOLDER%\dcap_quoteverify\inc\sgx_dcap_quoteverify.h" "%pwd%\Header Files\sgx_dcap_quoteverify.h"


copy /y "%DEBUGFILEFOLDER%\sgx_dcap_ql.lib" "%pwd%\lib\native\Debug Support\sgx_dcap_ql.lib"
copy /y "%DEBUGFILEFOLDER%\sgx_dcap_quoteverify.lib" "%pwd%\lib\native\Debug Support\sgx_dcap_quoteverify.lib"

copy /y "%RELEASEFILEFOLDER%\sgx_dcap_ql.lib" "%pwd%\lib\native\Libraries\sgx_dcap_ql.lib"
copy /y "%RELEASEFILEFOLDER%\sgx_dcap_quoteverify.lib" "%pwd%\lib\native\Libraries\sgx_dcap_quoteverify.lib"

if exist %PACKAGETNAME%.nupkg del /Q %PACKAGETNAME%.nupkg

"nuget.exe" pack "%~dp0DCAP_Components\DCAP_Components.nuspec"

popd
