@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict /arch:SSE2 work\probe_native_live_identity_registry.cpp /Fowork\probe_native_live_identity_registry.obj /Fework\probe_native_live_identity_registry.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker'
if errorlevel 1 exit /b 1

