@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set TMP=%CD%\work
set TEMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 work\test_controller_path_prefix.cpp /Fework\test_controller_path_prefix.exe /Fowork\test_controller_path_prefix.obj
if errorlevel 1 exit /b 1
work\test_controller_path_prefix.exe
