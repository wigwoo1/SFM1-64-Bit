@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\generic-shader-skin-blend-20261025"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_generic_shader_skin_blend.cpp /Fo:"%OUT%\blend.obj" /Fe:"%OUT%\shader_skin_blend_x64.exe" >"%OUT%\build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\build.log"
 exit /b 1
)
exit /b 0
