@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\phy-raw-collision-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_PHY_RAW_COLLISION work\probe_phy_raw_collision.cpp /Fo%OUT%\native.obj /Fe%OUT%\raw_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_phy_raw_collision.cpp /Fo%OUT%\owned.obj /Fe%OUT%\raw_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_owned_phy_compact_geometry.cpp /Fo%OUT%\legacy.obj /Fe%OUT%\geometry_legacy_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\legacy-build.log 2>&1
exit /b %errorlevel%
