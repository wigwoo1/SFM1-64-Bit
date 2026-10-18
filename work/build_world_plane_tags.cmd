@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\world-plane-tags-20261016"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Zs /DSFM64_NATIVE_BRUSH_ORACLE work\probe_native_convex_brush_ray.cpp > %OUT%\compat_native_general-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Zs /DSFM64_NATIVE_BRUSH_ORACLE /DSFM64_NATIVE_BOX_ORACLE work\probe_native_convex_brush_ray.cpp > %OUT%\compat_native_box-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Zs /DSFM64_NATIVE_BRUSH_ORACLE /DSFM64_AUTO_BRUSH_DISPATCH work\probe_native_convex_brush_ray.cpp > %OUT%\compat_native_dispatch-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Zs /DSFM64_NATIVE_BRUSH_ORACLE /DSFM64_AUTO_BRUSH_DISPATCH /DSFM64_STATIONARY_BRUSH work\probe_native_convex_brush_ray.cpp > %OUT%\compat_native_stationary-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_PLANE_IMPORT work\probe_world_plane_import.cpp /Fo%OUT%\plane_native_x86.obj /Fe%OUT%\plane_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\plane_native_x86-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_BRUSH_ORACLE /DSFM64_AUTO_BRUSH_DISPATCH /DWORLD_PLANE_TAGS work\probe_native_convex_brush_ray.cpp /Fo%OUT%\brush_native_x86.obj /Fe%OUT%\brush_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\brush_native_x86-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_BRUSH_WORLD /DFULL_WORLD_TRACE /DWORLD_SURFACE_FLAGS /DWORLD_DISPLACEMENT_SURFACE_IDENTITY /DWORLD_PLANE_TAGS work\probe_swept_world_trace.cpp /Fo%OUT%\world_native_x86.obj /Fe%OUT%\world_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\world_native_x86-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_BRUSH_WORLD /DFULL_WORLD_TRACE /DWORLD_SURFACE_FLAGS /DWORLD_DISPLACEMENT_SURFACE_IDENTITY /DWORLD_PLANE_TAGS /DCONTROLLED_WORLD_GEOMETRY work\probe_swept_world_trace.cpp /Fo%OUT%\controlled_native_x86.obj /Fe%OUT%\controlled_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\controlled_native_x86-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Zs  work\probe_native_convex_brush_ray.cpp > %OUT%\compat_owned_general-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Zs /DSFM64_OWNED_BOX_CANDIDATE work\probe_native_convex_brush_ray.cpp > %OUT%\compat_owned_box-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Zs /DSFM64_AUTO_BRUSH_DISPATCH work\probe_native_convex_brush_ray.cpp > %OUT%\compat_owned_dispatch-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Zs /DSFM64_AUTO_BRUSH_DISPATCH /DSFM64_STATIONARY_BRUSH work\probe_native_convex_brush_ray.cpp > %OUT%\compat_owned_stationary-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2  work\probe_world_plane_import.cpp /Fo%OUT%\plane_owned_x64.obj /Fe%OUT%\plane_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\plane_owned_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_AUTO_BRUSH_DISPATCH /DWORLD_PLANE_TAGS work\probe_native_convex_brush_ray.cpp /Fo%OUT%\brush_owned_x64.obj /Fe%OUT%\brush_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\brush_owned_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DFULL_WORLD_TRACE /DWORLD_SURFACE_FLAGS /DWORLD_DISPLACEMENT_SURFACE_IDENTITY /DWORLD_PLANE_TAGS work\probe_swept_world_trace.cpp /Fo%OUT%\world_owned_x64.obj /Fe%OUT%\world_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\world_owned_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DFULL_WORLD_TRACE /DWORLD_SURFACE_FLAGS /DWORLD_DISPLACEMENT_SURFACE_IDENTITY /DWORLD_PLANE_TAGS /DCONTROLLED_WORLD_GEOMETRY work\probe_swept_world_trace.cpp /Fo%OUT%\controlled_owned_x64.obj /Fe%OUT%\controlled_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\controlled_owned_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2  work\test_world_plane_tags.cpp /Fo%OUT%\tag_safety_x64.obj /Fe%OUT%\tag_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\tag_safety_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2  work\test_world_trace_surface.cpp /Fo%OUT%\surface_safety_x64.obj /Fe%OUT%\surface_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\surface_safety_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2  work\test_world_ray_trace.cpp /Fo%OUT%\world_safety_x64.obj /Fe%OUT%\world_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\world_safety_x64-build.log 2>&1
if errorlevel 1 exit /b 1
exit /b 0
