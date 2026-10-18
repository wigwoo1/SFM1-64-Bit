@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_default_render_state_owner.cpp /Fowork\test_default_render_state_owner.obj /Fework\test_default_render_state_owner.exe
if errorlevel 1 exit /b 1
work\test_default_render_state_owner.exe



