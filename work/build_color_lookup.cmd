@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_color_lookup.cpp /Fowork\test_color_lookup.obj /Fework\test_color_lookup.exe
if errorlevel 1 exit /b 1
work\test_color_lookup.exe
