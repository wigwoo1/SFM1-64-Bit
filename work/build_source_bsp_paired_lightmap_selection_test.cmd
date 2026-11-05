@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\paired-bsp-dataset-test"
if not "%~1"=="" set "OUT=%~f1"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_source_bsp_paired_lightmap_selection.cpp /Fo:"%OUT%\test.obj" /Fe:"%OUT%\test_source_bsp_paired_lightmap_selection_x64.exe" >"%OUT%\build.log" 2>&1
if errorlevel 1 type "%OUT%\build.log"
exit /b %errorlevel%
