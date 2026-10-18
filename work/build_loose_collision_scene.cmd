@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\loose-collision-scene-20261016"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_ENGINE_STUDIO_POINT_CLIP work\probe_static_prop_registry_native.cpp /Fo%OUT%\clip_native.obj /Fe%OUT%\clip_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\clip-native-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_STUDIO_SURFACE_PROPERTIES work\probe_studio_surface_properties.cpp /Fo%OUT%\surface_native.obj /Fe%OUT%\surface_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\surface-native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo%OUT%\sincos.obj work\x64_fsin_cos.asm > %OUT%\asm-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_loose_collision_scene.cpp %OUT%\sincos.obj /Fo%OUT%\owned.obj /Fe%OUT%\scene_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_loose_collision_scene.cpp %OUT%\sincos.obj /Fo%OUT%\safety.obj /Fe%OUT%\scene_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\safety-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_static_prop_point_spatial.cpp %OUT%\sincos.obj /Fo%OUT%\spatial.obj /Fe%OUT%\spatial_regression_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\spatial-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_static_prop_point_spatial.cpp %OUT%\sincos.obj /Fo%OUT%\spatial_safety.obj /Fe%OUT%\spatial_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\spatial-safety-build.log 2>&1
exit /b %errorlevel%
