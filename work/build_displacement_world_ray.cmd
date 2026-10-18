@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\displacement-world-ray-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_WORLD_RAY_ORACLE work\probe_displacement_world_ray.cpp /Fo%OUT%\native.obj /Fe%OUT%\world_ray_native_x86.exe > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_displacement_world_ray.cpp /Fo%OUT%\owned.obj /Fe%OUT%\world_ray_owned_x64.exe > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_displacement_world_ray.cpp /Fo%OUT%\safety.obj /Fe%OUT%\world_ray_safety_x64.exe > %OUT%\safety-build.log 2>&1
if errorlevel 1 exit /b 1
%OUT%\world_ray_safety_x64.exe > %OUT%\safety-test.log 2>&1
exit /b %errorlevel%
