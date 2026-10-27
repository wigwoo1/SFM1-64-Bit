@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-store-bindings-20261027"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_ELEMENT_BINDING work\probe_material_element_binding.cpp /Fo:"%OUT%\reference.obj" /Fe:"%OUT%\bindings_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\reference-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
>"%OUT%\trig-wrapper.asm" echo include work\material_trig_x87_candidate.inc
>>"%OUT%\trig-wrapper.asm" echo END
ml64 /nologo /c /I"%CD%" /Fo"%OUT%\trig.obj" "%OUT%\trig-wrapper.asm" >"%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_material_store_bindings.cpp "%OUT%\trig.obj" /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\bindings_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\owned-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_material_store_session_scope.cpp "%OUT%\trig.obj" /Fo:"%OUT%\single.obj" /Fe:"%OUT%\single_regression_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\single-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_material_store_session_batch.cpp "%OUT%\trig.obj" /Fo:"%OUT%\batch.obj" /Fe:"%OUT%\batch_regression_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\batch-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_store.cpp "%OUT%\trig.obj" /Fo:"%OUT%\store.obj" /Fe:"%OUT%\store_regression_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\store-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_object_parameters.cpp "%OUT%\trig.obj" /Fo:"%OUT%\parameters.obj" /Fe:"%OUT%\parameters_regression_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\parameters-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_material_cached_backup.cpp "%OUT%\trig.obj" /Fo:"%OUT%\cached.obj" /Fe:"%OUT%\cached_regression_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\cached-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_store_bindings_benchmark.cpp "%OUT%\trig.obj" /Fo:"%OUT%\benchmark.obj" /Fe:"%OUT%\bindings_benchmark_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\benchmark-build.log" 2>&1
exit /b %errorlevel%
