@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_shader_color_conversion.cpp /Fowork\test_shader_color_conversion.obj /Fework\test_shader_color_conversion.exe
if errorlevel 1 exit /b 1
work\test_shader_color_conversion.exe
