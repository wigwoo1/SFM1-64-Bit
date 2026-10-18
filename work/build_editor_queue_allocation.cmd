@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_editor_queue_allocation.cpp /Fowork\test_editor_queue_allocation.obj /Fework\test_editor_queue_allocation.exe user32.lib comctl32.lib
if errorlevel 1 exit /b 1
