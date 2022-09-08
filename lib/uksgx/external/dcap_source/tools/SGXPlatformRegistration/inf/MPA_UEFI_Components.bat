echo

set TOPFOLDER="..\"
set DEBUGFILEFOLDER="..\x64\Debug\"
set RELEASEFILEFOLDER="..\x64\Release\"
set PACKAGETNAME=MPA_UEFI_Components1.6.90.1
set pwd=%~dp0MPA_UEFI_Components

pushd "%~dp0"

if not exist "%pwd%\Header Files\" mkdir "%pwd%\Header Files"
if not exist "%pwd%\lib\native\Debug Support\" mkdir "%pwd%\lib\native\Debug Support"
if not exist "%pwd%\lib\native\Libraries\" mkdir "%pwd%\lib\native\Libraries"

copy /y "%TOPFOLDER%\include\MultiPackageDefs.h" "%pwd%\Header Files\MultiPackageDefs.h"
copy /y "%TOPFOLDER%\include\MPUefi.h" "%pwd%\Header Files\MPUefi.h"
copy /y "%TOPFOLDER%\include\c_wrapper\mp_uefi.h" "%pwd%\Header Files\mp_uefi.h"

copy /y "%DEBUGFILEFOLDER%\mp_uefi.lib" "%pwd%\lib\native\Debug Support\mp_uefi.lib"

copy /y "%RELEASEFILEFOLDER%\mp_uefi.lib" "%pwd%\lib\native\Libraries\mp_uefi.lib"

if exist %PACKAGETNAME%.nupkg del /Q %PACKAGETNAME%.nupkg

"nuget.exe" pack "%~dp0MPA_UEFI_Components\MPA_UEFI_Components.nuspec"

popd
