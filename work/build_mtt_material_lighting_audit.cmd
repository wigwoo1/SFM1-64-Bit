@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-lighting-audit-20261014"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\audit_mtt_material_lighting.cpp /Fo:"%OUT%\audit_mtt_material_lighting.obj" /Fe:"%OUT%\audit_mtt_material_lighting.exe" > "%OUT%\audit-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\audit-build.log"
 exit /b 1
)
exit /b 0
