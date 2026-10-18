@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\dag-world-position-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_DAG_WORLD_POSITION work\probe_dag_world_position.cpp /Fo%OUT%\native.obj /Fe%OUT%\dag_world_native_x86.exe > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_dag_world_position.cpp /Fo%OUT%\owned.obj /Fe%OUT%\dag_world_owned_x64.exe > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo%OUT%\math.obj work\x64_fsin_cos.asm > %OUT%\math-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\export_session_dag_world_fixture.cpp %OUT%\math.obj /Fo%OUT%\export.obj /Fe%OUT%\dag_world_export_x64.exe > %OUT%\export-build.log 2>&1
exit /b %errorlevel%
