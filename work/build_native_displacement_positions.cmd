@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\native-displacement-positions-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_native_displacement_positions.cpp /Fo%OUT%\native.obj /Fe%OUT%\positions_native_x86.exe > %OUT%\build.log 2>&1
exit /b %errorlevel%
