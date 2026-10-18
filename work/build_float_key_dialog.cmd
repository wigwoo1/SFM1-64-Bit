@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_float_key_dialog.cpp /Fowork\test_float_key_dialog.obj /Fework\test_float_key_dialog.exe user32.lib gdi32.lib comctl32.lib
if errorlevel 1 exit /b 1
work\test_float_key_dialog.exe
