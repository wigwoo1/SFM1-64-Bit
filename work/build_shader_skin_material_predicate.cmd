@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\shader-skin-material-predicate-20261025"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_SKIN_PREDICATE work\probe_shader_skin_material_predicate.cpp /Fo:"%OUT%\native.obj" /Fe:"%OUT%\skin_material_native_x86.exe" >"%OUT%\native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\native-build.log"
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_shader_skin_material_predicate.cpp /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\skin_material_owned_x64.exe" >"%OUT%\owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_shader_skin_material_predicate.cpp /Fo:"%OUT%\test.obj" /Fe:"%OUT%\skin_material_test_x64.exe" >"%OUT%\test-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\test-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\audit_skin_material_requests.cpp /Fo:"%OUT%\audit.obj" /Fe:"%OUT%\skin_material_requests_x64.exe" >"%OUT%\audit-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\audit-build.log"
 exit /b 1
)
exit /b 0
