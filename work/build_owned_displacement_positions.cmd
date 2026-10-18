@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\native-displacement-positions-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_owned_displacement_positions.cpp /Fo%OUT%\owned.obj /Fe%OUT%\positions_owned_x64.exe > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_displacement_grid_positions.cpp /Fo%OUT%\safety.obj /Fe%OUT%\positions_safety_x64.exe > %OUT%\safety-build.log 2>&1
if errorlevel 1 exit /b 1
%OUT%\positions_safety_x64.exe > %OUT%\safety-test.log 2>&1
exit /b %errorlevel%
