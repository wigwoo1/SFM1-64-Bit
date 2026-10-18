@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\session-illumination-dag-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
ml64 /nologo /c /Fo%OUT%\math.obj work\x64_fsin_cos.asm > %OUT%\math-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\audit_session_illumination_dag.cpp %OUT%\math.obj /Fo%OUT%\audit.obj /Fe%OUT%\dag_audit_x64.exe > %OUT%\audit-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_session_illumination_dag.cpp /Fo%OUT%\safety.obj /Fe%OUT%\dag_safety_x64.exe > %OUT%\safety-build.log 2>&1
exit /b %errorlevel%
