@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_texture_binding_callbacks.cpp /Fowork\test_texture_binding_callbacks.obj /Fework\test_texture_binding_callbacks.exe
if errorlevel 1 exit /b 1
work\test_texture_binding_callbacks.exe
