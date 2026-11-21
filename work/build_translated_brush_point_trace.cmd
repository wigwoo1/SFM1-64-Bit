@echo off
setlocal
cd /d "%~dp0.."
set "OUT=%SFM64_BRUSH_POINT_TEST_DIRECTORY%"
if not defined OUT set "OUT=outputs\translated-brush-point-trace"
if not exist "%OUT%" mkdir "%OUT%"

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_BRUSH_WORLD /DFULL_WORLD_TRACE /DTRANSLATED_BRUSH_POINT_TRACE work\probe_swept_world_trace.cpp /Fo"%OUT%\translated-native.obj" /Fe"%OUT%\translated_native_x86.exe" > "%OUT%\translated-native-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DFULL_WORLD_TRACE /DTRANSLATED_BRUSH_POINT_TRACE work\probe_swept_world_trace.cpp /Fo"%OUT%\translated-owned.obj" /Fe"%OUT%\translated_owned_x64.exe" > "%OUT%\translated-owned-build.log" 2>&1
exit /b %ERRORLEVEL%
