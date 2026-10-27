@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-pinned-proxy-profile-20261027"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_OBJECT_PARAMETERS work\probe_material_proxy_lifetime.cpp /Fo:"%OUT%\caller-native.obj" /Fe:"%OUT%\caller_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\caller-native-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
>"%OUT%\arithmetic.asm" echo include work\material_trig_x87_candidate.inc
>>"%OUT%\arithmetic.asm" echo include work\material_sine_double_x87_candidate.inc
>>"%OUT%\arithmetic.asm" echo include work\material_cosine_double_x87_candidate.inc
>>"%OUT%\arithmetic.asm" echo include work\material_multiply_scalar_x87_candidate.inc
>>"%OUT%\arithmetic.asm" echo END
ml64 /nologo /c /I"%CD%" /Fo"%OUT%\arithmetic.obj" "%OUT%\arithmetic.asm" >"%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_material_pinned_proxy_profile.cpp "%OUT%\arithmetic.obj" /Fo:"%OUT%\profile.obj" /Fe:"%OUT%\pinned_proxy_profile_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\profile-build.log" 2>&1
if errorlevel 1 exit /b 1
for %%S in (material_proxy_lifetime unlit_two_texture_parameters world_transition_parameters lightmapped_parameters material_object_parameters material_store) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_%%S.cpp "%OUT%\arithmetic.obj" /Fo:"%OUT%\%%S-owned.obj" /Fe:"%OUT%\%%S_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\%%S-build.log" 2>&1
 if errorlevel 1 exit /b 1
)
exit /b %errorlevel%
