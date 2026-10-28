@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\generic-blend-resource-reference-20261028"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_generic_blend_resource_reference.cpp /Fo:"%OUT%\reference.obj" /Fe:"%OUT%\generic_blend_resource_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\reference-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\reference-build.log"
 exit /b 1
)
cl /nologo /std:c++17 /EHsc /W4 /O2 work\probe_private_boundary_observer_self_test.cpp /Fo:"%OUT%\observer-self-test.obj" /Fe:"%OUT%\observer-self-test_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\observer-self-test-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%S in (probe_generic_blend_resource_reference probe_generic_material_draw_dispatch) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\%%S.cpp /Fo:"%OUT%\%%S.obj" /Fe:"%OUT%\%%S_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\%%S-build.log" 2>&1
 if errorlevel 1 (
  type "%OUT%\%%S-build.log"
  exit /b 1
 )
)
exit /b 0
