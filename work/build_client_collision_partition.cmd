@echo off
setlocal
cd /d "%~dp0.."
set "OUT=%SFM64_CLIENT_COLLISION_TEST_DIRECTORY%"
if not defined OUT set "OUT=outputs\client-collision-partition"
if not exist "%OUT%" mkdir "%OUT%"

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_CLIENT_COLLISION_PARTITION work\probe_client_collision_partition.cpp /Fo"%OUT%\partition_x86.obj" /Fe"%OUT%\partition_x86.exe" > "%OUT%\partition_x86-build.log" 2>&1
if errorlevel 1 exit /b 1

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_client_collision_partition.cpp /Fo"%OUT%\partition_x64.obj" /Fe"%OUT%\partition_x64.exe" > "%OUT%\partition_x64-build.log" 2>&1
exit /b %ERRORLEVEL%
