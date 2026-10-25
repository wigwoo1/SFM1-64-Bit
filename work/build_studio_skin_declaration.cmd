@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\generic-compressed-input-20261025"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_SKIN_DECL work\probe_studio_skin_declaration.cpp /Fo:"%OUT%\native.obj" /Fe:"%OUT%\skin_declaration_native_x86.exe" >"%OUT%\native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\native-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_VERTEX_WEIGHTS work\probe_studio_vertex_weights.cpp /Fo:"%OUT%\regression.obj" /Fe:"%OUT%\vertex_weights_regression_x86.exe" >"%OUT%\regression-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\regression-build.log"
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_studio_skin_declaration.cpp /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\skin_declaration_owned_x64.exe" >"%OUT%\owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_generic_compressed_weight_input.cpp /Fo:"%OUT%\input.obj" /Fe:"%OUT%\compressed_weight_input_x64.exe" >"%OUT%\input-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\input-build.log"
 exit /b 1
)
exit /b 0
