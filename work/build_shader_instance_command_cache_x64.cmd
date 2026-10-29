@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\shader-instance-cache-component-20261028"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_shader_instance_command_cache.cpp /Fo:"%OUT%\probe_shader_instance_command_cache_x64.obj" /Fe:"%OUT%\probe_shader_instance_command_cache_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\probe_shader_instance_command_cache_x64-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\probe_shader_instance_command_cache_x64-build.log"
 exit /b 1
)
exit /b 0
