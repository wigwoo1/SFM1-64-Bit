@echo off
setlocal
cd /d "%~dp0.."
set "OUT=%SFM64_SOLID_TRIGGER_TEST_DIRECTORY%"
if not defined OUT set "OUT=outputs\solid-trigger-scan"
if not exist "%OUT%" mkdir "%OUT%"

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_SOLID_TRIGGER_SCAN work\probe_solid_trigger_scan.cpp /Fo"%OUT%\scan_x86.obj" /Fe"%OUT%\scan_x86.exe" > "%OUT%\scan_x86-build.log" 2>&1
if errorlevel 1 exit /b 1

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_solid_trigger_scan.cpp /Fo"%OUT%\scan_x64.obj" /Fe"%OUT%\scan_x64.exe" > "%OUT%\scan_x64-build.log" 2>&1
exit /b %ERRORLEVEL%
