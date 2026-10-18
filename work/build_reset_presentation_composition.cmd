@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_reset_presentation_composition.cpp /Fowork\test_reset_presentation_composition.obj /Fework\test_reset_presentation_composition.exe
if errorlevel 1 exit /b 1
work\test_reset_presentation_composition.exe


