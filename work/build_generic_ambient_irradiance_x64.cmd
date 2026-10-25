@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\generic-ambient-shader-20261024"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_generic_ambient_irradiance.cpp /Fo:"%OUT%\ambient_irradiance_x64.obj" /Fe:"%OUT%\ambient_irradiance_x64.exe" > "%OUT%\build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\build.log"
 exit /b 1
)
dumpbin /headers "%OUT%\ambient_irradiance_x64.exe" > "%OUT%\binary-headers.txt"
if errorlevel 1 exit /b 1
findstr /C:"8664 machine (x64)" "%OUT%\binary-headers.txt" >nul
if errorlevel 1 exit /b 1
findstr /C:"20B magic # (PE32+)" "%OUT%\binary-headers.txt" >nul
if errorlevel 1 exit /b 1
exit /b 0
