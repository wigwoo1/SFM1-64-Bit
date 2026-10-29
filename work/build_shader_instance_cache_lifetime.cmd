@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\shader-instance-cache-lifetime-20261028"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
for %%S in (probe_shader_instance_cache_lifetime probe_shader_snapshot_instance_release probe_vertexlit_generic_initial_commands probe_vertexlit_generic_preparation probe_private_boundary_observer_self_test) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\%%S.cpp /Fo:"%OUT%\%%S_x86.obj" /Fe:"%OUT%\%%S_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\%%S_x86-build.log" 2>&1
 if errorlevel 1 (
  type "%OUT%\%%S_x86-build.log"
  exit /b 1
 )
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%S in (probe_vertexlit_generic_initial_commands probe_vertexlit_generic_preparation probe_generic_blend_resource_reference) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\%%S.cpp /Fo:"%OUT%\%%S_x64.obj" /Fe:"%OUT%\%%S_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\%%S_x64-build.log" 2>&1
 if errorlevel 1 (
  type "%OUT%\%%S_x64-build.log"
  exit /b 1
 )
)
exit /b 0
