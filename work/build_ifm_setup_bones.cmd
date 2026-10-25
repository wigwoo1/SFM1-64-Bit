@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\setup-bones-20261025"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_SETUP_BONES work\probe_ifm_setup_bones.cpp /Fo:"%OUT%\native.obj" /Fe:"%OUT%\setup_native_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\native-build.log"
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo"%OUT%\sincos.obj" work\x64_fsin_cos.asm >"%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_ifm_setup_bones.cpp "%OUT%\sincos.obj" /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\setup_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\export_benchmark_setup_bones.cpp /Fo:"%OUT%\export.obj" /Fe:"%OUT%\setup_benchmark_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\export-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\export-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_ifm_setup_bones.cpp "%OUT%\sincos.obj" /Fo:"%OUT%\safety.obj" /Fe:"%OUT%\setup_safety_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\safety-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\safety-build.log"
 exit /b 1
)
exit /b 0
