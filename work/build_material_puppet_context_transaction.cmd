@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-puppet-context-transaction-20261027"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
>"%OUT%\trig-wrapper.asm" echo include work\material_trig_x87_candidate.inc
>>"%OUT%\trig-wrapper.asm" echo include work\material_sine_double_x87_candidate.inc
>>"%OUT%\trig-wrapper.asm" echo include work\material_cosine_double_x87_candidate.inc
>>"%OUT%\trig-wrapper.asm" echo END
ml64 /nologo /c /I"%CD%" /Fo"%OUT%\trig.obj" "%OUT%\trig-wrapper.asm" >"%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_puppet_context_transaction.cpp "%OUT%\trig.obj" /Fo:"%OUT%\puppet.obj" /Fe:"%OUT%\puppet_transaction_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\puppet-build.log" 2>&1
if errorlevel 1 exit /b 1
for %%S in (material_equals_evaluation material_texture_transform_evaluation material_texture_scroll_evaluation) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_%%S.cpp "%OUT%\trig.obj" /Fo:"%OUT%\%%S.obj" /Fe:"%OUT%\%%S_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\%%S-build.log" 2>&1
 if errorlevel 1 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_material_matrix_write_store.cpp "%OUT%\trig.obj" /Fo:"%OUT%\matrix.obj" /Fe:"%OUT%\matrix_store_safety_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\matrix-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_session_model_proxy_context.cpp /Fo:"%OUT%\context.obj" /Fe:"%OUT%\model_context_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\context-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_weapon_invis_contract.cpp "%OUT%\trig.obj" /Fo:"%OUT%\weapon.obj" /Fe:"%OUT%\weapon_invis_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\weapon-build.log" 2>&1
exit /b %errorlevel%
