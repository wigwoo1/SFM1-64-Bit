@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set TMP=%CD%\work
set TEMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 work\test_graph_bookmark_stack_boundary.cpp /Fework\test_graph_bookmark_stack_boundary.exe /Fowork\test_graph_bookmark_stack_boundary.obj
if errorlevel 1 exit /b 1
work\test_graph_bookmark_stack_boundary.exe


