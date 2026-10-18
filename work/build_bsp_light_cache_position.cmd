@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\bsp-light-cache-position-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_BRUSH_WORLD /DFULL_WORLD_TRACE /DCACHE_WORLD_POSITION work\probe_swept_world_trace.cpp /Fo%OUT%\native.obj /Fe%OUT%\anchor_native_x86.exe > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DFULL_WORLD_TRACE /DCACHE_WORLD_POSITION work\probe_swept_world_trace.cpp /Fo%OUT%\owned.obj /Fe%OUT%\anchor_owned_x64.exe > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_bsp_light_cache_position.cpp /Fo%OUT%\safety.obj /Fe%OUT%\anchor_safety_x64.exe > %OUT%\safety-build.log 2>&1
exit /b %errorlevel%
