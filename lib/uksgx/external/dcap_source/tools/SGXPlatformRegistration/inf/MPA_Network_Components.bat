echo

set TOPFOLDER="..\"
set DEBUGFILEFOLDER="..\x64\Debug\"
set RELEASEFILEFOLDER="..\x64\Release\"
set PACKAGETNAME=MPA_Network_Components1.6.90.1
set pwd=%~dp0MPA_NETWORK_Components

pushd "%~dp0"

if not exist "%pwd%\Header Files\" mkdir "%pwd%\Header Files"
if not exist "%pwd%\lib\native\Debug Support\" mkdir "%pwd%\lib\native\Debug Support"
if not exist "%pwd%\lib\native\Libraries\" mkdir "%pwd%\lib\native\Libraries"

copy /y "%TOPFOLDER%\include\MPNetworkDefs.h" "%pwd%\Header Files\MPNetworkDefs.h"
copy /y "%TOPFOLDER%\include\MPNetwork.h" "%pwd%\Header Files\MPNetwork.h"
copy /y "%TOPFOLDER%\include\c_wrapper\mp_network.h" "%pwd%\Header Files\mp_network.h"

copy /y "%DEBUGFILEFOLDER%\mp_network.lib" "%pwd%\lib\native\Debug Support\mp_network.lib"

copy /y "%RELEASEFILEFOLDER%\mp_network.lib" "%pwd%\lib\native\Libraries\mp_network.lib"

if exist %PACKAGETNAME%.nupkg del /Q %PACKAGETNAME%.nupkg

"nuget.exe" pack "%~dp0MPA_NETWORK_Components\MPA_NETWORK_Components.nuspec"

popd
