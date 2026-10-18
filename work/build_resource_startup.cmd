@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_resource_startup.cpp /Fowork\resource_startup64.obj /Fework\sfm_resource_startup64.exe
if not "%errorlevel%"=="0" exit /b 1
work\sfm_resource_startup64.exe
if not "%errorlevel%"=="0" exit /b 1




