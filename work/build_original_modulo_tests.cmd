@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_original_modulo.cpp /Fowork\test_original_modulo.obj /Fework\test_original_modulo.exe
if errorlevel 1 exit /b 1
work\test_original_modulo.exe


