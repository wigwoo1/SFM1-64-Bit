@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\generic-vertex-skin-20261025"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_PACK_NORMAL work\probe_generic_pack_normal.cpp /Fo:"%OUT%\native.obj" /Fe:"%OUT%\pack_normal_native_x86.exe" >"%OUT%\native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\native-build.log"
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_generic_pack_normal.cpp /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\pack_normal_owned_x64.exe" >"%OUT%\owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_generic_shader_vertex_skin.cpp /Fo:"%OUT%\skin.obj" /Fe:"%OUT%\shader_vertex_skin_x64.exe" >"%OUT%\skin-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\skin-build.log"
 exit /b 1
)
exit /b 0
