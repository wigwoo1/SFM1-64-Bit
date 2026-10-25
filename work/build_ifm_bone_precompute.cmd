@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\precomputed-bone-palette-20261025"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_PRECOMPUTE work\probe_ifm_bone_precompute.cpp /Fo:"%OUT%\native.obj" /Fe:"%OUT%\precompute_native_x86.exe" >"%OUT%\native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\native-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_BONE_PALETTE work\probe_ifm_bone_world_palette.cpp /Fo:"%OUT%\normal-native.obj" /Fe:"%OUT%\normal_native_x86.exe" >"%OUT%\normal-native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\normal-native-build.log"
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_ifm_bone_precompute.cpp /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\precompute_owned_x64.exe" >"%OUT%\owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_ifm_bone_world_palette.cpp /Fo:"%OUT%\normal-owned.obj" /Fe:"%OUT%\normal_owned_x64.exe" >"%OUT%\normal-owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\normal-owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\audit_benchmark_bone_references.cpp /Fo:"%OUT%\audit.obj" /Fe:"%OUT%\benchmark_bone_references_x64.exe" >"%OUT%\audit-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\audit-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_ifm_bone_precompute.cpp /Fo:"%OUT%\safety.obj" /Fe:"%OUT%\precompute_safety_x64.exe" >"%OUT%\safety-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\safety-build.log"
 exit /b 1
)
exit /b 0
