@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-typed-backup-composition-20261027"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_GET_STRING work\probe_material_get_string.cpp /Fo:"%OUT%\getter.obj" /Fe:"%OUT%\getter_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\getter-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_SESSION_BACKUP work\probe_material_session_backup.cpp /Fo:"%OUT%\capture.obj" /Fe:"%OUT%\capture_reference_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\capture-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
>"%OUT%\trig-wrapper.asm" echo include work\material_trig_x87_candidate.inc
>>"%OUT%\trig-wrapper.asm" echo END
ml64 /nologo /c /I"%CD%" /Fo"%OUT%\trig.obj" "%OUT%\trig-wrapper.asm" >"%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_parameter_backup.cpp /Fo:"%OUT%\owned.obj" /Fe:"%OUT%\parameter_backup_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\owned-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_material_parameter_backup.cpp "%OUT%\trig.obj" /Fo:"%OUT%\safety.obj" /Fe:"%OUT%\parameter_backup_safety_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\safety-build.log" 2>&1
exit /b %errorlevel%
