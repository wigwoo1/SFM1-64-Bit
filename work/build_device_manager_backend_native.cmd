@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_device_manager_backend.cpp /Fowork\manager_backend32.obj /Fework\sfm_manager_backend32.exe
if not "%errorlevel%"=="0" exit /b 1
work\sfm_manager_backend32.exe
if not "%errorlevel%"=="0" exit /b 1


