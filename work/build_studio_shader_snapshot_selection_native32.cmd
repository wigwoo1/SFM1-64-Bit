@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_studio_shader_snapshot_selection.cpp /Fowork\test_studio_shader_snapshot_selection32.obj /Fework\test_studio_shader_snapshot_selection32.exe
if errorlevel 1 exit /b 1
work\test_studio_shader_snapshot_selection32.exe

