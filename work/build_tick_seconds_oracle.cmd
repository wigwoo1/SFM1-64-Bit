@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /O2 work\probe_native_tick_seconds_x86.cpp /Fo:build\session-core\probe_native_tick_seconds_x86.obj /Fe:build\session-core\probe_native_tick_seconds_x86.exe
exit /b %errorlevel%
