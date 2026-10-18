@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\static-prop-point-spatial-20261016"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_native_spatial_partition.cpp /Fo%OUT%\partition_native.obj /Fe%OUT%\partition_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\partition-native-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_static_prop_partition_registration_native.cpp /Fo%OUT%\registration_native.obj /Fe%OUT%\registration_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\registration-native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo%OUT%\sincos.obj work\x64_fsin_cos.asm > %OUT%\asm-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_static_prop_point_spatial.cpp %OUT%\sincos.obj /Fo%OUT%\spatial_owned.obj /Fe%OUT%\spatial_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\spatial-owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_static_prop_point_spatial.cpp %OUT%\sincos.obj /Fo%OUT%\spatial_safety.obj /Fe%OUT%\spatial_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\spatial-safety-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_static_prop_point_registry.cpp %OUT%\sincos.obj /Fo%OUT%\registry_owned.obj /Fe%OUT%\registry_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\registry-owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_static_prop_point_registry.cpp %OUT%\sincos.obj /Fo%OUT%\registry_safety.obj /Fe%OUT%\registry_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\registry-safety-build.log 2>&1
exit /b %errorlevel%
