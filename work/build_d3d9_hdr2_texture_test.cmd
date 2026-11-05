@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
if "%~1"=="" exit /b 2
set "HDR2_TEXTURE_OUT=%~f1"
if exist "%HDR2_TEXTURE_OUT%\probe_d3d9_hdr2_texture_x64.exe" exit /b 2
if not exist "%HDR2_TEXTURE_OUT%" mkdir "%HDR2_TEXTURE_OUT%"
set "TEMP=%HDR2_TEXTURE_OUT%"
set "TMP=%HDR2_TEXTURE_OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\probe_d3d9_hdr2_texture_x64.cpp /Fo:"%HDR2_TEXTURE_OUT%\probe_d3d9_hdr2_texture.obj" /Fe:"%HDR2_TEXTURE_OUT%\probe_d3d9_hdr2_texture_x64.exe" d3d9.lib d3dcompiler.lib user32.lib /link /MACHINE:X64 /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%HDR2_TEXTURE_OUT%\build.log" 2>&1
if errorlevel 1 type "%HDR2_TEXTURE_OUT%\build.log"
exit /b %errorlevel%
