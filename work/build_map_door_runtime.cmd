@echo off
setlocal
cd /d "%~dp0.."
set "OUT=%SFM64_MAP_DOOR_RUNTIME_TEST_DIRECTORY%"
if not defined OUT set "OUT=outputs\map-door-runtime"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo"%OUT%\runtime-sincos.obj" work\x64_fsin_cos.asm > "%OUT%\runtime-sincos-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_map_door_runtime.cpp "%OUT%\runtime-sincos.obj" /Fo"%OUT%\runtime_x64.obj" /Fe"%OUT%\runtime_x64.exe" > "%OUT%\runtime_x64-build.log" 2>&1
exit /b %ERRORLEVEL%
