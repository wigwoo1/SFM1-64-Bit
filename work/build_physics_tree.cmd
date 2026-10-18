@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\physics-tree-20261015"
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%OUT%\regression" mkdir "%OUT%\regression"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2  work\probe_physics_tree_capture.cpp /Fo%OUT%\tree_capture_x86.obj /Fe%OUT%\tree_capture_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\tree_capture_x86-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_PHY_RAW_COLLISION work\probe_phy_raw_collision.cpp /Fo%OUT%\regression\raw_native_x86.obj /Fe%OUT%\regression\raw_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\regression\raw_native_x86-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2  work\probe_owned_phy_tree_traversal.cpp /Fo%OUT%\tree_owned_x64.obj /Fe%OUT%\tree_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\tree_owned_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2  work\test_physics_tree_traversal.cpp /Fo%OUT%\tree_safety_x64.obj /Fe%OUT%\tree_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\tree_safety_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2  work\probe_phy_raw_collision.cpp /Fo%OUT%\regression\raw_owned_x64.obj /Fe%OUT%\regression\raw_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\regression\raw_owned_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2  work\probe_owned_phy_compact_geometry.cpp /Fo%OUT%\regression\geometry_legacy_owned_x64.obj /Fe%OUT%\regression\geometry_legacy_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\regression\geometry_legacy_owned_x64-build.log 2>&1
if errorlevel 1 exit /b 1
exit /b 0
