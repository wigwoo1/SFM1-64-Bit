@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\physics-aabb-20261016"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_PHY_AABB work\probe_phy_aabb.cpp /Fo%OUT%\native.obj /Fe%OUT%\phy_aabb_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo%OUT%\sincos.obj work\x64_fsin_cos.asm > %OUT%\asm-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_phy_aabb.cpp %OUT%\sincos.obj /Fo%OUT%\owned.obj /Fe%OUT%\phy_aabb_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_phy_aabb.cpp %OUT%\sincos.obj /Fo%OUT%\safety.obj /Fe%OUT%\phy_aabb_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\safety-build.log 2>&1
if errorlevel 1 exit /b 1
if not exist "%OUT%\regression" mkdir "%OUT%\regression"
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_static_prop_point_registry.cpp %OUT%\sincos.obj /Fo%OUT%\regression\registry.obj /Fe%OUT%\regression\prop_registry_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\regression\registry-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_static_prop_point_registry.cpp %OUT%\sincos.obj /Fo%OUT%\regression\safety.obj /Fe%OUT%\regression\prop_registry_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\regression\safety-build.log 2>&1
exit /b %errorlevel%
