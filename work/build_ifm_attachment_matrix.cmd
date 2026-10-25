@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\attachment-matrix-20261024"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_IFM_ATTACHMENT_MATRIX work\probe_ifm_attachment_matrix.cpp /Fo:"%OUT%\native.obj" /Fe:"%OUT%\attachment_native_x86.exe" > "%OUT%\native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\native-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_MODEL_LIGHTING_POINT work\probe_model_lighting_point.cpp /Fo:"%OUT%\point_native.obj" /Fe:"%OUT%\point_native_x86.exe" > "%OUT%\point-native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\point-native-build.log"
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo"%OUT%\sincos.obj" work\x64_fsin_cos.asm > "%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_ifm_attachment_matrix.cpp "%OUT%\sincos.obj" /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\attachment_owned_x64.exe" > "%OUT%\owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_ifm_attachment_matrix.cpp "%OUT%\sincos.obj" /Fo:"%OUT%\safety.obj" /Fe:"%OUT%\attachment_safety_x64.exe" > "%OUT%\safety-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\safety-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_model_lighting_point.cpp /Fo:"%OUT%\point_owned.obj" /Fe:"%OUT%\point_owned_x64.exe" > "%OUT%\point-owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\point-owned-build.log"
 exit /b 1
)
exit /b 0
