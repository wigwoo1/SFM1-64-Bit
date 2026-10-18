@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\light-cache-position-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_source_light_cache_position.cpp /Fo%OUT%\test_source_light_cache_position.obj /Fe%OUT%\test_source_light_cache_position.exe > %OUT%\build.log 2>&1
if errorlevel 1 (
 type %OUT%\build.log
 exit /b 1
)
%OUT%\test_source_light_cache_position.exe > %OUT%\test.log 2>&1
set "RESULT=%errorlevel%"
type %OUT%\test.log
exit /b %RESULT%
