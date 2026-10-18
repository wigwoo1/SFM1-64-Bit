@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\bsp-lightmap-addressing-20261014"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_bsp_lightmap_addressing.cpp /Fo:"%OUT%\test_bsp_lightmap_addressing.obj" /Fe:"%OUT%\test_bsp_lightmap_addressing.exe" > "%OUT%\test-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\test-build.log"
 exit /b 1
)
"%OUT%\test_bsp_lightmap_addressing.exe"
exit /b %errorlevel%
