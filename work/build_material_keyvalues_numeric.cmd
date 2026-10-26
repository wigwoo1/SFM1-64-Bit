@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-keyvalues-numeric-20261026"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_MATERIAL_KEYVALUES_PARSER work\probe_material_keyvalues_parser.cpp /Fo:"%OUT%\native.obj" /Fe:"%OUT%\parser_native_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\native-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_material_keyvalues_files_native.cpp /Fo:"%OUT%\files-native.obj" /Fe:"%OUT%\files_native_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\files-native-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_PREFIX_STATUS work\probe_material_decimal_prefix_status.cpp /Fo:"%OUT%\prefix-native.obj" /Fe:"%OUT%\prefix_native_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\prefix-native-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%T in (parser parser_safety files_safety numeric_safety files prefix) do (
 call :owned %%T
 if errorlevel 1 exit /b 1
)
exit /b 0
:owned
set "SOURCE=work\probe_material_keyvalues_parser.cpp"
if "%1"=="parser_safety" set "SOURCE=work\test_material_keyvalues_parser.cpp"
if "%1"=="files_safety" set "SOURCE=work\test_material_keyvalues_files.cpp"
if "%1"=="numeric_safety" set "SOURCE=work\test_material_keyvalues_numeric.cpp"
if "%1"=="files" set "SOURCE=work\probe_material_keyvalues_files_owned.cpp"
if "%1"=="prefix" set "SOURCE=work\probe_material_decimal_prefix_status.cpp"
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 "%SOURCE%" /Fo:"%OUT%\%1.obj" /Fe:"%OUT%\%1_owned_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\%1-build.log" 2>&1
if errorlevel 1 exit /b 1
exit /b 0
