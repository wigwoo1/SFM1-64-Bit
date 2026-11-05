@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "HDR2_REGION_OUT=%CD%\outputs\native-lightmap-hdr2-region-x64"
if not "%~1"=="" set "HDR2_REGION_OUT=%~f1"
if exist "%HDR2_REGION_OUT%\test_native_lightmap_hdr2_region_x64.exe" exit /b 2
if not exist "%HDR2_REGION_OUT%" mkdir "%HDR2_REGION_OUT%"
if errorlevel 1 exit /b 1
set "TEMP=%HDR2_REGION_OUT%"
set "TMP=%HDR2_REGION_OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_native_lightmap_hdr2_region.cpp /Fo:"%HDR2_REGION_OUT%\test_native_lightmap_hdr2_region.obj" /Fe:"%HDR2_REGION_OUT%\test_native_lightmap_hdr2_region_x64.exe" /link /MACHINE:X64 /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%HDR2_REGION_OUT%\build.log" 2>&1
if errorlevel 1 (
 type "%HDR2_REGION_OUT%\build.log"
 exit /b 1
)
exit /b 0
