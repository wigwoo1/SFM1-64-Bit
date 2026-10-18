@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_specialized_draw_factor.cpp /Fowork\test_specialized_draw_factor32.obj /Fework\test_specialized_draw_factor32.exe
if errorlevel 1 exit /b 1
work\test_specialized_draw_factor32.exe
