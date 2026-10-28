@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\model-material-draw-mapping-20261028"
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
for %%S in (test_model_material_draw_mapping test_model_material_prebind_draw) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\%%S.cpp "%OUT%\arithmetic.obj" /Fo:"%OUT%\%%S.obj" /Fe:"%OUT%\%%S_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\%%S-build.log" 2>&1
 if errorlevel 1 (
  type "%OUT%\%%S-build.log"
  exit /b 1
 )
)
exit /b 0
