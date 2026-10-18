@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\physics-support-metadata-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_physics_support_metadata.cpp /Fo%OUT%\native.obj /Fe%OUT%\metadata_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_PHYSICS_SUPPORT_METADATA work\probe_physics_support_metadata_kernel.cpp /Fo%OUT%\kernel-native.obj /Fe%OUT%\metadata_kernel_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\kernel-native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_physics_support_metadata_kernel.cpp /Fo%OUT%\kernel-owned.obj /Fe%OUT%\metadata_kernel_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\kernel-owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_owned_phy_support_metadata.cpp /Fo%OUT%\owned.obj /Fe%OUT%\metadata_phy_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_physics_support_metadata.cpp /Fo%OUT%\safety.obj /Fe%OUT%\metadata_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\safety-build.log 2>&1
exit /b %errorlevel%
