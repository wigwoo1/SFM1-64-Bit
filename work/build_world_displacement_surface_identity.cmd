@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\world-displacement-surface-20261016"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_BRUSH_WORLD /DFULL_WORLD_TRACE /DWORLD_SURFACE_FLAGS /DWORLD_DISPLACEMENT_SURFACE_IDENTITY work\probe_swept_world_trace.cpp /Fo%OUT%\native.obj /Fe%OUT%\world_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_BRUSH_WORLD /DFULL_WORLD_TRACE /DWORLD_SURFACE_FLAGS /DWORLD_DISPLACEMENT_SURFACE_IDENTITY /DCONTROLLED_WORLD_GEOMETRY work\probe_swept_world_trace.cpp /Fo%OUT%\controlled_native.obj /Fe%OUT%\controlled_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\controlled-native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DFULL_WORLD_TRACE /DWORLD_SURFACE_FLAGS /DWORLD_DISPLACEMENT_SURFACE_IDENTITY work\probe_swept_world_trace.cpp /Fo%OUT%\owned.obj /Fe%OUT%\world_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DFULL_WORLD_TRACE /DWORLD_SURFACE_FLAGS /DWORLD_DISPLACEMENT_SURFACE_IDENTITY /DCONTROLLED_WORLD_GEOMETRY work\probe_swept_world_trace.cpp /Fo%OUT%\controlled_owned.obj /Fe%OUT%\controlled_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\controlled-owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_displacement_trace_surface_properties.cpp /Fo%OUT%\binding_safety.obj /Fe%OUT%\binding_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\binding-safety-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_world_trace_surface.cpp /Fo%OUT%\surface_safety.obj /Fe%OUT%\surface_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\surface-safety-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_world_ray_trace.cpp /Fo%OUT%\world_safety.obj /Fe%OUT%\world_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\world-safety-build.log 2>&1
exit /b %errorlevel%
