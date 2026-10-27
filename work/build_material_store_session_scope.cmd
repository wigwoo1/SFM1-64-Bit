@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-store-session-scope-20261027"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_session_scope_reference.cpp /Fo:"%OUT%\reference.obj" /Fe:"%OUT%\scope_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\reference-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
>"%OUT%\trig-wrapper.asm" echo include work\material_trig_x87_candidate.inc
>>"%OUT%\trig-wrapper.asm" echo END
ml64 /nologo /c /I"%CD%" /Fo"%OUT%\trig.obj" "%OUT%\trig-wrapper.asm" >"%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_material_store_session_scope.cpp "%OUT%\trig.obj" /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\scope_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\owned-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_store.cpp "%OUT%\trig.obj" /Fo:"%OUT%\store.obj" /Fe:"%OUT%\store_regression_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\store-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_object_parameters.cpp "%OUT%\trig.obj" /Fo:"%OUT%\parameters.obj" /Fe:"%OUT%\parameters_regression_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\parameters-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_material_cached_backup.cpp "%OUT%\trig.obj" /Fo:"%OUT%\cached.obj" /Fe:"%OUT%\cached_regression_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\cached-build.log" 2>&1
exit /b %errorlevel%
