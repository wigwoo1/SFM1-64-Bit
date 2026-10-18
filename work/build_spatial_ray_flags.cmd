@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\spatial-ray-flags-20261016"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_native_spatial_partition.cpp /Fo%OUT%\partition_native.obj /Fe%OUT%\partition_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\partition-native-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_ENGINE_POINT_TRACE work\probe_engine_point_trace.cpp /Fo%OUT%\caller_native.obj /Fe%OUT%\caller_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\caller-native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_spatial_point_partition.cpp /Fo%OUT%\partition_owned.obj /Fe%OUT%\partition_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\partition-owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_engine_point_trace.cpp /Fo%OUT%\caller_owned.obj /Fe%OUT%\caller_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\caller-owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_spatial_point_partition.cpp /Fo%OUT%\partition_safety.obj /Fe%OUT%\partition_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\partition-safety-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_spatial_point_ray_adapter.cpp /Fo%OUT%\adapter_safety.obj /Fe%OUT%\adapter_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\adapter-safety-build.log 2>&1
exit /b %errorlevel%
