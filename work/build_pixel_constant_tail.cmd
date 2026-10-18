@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_pixel_constant_tail.cpp /Fowork\test_pixel_constant_tail.obj /Fework\test_pixel_constant_tail.exe
if errorlevel 1 exit /b 1
work\test_pixel_constant_tail.exe
