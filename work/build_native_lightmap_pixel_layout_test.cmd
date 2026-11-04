@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "LAYOUT_OUT=%CD%\outputs\native-lightmap-pixel-layout-x64"
if not "%~1"=="" set "LAYOUT_OUT=%~f1"
if exist "%LAYOUT_OUT%\test_native_lightmap_pixel_layout_x64.exe" (
 echo Choose a fresh output directory; a previous executable already exists.
 exit /b 2
)
if not exist "%LAYOUT_OUT%" mkdir "%LAYOUT_OUT%"
if errorlevel 1 exit /b 1
set "TEMP=%LAYOUT_OUT%"
set "TMP=%LAYOUT_OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_native_lightmap_pixel_layout.cpp /Fo:"%LAYOUT_OUT%\test_native_lightmap_pixel_layout.obj" /Fe:"%LAYOUT_OUT%\test_native_lightmap_pixel_layout_x64.exe" /link /MACHINE:X64 /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%LAYOUT_OUT%\build.log" 2>&1
if errorlevel 1 (
 type "%LAYOUT_OUT%\build.log"
 exit /b 1
)
exit /b 0
