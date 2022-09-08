# Copyright (c) 2019, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright notice,
#      this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the name of Intel Corporation nor the names of its contributors
#      may be used to endorse or promote products derived from this software
#      without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

trap {
	Set-Location -Path $cwd
	exit $LastExitCode
}

$vsBasePath = "C:\Program Files (x86)\Microsoft Visual Studio\" # base path for VS

$anyVs2017Pattern = "$vsBasePath\2017\*"
$cmake = $(
	Get-Childitem -Path "$anyVs2017Pattern\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -Recurse |
	Select-Object FullName |
	Select-Object -Last 1
).FullName

$msbuild = $(
	Get-Childitem -Path "$anyVs2017Pattern\MSBuild\*\Bin\MSBuild.exe" -Include MSBuild.exe -Recurse |
	Select-Object FullName |
	Where-Object { $_.FullName -notlike "*amd*" } |
	Select-Object -Last 1
).FullName

# Start workarounds

$devenv = $(
	Get-Childitem -Path "$anyVs2017Pattern\Common7\IDE\devenv.com" -Recurse |
	Select-Object FullName |
	Select-Object -Last 1
).FullName


$env:Path += ";$cmake;$msbuild"
$env:CMAKE_VS_MSBUILD_COMMAND = "$msbuild"
$env:CMAKE_VS_DEVENV_COMMAND = "$devenv"

# End workarounds

$cwd = Get-Location
New-Item -ItemType Directory -Force -Path ${PSScriptRoot}\Build\solution
Set-Location -Path ${PSScriptRoot}\Build\solution

$cmakeArguments = @('-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CONFIGURATION_TYPES="Release"', '-DATTESTATION_APP=ON', '-G', 'Visual Studio 15 2017 Win64', ${PSScriptRoot})

Write-Host "--------------"
Write-Host "Command: $cmake $cmakeArguments"
& $cmake $cmakeArguments
Write-Host "--------------"

if($LastExitCode -ne 0)
{
    Set-Location -Path $cwd
    exit $LastExitCode
}

Write-Host "--------------"
Write-Host "Command: $msbuild ${PSScriptRoot}\Build\solution\SgxEcdsaAttestation.sln"
& $msbuild ${PSScriptRoot}\Build\solution\SgxEcdsaAttestation.sln 

if($LastExitCode -ne 0)
{
    Set-Location -Path $cwd
    exit $LastExitCode
}
Write-Host "--------------"

Set-Location -Path $cwd