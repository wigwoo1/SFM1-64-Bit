@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "PACK_OUT=%CD%\outputs\native-lightmap-hdr0-pixel-x64"
if not "%~1"=="" set "PACK_OUT=%~f1"
if exist "%PACK_OUT%\test_native_lightmap_hdr0_pixel_x64.exe" exit /b 2
if not exist "%PACK_OUT%" mkdir "%PACK_OUT%"
if errorlevel 1 exit /b 1
set "TEMP=%PACK_OUT%"
set "TMP=%PACK_OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_native_lightmap_hdr0_pixel.cpp /Fo:"%PACK_OUT%\test_native_lightmap_hdr0_pixel.obj" /Fe:"%PACK_OUT%\test_native_lightmap_hdr0_pixel_x64.exe" /link /MACHINE:X64 /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%PACK_OUT%\build.log" 2>&1
if errorlevel 1 (
 type "%PACK_OUT%\build.log"
 exit /b 1
)
exit /b 0
