@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_native_d3d9_shadow_commands.cpp /Fowork\test_native_d3d9_shadow_commands.obj /Fework\test_native_d3d9_shadow_commands.exe
if errorlevel 1 exit /b 1
work\test_native_d3d9_shadow_commands.exe
