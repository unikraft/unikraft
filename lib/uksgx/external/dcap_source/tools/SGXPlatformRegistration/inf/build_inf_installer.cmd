@echo off
SetLocal EnableDelayedExpansion

IF "%selfWrapped%"=="" (
  REM ***** this is necessary so that we can use "exit" to terminate the batch file, ****
  REM ***** and all subroutines, but not the original cmd.exe                        ****
  SET selfWrapped=true
  %ComSpec% /s /c ""%~0" %*"
  GOTO :EOF
)

FOR /F "tokens=* USEBACKQ" %%F IN (`PowerShell -NoProfile -ExecutionPolicy Bypass -Command "& './get_version.ps1'"`) DO (
    SET VERSION=%%F
)

echo *** Welcome to the SGX Muti-Package Registration Agent Package Installer Script ***
echo:

echo ============ Clean old INF installers ==============
for /f %%i in ('dir /a:d /s /b sgx_mpa_*') do rd /s /q %%i
rd /s /q Output
echo Cleaning done.
echo:

echo ========== Build SGX Multi-Pacakge Registration INF %VERSION% ================
call inf_build.cmd %VERSION%
echo:

echo ========== Pack INF installers ================
mkdir sgx_mpa_%VERSION%
xcopy Output sgx_mpa_%VERSION%\ /s /i
cd ..
echo:

:exit
echo:
echo *** SGX Muti-Package Registration Agent Package Builder exsiting. Bye bye. ***
exit

