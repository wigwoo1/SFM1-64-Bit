@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
if "%~1"=="" exit /b 2
set "SFM64_TOPOLOGY_OUT=%~f1"
if not exist "%SFM64_TOPOLOGY_OUT%" mkdir "%SFM64_TOPOLOGY_OUT%"
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_translucent_fixture_geometry_x64.cpp /Fo:"%SFM64_TOPOLOGY_OUT%\probe.obj" /Fe:"%SFM64_TOPOLOGY_OUT%\translucent_fixture_geometry_x64.exe" > "%SFM64_TOPOLOGY_OUT%\build.log" 2>&1
if errorlevel 1 (
 type "%SFM64_TOPOLOGY_OUT%\build.log"
 exit /b 1
)
type "%SFM64_TOPOLOGY_OUT%\build.log"
exit /b 0
