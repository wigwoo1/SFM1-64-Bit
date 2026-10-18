@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_create_float_curveinfo.cpp /Fowork\test_create_float_curveinfo.obj /Fework\test_create_float_curveinfo.exe
if errorlevel 1 exit /b 1

