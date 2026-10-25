@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\benchmark-draw-bounds-20261025"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_IFM_BOUNDS_COMPOSITE /DIFM_DRAW_BOUNDS_COMPOSITE work\probe_ifm_bounds_composite.cpp /Fo:"%OUT%\draw-native.obj" /Fe:"%OUT%\draw_bounds_native_x86.exe" > "%OUT%\draw-native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\draw-native-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_IFM_BOUNDS_COMPOSITE work\probe_ifm_bounds_composite.cpp /Fo:"%OUT%\bounds-native.obj" /Fe:"%OUT%\bounds_native_x86.exe" > "%OUT%\bounds-native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\bounds-native-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_BONE_PALETTE work\probe_ifm_bone_world_palette.cpp /Fo:"%OUT%\palette-native.obj" /Fe:"%OUT%\palette_native_x86.exe" > "%OUT%\palette-native-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\palette-native-build.log"
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo"%OUT%\sincos.obj" work\x64_fsin_cos.asm > "%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DIFM_DRAW_BOUNDS_COMPOSITE work\probe_ifm_bounds_composite.cpp "%OUT%\sincos.obj" /Fo:"%OUT%\draw-owned.obj" /Fe:"%OUT%\draw_bounds_owned_x64.exe" > "%OUT%\draw-owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\draw-owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_ifm_bounds_composite.cpp "%OUT%\sincos.obj" /Fo:"%OUT%\bounds-owned.obj" /Fe:"%OUT%\bounds_owned_x64.exe" > "%OUT%\bounds-owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\bounds-owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_ifm_bone_world_palette.cpp /Fo:"%OUT%\palette-owned.obj" /Fe:"%OUT%\palette_owned_x64.exe" > "%OUT%\palette-owned-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\palette-owned-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\export_benchmark_draw_bounds.cpp "%OUT%\sincos.obj" /Fo:"%OUT%\export.obj" /Fe:"%OUT%\benchmark_draw_bounds_x64.exe" > "%OUT%\export-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\export-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_ifm_draw_bounds_geometry.cpp "%OUT%\sincos.obj" /Fo:"%OUT%\safety.obj" /Fe:"%OUT%\draw_bounds_safety_x64.exe" > "%OUT%\safety-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\safety-build.log"
 exit /b 1
)
exit /b 0
