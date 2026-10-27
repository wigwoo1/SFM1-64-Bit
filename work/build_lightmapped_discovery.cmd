@echo off
setlocal
cd /d "%~dp0.."
set "OUT=%CD%\outputs\lightmapped-parameters-20261026"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_lightmapped_parameters_discovery.cpp /Fo:"%OUT%\discovery.obj" /Fe:"%OUT%\discovery_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" >"%OUT%\discovery-build.log" 2>&1
exit /b %errorlevel%
