@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\bsp-displacement-collision-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_bsp_displacement_collision.cpp /Fo%OUT%\owned.obj /Fe%OUT%\bsp_collision_owned_x64.exe > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_bsp_displacement_collision.cpp /Fo%OUT%\safety.obj /Fe%OUT%\bsp_collision_safety_x64.exe > %OUT%\safety-build.log 2>&1
exit /b %errorlevel%
