@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-test-fixtures-20261027"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%\temp"
set "TMP=%TEMP%"
if not exist "%TEMP%" mkdir "%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_selfillum_material_cache.cpp /Fo:"%OUT%\test_selfillum_material_cache.obj" /Fe:"%OUT%\test_selfillum_material_cache.exe" >"%OUT%\test_selfillum_material_cache-build.log" 2>&1
if errorlevel 1 exit /b 1
"%OUT%\test_selfillum_material_cache.exe" %*
exit /b %errorlevel%
