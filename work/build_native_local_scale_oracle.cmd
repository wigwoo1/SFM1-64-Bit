@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /EHsc /O2 work\probe_native_local_scale.cpp /Fo:build\session-core\probe_native_local_scale.obj /Fe:build\session-core\probe_native_local_scale.exe
if errorlevel 1 exit /b 1
build\session-core\probe_native_local_scale.exe
