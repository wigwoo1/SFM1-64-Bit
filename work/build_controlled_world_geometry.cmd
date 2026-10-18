@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\controlled-world-geometry-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_BRUSH_WORLD /DFULL_WORLD_TRACE /DCONTROLLED_WORLD_GEOMETRY work\probe_swept_world_trace.cpp /Fo%OUT%\native.obj /Fe%OUT%\geometry_native_x86.exe > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DFULL_WORLD_TRACE /DCONTROLLED_WORLD_GEOMETRY work\probe_swept_world_trace.cpp /Fo%OUT%\owned.obj /Fe%OUT%\geometry_owned_x64.exe > %OUT%\owned-build.log 2>&1
exit /b %errorlevel%
