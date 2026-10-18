@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /Od work\test_shader_shadow_setup_native.cpp /Fowork\test_shader_shadow_setup_diagnostic32.obj /Fework\test_shader_shadow_setup_diagnostic32.exe
if errorlevel 1 exit /b 1
work\test_shader_shadow_setup_diagnostic32.exe


