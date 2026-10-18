@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /O2 work\audit_timeline_overlap.cpp /Fo:build\session-core\audit_timeline_overlap.obj /Fe:build\session-core\audit_timeline_overlap.exe
if errorlevel 1 exit /b 1
build\session-core\audit_timeline_overlap.exe
