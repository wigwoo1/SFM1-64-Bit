@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_float_interpolation.cpp /Fowork\test_float_interpolation.obj /Fework\test_float_interpolation.exe
if errorlevel 1 exit /b 1
work\test_float_interpolation.exe
