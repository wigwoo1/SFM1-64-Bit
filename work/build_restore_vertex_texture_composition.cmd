@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_restore_vertex_texture_composition.cpp /Fowork\test_restore_vertex_texture_composition.obj /Fework\test_restore_vertex_texture_composition.exe
if not "%errorlevel%"=="0" exit /b 1
work\test_restore_vertex_texture_composition.exe
if not "%errorlevel%"=="0" exit /b 1
