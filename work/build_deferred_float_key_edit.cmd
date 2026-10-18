@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_deferred_float_key_edit.cpp /Fowork\test_deferred_float_key_edit.obj /Fework\test_deferred_float_key_edit.exe user32.lib gdi32.lib
if errorlevel 1 exit /b 1
work\test_deferred_float_key_edit.exe
