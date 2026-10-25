@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\bone-world-palette-20261024"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_BONE_PALETTE work\probe_ifm_bone_world_palette.cpp /Fo:"%OUT%\native.obj" /Fe:"%OUT%\palette_native_x86.exe" > "%OUT%\native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\native-build.log"
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_ifm_bone_world_palette.cpp /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\palette_owned_x64.exe" > "%OUT%\owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_benchmark_ifm_bone_inputs.cpp /Fo:"%OUT%\benchmark.obj" /Fe:"%OUT%\benchmark_inputs_x64.exe" > "%OUT%\benchmark-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\benchmark-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_ifm_bone_world_palette.cpp /Fo:"%OUT%\safety.obj" /Fe:"%OUT%\palette_safety_x64.exe" > "%OUT%\safety-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\safety-build.log"
 exit /b 1
)
exit /b 0
