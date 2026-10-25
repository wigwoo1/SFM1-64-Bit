@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\studio-vertex-tangent-20261025"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_VERTEX_TANGENT work\probe_studio_vertex_tangent.cpp /Fo:"%OUT%\native.obj" /Fe:"%OUT%\vertex_tangent_native_x86.exe" >"%OUT%\native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\native-build.log"
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_studio_vertex_tangent.cpp /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\vertex_tangent_owned_x64.exe" >"%OUT%\owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_studio_vertex_tangent.cpp /Fo:"%OUT%\safety.obj" /Fe:"%OUT%\vertex_tangent_safety_x64.exe" >"%OUT%\safety-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\safety-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_phong_shader_vertex_skin.cpp /Fo:"%OUT%\handoff.obj" /Fe:"%OUT%\phong_vertex_skin_handoff_x64.exe" >"%OUT%\handoff-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\handoff-build.log"
 exit /b 1
)
exit /b 0
