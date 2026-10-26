@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\unlit-material-integration-20261026"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_fallback_failures.cpp /Fo:"%OUT%\reference.obj" /Fe:"%OUT%\reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\reference-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
>"%OUT%\trig-wrapper.asm" echo include work\material_trig_x87_candidate.inc
>>"%OUT%\trig-wrapper.asm" echo END
ml64 /nologo /c /I"%CD%" /Fo"%OUT%\trig.obj" "%OUT%\trig-wrapper.asm" >"%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
for %%T in (directive partial parser_safety files_safety numeric_safety patch_safety parser files fallback parameters pipeline builtin) do (
 call :owned %%T
 if errorlevel 1 exit /b 1
)
exit /b 0
:owned
set "SOURCE=work\test_material_partial_load.cpp"
if "%1"=="builtin" set "SOURCE=work\probe_builtin_error_material_seed.cpp"
if "%1"=="directive" set "SOURCE=work\test_material_directive_recovery.cpp"
if "%1"=="parser_safety" set "SOURCE=work\test_material_keyvalues_parser.cpp"
if "%1"=="files_safety" set "SOURCE=work\test_material_keyvalues_files.cpp"
if "%1"=="numeric_safety" set "SOURCE=work\test_material_keyvalues_numeric.cpp"
if "%1"=="patch_safety" set "SOURCE=work\test_material_patch_files.cpp"
if "%1"=="parser" set "SOURCE=work\probe_material_keyvalues_parser.cpp"
if "%1"=="files" set "SOURCE=work\probe_material_patch_files_owned.cpp"
if "%1"=="fallback" set "SOURCE=work\probe_material_fallback_owned.cpp"
if "%1"=="parameters" set "SOURCE=work\probe_material_parameter_orchestration.cpp"
if "%1"=="pipeline" set "SOURCE=work\probe_material_file_parameters.cpp"
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 "%SOURCE%" "%OUT%\trig.obj" /Fo:"%OUT%\%1.obj" /Fe:"%OUT%\%1_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\%1-build.log" 2>&1
exit /b %errorlevel%
