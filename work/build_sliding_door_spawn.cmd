@echo off
setlocal
cd /d "%~dp0.."
set "OUT=%SFM64_MAP_DOOR_TEST_DIRECTORY%"
if not defined OUT set "OUT=outputs\map-door-state"
if not exist "%OUT%" mkdir "%OUT%"

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_SLIDING_DOOR_SPAWN work\probe_sliding_door_spawn.cpp /Fo"%OUT%\door_x86.obj" /Fe"%OUT%\door_x86.exe" > "%OUT%\door_x86-build.log" 2>&1
if errorlevel 1 exit /b 1

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo"%OUT%\door-sincos.obj" work\x64_fsin_cos.asm > "%OUT%\door-sincos-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_sliding_door_spawn.cpp "%OUT%\door-sincos.obj" /Fo"%OUT%\door_x64.obj" /Fe"%OUT%\door_x64.exe" > "%OUT%\door_x64-build.log" 2>&1
exit /b %ERRORLEVEL%
