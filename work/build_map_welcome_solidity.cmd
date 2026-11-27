@echo off
setlocal
cd /d "%~dp0.."
set "OUT=%~1"
if not defined OUT exit /b 2
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_map_welcome_solidity.cpp /Fo"%OUT%\welcome_x86.obj" /Fe"%OUT%\welcome_x86.exe" > "%OUT%\x86-build.log" 2>&1
exit /b %ERRORLEVEL%
