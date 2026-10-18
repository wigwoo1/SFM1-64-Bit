@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict /arch:SSE2 work\probe_drift_correction.cpp /Fowork\probe_drift_correction_x86.obj /Fework\probe_drift_correction_x86.exe
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fowork\clock_drift_mean_x64.obj work\clock_drift_mean_x64.asm
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\probe_drift_correction.cpp work\clock_drift_mean_x64.obj /Fowork\probe_drift_correction_x64.obj /Fework\probe_drift_correction_x64.exe
exit /b %errorlevel%
