@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_cvar_startup_native.cpp /Fowork\cvar_startup_native32.obj /Fework\sfm_cvar_startup_native32.exe
if not "%errorlevel%"=="0" exit /b 1
work\sfm_cvar_startup_native32.exe
if not "%errorlevel%"=="0" exit /b 1









