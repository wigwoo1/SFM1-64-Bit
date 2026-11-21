@echo off
setlocal
cd /d "%~dp0.."
set "OUT=%SFM64_TRIGGER_ACTIVATION_TEST_DIRECTORY%"
if not defined OUT set "OUT=outputs\trigger-activation-scan"
if not exist "%OUT%" mkdir "%OUT%"

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_TRIGGER_ACTIVATION_SCAN work\probe_trigger_activation_scan.cpp /Fo"%OUT%\activation_x86.obj" /Fe"%OUT%\activation_x86.exe" > "%OUT%\activation_x86-build.log" 2>&1
if errorlevel 1 exit /b 1

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_trigger_activation_scan.cpp /Fo"%OUT%\activation_x64.obj" /Fe"%OUT%\activation_x64.exe" > "%OUT%\activation_x64-build.log" 2>&1
exit /b %ERRORLEVEL%
