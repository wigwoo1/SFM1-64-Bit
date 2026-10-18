@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_shader_cull_transition.cpp /Fowork\test_shader_cull_transition.obj /Fework\test_shader_cull_transition.exe
if errorlevel 1 exit /b 1
work\test_shader_cull_transition.exe
