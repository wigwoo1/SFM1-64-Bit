@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\benchmark-profile-materials-20261027"
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
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\audit_benchmark_profile_materials.cpp "%OUT%\arithmetic.obj" /Fo:"%OUT%\inventory.obj" /Fe:"%OUT%\benchmark_profile_materials_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\inventory-build.log" 2>&1
exit /b %errorlevel%
