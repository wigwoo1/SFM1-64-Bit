@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\world-surface-table-20261017"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_BSP_SURFACE_TABLE work\probe_bsp_collision_surface_table.cpp /Fo%OUT%\table_native_x86.obj /Fe%OUT%\table_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\table_native_x86-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2  work\probe_bsp_collision_surface_table.cpp /Fo%OUT%\table_owned_x64.obj /Fe%OUT%\table_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\table_owned_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2  work\test_bsp_collision_surface_table.cpp /Fo%OUT%\table_safety_x64.obj /Fe%OUT%\table_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\table_safety_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2  work\test_world_trace_surface.cpp /Fo%OUT%\surface_safety_x64.obj /Fe%OUT%\surface_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\surface_safety_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DFULL_WORLD_TRACE /DWORLD_SURFACE_FLAGS /DWORLD_DISPLACEMENT_SURFACE_IDENTITY /DWORLD_PLANE_TAGS work\probe_swept_world_trace.cpp /Fo%OUT%\world_regression_x64.obj /Fe%OUT%\world_regression_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\world_regression_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DFULL_WORLD_TRACE /DWORLD_SURFACE_FLAGS /DWORLD_DISPLACEMENT_SURFACE_IDENTITY /DWORLD_PLANE_TAGS /DCONTROLLED_WORLD_GEOMETRY work\probe_swept_world_trace.cpp /Fo%OUT%\controlled_regression_x64.obj /Fe%OUT%\controlled_regression_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\controlled_regression_x64-build.log 2>&1
if errorlevel 1 exit /b 1
exit /b 0
