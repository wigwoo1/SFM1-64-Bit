@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_shader_vertex_layout_native.cpp /Fowork\test_shader_vertex_layout_native.obj /Fework\test_shader_vertex_layout_native.exe
if errorlevel 1 exit /b 1
work\test_shader_vertex_layout_native.exe

if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_shader_vertex_layout_native.cpp /Fowork\test_shader_vertex_layout_candidate_64.obj /Fework\test_shader_vertex_layout_candidate_64.exe
if errorlevel 1 exit /b 1
work\test_shader_vertex_layout_candidate_64.exe
