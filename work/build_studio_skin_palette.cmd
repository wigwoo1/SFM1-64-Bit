@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\studio-skin-palette-20261025"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_STUDIO_SKIN work\probe_studio_skin_palette.cpp /Fo:"%OUT%\native.obj" /Fe:"%OUT%\skin_native_x86.exe" >"%OUT%\native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\native-build.log"
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo"%OUT%\sincos.obj" work\x64_fsin_cos.asm >"%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_studio_skin_palette.cpp /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\skin_owned_x64.exe" >"%OUT%\owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\audit_benchmark_studio_skin.cpp "%OUT%\sincos.obj" /Fo:"%OUT%\audit.obj" /Fe:"%OUT%\skin_benchmark_x64.exe" >"%OUT%\audit-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\audit-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_studio_skin_palette.cpp /Fo:"%OUT%\safety.obj" /Fe:"%OUT%\skin_safety_x64.exe" >"%OUT%\safety-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\safety-build.log"
 exit /b 1
)
exit /b 0
