@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_vertex_constant_runs.cpp /Fowork\test_vertex_constant_runs.obj /Fework\test_vertex_constant_runs.exe
if errorlevel 1 exit /b 1
work\test_vertex_constant_runs.exe
