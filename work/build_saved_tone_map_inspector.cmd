@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
if "%~1"=="" exit /b 2
set "OUT=%~f1"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\inspect_saved_tone_map_scale_x64.cpp /Fo:"%OUT%\inspect.obj" /Fe:"%OUT%\inspect_saved_tone_map_scale_x64.exe" >"%OUT%\build.log" 2>&1
exit /b %errorlevel%
