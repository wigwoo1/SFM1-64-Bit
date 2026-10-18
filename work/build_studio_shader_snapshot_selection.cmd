@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_studio_shader_snapshot_selection.cpp /Fowork\test_studio_shader_snapshot_selection.obj /Fework\test_studio_shader_snapshot_selection.exe
if errorlevel 1 exit /b 1
work\test_studio_shader_snapshot_selection.exe
