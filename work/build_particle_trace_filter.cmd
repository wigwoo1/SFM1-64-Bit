@echo off
setlocal
cd /d "%~dp0.."
set "OUT=%SFM64_ENGINE_POINT_TEST_DIRECTORY%"
if not defined OUT set "OUT=outputs\particle-trace-filter"
if not exist "%OUT%" mkdir "%OUT%"

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /DNATIVE_PARTICLE_TRACE_FILTER work\probe_particle_trace_filter.cpp /Fo"%OUT%\particle_filter_x86.obj" /Fe"%OUT%\particle_filter_x86.exe" > "%OUT%\particle_filter_x86-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\probe_particle_trace_filter.cpp /Fo"%OUT%\particle_filter_x64.obj" /Fe"%OUT%\particle_filter_x64.exe" > "%OUT%\particle_filter_x64-build.log" 2>&1
exit /b %ERRORLEVEL%
