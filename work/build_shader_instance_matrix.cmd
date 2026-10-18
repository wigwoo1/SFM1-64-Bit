@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_shader_instance_matrix.cpp /Fowork\test_shader_instance_matrix32.obj /Fework\test_shader_instance_matrix32.exe
if errorlevel 1 exit /b 1
work\test_shader_instance_matrix32.exe
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_shader_instance_matrix.cpp /Fowork\test_shader_instance_matrix64.obj /Fework\test_shader_instance_matrix64.exe
if errorlevel 1 exit /b 1
work\test_shader_instance_matrix64.exe
