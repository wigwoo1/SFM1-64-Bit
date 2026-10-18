@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set "TMP=%CD%\work"
set "TEMP=%CD%\work"
cl /nologo /std:c++17 /EHsc /W4 work\test_attribute_initialize.cpp /Fowork\test_attribute_initialize.obj /Fework\test_attribute_initialize.exe
if errorlevel 1 exit /b 1
work\test_attribute_initialize.exe

