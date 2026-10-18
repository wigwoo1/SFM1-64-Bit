@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\physics-leaf-20261016"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_physics_leaf_capture.cpp /Fo%OUT%\leaf_capture_x86.obj /Fe%OUT%\leaf_capture_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\leaf_capture_x86-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_PHYSICS_LEAF_HIT work\probe_physics_leaf_hit.cpp /Fo%OUT%\leaf_hit_native_x86.obj /Fe%OUT%\leaf_hit_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\leaf_hit_native_x86-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_physics_leaf.cpp /Fo%OUT%\leaf_owned_x64.obj /Fe%OUT%\leaf_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\leaf_owned_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_physics_leaf.cpp /Fo%OUT%\leaf_safety_x64.obj /Fe%OUT%\leaf_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\leaf_safety_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_physics_leaf_hit.cpp /Fo%OUT%\leaf_hit_owned_x64.obj /Fe%OUT%\leaf_hit_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\leaf_hit_owned_x64-build.log 2>&1
if errorlevel 1 exit /b 1
exit /b 0
