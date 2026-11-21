@echo off
setlocal
cd /d "%~dp0.."
set "OUT=%SFM64_BRUSH_BOX_TEST_DIRECTORY%"
if not defined OUT set "OUT=outputs\translated-brush-box-trace"
if not exist "%OUT%" mkdir "%OUT%"
set "DEFINES=/DFULL_WORLD_TRACE /DTRANSLATED_BRUSH_POINT_TRACE /DTRANSLATED_BRUSH_BOX_TRACE"

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_BRUSH_WORLD %DEFINES% work\probe_swept_world_trace.cpp /Fo"%OUT%\brush-box-x86.obj" /Fe"%OUT%\brush_box_x86.exe" > "%OUT%\brush-box-x86-build.log" 2>&1
if errorlevel 1 exit /b 1

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 %DEFINES% work\probe_swept_world_trace.cpp /Fo"%OUT%\brush-box-x64.obj" /Fe"%OUT%\brush_box_x64.exe" > "%OUT%\brush-box-x64-build.log" 2>&1
exit /b %ERRORLEVEL%
