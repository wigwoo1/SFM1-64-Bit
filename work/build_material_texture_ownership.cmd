@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-texture-ownership-20261027"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
>"%OUT%\arithmetic.asm" echo include work\material_trig_x87_candidate.inc
>>"%OUT%\arithmetic.asm" echo include work\material_sine_double_x87_candidate.inc
>>"%OUT%\arithmetic.asm" echo include work\material_cosine_double_x87_candidate.inc
>>"%OUT%\arithmetic.asm" echo include work\material_multiply_scalar_x87_candidate.inc
>>"%OUT%\arithmetic.asm" echo END
ml64 /nologo /c /I"%CD%" /Fo"%OUT%\arithmetic.obj" "%OUT%\arithmetic.asm" >"%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
for %%S in (test_material_texture_ownership test_material_store_session_scope test_material_store_session_batch test_material_session_draw_transaction test_material_store_bindings test_material_pinned_proxy_profile test_material_cached_backup probe_material_object_parameters probe_material_store) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\%%S.cpp "%OUT%\arithmetic.obj" /Fo:"%OUT%\%%S.obj" /Fe:"%OUT%\%%S_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\%%S-build.log" 2>&1
 if errorlevel 1 exit /b 1
)
exit /b 0
