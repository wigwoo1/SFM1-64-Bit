@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "HALF_OUT=%CD%\outputs\native-lightmap-half-x64"
if not "%~1"=="" set "HALF_OUT=%~f1"
if exist "%HALF_OUT%\test_native_lightmap_half_x64.exe" (
 echo Choose a fresh output directory; a previous executable already exists.
 exit /b 2
)
if not exist "%HALF_OUT%" mkdir "%HALF_OUT%"
if errorlevel 1 exit /b 1
set "TEMP=%HALF_OUT%"
set "TMP=%HALF_OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_native_lightmap_half.cpp /Fo:"%HALF_OUT%\test_native_lightmap_half.obj" /Fe:"%HALF_OUT%\test_native_lightmap_half_x64.exe" /link /MACHINE:X64 /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%HALF_OUT%\build.log" 2>&1
if errorlevel 1 (
 type "%HALF_OUT%\build.log"
 exit /b 1
)
exit /b 0
