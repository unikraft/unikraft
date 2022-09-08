@echo off
rem This script uses OpenCppCoverage tool to merge all the .cov files in the current directory into one file.
rem The final merged file is a Cobertura compatible XML file

setlocal enabledelayedexpansion
set myvar=OpenCppCoverage.exe --verbose --export_type=cobertura:coverage.xml  
for /r %%i in (*.cov) do (
  call :concat %%~nxi
)
@echo on
%myvar%
goto :eof

:concat
set myvar=%myvar% --input_coverage %1
goto :eof
