@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_mesh_primitive_count_native.cpp /Fowork\test_mesh_primitive_count_native.obj /Fework\test_mesh_primitive_count_native.exe
if errorlevel 1 exit /b 1
work\test_mesh_primitive_count_native.exe




