@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-matrix-write-20261027"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_CLIENT_EQUALS work\probe_material_equals_evaluation.cpp /Fo:"%OUT%\equals-native.obj" /Fe:"%OUT%\equals_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\equals-native-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_PROXY_COMPONENT work\probe_material_proxy_component.cpp /Fo:"%OUT%\component-native.obj" /Fe:"%OUT%\component_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\component-native-build.log" 2>&1
if errorlevel 1 exit /b 1
for %%S in (material_proxy_lifetime unlit_two_texture_parameters world_transition_parameters lightmapped_parameters material_object_parameters) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_OBJECT_PARAMETERS work\probe_%%S.cpp /Fo:"%OUT%\%%S-native.obj" /Fe:"%OUT%\%%S_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\%%S-native-build.log" 2>&1
 if errorlevel 1 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_SINE_MATH work\probe_material_sine_math.cpp /Fo:"%OUT%\sine-math-native.obj" /Fe:"%OUT%\sine_math_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\sine-math-native-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_CLIENT_EQUALS work\probe_material_sine_evaluation.cpp /Fo:"%OUT%\sine-native.obj" /Fe:"%OUT%\sine_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\sine-native-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_CLIENT_EQUALS work\probe_material_proxy_float_input.cpp /Fo:"%OUT%\float-input-native.obj" /Fe:"%OUT%\float_input_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\float-input-native-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_MATRIX_WRITE work\probe_material_matrix_value_write.cpp /Fo:"%OUT%\matrix-value-native.obj" /Fe:"%OUT%\matrix_value_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\matrix-value-native-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
>"%OUT%\trig-wrapper.asm" echo include work\material_trig_x87_candidate.inc
>>"%OUT%\trig-wrapper.asm" echo include work\material_sine_double_x87_candidate.inc
>>"%OUT%\trig-wrapper.asm" echo include work\material_cosine_double_x87_candidate.inc
>>"%OUT%\trig-wrapper.asm" echo END
ml64 /nologo /c /I"%CD%" /Fo"%OUT%\trig.obj" "%OUT%\trig-wrapper.asm" >"%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_equals_evaluation.cpp "%OUT%\trig.obj" /Fo:"%OUT%\equals-owned.obj" /Fe:"%OUT%\equals_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\equals-owned-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_proxy_component.cpp /Fo:"%OUT%\component-owned.obj" /Fe:"%OUT%\component_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\component-owned-build.log" 2>&1
if errorlevel 1 exit /b 1
for %%S in (material_proxy_lifetime unlit_two_texture_parameters world_transition_parameters lightmapped_parameters material_object_parameters material_store) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_%%S.cpp "%OUT%\trig.obj" /Fo:"%OUT%\%%S-owned.obj" /Fe:"%OUT%\%%S_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\%%S-owned-build.log" 2>&1
 if errorlevel 1 exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_sine_math.cpp "%OUT%\trig.obj" /Fo:"%OUT%\sine-math-owned.obj" /Fe:"%OUT%\sine_math_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\sine-math-owned-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_sine_evaluation.cpp "%OUT%\trig.obj" /Fo:"%OUT%\sine-owned.obj" /Fe:"%OUT%\sine_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\sine-owned-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_proxy_float_input.cpp "%OUT%\trig.obj" /Fo:"%OUT%\float-input-owned.obj" /Fe:"%OUT%\float_input_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\float-input-owned-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_matrix_value_write.cpp "%OUT%\trig.obj" /Fo:"%OUT%\matrix-value-owned.obj" /Fe:"%OUT%\matrix_value_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\matrix-value-owned-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_material_matrix_write_store.cpp "%OUT%\trig.obj" /Fo:"%OUT%\matrix-store.obj" /Fe:"%OUT%\matrix_store_safety_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\matrix-store-build.log" 2>&1
if errorlevel 1 exit /b 1
exit /b %errorlevel%
