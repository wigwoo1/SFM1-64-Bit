@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\leaf-ambient-interpolation-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_source_leaf_ambient_interpolation.cpp /Fo%OUT%\probe_source_leaf_ambient_interpolation.obj /Fe%OUT%\probe_source_leaf_ambient_interpolation.exe > %OUT%\build.log 2>&1
if errorlevel 1 type %OUT%\build.log
exit /b %errorlevel%
