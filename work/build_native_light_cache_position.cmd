@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\light-cache-position-oracle-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_POSITION_ORACLE work\probe_native_light_cache_position.cpp /Fo%OUT%\native.obj /Fe%OUT%\native_position_x86.exe > %OUT%\native-build.log 2>&1
if errorlevel 1 (
 type %OUT%\native-build.log
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_native_light_cache_position.cpp /Fo%OUT%\candidate.obj /Fe%OUT%\position_candidate_x64.exe > %OUT%\candidate-build.log 2>&1
if errorlevel 1 type %OUT%\candidate-build.log
exit /b %errorlevel%
