@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\world-trace-surface-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_BRUSH_WORLD /DFULL_WORLD_TRACE /DWORLD_SURFACE_FLAGS work\probe_swept_world_trace.cpp /Fo%OUT%\native.obj /Fe%OUT%\surface_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DFULL_WORLD_TRACE /DWORLD_SURFACE_FLAGS work\probe_swept_world_trace.cpp /Fo%OUT%\owned.obj /Fe%OUT%\surface_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_world_trace_surface.cpp /Fo%OUT%\safety.obj /Fe%OUT%\surface_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\safety-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_bsp_world_light_query.cpp /Fo%OUT%\lighting-owned.obj /Fe%OUT%\bsp_lighting_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\lighting-owned-build.log 2>&1
exit /b %errorlevel%
