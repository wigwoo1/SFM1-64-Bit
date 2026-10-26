@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-keyvalues-numeric-20261026\regression-build"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%T in (value_construction vector_literal matrix_scan) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_%%T.cpp /Fo:"%OUT%\%%T.obj" /Fe:"%OUT%\%%T_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\%%T-build.log" 2>&1
 if errorlevel 1 exit /b 1
)
>"%OUT%\trig-wrapper.asm" echo include work\material_trig_x87_candidate.inc
>>"%OUT%\trig-wrapper.asm" echo END
ml64 /nologo /c /I"%CD%" /Fo"%OUT%\trig.obj" "%OUT%\trig-wrapper.asm" >"%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_typed_dispatch.cpp "%OUT%\trig.obj" /Fo:"%OUT%\typed.obj" /Fe:"%OUT%\typed_dispatch_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\typed_dispatch-build.log" 2>&1
exit /b %errorlevel%
