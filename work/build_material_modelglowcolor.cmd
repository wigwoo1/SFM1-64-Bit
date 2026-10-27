@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-modelglowcolor-proxy-20261027"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_MODELGLOWCOLOR work\probe_material_modelglowcolor.cpp /Fo:"%OUT%\native.obj" /Fe:"%OUT%\modelglowcolor_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\native-build.log" 2>&1
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
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_modelglowcolor.cpp "%OUT%\arithmetic.obj" /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\modelglowcolor_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\owned-build.log" 2>&1
exit /b %errorlevel%
