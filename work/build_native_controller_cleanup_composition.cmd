@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set TMP=%CD%\work
set TEMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 work\test_native_controller_cleanup_composition.cpp /Fework\test_native_controller_cleanup_composition.exe /Fowork\test_native_controller_cleanup_composition.obj
if errorlevel 1 exit /b 1
work\test_native_controller_cleanup_composition.exe
