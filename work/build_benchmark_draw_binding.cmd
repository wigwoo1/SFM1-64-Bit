@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\benchmark-draw-binding-20261025"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_VIRTUAL_SEQUENCE_APPEND work\probe_virtual_sequence_append.cpp /Fo:"%OUT%\native.obj" /Fe:"%OUT%\sequence_native_x86.exe" > "%OUT%\native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\native-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_VIRTUAL_SEQUENCE_LOOKUP work\probe_virtual_sequence_lookup.cpp /Fo:"%OUT%\lookup-native.obj" /Fe:"%OUT%\sequence_lookup_native_x86.exe" > "%OUT%\lookup-native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\lookup-native-build.log"
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_virtual_sequence_lookup.cpp /Fo:"%OUT%\lookup-owned.obj" /Fe:"%OUT%\sequence_lookup_owned_x64.exe" > "%OUT%\lookup-owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\lookup-owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_virtual_sequence_append.cpp /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\sequence_owned_x64.exe" > "%OUT%\owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_virtual_sequence_append.cpp /Fo:"%OUT%\safety.obj" /Fe:"%OUT%\sequence_safety_x64.exe" > "%OUT%\safety-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\safety-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\audit_benchmark_draw_binding.cpp /Fo:"%OUT%\audit.obj" /Fe:"%OUT%\benchmark_draw_binding_x64.exe" > "%OUT%\audit-build.log" 2>&1
if errorlevel 1 type "%OUT%\audit-build.log"
exit /b %errorlevel%
