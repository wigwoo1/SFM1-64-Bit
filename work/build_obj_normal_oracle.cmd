@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\emit_obj_normal_oracle.cpp /Fo:build\session-core\emit_obj_normal_oracle.obj /Fe:build\session-core\emit_obj_normal_oracle.exe
if errorlevel 1 exit /b 1
build\session-core\emit_obj_normal_oracle.exe
exit /b %errorlevel%
