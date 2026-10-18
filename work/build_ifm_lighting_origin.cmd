@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\ifm-lighting-origin-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_IFM_LIGHTING_ORIGIN work\probe_ifm_lighting_origin.cpp /Fo%OUT%\native.obj /Fe%OUT%\origin_native_x86.exe > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_ifm_lighting_origin.cpp /Fo%OUT%\owned.obj /Fe%OUT%\origin_owned_x64.exe > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_ifm_lighting_origin.cpp /Fo%OUT%\safety.obj /Fe%OUT%\origin_safety_x64.exe > %OUT%\safety-build.log 2>&1
exit /b %errorlevel%
