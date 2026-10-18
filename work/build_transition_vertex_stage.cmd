@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_transition_vertex_stage.cpp /Fowork\test_transition_vertex_stage.obj /Fework\test_transition_vertex_stage.exe
if errorlevel 1 exit /b 1
work\test_transition_vertex_stage.exe

