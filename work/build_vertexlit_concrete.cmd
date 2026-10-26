@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\vertexlit-concrete-20261026"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_VERTEXLIT_CONCRETE work\probe_vertexlit_concrete.cpp /Fo:"%OUT%\native.obj" /Fe:"%OUT%\concrete_native_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\native-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
>"%OUT%\trig-wrapper.asm" echo include work\material_trig_x87_candidate.inc
>>"%OUT%\trig-wrapper.asm" echo END
ml64 /nologo /c /I"%CD%" /Fo"%OUT%\trig.obj" "%OUT%\trig-wrapper.asm" >"%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_vertexlit_concrete.cpp "%OUT%\trig.obj" /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\concrete_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\owned-build.log" 2>&1
exit /b %errorlevel%
