@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_vertex_counter_strings_native.cpp /Fowork\vertex_strings32.obj /Fework\sfm_vertex_strings32.exe
if not "%errorlevel%"=="0" exit /b 1
work\sfm_vertex_strings32.exe
if not "%errorlevel%"=="0" exit /b 1
