@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\engine-point-trace-20261016"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_ENGINE_POINT_TRACE work\probe_engine_point_trace.cpp /Fo%OUT%\engine_point_native_x86.obj /Fe%OUT%\engine_point_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\engine_point_native_x86-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2  work\probe_engine_point_trace.cpp /Fo%OUT%\engine_point_owned_x64.obj /Fe%OUT%\engine_point_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\engine_point_owned_x64-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_engine_point_trace.cpp /Fo%OUT%\safety.obj /Fe%OUT%\engine_point_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\safety-build.log 2>&1
if errorlevel 1 exit /b 1
exit /b 0
